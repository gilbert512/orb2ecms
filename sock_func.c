#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "sock_func.h"
#include <unistd.h>
#include <fcntl.h>

#ifndef FNDELAY
    #define FNDELAY O_NDELAY
#endif


/////////////////////////// TCP ///////////////////////////
int TCPSock()
{
    int iTCPSock = -1;
    int nOptValue = 1;

    iTCPSock = socket(AF_INET, SOCK_STREAM, 0);
    if (iTCPSock < 0)
    {
        return -1;
    }

    nOptValue = 1;
    setsockopt(iTCPSock, SOL_SOCKET, SO_REUSEADDR,
            (const char*)&nOptValue, sizeof(nOptValue));

    return iTCPSock;
}


int TCPServer(char *a_pcIP, int a_iPort)
{
    SOCKADDR_IN stSvrAddr;
    int iTCPSvrSock = 0;
    int iRtv;

    if (a_iPort <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    iTCPSvrSock = TCPSock();
    if (iTCPSvrSock < 0)
    {
        int iErrno = errno;
        close(iTCPSvrSock);
        errno = iErrno;
        return -(HN_ERR_SOCK_SOCKET);
    }

    memset(&stSvrAddr, 0, sizeof(stSvrAddr));
    stSvrAddr.sin_family = AF_INET;
    if ((a_pcIP == NULL) || (strlen(a_pcIP) == 0))
    {
        stSvrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        stSvrAddr.sin_addr.s_addr = inet_addr(a_pcIP);
    }
    stSvrAddr.sin_port = htons(a_iPort);

    iRtv = bind(iTCPSvrSock, (SOCKADDR *)&stSvrAddr, sizeof(stSvrAddr));
    if (iRtv < 0)
    {
        int iErrno = errno;
        close(iTCPSvrSock);
        errno = iErrno;
        return -(HN_ERR_SOCK_BIND);
    }

    iRtv = listen(iTCPSvrSock, 5);
    if (iRtv < 0)
    {
        int iErrno = errno;
        close(iTCPSvrSock);
        errno = iErrno;
        return -(HN_ERR_SOCK_LISTEN);
    }

    return iTCPSvrSock;
}

//////////////////////////////////////////////////////////////////////////////
// TCP Socket's accept
// input : int(Svr socket), char*(Client IP:Array), int(Client IP Array Count)
// output: Client Sock ID or Error(-1)
//////////////////////////////////////////////////////////////////////////////
int TCPAccept(int a_iSvrSock, char *a_pcCltIP, int a_nCltIPSize)
{
    int  nSize = 0;
    int  iCltSock;
    struct sockaddr_in stSockAddr;
    char sCltIP[50] = "";

    nSize = sizeof(struct sockaddr_in);
    iCltSock = accept(a_iSvrSock, (struct sockaddr *)&stSockAddr, &nSize);
    if ((iCltSock >= 0) && (a_pcCltIP))
    {
        sprintf(sCltIP, "%s", inet_ntoa(stSockAddr.sin_addr));
        strncpy(a_pcCltIP, sCltIP, a_nCltIPSize-1);
    }
    return iCltSock;
}


//////////////////////////////////////////////////////////////////////////////
// TCP Socket's connect
// input : char *(Svr IP), int(Svr Port)
// output: Client Sock ID or Error(-1)
//////////////////////////////////////////////////////////////////////////////
int TCPConnect(char *a_pcIP, int a_iPort)
{
    SOCKADDR_IN stSvrAddr;
    int iTCPCltSock;
    int iRtv = 0;

    if ((a_pcIP == NULL) || (a_iPort <= 0))
    {
        errno = EINVAL;
        return -1;
    }

    iTCPCltSock = TCPSock();
    if (iTCPCltSock < 0)
    {
#ifdef TRACE_SOCK
        int iErrno = errno;
        fprintf(stderr, "%s[%d]:%s():%s\n",
                __func__, __LINE__, "TCPSock", strerror(errno));
        errno = iErrno;
#endif
        return -(HN_ERR_SOCK_SOCKET);
    }

    memset(&stSvrAddr, 0, sizeof(stSvrAddr));
    stSvrAddr.sin_family      = AF_INET;
    stSvrAddr.sin_addr.s_addr = inet_addr(a_pcIP);
    stSvrAddr.sin_port        = htons(a_iPort);

    iRtv = connect(iTCPCltSock, (SOCKADDR *)&stSvrAddr, sizeof(stSvrAddr));
    if (iRtv < 0)
    {
#ifdef TRACE_SOCK
        int iErrno = errno;
        fprintf(stderr, "%s[%d]:%s():%s\n",
                __func__, __LINE__, "connect", strerror(errno));
        errno = iErrno;
#endif
        return -(HN_ERR_SOCK_CONNECT);
    }

    return iTCPCltSock;
}


/////////////////////////// UDP ///////////////////////////
int UDPSock()
{
    int iUDPSock;
    int nOptValue = 1;

    if ((iUDPSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
#ifdef TRACE_SOCK
        int iErrno = errno;
        fprintf(stderr, "%s[%d]:%s():%s\n",
                __func__, __LINE__, "socket", strerror(errno));
        errno = iErrno;
#endif
        return -(HN_ERR_SOCK_SOCKET);
    }

    nOptValue = 1;
    setsockopt(iUDPSock, SOL_SOCKET, SO_REUSEADDR,
            (const char*)&nOptValue, sizeof(nOptValue));

    return iUDPSock;
}

int UDPServer(char *a_pcIP, int a_iPort)
{
    SOCKADDR_IN stSvrAddr;
    int iUDPSvrSock;
	int iRtv;

    if (a_iPort <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    iUDPSvrSock = UDPSock();
    if (iUDPSvrSock < 0)
    {
#ifdef TRACE_SOCK
        int iErrno = errno;
        fprintf(stderr, "%s[%d]:%s():%s\n",
                __func__, __LINE__, "UDPSock", strerror(errno));
        errno = iErrno;
#endif
        return -(HN_ERR_SOCK_SOCKET);
    }

    memset(&stSvrAddr, 0, sizeof(stSvrAddr));
    stSvrAddr.sin_family = AF_INET;
    if ((a_pcIP == NULL) || (strlen(a_pcIP) == 0))
    {
        stSvrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        stSvrAddr.sin_addr.s_addr = inet_addr(a_pcIP);
    }
    stSvrAddr.sin_port = htons(a_iPort);

    iRtv = bind(iUDPSvrSock, (SOCKADDR *)&stSvrAddr, sizeof(stSvrAddr));
    if (iRtv < 0)
    {
#ifdef TRACE_SOCK
        int iErrno = errno;
        fprintf(stderr, "%s[%d]:%s():%s\n",
                __func__, __LINE__, "socket", strerror(errno));
        errno = iErrno;
#endif
        close(iUDPSvrSock);
        return -(HN_ERR_SOCK_BIND);
    }

#if 0
	iRtv = listen(iUDPSvrSock, 5);
	if (iRtv < 0)
	{
		int iErrno = errno;
		close(iUDPSvrSock);
		errno = iErrno;
		return -(HN_ERR_SOCK_LISTEN);
	}
#endif

    return iUDPSvrSock;
}

//////////////////////////////////////////////////////////////////////////////
// UDP Socket's connect
// input : char *(Svr IP), int(Svr Port)
// output: Client Sock ID or Error(-1)
//////////////////////////////////////////////////////////////////////////////
int UDPConnect(char *a_pcIP, int a_iPort)
{
    SOCKADDR_IN stSvrAddr;
    int iUDPCltSock;
    int iRtv = 0;

    if ((a_pcIP == NULL) || (a_iPort <= 0))
    {
        errno = EINVAL;
        return -1;
    }

    iUDPCltSock = UDPSock();
    if (iUDPCltSock < 0)
    {
#ifdef TRACE_SOCK
        int iErrno = errno;
        fprintf(stderr, "%s[%d]:%s():%s\n",
                __func__, __LINE__, "UDPSock", strerror(errno));
        errno = iErrno;
#endif
        return -(HN_ERR_SOCK_SOCKET);
    }

    memset(&stSvrAddr, 0, sizeof(stSvrAddr));
    stSvrAddr.sin_family = AF_INET;
    stSvrAddr.sin_addr.s_addr = inet_addr(a_pcIP);
    stSvrAddr.sin_port = htons(a_iPort);

    iRtv = connect(iUDPCltSock, (SOCKADDR *)&stSvrAddr, sizeof(stSvrAddr));
    if (iRtv < 0)
    {
#ifdef TRACE_SOCK
        int iErrno = errno;
        fprintf(stderr, "%s[%d]:%s():%s\n",
                __func__, __LINE__, "connect", strerror(errno));
        errno = iErrno;
#endif
        return -(HN_ERR_SOCK_CONNECT);
    }

    return iUDPCltSock;
}

////////////////////// SEND & RECV /////////////////////////
int recvfromNdata(int a_iSock, char *a_pcBuffer, int a_iLen,
        SOCKADDR_IN *a_pstClientAddr)
{
    int iRtv = 0;
    int iRemain = a_iLen;
    int nSize = 0;
    char *pcPoint = a_pcBuffer;

    if ((a_iSock < 0) || (a_pcBuffer == NULL) || (a_iLen <= 0))
    {
        errno = EINVAL;
        return -1;
    }

    while (iRemain)
    {
        iRtv = recvfrom(a_iSock, pcPoint, iRemain, 0,
                    (SOCKADDR *)a_pstClientAddr, &nSize);
        if (iRtv <= 0)
        {
#ifdef TRACE_SOCK
            int iErrno = errno;
            fprintf(stderr, "%s[%d]:%s():%s\n",
                    __func__, __LINE__, "recvfrom", strerror(errno));
            errno = iErrno;
#endif
            return a_iLen - iRemain;
        }
        iRemain -= iRtv;
        pcPoint += iRtv;
    }

    return a_iLen;
}

int sendtoNdata(int a_iSock, char *a_pcBuffer, int a_iLen,
        SOCKADDR_IN *a_pstClientAddr)
{
    int iRtv = 0;
    int iRemain = a_iLen;
    char *pcPoint = a_pcBuffer;

    if ((a_iSock < 0) || (a_pcBuffer == NULL) || (a_iLen <= 0))
    {
        errno = EINVAL;
        return -1;
    }

    while (iRemain)
    {
        iRtv = sendto(a_iSock, pcPoint, iRemain, 0,
                    (SOCKADDR *)a_pstClientAddr, sizeof(SOCKADDR));
        if (iRtv <= 0)
        {
#ifdef TRACE_SOCK
            int iErrno = errno;
            fprintf(stderr, "%s[%d]:%s(%s, %d):%s\n",
                    __func__, __LINE__, "sendto",
                    pcPoint, a_iLen, strerror(errno));
            errno = iErrno;
#endif
            return a_iLen - iRemain;
        }
        iRemain -= iRtv;
        pcPoint += iRtv;
    }

    return a_iLen;
}

int recvNdata(int a_iSock, char *a_pcBuffer, int a_iLen)
{
    char *pcPoint = a_pcBuffer;
    int iRemain = a_iLen;
    int iRtv = 0;

    if ((a_iSock < 0) || (a_pcBuffer == NULL) || (a_iLen <= 0))
    {
        errno = EINVAL;
        return -1;
    }

    while (iRemain)
    {
        iRtv = recv(a_iSock, pcPoint, iRemain, 0);
        if (iRtv <= 0)
        {
#ifdef TRACE_SOCK
            int iErrno = errno;
            fprintf(stderr, "%s[%d]:%s():%s\n",
                    __func__, __LINE__, "recv", strerror(errno));
            errno = iErrno;
#endif
            return a_iLen - iRemain;
        }
        iRemain -= iRtv;
        pcPoint += iRtv;
    }

    return a_iLen;
}

int sendNdata(int a_iSock, char *a_pcBuffer, int a_iLen)
{
    char *pcPoint = a_pcBuffer;
    int iRemain = a_iLen;
    int iRtv = 0;

    if ((a_iSock < 0) || (a_pcBuffer == NULL) || (a_iLen <= 0))
    {
        errno = EINVAL;
        return -1;
    }

    while (iRemain)
    {
        iRtv = send(a_iSock, pcPoint, iRemain, 0);
        if (iRtv <= 0)
        {
#ifdef TRACE_SOCK
            int iErrno = errno;
            fprintf(stderr, "%s[%d]:%s(%s, %d):%s\n",
                    __func__, __LINE__, "send",
                    pcPoint, a_iLen, strerror(errno));
            errno = iErrno;
#endif
            return a_iLen - iRemain;
        }
        iRemain -= iRtv;
        pcPoint += iRtv;
    }

    return a_iLen;
}

/////////////////////////// OPT ///////////////////////////
int setSendBuffer(int a_iSock, int a_iValue)
{
    return setsockopt(a_iSock, SOL_SOCKET, SO_SNDBUF,
            &a_iValue, sizeof(a_iValue));
}

int setRecvBuffer(int a_iSock, int a_iValue)
{
    return setsockopt(a_iSock, SOL_SOCKET, SO_RCVBUF,
            &a_iValue, sizeof(a_iValue));
}

int setNonblocking(int a_iSock)
{
    int nFlags;
    int nFlagsOrg;

    nFlagsOrg = fcntl(a_iSock, F_GETFL, 0);
    nFlags = nFlagsOrg | FNDELAY;
    return fcntl(a_iSock, F_SETFL, nFlags);
}

