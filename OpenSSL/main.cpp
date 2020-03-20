#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <iostream>
#include <string>

#include <winsock2.h>

#include "openssl/rsa.h"      
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcrypto.lib")
/*****************************************************************
*SSL/TLS�ͻ��˳���WIN32��(��demos/cli.cppΪ����)
*��Ҫ�õ���̬���ӿ�libeay32.dll,ssleay.dll,
*ͬʱ��setting�м���ws2_32.lib libeay32.lib ssleay32.lib,
*���Ͽ��ļ��ڱ���openssl�����out32dllĿ¼���ҵ�,
*����֤���ļ������������������*/
char * wchar2char(const wchar_t* wchar)
{
	char * m_char;
	int len = WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), NULL, 0, NULL, NULL);
	m_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	return m_char;
}

wchar_t * char2wchar(const char* cchar)
{
	wchar_t *m_wchar;
	int len = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
	m_wchar = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), m_wchar, len);
	m_wchar[len] = '\0';
	return m_wchar;
}
std::string mb2u8(const std::string& cont)
{
	if (cont.empty())
	{
		return "";
	}
	int num = MultiByteToWideChar(CP_ACP, NULL, cont.c_str(), -1, NULL, NULL);
	if (num <= 0)
	{
		return "";
	}
	wchar_t* buffw = new (std::nothrow) wchar_t[num];
	if (NULL == buffw)
	{
		return "";
	}
	MultiByteToWideChar(CP_ACP, NULL, cont.c_str(), -1, buffw, num);
	int len = WideCharToMultiByte(CP_UTF8, 0, buffw, num - 1, NULL, NULL, NULL, NULL);
	if (len <= 0)
	{
		delete[] buffw;
		return "";
	}
	char* lpsz = new (std::nothrow) char[len + 1];
	if (NULL == lpsz)
	{
		delete[] buffw;
		return "";
	}
	WideCharToMultiByte(CP_UTF8, 0, buffw, num - 1, lpsz, len, NULL, NULL);
	lpsz[len] = '\0';
	delete[] buffw;
	std::string rtn(lpsz);
	delete[] lpsz;
	return rtn;
}

std::string u82mb(const std::string& cont)
{
	if (cont.empty())
	{
		return "";
	}

	int num = MultiByteToWideChar(CP_UTF8, NULL, cont.c_str(), -1, NULL, NULL);
	if (num <= 0)
	{
		return "";
	}
	wchar_t* buffw = new (std::nothrow) wchar_t[num];
	if (NULL == buffw)
	{
		return "";
	}
	MultiByteToWideChar(CP_UTF8, NULL, cont.c_str(), -1, buffw, num);
	int len = WideCharToMultiByte(CP_ACP, 0, buffw, num - 1, NULL, NULL, NULL, NULL);
	if (len <= 0)
	{
		delete[] buffw;
		return "";
	}
	char* lpsz = new (std::nothrow) char[len + 1];
	if (NULL == lpsz)
	{
		delete[] buffw;
		return "";
	}
	WideCharToMultiByte(CP_ACP, 0, buffw, num - 1, lpsz, len, NULL, NULL);
	lpsz[len] = '\0';
	delete[] buffw;
	std::string rtn(lpsz);
	delete[] lpsz;
	return rtn;
}

#if 1


/*������Ҫ�Ĳ�����Ϣ���ڴ˴���#define����ʽ�ṩ*/
#define CERTF  "client.crt"  /*�ͻ��˵�֤��(�辭CAǩ��)*/
#define KEYF  "client.key"   /*�ͻ��˵�˽Կ(������ܴ洢)*/
#define CACERT "ca.crt"      /*CA ��֤��*/
#define PORT   443          /*����˵Ķ˿�*/
#define SERVER_ADDR "106.11.208.151"  /*����ε�IP��ַ*/

#define CHK_NULL(x) if ((x)==NULL) exit (-1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(-2); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(-3); }

int main()
{
	int            err;
	int            sd;
	struct sockaddr_in sa;
	SSL_CTX*        ctx;
	SSL*            ssl;
	X509*            server_cert;
	char*            str;
	char            buf[4096];
	const SSL_METHOD    *meth;
	int            seed_int[100]; /*����������*/

	WSADATA        wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup()fail:%d/n", GetLastError());
		return -1;
	}

	/*��ʼ��*/
	SSL_library_init();
	OpenSSL_add_ssl_algorithms();
	/*Ϊ��ӡ������Ϣ��׼��*/
	SSL_load_error_strings();

	/*����ʲôЭ��(SSLv2/SSLv3/TLSv1)�ڴ�ָ��*/
	meth = TLSv1_2_client_method();
	/*����SSL�Ự����*/
	ctx = SSL_CTX_new(meth);
	CHK_NULL(ctx);
