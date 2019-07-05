#ifndef __HN_SOCK_FUNCTION__HEADER__
#define __HN_SOCK_FUNCTION__HEADER__

#include <arpa/inet.h>

#ifndef HN_MAX_SOCK_BUFFER
    #define HN_MAX_SOCK_BUFFER 1024
#endif

#ifndef HN_IP_SIZE
    #define HN_IP_SIZE 30
#endif

#ifndef SOCKADDR 
    #define SOCKADDR struct sockaddr
#endif

#ifndef SOCKADDR_IN
    #define SOCKADDR_IN struct sockaddr_in
#endif

// *********************************************************
// Define Error List
// *********************************************************
#ifndef HN_ERR_SOCK
    #define HN_ERR_SOCK
    #define HN_ERR_SOCK_SOCKET    0x0000
    #define HN_ERR_SOCK_BIND      0x0001
    #define HN_ERR_SOCK_LISTEN    0x0002
    #define HN_ERR_SOCK_CONNECT   0x0003
#endif


/////////////////////////// TCP ///////////////////////////
int TCPSock();
int TCPServer(char *a_pcIP, int a_iPort);
int TCPAccept(int a_iSvrSock, char *a_pcCltIP, int a_nCltIPSize);
int TCPConnect(char *a_pcIP, int a_iPort);

/////////////////////////// UDP ///////////////////////////
int UDPSock();
int UDPServer(char *a_pcIP, int a_iPort);
int UDPConnect(char *a_pcIP, int a_iPort);

////////////////////// SEND & RECV /////////////////////////
int sendNdata(int a_iSock, char *a_pcBuffer, int a_iLen);
int recvNdata(int a_iSock, char *a_pcBuffer, int a_iLen);
int sendtoNdata(int a_iSock, char *a_pcBuffer, int a_iLen,
        SOCKADDR_IN *a_pstClientAddress);
int recvfromNdata(int a_iSock, char *a_pcBuffer, int a_iLen,
        SOCKADDR_IN *a_pstClientAddress);

/////////////////////////// OPT  ///////////////////////////
int setSendBuffer(int a_iSock, int a_iValue);
int setRecvBuffer(int a_iSock, int a_iValue);
int setNonblocking(int a_iSock);

#endif

