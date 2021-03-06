/*
 * Project: udptunnel
 * File: message.h
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

#ifndef MESSAGE_H
#define MESSAGE_H

#ifndef WIN32
#include <inttypes.h>
#include <arpa/inet.h>
#endif /*WIN32*/

#include "common.h"
#include "socket.h"

#define MSG_MAX_LEN 1448 /* max bytes to send in body of message (16 bits) */
#define MSG_BUF_LEN 1448
#define KEEP_ALIVE_SECS 60
#define KEEP_ALIVE_TIMEOUT_SECS (7*60+1) /* has 7 tries to send a keep alive */

/* Message types: max 8 bits */
#define MSG_TYPE_GOODBYE   0x01
#define MSG_TYPE_HELLO     0x02
#define MSG_TYPE_HELLOACK  0x03
#define MSG_TYPE_KEEPALIVE 0x04
#define MSG_TYPE_DATA0     0x05
#define MSG_TYPE_DATA1     0x06
#define MSG_TYPE_ACK0      0x07
#define MSG_TYPE_RELAYKEY  0x08
#define MSG_TYPE_REG       0x09
#define MSG_TYPE_REGACK    0x0A
#define MSG_TYPE_REQ	   0x0B
#define MSG_TYPE_REQACK	   0x0C
#define MSG_TYPE_RELAYREQ  0x0D
#define MSG_TYPE_RELAYACK  0x0E

#ifndef WIN32
struct msg_hdr
{
    uint16_t client_id;
    uint8_t type;
    uint16_t length;
	uint32_t sourceid;
} __attribute__ ((__packed__));
#else
#pragma pack(push, 1)
struct msg_hdr
{
    uint16_t client_id;
    uint8_t type;
    uint16_t length;
};
#pragma pack(pop)
#endif /*WIN32*/

typedef struct msg_hdr msg_hdr_t;

typedef struct data_buf
{
    char buf[MSG_BUF_LEN];
    int len;
	struct timeval time;
} data_buf_t;

int msg_send_msg(socket_t *to, uint16_t client_id,uint32_t sourceid, uint8_t type,
                 char *data, int data_len);
int msg_send_reg(socket_t *to, char *host, char *port,uint16_t req_id, uint32_t sourceid);
int msg_send_req(socket_t *to, char *host, char *port,uint16_t req_id, uint32_t sourceid);
int msg_send_relaykey(socket_t *to,uint32_t sourceid, char * key);
int msg_send_regrelay(socket_t *to, char *host, char *port,uint16_t req_id, uint32_t localid,uint32_t sourceid);
int msg_send_reqrelay(socket_t *to, char *host, char *port,uint16_t req_id, uint32_t localid,uint32_t sourceid,uint16_t relays);
int msg_send_hello(socket_t *to, char *host, uint16_t req_id,uint32_t localid,char * rsakey);
int msg_send_helloack(socket_t *to,char * key,uint16_t req_id,uint16_t client_id,uint32_t sourceid,char * rsakey);
int msg_send_reqack(socket_t *to, char *host, uint16_t req_id,uint32_t targetid,uint32_t sourceid);
int msg_send_relayack(socket_t *to, char *host, uint16_t req_id,uint32_t tunnelid,uint32_t sourceid,uint8_t exp);
int msg_recv_msg(socket_t *sock, 
                 char *data, int data_len,
                 uint16_t *client_id, uint8_t *type, uint16_t *length, uint32_t *sourceid);
unsigned char *crypt(char *data, int data_len,char * k,int crypt);
/* Inline functions for working with the message header struct */
static _inline_ void msg_init_header(msg_hdr_t *hdr, uint16_t client_id,uint32_t sourceid,
                                   uint8_t type, uint16_t len)
{
    hdr->client_id = htons(client_id);
    hdr->type = type;
    hdr->length = htons(len);
	hdr->sourceid= htonl(sourceid);
}

static _inline_ uint16_t msg_get_client_id(msg_hdr_t *h)
{
    return ntohs(h->client_id);
}


static _inline_ uint8_t msg_get_type(msg_hdr_t *h)
{
    return h->type;
}

static _inline_ uint16_t msg_get_length(msg_hdr_t *h)
{
    return ntohs(h->length);
}

static _inline_ uint32_t msg_get_sourceid(msg_hdr_t *h)
{
    return ntohl(h->sourceid);
}

#endif /* MESSAGE_H */