#if 0
	/*��֤���,�Ƿ�Ҫ��֤�Է�*/
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	/*����֤�Է�,�����CA֤��*/
	SSL_CTX_load_verify_locations(ctx, CACERT, NULL);

	/*�����Լ���֤��*/
	if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(-2);
	}

	/*�����Լ���˽Կ,������ǩ��*/
	if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(-3);
	}
	/*��������������������,����һ���Լ���֤����˽Կ�Ƿ����*/
	if (!SSL_CTX_check_private_key(ctx))
	{
		printf("Private key does not match the certificate public key/n");
		exit(-4);
	}

	/*������������ɻ���,WIN32ƽ̨����*/
	srand((unsigned)time(NULL));
	for (int i = 0; i < 100; i++)
		seed_int[i] = rand();
	RAND_seed(seed_int, sizeof(seed_int));
#endif
	/*������������TCP socket�������� .............................. */
	printf("Begin tcp socket...\n");

	sd = socket(AF_INET, SOCK_STREAM, 0);
	CHK_ERR(sd, "socket");

	memset(&sa, '/0', sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(SERVER_ADDR);   /* Server IP */
	sa.sin_port = htons(PORT);          /* Server Port number */

	err = connect(sd, (struct sockaddr*) &sa, sizeof(sa));
	CHK_ERR(err, "connect");

	/* TCP �����ѽ���.��ʼ SSL ���ֹ���.......................... */
	printf("Begin SSL negotiation \n");

	/*����һ��SSL�׽���*/
	ssl = SSL_new(ctx);
	CHK_NULL(ssl);

	/*�󶨶�д�׽���*/
	SSL_set_fd(ssl, sd);
	err = SSL_connect(ssl);
	CHK_SSL(err);

	/*��ӡ���м����㷨����Ϣ(��ѡ)*/
	printf("SSL connection using %s\n", SSL_get_cipher(ssl));

	/*�õ�����˵�֤�鲢��ӡЩ��Ϣ(��ѡ) */
	/*server_cert = SSL_get_peer_certificate(ssl);
	CHK_NULL(server_cert);
	printf("Server certificate:/n");
	str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
	CHK_NULL(str);
	printf("/t subject: %s/n", str);
	Free(str);
	str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
	CHK_NULL(str);
	printf("/t issuer: %s/n", str);
	Free(str);*/

	//X509_free(server_cert);  /*�粻����Ҫ,�轫֤���ͷ� */

	/* ���ݽ�����ʼ,��SSL_write,SSL_read����write,read */
	//printf("Begin SSL data exchange/n");

		//https://passport.alibaba.com
		std::string strWrite =

		"GET /member/reg/fast/fast_reg.htm?_regfrom=SMCQ&_isUpgrade=false HTTP/1.1\r\n"
		"Host: passport.alibaba.com\r\n"
		"Connection: keep-alive\r\n"
		"Upgrade-Insecure-Requests: 1\r\n"
		"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 Safari/537.36\r\n"
		"Sec-Fetch-Dest: document\r\n"
		"Sec-Fetch-Site: none\r\n"
		"Sec-Fetch-Mode: navigate\r\n"
		"Sec-Fetch-User: ?1\r\n"
		"Accept-Encoding: gzip, deflate, br\r\n"
		"Accept-Language: zh-CN,zh;q=0.9\r\n"
		"\r\n";

	INT iErrorWrite = SSL_write(ssl, strWrite.c_str(), strWrite.length()) < 0;
	if (iErrorWrite < 0)
	{
		std::wcout << "Error SSL_write" << std::endl;
		return -1;
	}
	/*err = SSL_write(ssl, "Hello World!", strlen("Hello World!"));
	CHK_SSL(err);*/


	int sumbit = 0;
	char* lpszRead = new CHAR[8196];
		int iLength = 8196;
		while (iLength == 8196)
		{
			iLength = SSL_read(ssl, lpszRead, 8196);
			if (iLength < 0)
			{
				std::wcout << "Error SSL_read" << std::endl;
				delete[] lpszRead;
				return -1;
			}
			sumbit += iLength;
			//lpszRead[iLength] = TEXT('\0');
			//std::cout << u82mb(std::string(lpszRead, iLength));// char2wchar(lpszRead);
		}
		printf("�յ�%08d�ֽ�\n", sumbit);
		delete[] lpszRead;

	/*err = SSL_read(ssl, buf, sizeof(buf) - 1);
	CHK_SSL(err);

	buf[err] = '/0';
	printf("Got %d chars:'%s'/n", err, buf);*/
	SSL_shutdown(ssl);  /* send SSL/TLS close_notify */

	/* ��β���� */
	shutdown(sd, 2);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	return 0;
}
/*****************************************************************
* EOF - cli.cpp
*****************************************************************/
#else

