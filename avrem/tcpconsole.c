#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "usart.h"

#ifdef USE_TCP
#include <winsock2.h>
#include <ws2tcpip.h>


WSADATA wsaData;
SOCKET listen_socket[USART_MAX_COUNT + 1], msgsock[USART_MAX_COUNT + 1]; //extra socket for external extensions
char* ip_address = NULL;

int fromlen[USART_MAX_COUNT + 1];
int socket_type = SOCK_STREAM;
struct sockaddr_in local[USART_MAX_COUNT + 1], from[USART_MAX_COUNT + 1];
volatile uint8_t didstartup = 0;

void ext_rx(uint8_t val);
#endif

int tcpconsole_init(struct usart_s* usart, uint16_t port) {
#ifdef USE_TCP
    int retval;
    unsigned long iMode = 1;
    uint8_t index;
    index = (usart == NULL) ? USART_MAX_COUNT : usart->usart_index;
    if (index > USART_MAX_COUNT) return -1;

    if (!didstartup) {
        // Request Winsock version 2.2
        if ((retval = WSAStartup(0x202, (LPWSADATA) &wsaData)) != 0) {
            fprintf(stderr, "Server: WSAStartup() failed with error %d\n", retval);
            WSACleanup();
            return -1;
        }
        didstartup = 1;
    }

    local[index].sin_family = AF_INET;
    if (ip_address)
        inet_pton(AF_INET, ip_address, &local[index].sin_addr.s_addr);
    else
        local[index].sin_addr.s_addr = INADDR_ANY;
    local[index].sin_port = htons(port);
    listen_socket[index] = socket(AF_INET, socket_type, 0);

    if (listen_socket[index] == INVALID_SOCKET) {
        fprintf(stderr, "Server: socket() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    if (bind(listen_socket[index], (struct sockaddr*)&local[index], sizeof(local[index])) == SOCKET_ERROR) {
        fprintf(stderr, "Server: bind() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    if (listen(listen_socket[index], 5) == SOCKET_ERROR) {
        fprintf(stderr, "Server: listen() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    ioctlsocket(listen_socket[index], FIONBIO, &iMode); //we want accept() to not block

    msgsock[index] = INVALID_SOCKET;
#endif
    return 0;
}

void tcpconsole_dorecv(struct usart_s* usart) {
#ifdef USE_TCP
    int actuallen = 0;
    uint8_t val;
    unsigned long iMode = 1;
    uint8_t index;
    index = (usart == NULL) ? USART_MAX_COUNT : usart->usart_index;
    if (index > USART_MAX_COUNT) return;

    if (msgsock[index] == INVALID_SOCKET) {
        fromlen[index] = sizeof(from[index]);
        msgsock[index] = accept(listen_socket[index], (struct sockaddr*)&from[index], &fromlen[index]);
        ioctlsocket(msgsock[index], FIONBIO, &iMode); //non-blocking IO
    }

    if (msgsock[index] == INVALID_SOCKET) return;
    actuallen = recv(msgsock[index], &val, 1, 0);
    if (actuallen > 0) {
        if (usart != NULL) usart_rx(usart, val);
        else ext_rx(val);
    }
    else if (actuallen == 0) { //graceful close
        closesocket(msgsock[index]);
        msgsock[index] = INVALID_SOCKET;
    }
    else {
        //printf("%d\n", WSAGetLastError());
        switch (WSAGetLastError()) {
        case WSAEWOULDBLOCK: //this is ok, just no data yet
            break;
        case WSANOTINITIALISED: //the rest of these are bad...
        case WSAENETDOWN:
        case WSAENOTCONN:
        case WSAENETRESET:
        case WSAENOTSOCK:
        case WSAESHUTDOWN:
        case WSAEINVAL:
        case WSAECONNABORTED:
        case WSAETIMEDOUT:
        case WSAECONNRESET:
            closesocket(msgsock[index]);
            msgsock[index] = INVALID_SOCKET;
            break;
        default:
            break;
        }
    }
#endif
}

void tcpconsole_send(struct usart_s* usart, uint8_t val) {
#ifdef USE_TCP
    int ret;
    uint8_t index;
    index = (usart == NULL) ? USART_MAX_COUNT : usart->usart_index;
    if (index > USART_MAX_COUNT) return;
    if (msgsock[index] == INVALID_SOCKET) return;
    ret = send(msgsock[index], &val, 1, 0);
    //printf("send %d\n", ret);
    if (ret < 1) {
        //TODO: error handling
    }
#endif
}

void tcpconsole_send_array(struct usart_s* usart, uint8_t* data, int len) {
#ifdef USE_TCP
    int ret;
    uint8_t index;
    index = (usart == NULL) ? USART_MAX_COUNT : usart->usart_index;
    if (index > USART_MAX_COUNT) return;
    if (msgsock[index] == INVALID_SOCKET) return;
    ret = send(msgsock[index], data, len, 0);
    //printf("send %d\n", ret);
    if (ret < 1) {
        //TODO: error handling
    }
#endif
}
