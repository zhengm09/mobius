/***************************************************************************
 *            crypt.h
 *
 *  Fri August 15 01:52:46 2014
 *  Copyright  2014  John Doe
 *  <user@host>
 ****************************************************************************/

#include <inttypes.h>
#include <sys/socket.h>
#include <openssl/rsa.h>
#include <openssl/des.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#define BUFFSIZE 1448

char* my_rsaencrypt(char *str,char *path_key);//加密
char* my_rsadecrypt(char *str,char *path_key);//解密
unsigned char *desencrypt(char * data, int data_len,char * k);
char *desdecrypt(char * data, int data_len,char * k);
void generate_rsakey(char * filename);