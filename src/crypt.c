/***************************************************************************
 *            crypt.c
 *
 *  Fri August 15 01:52:46 2014
 *  Copyright  2014  John Doe
 *  <user@host>
 ****************************************************************************/

#include "crypt.h"
#include "common.h"

extern int debug_level;

char* my_rsaencrypt(char *str,char *path_key);//加密
char* my_rsadecrypt(char *str,char *path_key);//解密
unsigned char *desencrypt(char * data, int data_len,char * k);
char *desdecrypt(char * data, int data_len,char * k);
void generate_rsakey(char * filename);


char *my_rsaencrypt(char *str,char *path_key){
	char *p_en;
	RSA *p_rsa;
	FILE *file;
	int rsa_len;
	//if(debug_level >= DEBUG_LEVEL2)
	//	printf("encrypt %s \n",path_key);
	if((file=fopen(path_key,"r"))==NULL){
		perror("open key file error");
		return NULL;    
	}   
	if((p_rsa=PEM_read_RSA_PUBKEY(file,NULL,NULL,NULL))==NULL){
		//if((p_rsa=PEM_read_RSAPublicKey(file,NULL,NULL,NULL))==NULL){   换成这句死活通不过，无论是否将公钥分离源文件
		ERR_print_errors_fp(stdout);
		return NULL;
	}   
	//printf("mark \n");	
	rsa_len=RSA_size(p_rsa);
	p_en=(unsigned char *)malloc(rsa_len+1);
	memset(p_en,0,rsa_len+1);
	//printf("str %s",str);
	if(RSA_public_encrypt(rsa_len,(unsigned char *)str,(unsigned char*)p_en,p_rsa,RSA_NO_PADDING)<0){
		return NULL;
	}
	RSA_free(p_rsa);
	fclose(file);
	return p_en;
}
char *my_rsadecrypt(char *str,char *path_key){
	char *p_de;
	RSA *p_rsa;
	FILE *file;
	int rsa_len;
	//if(debug_level >= DEBUG_LEVEL2)
	//	printf("decrypt %s \n",path_key);
	if((file=fopen(path_key,"r"))==NULL){
		perror("open key file error");
		return NULL;
	}
	if((p_rsa=PEM_read_RSAPrivateKey(file,NULL,NULL,NULL))==NULL){
		ERR_print_errors_fp(stdout);
		return NULL;
	}
	rsa_len=RSA_size(p_rsa);
	p_de=(unsigned char *)malloc(rsa_len+1);
	memset(p_de,0,rsa_len+1);
	if(RSA_private_decrypt(rsa_len,(unsigned char *)str,(unsigned char*)p_de,p_rsa,RSA_NO_PADDING)<0){
		return NULL;
	}
	RSA_free(p_rsa);
	fclose(file);
	return p_de;
}

