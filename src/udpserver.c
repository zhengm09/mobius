/*
 * Project: udptunnel
 * File: udpserver.c
 *
 * Copyright (C) 2009 Daniel Meekins
	 * Contact: dmeekins - gmail
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
	 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#define OPENSSLKEY "inter.key"
#define PUBLICKEY "inter_pub.key"

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#else
#include "helpers/winhelpers.h"
#endif

#include "common.h"
#include "list.h"
#include "client.h"
#include "message.h"
#include "socket.h"
#include "crypt.h"


extern int debug_level;
extern int ipver;
static int running = 1;
static int next_client_id = 1;
static uint16_t next_req_id;
static char *lhost, *lport, *phost, *pport;
static uint32_t localid;
static socket_t *udp_serv = NULL;
static list_t *relay_clients;



/* internal functions */
static int handle_message(uint16_t id, uint8_t msg_type, char *data,
                          int data_len, uint32_t sourceid, list_t *clients,
                          fd_set *client_fds,int mode);
static void disconnect_and_remove_client(uint16_t id, list_t *clients,fd_set *fds, int full_disconnect);
static void signal_handler(int sig);

/*
 * UDP Tunnel server main(). Handles program arguments, initializes everything,
 * and runs the main loop.
 */
int udpserver(int argc, char *argv[],int mode)
{
	static char addrstr[ADDRSTRLEN];

	socket_t *next_sock = NULL;


	list_t *clients = NULL;
	list_t *conn_clients;
	//list_t *acls = NULL;

	//socket_t *udp_from = NULL;

	char data[MSG_MAX_LEN+ sizeof(msg_hdr_t)];

	client_t *client;

	uint16_t tmp_id;
	uint8_t tmp_type;
	uint16_t tmp_len;
	uint32_t source_id;
	//acl_t *tmp_acl;

	struct timeval curr_time;
	struct timeval timeout;
	struct timeval check_time;
	struct timeval check_interval;
	fd_set client_fds;
	fd_set read_fds;
	int num_fds;

	int i;
	int ret;

	signal(SIGINT, &signal_handler);

	i = 0; 
	if (mode==2)
	{
		lhost = (argc - i == 1) ? NULL : argv[i++];
		lport = argv[i++];
		ERROR_GOTO(!isnum(lport), "Invalid local port.", done);
	}
	else if (mode<2)
	{
		lhost = (argc - i == 3) ? NULL : argv[i++];
		lport = argv[i++];
		phost = argv[i++];
		pport = argv[i++];


		/* Check validity of ports (can't check ip's b/c might be host names) */
		ERROR_GOTO(!isnum(lport), "Invalid local port.", done);
		ERROR_GOTO(!isnum(pport), "Invalid proxy port.", done);
		//ERROR_GOTO(!isnum(rport), "Invalid remote port.", done);
	}
	srand(inet_addr(lhost));
	localid=(rand());
	if(debug_level >= DEBUG_LEVEL1)
	{
		printf("local id %d \n",localid);
	}
	next_req_id = rand() % 0xffff;


	/* Create an empty list for the clients */
	clients = list_create(sizeof(client_t), p_client_cmp, p_client_copy,
	                      p_client_free, 1);
	if(!clients)
		goto done;
	/* Create and empty list for the connecting clients */
	conn_clients = list_create(sizeof(client_t), p_client_cmp, p_client_copy,
	                           p_client_free, 1);
	ERROR_GOTO(conn_clients == NULL, "Error creating clients list.", done);

	relay_clients = list_create(sizeof(client_t), p_client_cmp, p_client_copy,
	                            p_client_free, 1);
	ERROR_GOTO(relay_clients == NULL, "Error creating clients list.", done);

	/* Create the socket to receive UDP messages on the specified port */
	udp_serv = sock_create(lhost, lport,ipver, SOCK_TYPE_UDP, 1, 1);
	generate_rsakey(lhost);
	if (mode<2)
	{

		next_sock = sock_create(phost, pport, ipver, SOCK_TYPE_UDP, 0, 1);

		if (mode==1)
		{
			msg_send_reg(next_sock,lhost,lport,0,localid);
		}
		else
		{
			msg_send_req(next_sock,lhost,lport,0,localid);
		}                             

		sock_free(next_sock);		
		next_sock = NULL;		
	}
	if(!udp_serv)
		goto done;
	if(debug_level >= DEBUG_LEVEL1)
	{
		printf("Listening on UDP %s\n",
		       sock_get_str(udp_serv, addrstr, sizeof(addrstr)));
	}

	/* Create empty udp socket for getting source address of udp packets */
	//udp_from = sock_create(NULL, NULL, ipver, SOCK_TYPE_UDP, 0, 0);
	//if(!udp_from)
	//    goto done;

	FD_ZERO(&client_fds);

	timerclear(&timeout);
	gettimeofday(&check_time, NULL);
	check_interval.tv_sec = 0;
	check_interval.tv_usec = 500000;


	while(running)
	{
		if(!timerisset(&timeout))
			timeout.tv_usec = 50000;

		/* Reset the file desc. set */
		read_fds = client_fds;
		FD_SET(SOCK_FD(udp_serv), &read_fds);

		ret = select(FD_SETSIZE, &read_fds, NULL, NULL, &timeout);
		PERROR_GOTO(ret < 0, "select", done);
		num_fds = ret;

		gettimeofday(&curr_time, NULL);

		/* Go through all the clients and check if didn't get an ACK for sent
		 data during the timeout period */
		if(timercmp(&curr_time, &check_time, >))
		{
			for(i = 0; i < LIST_LEN(clients); i++)
			{
				client = list_get_at(clients, i);

				if(client_timed_out(client, curr_time))
				{
					disconnect_and_remove_client(CLIENT_ID(client), clients,
					                             &client_fds, 1);
					i--;
					continue;
				}

				ret = client_check_and_resend(client, curr_time);
				if(ret == -2)
				{
					disconnect_and_remove_client(CLIENT_ID(client), clients,
					                             &client_fds, 1);
					i--;
					continue;
				}
			}

			/* Set time to chech this stuff next */
			timeradd(&curr_time, &check_interval, &check_time);
		}

		if(num_fds == 0)
			continue;

		/* Get any data received on the UDP socket */
		if(FD_ISSET(SOCK_FD(udp_serv), &read_fds))
		{			
			ret = msg_recv_msg(udp_serv, data, sizeof(data),
			                   &tmp_id, &tmp_type, &tmp_len,&source_id);
			//if(debug_level >= DEBUG_LEVEL2)
			//	printf("recv msg from %d type %d %d bytes \n ",source_id,tmp_type,tmp_len);

			for(i = 0; i < LIST_LEN(relay_clients); i++)
			{
				client = list_get_at(relay_clients, i);
				if (client->sourceid==source_id)
				{
					//printf("got msg in relay_clients %d \n",CLIENT_ID(client));
					if (client->tcp2udp_state == CLIENT_WAIT_HELLO)
					{
						ret=1;
						break;
						//printf("not export relay \n");
					}
				}

			}
			if (ret==1)
				msg_send_msg(client->tcp_sock,tmp_id,source_id,tmp_type,data,tmp_len);
			else if(ret == 0)
				ret = handle_message(tmp_id, tmp_type, data, tmp_len,
				                     source_id, clients, &client_fds,mode);


			if(ret < 0)
				disconnect_and_remove_client(tmp_id, clients, &client_fds, 1);				

		}

		/* Go through all the clients and get any TCP data that is ready */
		for(i = 0; i < LIST_LEN(clients); i++)
		{
			client = list_get_at(clients, i);

			if(num_fds > 0 && client_tcp_fd_isset(client, &read_fds))
			{
				ret = client_recv_tcp_data(client);
				if(ret == -1)
				{
					disconnect_and_remove_client(CLIENT_ID(client),
					                             clients, &client_fds, 1);
					i--; /* Since there will be one less element in list */
					continue;
				}
				else if(ret == -2)
				{
					client_mark_to_disconnect(client);
					disconnect_and_remove_client(CLIENT_ID(client),
					                             clients, &client_fds, 0);
				}

				num_fds--;
			}

			/* send any TCP data that was ready */
			ret = client_send_udp_data(client);
			if(ret < 0)
			{
				disconnect_and_remove_client(CLIENT_ID(client),
				                             clients, &client_fds, 1);
				i--; /* Since there will be one less element in list */
				continue;
			}
		}

		/* Finally, send any udp data that's still in the queue */
		for(i = 0; i < LIST_LEN(clients); i++)
		{
			client = list_get_at(clients, i);
			ret = client_send_udp_data(client);

			if(ret < 0 || client_ready_to_disconnect(client))
			{
				disconnect_and_remove_client(CLIENT_ID(client), clients,
				                             &client_fds, 1);
				i--;
				continue;
			}
		}
	}

	done:
		if(debug_level >= DEBUG_LEVEL1)
			printf("Cleaning up...\n");
		// if(acls)
		//   list_free(acls);
		if(clients)
			list_free(clients);
		if(udp_serv)
	{
		sock_close(udp_serv);
		sock_free(udp_serv);
	}
		//  if(udp_from)
		//     sock_free(udp_from);
		if(debug_level >= DEBUG_LEVEL1)
			printf("Goodbye.\n");

		return 0;
}

