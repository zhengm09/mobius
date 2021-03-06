/*
 * Project: udptunnel
 * File: message.c
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/des.h> 
#include <openssl/rsa.h> 

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#endif /*WIN32*/

#include "common.h"
#include "message.h"
#include "socket.h"
#include "crypt.h"

extern int debug_level;


/*
 * Sends a message to the UDP tunnel with the specified client ID, type, and
 * data. The data can be NULL and data_len 0 if the type of message won't have
 * a body, based on the protocol.
 * Returns 0 for success, -1 on error, or -2 to close the connection.
 */
int msg_send_msg(socket_t *to, uint16_t client_id,uint32_t sourceid, uint8_t type,
                 char *data, int data_len)
{
    char buf[MSG_MAX_LEN + sizeof(msg_hdr_t)];
    int len; /* length for entire packet */

    if(data_len > MSG_MAX_LEN)
        return -1;
    
    switch(type)
    {
        case MSG_TYPE_HELLO:
        case MSG_TYPE_HELLOACK:
        case MSG_TYPE_DATA0:
        case MSG_TYPE_DATA1:
		case MSG_TYPE_RELAYKEY:
		case MSG_TYPE_REG:
		case MSG_TYPE_REGACK:
		case MSG_TYPE_REQ:
		case MSG_TYPE_REQACK:
		case MSG_TYPE_RELAYREQ:
		case MSG_TYPE_RELAYACK:
            memcpy(buf+sizeof(msg_hdr_t), data, data_len);
            break;

        case MSG_TYPE_GOODBYE:
        case MSG_TYPE_KEEPALIVE:
        case MSG_TYPE_ACK0:     
            data_len = 0;
            break;
            
        default:
            return -1;
    }

    len = data_len + sizeof(msg_hdr_t);
    msg_init_header((msg_hdr_t *)buf, client_id,sourceid, type, data_len);
	//if(debug_level >= DEBUG_LEVEL2)
	//	printf("tunnel id %d send type%d %d bytes \n",sourceid,type,data_len);
    len = sock_send(to, buf, len);
    if(len < 0)
        return -1;
    else if(len == 0)
        return -2;
    else
        return 0;
}

/*
 * Sends a HELLO type message to the UDP tunnel with the specified host and
 * port in the body.
 * Returns 0 for success, -1 on error, or -2 to disconnect.
 */
int msg_send_reg(socket_t *to, char *host, char *port, uint16_t req_id,uint32_t sourceid)
{
    char *data;
    int str_len;
    int len;
	//printf("mark");
    str_len = strlen(host) + strlen(port) + 2;
    len = str_len + sizeof(req_id);

    data = malloc(len);
    if(!data)
        return -1;

    *((uint16_t *)data) = htons(req_id);

#ifdef WIN32
    _snprintf(data + sizeof(req_id), str_len, "%s %s", host, port);
#else
    snprintf(data + sizeof(req_id), str_len, "%s %s", host, port);
#endif
    char * ptr_en;
	char * keyfile=malloc(64);
	char addrstr[ADDRSTRLEN];
	sprintf(keyfile,"%s_pub.key",sock_get_addrstr (to,addrstr,sizeof(addrstr)));
	ptr_en=my_rsaencrypt (data,keyfile);
    len = msg_send_msg(to, 0, sourceid,MSG_TYPE_REG, ptr_en, 128);
    free(data);
	free(ptr_en);

   return len;
}

int msg_send_regrelay(socket_t *to, char *host, char *port, uint16_t req_id,uint32_t localid,uint32_t sourceid)
{
    char *data;
    int str_len;
    int len;
	//printf("mark");
    str_len = strlen(host) + strlen(port) + 2;
    len = str_len + sizeof(req_id);

    data = malloc(len);
    if(!data)
        return -1;

    *((uint16_t *)data) = htons(req_id);

#ifdef WIN32
    _snprintf(data + sizeof(req_id), str_len, "%s %s", host, port);
#else
    snprintf(data + sizeof(req_id), str_len, "%s %s", host, port);
#endif
    char * ptr_en;
	char *keyfile=malloc(64);	
	
	char addrstr[ADDRSTRLEN];
	sprintf(keyfile,"%s_pub.key",sock_get_addrstr (to,addrstr,sizeof(addrstr)));
	ptr_en=my_rsaencrypt (data,keyfile);
    len = msg_send_msg(to, 0, localid,MSG_TYPE_REG, ptr_en, 128);
    free(data);
	free(keyfile);
	free(ptr_en);
	return len;
}

