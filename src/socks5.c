#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <errno.h>
#include <string.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include "socks5.h"

#define USER_NAME "yunshu"
#define PASS_WORD "ph4nt0m" 
//Selectauthmethod,return0ifsuccess,-1iffailed
int SelectMethod(int sock)
{ 
	char recv_buffer[BUFF_SIZE]={0};
	char reply_buffer[2]={0};
	METHOD_SELECT_REQUEST *method_request;
	METHOD_SELECT_RESPONSE *method_response; //recv	METHOD_SELECT_REQUEST 
	int ret=recv(sock,recv_buffer,BUFF_SIZE,0);
	if(ret<=0)
	{ 
		perror("recverror");
		close(sock);
		return-1;
	}
	printf("SelectMethod:recv%dbytes\n",ret); 
	//if client request a wrong version or a wrong number_method
	method_request=(METHOD_SELECT_REQUEST*)recv_buffer;
	method_response=(METHOD_SELECT_RESPONSE*)reply_buffer;
	method_response->version=VERSION; 
	//if not socks5 
	if((int)method_request->version!=VERSION)
	{ 
		method_response->select_method=0xff; 
		send(sock,method_response,sizeof(METHOD_SELECT_RESPONSE),0);
		close(sock);
		return -1;
	} 
	method_response->select_method=AUTH_CODE;
	if(-1==send(sock,method_response,sizeof(METHOD_SELECT_RESPONSE),0))
	{ 
		close(sock);
		return -1;
	}
	return 0;
}
//test password,return 0 for success.
int AuthPassword(int sock)
{
	char recv_buffer[BUFF_SIZE]={0};
	char reply_buffer[BUFF_SIZE]={0};
	AUTH_REQUEST *auth_request;
	AUTH_RESPONSE *auth_response; 
	//auth username and password 
	int ret=recv(sock,recv_buffer,BUFF_SIZE,0);
	if(ret<=0)
	{ 
		perror("recv username and password error");
		close(sock);
		return-1;
	} 
	printf("AuthPass:recv%dbytes\n",ret);
	auth_request=(AUTH_REQUEST*)recv_buffer;
	memset(reply_buffer,0,BUFF_SIZE); 
	auth_response=(AUTH_RESPONSE*)reply_buffer;
	auth_response->version=0x01;
	char recv_name[256]={0};
	char recv_pass[256]={0};
	//auth_request->name_lenisachar,maxnumberis0xff
	char pwd_str[2]={0};
	strncpy(pwd_str,auth_request->name+auth_request->name_len,1);
	int pwd_len=(int)pwd_str[0]; 
	strncpy(recv_name,auth_request->name,auth_request->name_len);
	strncpy(recv_pass,auth_request->name+auth_request->name_len+sizeof(auth_request->pwd_len),pwd_len); 
	printf("username:%s\npassword:%s\n",recv_name,recv_pass);
	//checkusernameandpassword 
	if((strncmp(recv_name,USER_NAME,strlen(USER_NAME))==0)&&(strncmp(recv_pass,PASS_WORD,strlen(PASS_WORD))==0))
	{
		auth_response->result=0x00;
		if(-1==send(sock,auth_response,sizeof(AUTH_RESPONSE),0))
		{
			close(sock);
			return -1;
		}
		else
		{ 
			return 0;
		}
	}
	else
	{
		auth_response->result=0x01; 
		send(sock,auth_response,sizeof(AUTH_RESPONSE),0);
		close(sock);
		return -1;
	}
} 
//parse command,and try to connect realserver.
//return socket for success,-1 for failed.
char *ParseCommand(int sock)
{ 
	char recv_buffer[BUFF_SIZE]={0};
	
	SOCKS5_REQUEST *socks5_request;
	
	//recv command 
	int ret=recv(sock,recv_buffer,BUFF_SIZE,0);
	if(ret<=0) 
	{ 
		perror( "recv connect command error"); 
		close(sock);
		
	}
	socks5_request=(SOCKS5_REQUEST*)recv_buffer;
	if((socks5_request->version!=VERSION)||(socks5_request->cmd!=CONNECT)|| (socks5_request->address_type==IPV6))
	{
		printf("connectcommanderror.\n");
		close(sock);
		
	} 
	//begain process connect request
	struct sockaddr_in sin; 
	memset((void*)&sin,0,sizeof(struct sockaddr_in));
	sin.sin_family=AF_INET; 
	//get realserver&#39;sipaddress 
	if(socks5_request->address_type==IPV4)
	{ 
		memcpy(&sin.sin_addr.s_addr,&socks5_request->address_type+sizeof(socks5_request->address_type),4);
		memcpy(&sin.sin_port,&socks5_request->address_type+sizeof(socks5_request->address_type)+4,2);
		
	}
	else if(socks5_request->address_type==DOMAIN)
	{
		char domain_length=*(&socks5_request->address_type+sizeof(socks5_request->address_type));
		char target_domain[256]={0}; 
		strncpy(target_domain,&socks5_request->address_type+2,(unsigned int)domain_length); 
		printf("target:%s\n",target_domain); 
		struct hostent *phost=gethostbyname(target_domain);
		if(phost==NULL)
		{ 
			printf("Resolve%serror!\n",target_domain); 
			close(sock);
			
		} 
		memcpy(&sin.sin_addr,phost->h_addr_list[0],phost->h_length);
		memcpy(&sin.sin_port,&socks5_request->address_type+sizeof(socks5_request->address_type)+ sizeof(domain_length)+domain_length,2);
	}
	printf("RealServer:%s %d\n",inet_ntoa(sin.sin_addr),ntohs(sin.sin_port));
	char *str;	
	str=malloc(64);
    sprintf(str, "%s %d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));	
	return str;
	
}