/*****************************************************************
*SSL/TLS����˳���WIN32��(��demos/server.cppΪ����)
*��Ҫ�õ���̬���ӿ�libeay32.dll,ssleay.dll,
*ͬʱ��setting�м���ws2_32.lib libeay32.lib ssleay32.lib,
*���Ͽ��ļ��ڱ���openssl�����out32dllĿ¼���ҵ�,
*����֤���ļ������������������.
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>

#include <winsock2.h>

#include "openssl/rsa.h"      
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

/*������Ҫ�Ĳ�����Ϣ���ڴ˴���#define����ʽ�ṩ*/
#define CERTF   "server.crt" /*����˵�֤��(�辭CAǩ��)*/
#define KEYF   "server.key"  /*����˵�˽Կ(������ܴ洢)*/
#define CACERT "ca.crt" /*CA ��֤��*/
#define PORT   1111   /*׼���󶨵Ķ˿�*/

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

int main2()
{
	int err;
	int listen_sd;
	int sd;
	struct sockaddr_in sa_serv;
	struct sockaddr_in sa_cli;
	int client_len;
	SSL_CTX* ctx;
	SSL*     ssl;
	X509*    client_cert;
	char*    str;
	char     buf[4096];
	const SSL_METHOD *meth;
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup()fail:%d/n", GetLastError());
		return -1;
	}

	SSL_load_error_strings();            /*Ϊ��ӡ������Ϣ��׼��*/
	OpenSSL_add_ssl_algorithms();        /*��ʼ��*/
	meth = TLSv1_server_method();  /*����ʲôЭ��(SSLv2/SSLv3/TLSv1)�ڴ�ָ��*/

	ctx = SSL_CTX_new(meth);
	CHK_NULL(ctx);

	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);   /*��֤���*/
	SSL_CTX_load_verify_locations(ctx, CACERT, NULL); /*����֤,�����CA֤��*/

	if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(4);
	}

	if (!SSL_CTX_check_private_key(ctx)) {
		printf("Private key does not match the certificate public key/n");
		exit(5);
	}

	SSL_CTX_set_cipher_list(ctx, "RC4-MD5");

	/*��ʼ������TCP socket����.................................*/
	printf("Begin TCP socket.../n");

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	CHK_ERR(listen_sd, "socket");

	memset(&sa_serv, '/0', sizeof(sa_serv));
	sa_serv.sin_family = AF_INET;
	sa_serv.sin_addr.s_addr = INADDR_ANY;
	sa_serv.sin_port = htons(PORT);

	err = bind(listen_sd, (struct sockaddr*) &sa_serv,

		sizeof(sa_serv));

	CHK_ERR(err, "bind");

	/*����TCP����*/
	err = listen(listen_sd, 5);
	CHK_ERR(err, "listen");

	client_len = sizeof(sa_cli);
	sd = accept(listen_sd, (struct sockaddr*) &sa_cli, &client_len);
	CHK_ERR(sd, "accept");
	closesocket(listen_sd);

	printf("Connection from %lx, port %x/n",
		sa_cli.sin_addr.s_addr, sa_cli.sin_port);

	/*TCP�����ѽ���,���з���˵�SSL����. */
	printf("Begin server side SSL/n");

	ssl = SSL_new(ctx);
	CHK_NULL(ssl);
	SSL_set_fd(ssl, sd);
	err = SSL_accept(ssl);
	printf("SSL_accept finished/n");
	CHK_SSL(err);

	/*��ӡ���м����㷨����Ϣ(��ѡ)*/
	printf("SSL connection using %s/n", SSL_get_cipher(ssl));

	/*�õ�����˵�֤�鲢��ӡЩ��Ϣ(��ѡ) */
	//client_cert = SSL_get_peer_certificate(ssl);
	//if (client_cert != NULL) {
	//	printf("Client certificate:/n");
	//	str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
	//	CHK_NULL(str);
	//	printf("/t subject: %s/n", str);
	//	Free(str);
	//	str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
	//	CHK_NULL(str);
	//	printf("/t issuer: %s/n", str);
	//	Free(str);
	//	X509_free(client_cert);/*�粻����Ҫ,�轫֤���ͷ� */
	//}
	//else
	//	printf("Client does not have certificate./n");
	/* ���ݽ�����ʼ,��SSL_write,SSL_read����write,read */
	err = SSL_read(ssl, buf, sizeof(buf) - 1);
	CHK_SSL(err);
	buf[err] = '/0';
	printf("Got %d chars:'%s'/n", err, buf);

	err = SSL_write(ssl, "I hear you.", strlen("I hear you."));
	CHK_SSL(err);

	/* ��β����*/
	shutdown(sd, 2);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	return 0;
}
/*****************************************************************
* EOF - serv.cpp
*****************************************************************/
#endif
//#include<tchar.h>
//#include<WinSock2.h>
//#include<WS2tcpip.h>
//#include<iostream>
//#include<openssl\ssl.h>
//#include <string>
//#pragma comment(lib,"ws2_32.lib")
//#pragma comment(lib,"libssl.lib")
//#pragma comment(lib,"libcrypto.lib")
//
//CONST INT RECV_SIZE = 8192;
//

