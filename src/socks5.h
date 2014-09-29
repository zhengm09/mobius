#ifndef SOCKS5_H
#define SOCKS5_H
#define VERSION 0x05
#define CONNECT 0x01
#define IPV4 0x01
#define DOMAIN 0x03
#define IPV6 0x04 
#define MAX_USER 10
#define BUFF_SIZE 1024
#define AUTH_CODE 0x00
#define TIME_OUT 6000000
typedef struct _method_select_response//Э�̷�����������Ӧ
{ 
	char version;	//������֧�ֵ�Socks�汾��0x04����0x05 
	char select_method;//������ѡ��ķ�����0x00Ϊ������0x02Ϊ������֤
}METHOD_SELECT_RESPONSE; 
typedef struct _method_select_request//Э�̷������������
{
	char version;//�ͻ���֧�ֵİ汾��0x04����0x05
	char number_methods;//�ͻ���֧�ֵķ��������� 
	char methods[255];//�ͻ���֧�ֵķ������ͣ����255����0x00Ϊ������0x02Ϊ������֤
}METHOD_SELECT_REQUEST; 
typedef struct _AUTH_RESPONSE//�û�������֤�������Ӧ
{ 
	char version;//�汾���˴��㶨Ϊ0x01 
	char result;//�������֤�����0x00Ϊ�ɹ���������Ϊʧ��
}AUTH_RESPONSE; 
typedef struct _AUTH_REQUEST//�û�������֤�ͻ�������
{ 
	char version;//�汾���˴��㶨Ϊ0x01 
	char name_len;//�������ֶ��û����ĳ��ȣ�һ���ֽڣ��Ϊ0xff
	char name[255];//�û��� 
	char pwd_len;//���ĸ��ֶ�����ĳ��ȣ�һ���ֽڣ��Ϊ0xff
	char pwd[255];//����
}AUTH_REQUEST; 
typedef struct _SOCKS5_RESPONSE//������ʵ������Socks�����������Ӧ
{ 
	char version;//������֧�ֵ�Socks�汾��0x04����0x05
	char reply;//���������������ʵ�����Ľ����0x00�ɹ�
	char reserved;//����λ���㶨λ0x00
	char address_type;//Socks����������󶨵ĵ�ַ���ͣ�IPV4Ϊ0x01,IPV6Ϊ0x04������Ϊ0x03 
	char address_port[1];//���address_typeΪ�������˴���һ�ֽ�Ϊ�������ȣ����Ϊ����������0�ַ���β,������ΪSocks����������󶨶˿�
}SOCKS5_RESPONSE; 
typedef struct _SOCKS5_REQUEST//�ͻ�������������ʵ����
{ 
	char version;//�ͻ���֧�ֵ�Socks�汾��0x04����0x05 
	char cmd;//�ͻ������CONNECTΪ0x01��BINDΪ0x02��UDPΪ0x03��һ��Ϊ0x01 
	char reserved;//����λ���㶨λ0x00 
	char address_type;//�ͻ����������ʵ�����ĵ�ַ���ͣ�IPV4Ϊ0x00,IPV6Ϊ0x04������Ϊ0x03
	char address_port[1];//���address_typeΪ�������˴���һ�ֽ�Ϊ�������ȣ����Ϊ����������0�ַ���β,������Ϊ��ʵ�����󶨶˿� 
}SOCKS5_REQUEST;
int SelectMethod(int sock);
int AuthPassword(int sock);
char *ParseCommand(int sock);

#endif