int msg_send_req(socket_t *to, char *host, char *port, uint16_t req_id,uint32_t sourceid)
{
    char *data;
    int str_len;
    int len;

    str_len = strlen(host) + strlen(port) + 2;
    len = str_len + sizeof(req_id);

    data = malloc(len);
    if(!data)
        return -1;

    *((uint16_t *)data) = htons(req_id);

#ifdef WIN32
    _snprintf(data + sizeof(req_id), str_len, "%s %s", host, port);
#else
    snprintf(data + sizeof(req_id), str_len, "%s %s", host, port);
#endif
	char * ptr_en;
	char * keyfile=malloc(64);
		
	char addrstr[ADDRSTRLEN];
	sprintf(keyfile,"%s_pub.key",sock_get_addrstr (to,addrstr,sizeof(addrstr)));

	ptr_en=my_rsaencrypt (data,keyfile);
    len = msg_send_msg(to, 0, sourceid,MSG_TYPE_REQ, ptr_en, 128);
    free(data);
	free(ptr_en);

	return len;
}

int msg_send_reqrelay(socket_t *to, char *host, char *port, uint16_t req_id,uint32_t localid,uint32_t sourceid,uint16_t relays)
{
    char *data;
    int str_len;
    int len;

    str_len = strlen(host) + strlen(port) + 2;
    len = str_len + sizeof(req_id)+ sizeof(relays);

    data = malloc(len);
    if(!data)
        return -1;
	if(debug_level >= DEBUG_LEVEL2)
		printf("msg relays need %d \n",relays);
    *((uint16_t *)data) = htons(req_id);
	*((uint16_t *)(data+sizeof(req_id))) = htons(relays);

#ifdef WIN32
    _snprintf(data + sizeof(req_id)+ sizeof(relays), str_len, "%s %s", host, port);
#else
    snprintf(data + sizeof(req_id)+ sizeof(relays), str_len, "%s %s", host, port);
#endif
	char * ptr_en;
	char *keyfile=malloc(64);	
		
	char addrstr[ADDRSTRLEN];
	sprintf(keyfile,"%s_pub.key",sock_get_addrstr (to,addrstr,sizeof(addrstr)));
	ptr_en=my_rsaencrypt (data,keyfile);	
    len = msg_send_msg(to, 0, localid,MSG_TYPE_RELAYREQ, ptr_en, 128);
    free(data);
	free(keyfile);
	free(ptr_en);

	return len;
}

int msg_send_hello(socket_t *to, char *host, uint16_t req_id,uint32_t sourceid,char * rsakey)
{
		char *data;
    int str_len;
    int len;	
	char *keyfile=malloc(64);
	printf("rsakey is %s",rsakey);
		
	
	sprintf(keyfile,"%s_pub.key",rsakey);
	char *ptr_en;
	ptr_en=my_rsaencrypt(host,keyfile);

    str_len = 128+1;
    len = str_len + sizeof(req_id);

    data = malloc(len);
    if(!data)
        return -1;

    *((uint16_t *)data) = htons(req_id);	

	memcpy(data+sizeof(req_id),ptr_en,128);		

	len=msg_send_msg(to, 0, sourceid,MSG_TYPE_HELLO,data,130);
	free(data);
	free(keyfile);
	free(ptr_en);
	return len;	
	
}
int msg_send_reqack(socket_t *to, char *host, uint16_t req_id,uint32_t targetid,uint32_t sourceid)
{
    char *data;
    int str_len;
    int len;

    str_len = strlen(host)+1;
    len = str_len + sizeof(req_id);

    data = malloc(len);
    if(!data)
        return -1;

    *((uint16_t *)data) = htons(req_id);

#ifdef WIN32
    _snprintf(data + sizeof(req_id), str_len, "%s", host);
#else
    snprintf(data + sizeof(req_id), str_len, "%s", host);
#endif
	char *keyfile=malloc(64);
	char addrstr[ADDRSTRLEN];
	sprintf(keyfile,"%s_pub.key",sock_get_addrstr (to,addrstr,sizeof(addrstr)));
	char *ptr_en;
	ptr_en=my_rsaencrypt(data,keyfile);	
    len = msg_send_msg(to, 0, sourceid,MSG_TYPE_REQACK, ptr_en, 128);
    free(data);
	free(keyfile);
	free(ptr_en);

	return len;
}
int msg_send_relaykey(socket_t *to,uint32_t sourceid, char * key)
{
	if(debug_level >= DEBUG_LEVEL2)
		printf("key is %s \n",key);
 
	char *keyfile=malloc(64);
	char addrstr[ADDRSTRLEN];
	sprintf(keyfile,"%s_pub.key",sock_get_addrstr(to,addrstr,sizeof(addrstr)));
	char *ptr_en;
	ptr_en=my_rsaencrypt(key,keyfile);
	return msg_send_msg(to, 0, sourceid,MSG_TYPE_RELAYKEY,
                        ptr_en, 128);
}