//INT _tmain(INT argc, LPTSTR argv[])
//{
//	//����wsa
//	WSADATA wsadData;
//	WSAStartup(MAKEWORD(2, 2), &wsadData);
//
//	//��ȡHost��IP��ַ����Ϣ
//	ADDRINFOT aiHints;
//	ZeroMemory(&aiHints, sizeof(ADDRINFOT));
//	aiHints.ai_family = AF_INET;
//	aiHints.ai_flags = AI_PASSIVE;
//	aiHints.ai_protocol = 0;
//	aiHints.ai_socktype = SOCK_STREAM;
//	std::string strHost = TEXT("passport.alibaba.com");
//	PADDRINFOT paiResult;
//	GetAddrInfo(strHost.c_str(), NULL, &aiHints, &paiResult);
//
//	//�����׽���
//	SOCKET sSocket = socket(AF_INET, SOCK_STREAM, 0);
//	if (sSocket == SOCKET_ERROR)
//	{
//		std::wcout << "Error socket" << std::endl;
//		return -1;
//	}
//
//	//����Host
//	SOCKADDR_IN sinHost;
//	sinHost.sin_addr = ((LPSOCKADDR_IN)paiResult->ai_addr)->sin_addr;
//	//sinHost.sin_addr.S_un.S_addr = inet_addr("47.107.117.56");
//	//sinHost.sin_port = htons(888);
//	sinHost.sin_family = AF_INET;
//	sinHost.sin_port = htons(443);
//	if (connect(sSocket, (LPSOCKADDR)&sinHost, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
//	{
//		std::cout << "Error connect" << std::endl;
//		return -1;
//	}
//
//	//��ʼ��OpenSSL��
//	//����Ȼ��֪��Ϊʲô�����ǲ����������ƺ������ᵼ��ʲô���⣬�ڲ�����3�е�����²����˼�����վ��û�з����κ���������
//	SSL_library_init();
//	SSLeay_add_ssl_algorithms();
//	SSL_load_error_strings();
//
//	//����SSL�Ự������
//	SSL_CTX *pctxSSL = SSL_CTX_new(TLSv1_2_client_method());
//	if (pctxSSL == NULL)
//	{
//		std::cout << "Error SSL_CTX_new" << std::endl;
//		return -1;
//	}
//	SSL *psslSSL = SSL_new(pctxSSL);
//	if (psslSSL == NULL)
//	{
//		std::cout << "Error SSL_new" << std::endl;
//		return -1;
//	}
//	SSL_set_fd(psslSSL, sSocket);
//	INT iErrorConnect = SSL_connect(psslSSL);
//	if (iErrorConnect < 0)
//	{
//		std::cout << "Error SSL_connect, iErrorConnect=" << iErrorConnect << std::endl;
//		return -1;
//	}
//	std::wcout << "SSL connection using " << SSL_get_cipher(psslSSL) << std::endl;
//
//	//����
//	std::string strWrite =
//
//		"GET https://passport.alibaba.com/member/reg/fast/fast_reg.htm?_regfrom=SMCQ&_isUpgrade=false HTTP/1.1\r\n"
//		"Host: passport.alibaba.com\r\n"
//		"Connection: keep-alive\r\n"
//		"Upgrade-Insecure-Requests: 1\r\n"
//		"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 Safari/537.36\r\n"
//		"Sec-Fetch-Dest: document\r\n"
//		"Sec-Fetch-Site: none\r\n"
//		"Sec-Fetch-Mode: navigate\r\n"
//		"Sec-Fetch-User: ?1\r\n"
//		"Accept-Encoding: gzip, deflate, br\r\n"
//		"Accept-Language: zh-CN,zh;q=0.9\r\n"
//		"\r\n";
//
//	INT iErrorWrite = SSL_write(psslSSL, strWrite.c_str(), strWrite.length()) < 0;
//	if (iErrorWrite < 0)
//	{
//		std::wcout << "Error SSL_write" << std::endl;
//		return -1;
//	}
//
//
//	////���Ͷ���
//	//strWrite = "GET https://passport.alibaba.com/member/checkcode/refactor/send_email_verification.do?type=EnterpriseRegistration&email=zgr110064%40163.com HTTP/1.1\r\n"
//	//	"Referer: https://passport.alibaba.com/member/reg/fast/fast_reg.htm?_regfrom=SMCQ&_isUpgrade=false\r\n"
//	//	"Host: passport.alibaba.com\r\n"
//	//	"Connection: Keep - Alive\r\n"
//	//	"\r\n";
//	//iErrorWrite = SSL_write(psslSSL, strWrite.c_str(), strWrite.length()) < 0;
//	//if (iErrorWrite < 0)
//	//{
//	//	std::wcout << "Error SSL_write" << std::endl;
//	//	return -1;
//	//}
//	//strWrite = "POST https://passport.alibaba.com/member/ultron/page_refresh.do?_input_charset=utf-8 HTTP/1.1\r\n"
//	//	"Host: passport.alibaba.com\r\n"
//	//	"Connection: keep-alive\r\n"
//	//	//"Content-Length: 2160\r\n"
//	//	"Accept: application/json, text/javascript\r\n"
//	//	"Sec-Fetch-Dest: empty\r\n"
//	//	"Content-Type: application/x-www-form-urlencoded\r\n"
//	//	"Origin: https://passport.alibaba.com\r\n"
//	//	"Referer: https://passport.alibaba.com/member/reg/fast/fast_reg.htm?_regfrom=SMCQ&_isUpgrade=false \r\n"
//	//	//"Accept-Encoding: gzip, deflate, br\r\n"
//	//	"Accept-Language: zh-CN,zh;q=0.9\r\n"
//	//"device=PC&showCountry=false&mobileArea=&regFrom=SMCQ&ncoSig=05zmxF2o9jWv9ZXXKag70cNMAJJpOs5DxGhZSf08rUDQQsDZuJ0vnkXyv7HyO3if3TNwLHRkNKExSA0unEyk04er3onG349s63Wzn8YJpemaMleyXDWQ604EHe7bCx9lDPV-R7rsWABzjGHJLCFRQUuU5JrqRr4qT4RqKwDXt_jsbHk8Wc7X4cyEks4IXYK14sOrPzNjfZPVjyWFNemcU5lAwCUJ_brF70clsyqrtY3UbGh8Z9IlLwxpt793B3XmJTmqboeQJ1yFJLyrNi2sXdWJ-J0LlTG3BLKmOIybjsfjHaB-9YMNiXltrEeWe4qbu2N5czeN1SXhg4FExcmVkicj7vOPbqKlukjDjBtnoOIqxCdEmmOoJM7CcgusFvksDSpmQvOFg1n_tYHxBng4JfqQ&ncoSessionid=01jA2ddCH1U5NfnXxbVqTX2AIKo1zPxM--ICRUwHxBN-3hVeOTNumhj6MWT94sFya8N8HPWMx6QlMWV-Nw9LKhFln6-0AXmw2z9nEi93BnEESq2K7xW6OMTw86GHdRpzXH1Cc_wnszpNFX8H6m1NzV3H1uWsoswSwPHIca11tLxwc&ncoToken=88fa6793c67ca65a710724d2675cf515&mobile=&ua=122%23GU2j1J0lEEaWdJpZMEpJEJponDJE7SNEEP7rEJ%2B%2Ff9t%2F2oQLpo7iEDpWnDEeK51HpyGZp9hBuDEEJFOPpC76EJponDJL7gNpEPXZpJRgu4Ep%2BFQLpoGUEELWn4yP7SQEEyuLpERdILf%2FprZCnaRx9kb%2FowQdKnoojYTYKLtvHyCY78n2aE8lbjSlQR5A1B421eAsbvaYNg30JKWtw7P76fPY4qoAbumrzYm8nHXA7QaH3J%2FRzW%2B2WoEShrCO9R8aS400opLYs5VDqMfEeYhj3wGJuOIEE8%2FmdeBY8zbbyFfmqMf2ENpC3VDb5y0EDLXT8BLUJNI9UV4n7W3bD93xnSL1elWEELXZ8oL6JNEEyBfDqMAbEEpangL4ul0EDLVr8CpUJ4bEyF3mqWivDEpx7Sf1uO0EoL34HBWHunvLh4mNnH42dV9LcStVd6v6wFgyjJvu5vS0mNgRwZyPy6aiFK8P3x7hGwsIVdMd5sfvPKa%2BqyTf6GgwzXd%2FwmL62lRASpybLRRc%2FBxOZ8us0Qbx3e9qt4s3ETBT2TQ0Jh2VPoQ83o9QbfmwA6S0sLs0NKLbYn23vdzlpheZLr3tXVwF3hrPmyhO%2FLNyzWG%2BmiZv8C7ytHOPBcQKEDdKzFjiYpFWpDFy0%2FTHy4AwDRnAcZqfJYB8GJLD1CLuviwgUBNsA%2B6G%2BnrJ5RvHRKUXEng5id194BJ%2FbPadjvv7EvJ%2FZIciofJE4SFG5gVnm0djvU1XhGdPDRoih094fShRUROTFgkDuIuiC7t66tCHpz4zit5zPK0ycRcMf1G0jFt6j39KjPoNPTXEkAgjy74vuHiyeO1o%2B0sXeW865efaYvGSaskNjHtsi5mZxsDMNsdhMZ%2FLirEBZdhUia8AAzILOm8Xgv22J08eB9VSRkAFybYx%2F3oRPyvSL3PGQ4FVnj3IflX2LHYLTJOI2LRV%2BTPkhNq%2BQcgG5eB3iSpjCu8RHKmngJpC9zES6Sv7zfpCh8AYfaKigB1aBIx%2BMhGK2udskVn8X1Xx%2Fcg6Z1ym3mFUZZAgG6o7iKaiOfQ9WAmTvhm7uTy1vB1zRR81tmlCzAcgJvDesCoQ5lPPy%2FIEGSESeh07%2BT1KRE%3D%3D&country=OT&tradeRole=1&regType=1&email=jbmow23%40163.com&password=aa120120&rePassword=aa120120&checkbox=true\r\n\r\n"
//	//	"\r\n";
//	//iErrorWrite = SSL_write(psslSSL, strWrite.c_str(), strWrite.length()) < 0;
//	//if (iErrorWrite < 0)
//	//{
//	//	std::wcout << "Error SSL_write" << std::endl;
//	//	return -1;
//	//}
//	
//
//	//�հ������
//	//������ܵ���char��ʽ�ģ��������Ļ�����
//	//���Ҫ������ʾ���ģ���Ҫ��ת��Ϊwchar_t��std::wstring
//	char* lpszRead = new CHAR[RECV_SIZE];
//	int iLength = 1;
//	while (iLength)
//	{
//		iLength = SSL_read(psslSSL, lpszRead, RECV_SIZE - 1);
//		if (iLength < 0)
//		{
//			std::wcout << "Error SSL_read" << std::endl;
//			delete[] lpszRead;
//			return -1;
//		}		
//		//lpszRead[iLength] = TEXT('\0');
//		std::cout << u82mb(std::string(lpszRead, iLength));// char2wchar(lpszRead);
//	}
//
//	delete[] lpszRead;
//
//	return 0;
//}