char *desdecrypt(char * data, int data_len,char * k)
{
	int docontinue = 1;      


	unsigned char *src = NULL; /* 补齐后的明文 */   
	unsigned char *dst = NULL; /* 解密后的明文 */   

	unsigned char tmp[8];   
	unsigned char in[8];   

	int key_len;   
#define LEN_OF_KEY 24   
	unsigned char key[LEN_OF_KEY]; /* 补齐后的密钥 */   
	unsigned char block_key[9];   
	DES_key_schedule ks,ks2,ks3;   
	/* 构造补齐后的密钥 */   
	key_len = strlen(k);   
	memcpy(key, k, key_len);   
	memset(key + key_len, 0x00, LEN_OF_KEY - key_len);   
	/* 分析补齐明文所需空间及补齐填充数据 */   	

	src = (unsigned char *)malloc(data_len);   
	dst = (unsigned char *)malloc(data_len);   
	if (NULL == src || NULL == dst)   
	{   
		docontinue = 0;   
	}   
	if (docontinue)   
	{   
		int count;   
		int i;   
		/* 构造补齐后的加密内容 */   
		memset(src, 0, data_len);   
		memcpy(src, data, data_len);   

		/* 密钥置换 */   
		memset(block_key, 0, sizeof(block_key));   
		memcpy(block_key, key + 0, 8);   
		DES_set_key_unchecked((const_DES_cblock*)block_key, &ks);   
		memcpy(block_key, key + 8, 8);   
		DES_set_key_unchecked((const_DES_cblock*)block_key, &ks2);   
		memcpy(block_key, key + 16, 8);   
		DES_set_key_unchecked((const_DES_cblock*)block_key, &ks3);  		 


		count = data_len / 8;   
		for (i = 0; i < count; i++)   
		{   
			memset(tmp, 0, 8);   
			memset(in, 0, 8);   

			memcpy(tmp, src + 8 * i, 8);   
			/* 加密 */   
			DES_ecb3_encrypt((const_DES_cblock*)tmp, (DES_cblock*)in, &ks, &ks2, &ks3,  DES_DECRYPT);			
			/* 将解密的内容拷贝到解密后的明文 */   
			memcpy(dst + 8 * i, in, 8);   
		}
	}   

	if (NULL != src)   
	{   
		free(src);  
		src = NULL;   
	}   

	return dst;   
}  
unsigned char *desencrypt(char * data, int data_len,char * k)
{
	int docontinue = 1;      
	int data_rest;   
	unsigned char ch;   
	unsigned char *src = NULL; /* 补齐后的明文 */   
	unsigned char *dst = NULL; /* 解密后的明文 */   
	int len;   
	unsigned char tmp[8];   
	unsigned char in[8];   
	unsigned char out[8]; 	
	int key_len;   
#define LEN_OF_KEY 24   
	unsigned char key[LEN_OF_KEY]; /* 补齐后的密钥 */   
	unsigned char block_key[9];   
	DES_key_schedule ks,ks2,ks3;   
	/* 构造补齐后的密钥 */   
	key_len = strlen(k);   
	memcpy(key, k, key_len);   
	memset(key + key_len, 0x00, LEN_OF_KEY - key_len);   
	/* 分析补齐明文所需空间及补齐填充数据 */   	 

	data_rest = data_len % 8;  
	if (data_rest==0)
		len=data_len;		
	else
		len = data_len + (8 - data_rest);   
	//ch = 8 - data_rest; 
	//printf("data_len %d,data_rest %d,ch %x \n",data_len,data_rest,ch);

	src = (unsigned char *)malloc(len);   
	dst = (unsigned char *)malloc(len);   
	if (NULL == src || NULL == dst)   
	{   
		docontinue = 0;   
	}   
	if (docontinue)   
	{   
		int count;   
		int i;   
		/* 构造补齐后的加密内容 */   
		memset(src, 0, len);   
		memcpy(src, data, data_len);
		if (data_rest!=0)
			memset(src + data_len, 0, 8 - data_rest);  
		/* 密钥置换 */   
		memset(block_key, 0, sizeof(block_key));   
		memcpy(block_key, key + 0, 8);   
		DES_set_key_unchecked((const_DES_cblock*)block_key, &ks);   
		memcpy(block_key, key + 8, 8);   
		DES_set_key_unchecked((const_DES_cblock*)block_key, &ks2);   
		memcpy(block_key, key + 16, 8);   
		DES_set_key_unchecked((const_DES_cblock*)block_key, &ks3);  		 


		count = len / 8;   
		for (i = 0; i < count; i++)   
		{   
			memset(tmp, 0, 8);   
			memset(in, 0, 8);   

			memcpy(tmp, src + 8 * i, 8);   
			/* 加密 */   
			DES_ecb3_encrypt((const_DES_cblock*)tmp, (DES_cblock*)in, &ks, &ks2, &ks3,  DES_ENCRYPT);			
			/* 将解密的内容拷贝到解密后的明文 */   
			memcpy(dst + 8 * i, in, 8);   
		}
	}   
	if (NULL != src)   
	{   
		free(src);  
		src = NULL;   
	}   	  
	return dst;   
}  

void generate_rsakey(char * filename)
{ 
	RSA *rsa=NULL;	
	char *keyfile=malloc(64);
	sprintf(keyfile,"%s_pub.key",filename);
	FILE *fp;
	if((fp=fopen(keyfile,"rt"))==NULL)
	{
		if(debug_level >= DEBUG_LEVEL3)
			printf("\n %s need to create",keyfile);
		rsa=RSA_new();
		//printf("正在产生RSA密钥...\n");
		BIGNUM *bn = BN_new();
		BN_set_word(bn, RSA_F4); 
		int ret;

		ret= RSA_generate_key_ex(rsa,1024, bn, NULL);
		if(ret<1)
		{
			printf("gen rsa error\n");
			exit(-1);
		}
		// 公钥
		BIO *bp = BIO_new(BIO_s_file());
		if(BIO_write_filename(bp, keyfile)<=0)
		{
			printf("write error\n");
			exit(-1);
		}
		if(PEM_write_bio_RSA_PUBKEY(bp, rsa)!=1)
		{
			printf("write public key error\n");
			exit(-1);
		}  
		//printf("保存公钥成功\n");
		BIO_free_all(bp);


		memset(keyfile,0,64);
		sprintf(keyfile,"%s.key",filename);
		// 私钥
		bp = BIO_new_file(keyfile, "w+");
		if(PEM_write_bio_RSAPrivateKey(bp, rsa, 0, NULL, 0, NULL, NULL)!=1)
		{
			printf("write public key error\n");
			exit(-1);
		}
		//printf("保存私钥成功\n");
		BIO_free_all(bp); 
	}
	 else
	   fclose(fp);
}