int msg_send_helloack(socket_t *to,char * key,uint16_t req_id,uint16_t client_id,uint32_t sourceid,char * rsakey)
{
	char *data;
    int str_len;
    int len;	
	char *keyfile=malloc(64);
		

	sprintf(keyfile,"%s_pub.key",rsakey);
	char *ptr_en;
	ptr_en=my_rsaencrypt(key,keyfile);

    str_len = 128+1;
    len = str_len + sizeof(req_id);

    data = malloc(len);
    if(!data)
        return -1;

    *((uint16_t *)data) = htons(req_id);	

	memcpy(data+sizeof(req_id),ptr_en,128);		
	
	len=msg_send_msg(to, client_id, sourceid,MSG_TYPE_HELLOACK,data,130);
	free(data);
	free(keyfile);
	free(ptr_en);
	return len;
}

int msg_send_relayack(socket_t *to, char *host, uint16_t req_id,uint32_t tunnelid,uint32_t sourceid,uint8_t exp)
{
    char *data;
    int str_len;
    int len;
	

    str_len = strlen(host)+1;
    len = str_len + sizeof(req_id)+ sizeof(exp);

    data = malloc(len);
    if(!data)
        return -1;

    *((uint16_t *)data) = htons(req_id);
	*((uint8_t *)(data+sizeof(req_id))) = exp;
	

#ifdef WIN32
    _snprintf(data + sizeof(req_id)+ sizeof(exp), str_len, "%s", host);
#else
    snprintf(data + sizeof(req_id)+ sizeof(exp), str_len, "%s", host);
#endif
	char *keyfile=malloc(64);
	char addrstr[ADDRSTRLEN];
	sprintf(keyfile,"%s_pub.key",sock_get_addrstr (to,addrstr,sizeof(addrstr)));
	char *ptr_en;
	ptr_en=my_rsaencrypt(data,keyfile);
    len = msg_send_msg(to, 0, tunnelid,MSG_TYPE_RELAYACK, ptr_en, 128);
    free(data);
	free(keyfile);
	free(ptr_en);
	return len;
}

/*
 * Receives a message that is ready to be read from the UDP socket. Writes the
 * body of the message into data, and sets the client ID, type, and length
 * of the message.
 * Returns 0 for success, -1 on error, or -2 to disconnect.
 */
int msg_recv_msg(socket_t *sock, char *data, int data_len,
                 uint16_t *client_id, uint8_t *type, uint16_t *length,uint32_t *sourceid)
{
    char buf[MSG_MAX_LEN + sizeof(msg_hdr_t)];
    msg_hdr_t *hdr_ptr;
    char *msg_ptr;
    int ret;
	
    hdr_ptr = (msg_hdr_t *)buf;
    msg_ptr = buf + sizeof(msg_hdr_t);
    
    ret = sock_recv(sock, buf, sizeof(buf));
	
    if(ret < 0)
        return -1;
    else if(ret == 0)
        return -2;
	
    *client_id = msg_get_client_id(hdr_ptr);
    *type = msg_get_type(hdr_ptr);
    *length = msg_get_length(hdr_ptr);
	*sourceid=msg_get_sourceid(hdr_ptr);
    
    if(ret-sizeof(msg_hdr_t) != *length)
	{
		printf("recv msg from %d %d bytes %d bytes \n",*sourceid,ret,*length);
        return -1;
	}

    *length = MIN(data_len, *length);
    memcpy(data, msg_ptr, *length);
	
    return 0;
}