/*
 * Closes the client's TCP socket (not UDP, since it is shared) and remove from
	 * the fd set. If full_disconnect is set, remove the list.
 */
void disconnect_and_remove_client(uint16_t id, list_t *clients,
                                  fd_set *fds, int full_disconnect)
{
	client_t *c;

	if(id == 0)
		return;

	c = list_get(clients, &id);
	if(!c)
		return;

	/* ok to call multiple times since fd will be -1 after first disconnect */
	client_remove_tcp_fd_from_set(c, fds);
	client_disconnect_tcp(c);

	if(full_disconnect)
	{
		client_send_goodbye(c);

		if(debug_level >= DEBUG_LEVEL1)
			printf("Client %d disconnected.\n", CLIENT_ID(c));

		list_delete(clients, &id);
	}
}

/*
 * Handles the message received from the UDP tunnel. Returns 0 for success, -1
 * for some error that it handled, and -2 if the connection should be
 * disconnected.
 */
int handle_message(uint16_t id, uint8_t msg_type, char *data, int data_len,
                   uint32_t sourceid, list_t *clients,fd_set *client_fds,int mode)
{
	client_t *c = NULL;
	client_t *tunnel=NULL;
	client_t *c2 = NULL;
	socket_t *tcp_sock = NULL;
	socket_t *udp_sock = NULL;
	int ret = 0;



	if(id != 0)
	{
		c = list_get(clients, &id);
		if(!c)
			return -1;
	}
	//if(id == 0 && (msg_type != MSG_TYPE_HELLO||msg_type !=MSG_TYPE_REQ||msg_type !=MSG_TYPE_REG))
	//    return -2;


	switch(msg_type)
	{
		case MSG_TYPE_GOODBYE:
			ret = -2;
			break;

			/* Data in the hello message will be like "hostname port", possibly
			 without the null terminator. This will look for the space and
			 parse out the hostname or ip address and port number */
		case MSG_TYPE_HELLO:
		{

			int i;
			char port[6]; /* need this so port str can have null term. */

			char dst_addrstr[ADDRSTRLEN];

			uint16_t req_id;

			if(id != 0)
				break;
			char * ptr_de=NULL;
			char * keyfile=malloc(64);
			sprintf(keyfile,"%s.key",lhost);
			//printf("keyfile %s, lhost %s",keyfile,lhost);
			req_id = ntohs(*((uint16_t*)data));
			data+= sizeof(req_id);
			ptr_de=my_rsadecrypt(data,keyfile);
			int ptr_len=strlen(ptr_de);
			if (ptr_len>20)
				break;
			//printf("ptr_de,%s ptr_len %d \n",ptr_de,ptr_len);
			/* look for the space separating the host and port */
			for(i = 0; i < ptr_len; i++)
				if(ptr_de[i] == ' ')
				break;
			if(i == ptr_len)
				break;

			/* null terminate the host and get the port number to the string */
			ptr_de[i++] = 0;
			strncpy(port, ptr_de+i, ptr_len-i);
			port[ptr_len-i] = 0;
			printf("req_id %d,host %s, port %s \n",req_id,ptr_de,port);
			/* Create an unconnected TCP socket for the remote host, the
			 client itself, add it to the list of clients */		

			tcp_sock = sock_create(ptr_de, port, ipver, SOCK_TYPE_TCP, 0, 0);
			ERROR_GOTO(tcp_sock == NULL, "Error creating tcp socket", error);

			for(i = 0; i < LIST_LEN(relay_clients); i++)
			{
				tunnel = list_get_at(relay_clients, i);
				if (tunnel->sourceid==sourceid)
				{
					udp_sock =sock_copy(CLIENT_TCP_SOCK(tunnel));

					if(debug_level >= DEBUG_LEVEL1)
						printf("clientid %d \n",CLIENT_ID(tunnel));
					break;
				}

			}
			c = client_create(next_client_id++,sourceid, tcp_sock, udp_sock, 0);
			memcpy(c->rsakey,tunnel->rsakey,strlen(tunnel->rsakey));
			printf("tunnel %d key is %s client key is %s \n",sourceid,tunnel->rsakey,c->rsakey);
			sock_free(tcp_sock);
			sock_free(udp_sock);
			ERROR_GOTO(c == NULL, "Error creating client", error);

			c2 = list_add(clients, c, 1);

			ERROR_GOTO(c2 == NULL, "Error adding client to list", error);

			if(debug_level >= DEBUG_LEVEL1)
			{
				//sock_get_str(CLIENT_UDP_SOCK(c2), src_addrstr,
				//             sizeof(src_addrstr));
				sock_get_str(CLIENT_TCP_SOCK(c2), dst_addrstr,
				             sizeof(dst_addrstr));
				printf("tunnel %d connection(%d):  remote:%s\n",
				       sourceid,CLIENT_ID(c2), dst_addrstr);
			}

			/* Send the Hello ACK message if created client successfully */
			client_send_helloack(c2,req_id);
			client_reset_keepalive(c2);
			client_free(c);

			break;
		}
			/* Can connect to TCP connection once received the Hello ACK */
		case MSG_TYPE_HELLOACK:
		{
			uint16_t req_id;
			char * ptr_de=NULL;
			char * keyfile=malloc(64);
			sprintf(keyfile,"%s.key",lhost);
			req_id = ntohs(*((uint16_t*)data));			
			//req_id = ntohs(*((uint16_t*)ptr_de));			
			data+=sizeof(req_id);	
			data_len-=sizeof(req_id);
			ptr_de=my_rsadecrypt(data,keyfile);	
			//printf("key is %s",ptr_de);

			if(client_connect_tcp(c) != 0)
				return -2;
			client_got_helloack(c);
			memcpy(c->deskey,ptr_de,strlen(ptr_de));
			client_add_tcp_fd_to_set(c, client_fds);
			break;
		}
			/* Resets the timeout of the client's keep alive time */

		case MSG_TYPE_KEEPALIVE:

			client_reset_keepalive(c);
			break;

			/* Receives the data it got from the UDP tunnel and sends it to the
			 TCP connection. */
		case MSG_TYPE_DATA0:

		//case MSG_TYPE_DATA1:
			ret = client_got_udp_data(c, data, data_len, msg_type);
			if(ret == 0)
				ret = client_send_tcp_data(c);
			break;

			/* Receives the ACK from the UDP tunnel to set the internal client
			 state. */
		// case MSG_TYPE_ACK0:
		// case MSG_TYPE_ACK1:
			//client_got_ack(c, msg_type);
			//    break;
		case MSG_TYPE_RELAYKEY:
		{		
			int i;
			client_t *c2;
			if(id != 0)
				break;
			char * ptr_de;
			char * keyfile=malloc(64);
			sprintf(keyfile,"%s.key",lhost);
			ptr_de=my_rsadecrypt(data,keyfile);
			
			for(i = 0; i < LIST_LEN(relay_clients); i++)
			{
				c2 = list_get_at(relay_clients, i);

				if (c2->sourceid==sourceid)
				{
					memcpy(c2->rsakey,ptr_de,strlen(ptr_de));
					break;
				}
			}				
			
			printf("tunnel %d key is %s \n",c2->sourceid,c2->rsakey);			
			break;

		}
		
		case MSG_TYPE_REG:
		{
			int i;
			char port[6]; /* need this so port str can have null term. */

			char dst_addrstr[ADDRSTRLEN];

			uint16_t req_id;

			if(id != 0)
				break;
			char * ptr_de=NULL;
			char * keyfile=malloc(64);

			sprintf(keyfile,"%s.key",lhost);
			//printf("keyfile %s, lhost %s",keyfile,lhost);
			ptr_de=my_rsadecrypt(data,keyfile);


			req_id = ntohs(*((uint16_t*)ptr_de));
			ptr_de += sizeof(uint16_t);

			int ptr_len=strlen(ptr_de);
			if (ptr_len>20)
				break;
			//printf("ptr_len %d \n",ptr_len);
			/* look for the space separating the host and port */
			for(i = 0; i < ptr_len; i++)
				if(ptr_de[i] == ' ')
				break;
			if(i == ptr_len)
				break;

			/* null terminate the host and get the port number to the string */
			ptr_de[i++] = 0;
			strncpy(port, ptr_de+i, ptr_len-i);
			port[ptr_len-i] = 0;
			//printf("req_id %d,host %s, port %s \n",req_id,ptr_de,port);
			/* Create an unconnected TCP socket for the remote host, the
			 client itself, add it to the list of clients */
			tcp_sock = sock_create(ptr_de, port, ipver, SOCK_TYPE_UDP, 0, 1);
			ERROR_GOTO(tcp_sock == NULL, "Error creating udp socket", error);

			int renew=0;
			for(i = 0; i < LIST_LEN(relay_clients); i++)
			{
				c2 = list_get_at(relay_clients, i);

				if (c2->sourceid==sourceid)
				{
					c2->tcp_sock=sock_copy(tcp_sock);	


					if (c2->resend_count>1)
						c2->resend_count--;
					renew=1;
					break;
				}
			}
			if (renew==0)	
			{
				c = client_create(next_client_id++, sourceid, tcp_sock,udp_serv,0);

				ERROR_GOTO(c == NULL, "Error creating client", error);			
				c2 = list_add(relay_clients, c, 1);		

				ERROR_GOTO(c2 == NULL, "Error adding client to list", error);
			}
			if(debug_level >= DEBUG_LEVEL1)
			{

				sock_get_str(CLIENT_TCP_SOCK(c2), dst_addrstr,
				             sizeof(dst_addrstr));
				if (renew==0)
					printf("New register server");
				else
					printf("renew server");
				printf("(%d): udp:%s\n",
				       CLIENT_ID(c2),  dst_addrstr);
			}

			/* Send the Hello ACK message if created client successfully */
			client_send_regack(c2, req_id);
			client_reset_keepalive(c2);
			client_free(c);            
			break;
		}
		case MSG_TYPE_REGACK:    
			if(id != 0)
				break;		
			break;
		case MSG_TYPE_REQ:
		{
			int i;
			char port[6]; /* need this so port str can have null term. */

			char dst_addrstr[ADDRSTRLEN];

			uint16_t req_id;

			if(id != 0)
				break;

			char * ptr_de=NULL;
			char * keyfile=malloc(64);
			sprintf(keyfile,"%s.key",lhost);
			ptr_de=my_rsadecrypt(data,keyfile);

			req_id = ntohs(*((uint16_t*)ptr_de));
			ptr_de += sizeof(uint16_t);

			int ptr_len=strlen(ptr_de);
			if (ptr_len>20)
				break;
			//printf("ptr_len %d \n",ptr_len);
			/* look for the space separating the host and port */
			for(i = 0; i < ptr_len; i++)
				if(ptr_de[i] == ' ')
				break;
			if(i == ptr_len)
				break;

			/* null terminate the host and get the port number to the string */
			ptr_de[i++] = 0;
			strncpy(port, ptr_de+i, ptr_len-i);
			port[ptr_len-i] = 0;
			//printf("req_id %d,host %s, port %s \n",req_id,ptr_de,port);
			/* Create an unconnected TCP socket for the remote host, the
			 client itself, add it to the list of clients */
			tcp_sock = sock_create(ptr_de, port, ipver, SOCK_TYPE_UDP, 0, 1);		
			ERROR_GOTO(tcp_sock == NULL, "Error creating udp socket", error);	
			if (LIST_LEN(relay_clients)>0)
			{
				uint32_t given_ip=0;
				int id=0;
				for(i = 0; i < LIST_LEN(relay_clients); i++)
				{
					c = list_get_at(relay_clients, i);

					if ((sock_get_ipaddr(c->tcp_sock)^sock_get_ipaddr(tcp_sock))<(sock_get_ipaddr(c->tcp_sock)^given_ip))
					{
						given_ip=sock_get_ipaddr(c->tcp_sock);						
						id=i;
					}
				}
				c2 = list_get_at(relay_clients, id);	
				//printf("clientid %d source id %d \n",c2->sourceid,sourceid);
				msg_send_reqack(tcp_sock, sock_get_str(c2->tcp_sock, dst_addrstr, sizeof(dst_addrstr)),0,sourceid,c2->sourceid);
			}

			else
				msg_send_msg(tcp_sock, localid,sourceid, MSG_TYPE_GOODBYE,NULL, 0);	

			break;
		}


		case MSG_TYPE_REQACK:
		{
			int i;
			char port[6]; /* need this so port str can have null term. */

			char dst_addrstr[ADDRSTRLEN];

			uint16_t req_id;

			if(id != 0)
				break;

			char * ptr_de=NULL;
			char * keyfile=malloc(64);
			sprintf(keyfile,"%s.key",lhost);

			ptr_de=my_rsadecrypt(data,keyfile);

			req_id = ntohs(*((uint16_t*)ptr_de));
			ptr_de += sizeof(uint16_t);

			int ptr_len=strlen(ptr_de);
			if (ptr_len>20)
				break;
			//printf("ptr_len %d \n",ptr_len);
			/* look for the space separating the host and port */
			for(i = 0; i < ptr_len; i++)
				if(ptr_de[i] == ' ')
				break;
			if(i == ptr_len)
				break;

			/* null terminate the host and get the port number to the string */
			ptr_de[i++] = 0;
			strncpy(port, ptr_de+i, ptr_len-i);
			port[ptr_len-i] = 0;
			//printf("req_id %d,host %s, port %s \n",req_id,ptr_de,port);
			/* Create an unconnected TCP socket for the remote host, the
			 client itself, add it to the list of clients */		

			tcp_sock = sock_create(ptr_de, port, ipver, SOCK_TYPE_UDP, 0, 1);
			ERROR_GOTO(tcp_sock == NULL, "Error creating udp socket", error);
			if(debug_level >= DEBUG_LEVEL1)
			{

				sock_get_str(tcp_sock, dst_addrstr,
				             sizeof(dst_addrstr));
				printf("intra server id %d host:%s\n",sourceid, dst_addrstr);
			}
			msg_send_regrelay(tcp_sock,lhost,lport, 0,localid,sourceid);
			break;
		}
		case MSG_TYPE_RELAYREQ:
		{
			int i;
			char port[6]; /* need this so port str can have null term. */
			int relays;
			char dst_addrstr[ADDRSTRLEN];

			uint16_t req_id;

			if(id != 0)
				break;
			char * ptr_de=NULL;
			char * keyfile=malloc(64);

			sprintf(keyfile,"%s.key",lhost);
			ptr_de=my_rsadecrypt(data,keyfile);

			req_id = ntohs(*((uint16_t*)ptr_de));
			ptr_de += sizeof(uint16_t);
			relays = ntohs(*((uint16_t*)ptr_de));
			ptr_de += sizeof(uint16_t);

			int ptr_len=strlen(ptr_de);
			if (ptr_len>20)
				break;
			if(debug_level >= DEBUG_LEVEL2)
				printf("ptr_len %d \n",ptr_len);
			/* look for the space separating the host and port */
			for(i = 0; i < ptr_len; i++)
				if(ptr_de[i] == ' ')
				break;
			if(i == ptr_len)
				break;

			/* null terminate the host and get the port number to the string */
			ptr_de[i++] = 0;
			strncpy(port, ptr_de+i, ptr_len-i);
			port[ptr_len-i] = 0;
			if(debug_level >= DEBUG_LEVEL2)
				printf("req_id %d,host %s, port %s \n",req_id,ptr_de,port);
			/* Create an unconnected TCP socket for the remote host, the
			 client itself, add it to the list of clients */

			tcp_sock = sock_create(ptr_de, port, ipver, SOCK_TYPE_UDP, 0, 1);
			ERROR_GOTO(tcp_sock == NULL, "Error creating udp socket", error);
			if (relays>LIST_LEN(relay_clients))
			{
				if(debug_level >= DEBUG_LEVEL1)
					printf("relay need more \n");
				break;
			}
			//char * exphost=malloc(64);
			//memset(exphost,0,64);
			uint8_t exp=0;
			int y=(relays+2)/2-1;
			socket_t *tmp_sock = NULL;
			socket_t *exp_sock = NULL;
			tmp_sock=tcp_sock;
			exp_sock=tmp_sock;
			uint32_t tmpid=sourceid;
			if(debug_level >= DEBUG_LEVEL1)
				printf("relays have %d need %d export on %d \n",LIST_LEN(relay_clients),relays,y);
			int j;
			char addrstr[ADDRSTRLEN];
			for(j = 0; j < relays; j++)
			{ 
				c2 = list_get_at(relay_clients, 0);
				for(i = 0; i < LIST_LEN(relay_clients); i++)
				{
					c = list_get_at(relay_clients, i);

					if (c->resend_count<=c2->resend_count)
					{
						c2=c;
					
					}
				}

				msg_send_relayack(tmp_sock,sock_get_str(CLIENT_TCP_SOCK(c2), dst_addrstr,
				                                        sizeof(dst_addrstr)),0,sourceid,tmpid,exp);		
				c2->resend_count++;

				tmpid=c2->sourceid;
				if(debug_level >= DEBUG_LEVEL1)
				{
					sock_get_str(tmp_sock, dst_addrstr,sizeof(dst_addrstr));
					printf("tunnel %d  %s ",sourceid,dst_addrstr);
					sock_get_str(CLIENT_TCP_SOCK(c2), dst_addrstr,sizeof(dst_addrstr));
					printf("to%d ip %s exp %d \n", tmpid,dst_addrstr,exp);
				}
				tmp_sock=sock_copy(CLIENT_TCP_SOCK(c2));
				if (j==y)
				{
					exp=1;					
					exp_sock=tmp_sock;
				}
				else
					exp=0;				
			}

			msg_send_relayack(tmp_sock,sock_get_str(tcp_sock, dst_addrstr,
			                                        sizeof(dst_addrstr)),0,sourceid,tmpid,exp);

			//sprintf(exphost,"%s",sock_get_addrstr(exp_sock,addrstr,sizeof(addrstr)));

			if(debug_level >= DEBUG_LEVEL1)
			{
				sock_get_str(tmp_sock, dst_addrstr,sizeof(dst_addrstr));
				printf("tunnel %d  %s ",sourceid,dst_addrstr);
				sock_get_str(tcp_sock, dst_addrstr,sizeof(dst_addrstr));
				printf("to%d ip %s exp %d \n", sourceid,dst_addrstr,exp);
			}		
			msg_send_relaykey(tcp_sock,sourceid,sock_get_addrstr(exp_sock,addrstr,sizeof(addrstr)));
			msg_send_relaykey(exp_sock,sourceid,sock_get_addrstr(tcp_sock,addrstr,sizeof(addrstr)));
			
			//if(debug_level >= DEBUG_LEVEL2)
			//	printf("exphost %s \n",exphost);
			break;
		}	
		case MSG_TYPE_RELAYACK:
		{
			int i;
			char port[6]; /* need this so port str can have null term. */
			uint8_t exp;
			char dst_addrstr[ADDRSTRLEN];

			uint16_t req_id;

			if(id != 0)
				break;
			char * ptr_de=NULL;
			char * keyfile=malloc(64);
			sprintf(keyfile,"%s.key",lhost);
			ptr_de=my_rsadecrypt(data,keyfile);

			req_id = ntohs(*((uint16_t*)ptr_de));
			ptr_de += sizeof(uint16_t);
			exp = *((uint8_t*)ptr_de);
			ptr_de += sizeof(uint8_t);

			int ptr_len=strlen(ptr_de);
			if (ptr_len>20)
				break;
			//printf("ptr_len %d \n",ptr_len);
			/* look for the space separating the host and port */
			for(i = 0; i < ptr_len; i++)
				if(ptr_de[i] == ' ')
				break;
			if(i == ptr_len)
				break;

			/* null terminate the host and get the port number to the string */
			ptr_de[i++] = 0;
			strncpy(port, ptr_de+i, ptr_len-i);
			port[ptr_len-i] = 0;
			//printf("req_id %d,host %s, port %s \n",req_id,ptr_de,port);

			tcp_sock = sock_create(ptr_de, port, ipver, SOCK_TYPE_UDP, 0, 1);
			ERROR_GOTO(tcp_sock == NULL, "Error creating udp socket", error);
			int renew=0;
			for(i = 0; i < LIST_LEN(relay_clients); i++)
			{
				c2 = list_get_at(relay_clients, i);

				if (c2->sourceid==sourceid)
				{
					c2->tcp_sock=sock_copy(tcp_sock);
					if (exp==0)
						c2->tcp2udp_state = CLIENT_WAIT_HELLO;					
					renew=1;
				}
			}
			if (renew==0)	
			{
				c = client_create(next_client_id++, sourceid, tcp_sock,udp_serv,0);

				if (exp==0)
					c->tcp2udp_state = CLIENT_WAIT_HELLO;
				ERROR_GOTO(c == NULL, "Error creating client", error);			
				c2 = list_add(relay_clients, c, 1);		
				if(debug_level >= DEBUG_LEVEL1)
					printf("relay %d added id %d \n",c2->sourceid,CLIENT_ID(c2));
				ERROR_GOTO(c2 == NULL, "Error adding client to list", error);
			}
			client_reset_keepalive(c2);
			sock_free(tcp_sock);

			if(debug_level >= DEBUG_LEVEL1)
			{
				sock_get_str(CLIENT_TCP_SOCK(c2), dst_addrstr,sizeof(dst_addrstr));
				printf("tunnel %d clientID %d next hop:%s\n",c2->sourceid,CLIENT_ID(c2),  dst_addrstr);
			}


			break;			 
		}


		default:
			ret = -1;
	}

	return ret;

	error:
		return -1;
}

void signal_handler(int sig)
{
	switch(sig)
	{
		case SIGINT:
			running = 0;
	}
}
