#ifndef VERNAMTUNNEL_H
#define VERNAMTUNNEL_H

#define VERSION "1.0"

#if defined(__MINGW32__) || defined(__CYGWIN__)
#include <winsock2.h>
#include <windows.h>
typedef int socklen_t;
#else
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <assert.h>

#endif

#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct thread_shared_settings 
{
    long long int FilePosition;
    unsigned int buffer_size;
    unsigned int local_port;
    unsigned int remote_port;

    char *bind_address;
    char *remote_host;
    char *key_file;
    char *log_file;

    bool DoLogging;
    bool CustomBindAddress;
    bool KeyFileProvided;
} SharedData;

typedef struct str_thdata
{
    unsigned int ClientSock;
    unsigned int TunnelSock;
} thdata;

void print_help(void);
int StartListener(void);
void LogIt(const char *Message);
char *GetCurrentTimeStamp();
void ClientThread(void* str_thdata);
void TunnelThread(void* str_thdata);
void SocketClose(unsigned int TheSocket);

#endif
