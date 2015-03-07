#include "vernamtunnel.h"

SharedData GlobalOptions;
FILE *LogFileHandle;
char NoTunnelMsg[] = "\nTunnel is not available\n";
void print_help(void)
{
    fprintf(stderr, "Usage: vernamtunnel [options]\n\n\
            Options: \n\
            --local-port=PORT    local port to listen\n\
            --remote-port=PORT   remote port to connect\n\
            --remote-host=HOST   remote host to connect\n\
            --bind-address=IP    local IP address to bind bind listening port\n\
            --key-file=keyfile   one-time-pad key file\n\
            --start-pos=number   starting position of one time pad file\n\
            --log-file=LOGFILE	 file to log connection details\n\
            --help\n\n");
}

void SocketClose(unsigned int TheSocket)
{
#if defined(__MINGW32__) || defined(__CYGWIN__)
    closesocket(TheSocket);
#else
    close(TheSocket);
#endif

}

void LogIt(const char *Message)
{
    printf("[Log] [%s]: %s\n", GetCurrentTimeStamp(), Message);
    if (GlobalOptions.DoLogging && LogFileHandle != NULL)
        fprintf(LogFileHandle, "[Log] [%s]: %s\n", GetCurrentTimeStamp(), Message);
}

char *GetCurrentTimeStamp(void)
{
    static char date_str[32];
    time_t date;

    time(&date);
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", localtime(&date));
    return date_str;
}

int main(int argc, char *argv[])
{
    int opt;
    int index;
    GlobalOptions.buffer_size = 131072;
    GlobalOptions.FilePosition = 0;
    static struct option long_options[] = {
    { "local-port",    	required_argument, NULL, 'a' },
    { "remote-host",   	required_argument, NULL, 'b' },
    { "remote-port",   	required_argument, NULL, 'c' },
    { "bind-address",  	required_argument, NULL, 'd' },
    { "key-file",	required_argument, NULL, 'e' },
    { "buffer-size",   	required_argument, NULL, 'f' },
    { "log-file",      	required_argument, NULL, 'g' },
    { "help",          	no_argument,       NULL, 'h' },
    { "start-pos",      required_argument, NULL, 'i' },
    { 0, 0, 0, 0 }
};


    do
    {
        opt = getopt_long(argc, argv, "", long_options, &index);
        switch (opt)
        {
        case 'a':
            GlobalOptions.local_port = atoi(optarg);
            break;

        case 'b':
            GlobalOptions.remote_host = optarg;
            break;

        case 'c':
            GlobalOptions.remote_port = atoi(optarg);
            break;

        case 'd':
        {
            GlobalOptions.bind_address = optarg;
            GlobalOptions.CustomBindAddress = true;
            break;
        }

        case 'e':
        {
            GlobalOptions.key_file = optarg;
            GlobalOptions.KeyFileProvided = true;
            break;
        }

        case 'f':
            GlobalOptions.buffer_size = atoi(optarg);
            break;

        case 'g':
        {
            GlobalOptions.log_file = optarg;
            LogFileHandle = fopen(GlobalOptions.log_file, "w");
            if (!LogFileHandle)
            {
                printf("Unable to open log file\n");
                exit(1);
            }
            GlobalOptions.DoLogging = true;
            break;
        }

        case 'h':
        case '?':
        {
            print_help();
            exit(0);
        }

        case 'i':
            GlobalOptions.FilePosition = atoll(optarg);
            break;
        }

    } while (opt != -1);

    if (GlobalOptions.local_port < 1)
    {
        fprintf(stderr,"missing or invalid '--local-port=' option.\nTry --help option for more details\n");
        exit(1);
    }

    if (GlobalOptions.remote_port < 1)
    {
        fprintf(stderr,"missing or invalid '--remote-port=' option.\nTry --help option for more details\n");
        exit(1);
    }

    if (strlen(GlobalOptions.remote_host) < 2 || strlen(GlobalOptions.remote_host) > 63)
    {
        fprintf(stderr,"\nmissing or invalid '--remote-host=' option.\nTry --help option for more details\n");
        exit(1);
    }

    if (!GlobalOptions.KeyFileProvided)
    {
        fprintf(stderr,"\nmissing or invalid '--key-file=' option.\nTry --help option for more details\n");
        exit(1);
    }

    if (GlobalOptions.buffer_size <= 0) GlobalOptions.buffer_size = 131072;
    if (!StartListener())
    {
        printf("\nUnable to start listener\n");
        exit(0);
    }
    return 1;
}

void TunnelThread(void* str_thdata)
{
    unsigned int MainSocket;
    unsigned int TunnelSocket;
    char buffer[GlobalOptions.buffer_size];
    char XORKey[GlobalOptions.buffer_size];
    struct str_thdata* params = (struct str_thdata*)str_thdata;
    char TheMessage[512];
    long long int FileSize;
    long long int CurrentFP;
    int i;
    FILE *fp;
#if defined(__MINGW32__) || defined(__CYGWIN__)
    int receiveResult;
#endif	
    MainSocket = params->ClientSock;
    TunnelSocket = params->TunnelSock;

    fp = fopen(GlobalOptions.key_file, "r");
    if (fp == NULL)
    {
        SocketClose(TunnelSocket);
        SocketClose(MainSocket);
        LogIt("Unable to open key-file");
        return;
    }
    fseek(fp, 0L, SEEK_END);
    FileSize = ftell(fp);
    if (FileSize < (GlobalOptions.FilePosition + GlobalOptions.buffer_size) )
    	fseek(fp, 0L, SEEK_SET);
    else
    	fseek(fp, GlobalOptions.FilePosition, SEEK_SET);

    for (;;)
    {
        memset(buffer, 0, sizeof(GlobalOptions.buffer_size));
        memset(XORKey, 0, sizeof(GlobalOptions.buffer_size));
        int count = recv(TunnelSocket, buffer, sizeof(buffer), 0);
#if defined(__MINGW32__) || defined(__CYGWIN__)
        receiveResult = WSAGetLastError();
        if (receiveResult == WSAEWOULDBLOCK && count <= 0)
            continue;
#endif
        if (count <= 0)
        {
            SocketClose(TunnelSocket);
            SocketClose(MainSocket);
            LogIt("Tunnel connection closed");
            fclose(fp);
            LogIt(TheMessage);
            return;
        }
        if (count > 0)
        {
            CurrentFP = ftell(fp);

            if (CurrentFP + count > FileSize)
                fseek(fp, 0L, SEEK_SET);
            fread(XORKey, count, 1, fp);
            for (i=0;i<count;i++)
                buffer[i] ^= XORKey[i];
            int retval = send(MainSocket, buffer, count, 0);
            if (retval < 0)
            {
                LogIt("MainSocket closed");
                SocketClose(MainSocket);
                SocketClose(MainSocket);
                fclose(fp);
                return;
            }
        }

    }
    fclose(fp);
}

void ClientThread(void* str_thdata)
{
    unsigned int MainSocket;
    struct sockaddr_in RemoteAddr;
    struct hostent *RemoteHost;
    unsigned int TunnelSocket;
    char buffer[GlobalOptions.buffer_size];
    char XORKey[GlobalOptions.buffer_size];
    struct str_thdata* params = (struct str_thdata*)str_thdata;
    MainSocket = params->ClientSock;
    char TheMessage[512];
    thdata ThreadParam;
    int flag;
    int sendbuff;
    long long int FileSize;
    long long int CurrentFP;
    int i;
    FILE *fp;

#if defined(__MINGW32__) || defined(__CYGWIN__)
    DWORD threadID;
    u_long iMode;
    int receiveResult;
#else
    pthread_t threadID;
#endif

    fp = fopen(GlobalOptions.key_file, "r");
    if (fp == NULL)
    {
        SocketClose(MainSocket);
        LogIt("Unable to open key-file");
        return;
    }
    fseek(fp, 0L, SEEK_END);
    FileSize = ftell(fp);
    if (FileSize < (GlobalOptions.FilePosition + GlobalOptions.buffer_size) )
    	fseek(fp, 0L, SEEK_SET);
    else
    	fseek(fp, GlobalOptions.FilePosition, SEEK_SET);

    RemoteHost = gethostbyname(GlobalOptions.remote_host);
    if (RemoteHost == NULL)
    {
        LogIt("Thread: Remote host gethostbyname() failed");
        send(MainSocket, NoTunnelMsg, strlen(NoTunnelMsg), 0);
        fclose(fp);
        SocketClose(MainSocket);
        return;
    }

    memset(&RemoteAddr, 0, sizeof(RemoteAddr));

    RemoteAddr.sin_family = AF_INET;
    RemoteAddr.sin_port = htons(GlobalOptions.remote_port);

    memcpy(&RemoteAddr.sin_addr.s_addr, RemoteHost->h_addr, RemoteHost->h_length);

    TunnelSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (TunnelSocket < 0)
    {
        LogIt("Thread: socket() call failed");
        fclose(fp);
        SocketClose(MainSocket);
        return;
    }

    if (connect(TunnelSocket, (struct sockaddr *) &RemoteAddr, sizeof(RemoteAddr)) < 0)
    {
        LogIt("Thread: connect() call failed");
        fclose(fp);
        SocketClose(MainSocket);
        return;
    }


    flag =1;
    setsockopt(TunnelSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

    flag =1;
    setsockopt(MainSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

    sendbuff = 131072;
    setsockopt(MainSocket, SOL_SOCKET, SO_SNDBUF, (char *) &sendbuff, sizeof(int));
    sendbuff = 131072;
    setsockopt(MainSocket, SOL_SOCKET, SO_RCVBUF, (char *) &sendbuff, sizeof(int));


    sendbuff = 131072;
    setsockopt(TunnelSocket, SOL_SOCKET, SO_SNDBUF, (char *) &sendbuff, sizeof(int));
    sendbuff = 131072;
    setsockopt(TunnelSocket, SOL_SOCKET, SO_RCVBUF, (char *) &sendbuff, sizeof(int));


#if defined(__MINGW32__) || defined(__CYGWIN__)
    iMode = 1;
    ioctlsocket(MainSocket,FIONBIO, &iMode);
    ioctlsocket(TunnelSocket,FIONBIO, &iMode);
#endif

    ThreadParam.ClientSock = MainSocket;
    ThreadParam.TunnelSock = TunnelSocket;
#if defined(__MINGW32__) || defined(__CYGWIN__)
    CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)TunnelThread, (LPVOID)&ThreadParam, 0, &threadID);
#else
    pthread_create (&threadID, NULL, (void *) &TunnelThread, (void *) &ThreadParam);
#endif

    for (;;)
    {
        memset(buffer, 0, sizeof(GlobalOptions.buffer_size));
        memset(XORKey, 0, sizeof(GlobalOptions.buffer_size));

        int count = recv(MainSocket, buffer, sizeof(buffer), 0);
#if defined(__MINGW32__) || defined(__CYGWIN__)
        receiveResult = WSAGetLastError();
        if (receiveResult == WSAEWOULDBLOCK && count <= 0)

            continue;

#endif
        if (count <= 0)
        {
            LogIt("Listener socket closed");
            SocketClose(MainSocket);
            SocketClose(TunnelSocket);
            fclose(fp);
            LogIt(TheMessage);
            return;
        }
        if (count > 0)
        {
            CurrentFP = ftell(fp);
            if (CurrentFP + count > FileSize)
                fseek(fp, 0L, SEEK_SET);
            fread(XORKey, count, 1, fp);
            for (i=0;i<count;i++)
                buffer[i] ^= XORKey[i];
            int retval = send(TunnelSocket, buffer, count, 0);
            if (retval < 0)
            {
                LogIt("Tunnel closed");
                fclose(fp);
                SocketClose(MainSocket);
                SocketClose(TunnelSocket);
                return;
            }
        }
    }
}

int StartListener(void)
{
    struct sockaddr_in listenerAddr;
    struct sockaddr_in clientAddr;
    unsigned int ListenSock;
    unsigned int ClientSock;
    char TheMessage[512];
    int optval = 1;
    thdata ThreadParam;
#if defined(__MINGW32__) || defined(__CYGWIN__)
    DWORD threadID;
    WSADATA info;
    if (WSAStartup(MAKEWORD(2,2), &info) != 0)
    {
        LogIt("main: WSAStartup()");
        exit(1);
    }
#else
    pthread_t threadID;
#endif


    listenerAddr.sin_port = htons(GlobalOptions.local_port);
    listenerAddr.sin_family = AF_INET;
    listenerAddr.sin_addr.s_addr = INADDR_ANY;
    ListenSock = socket(AF_INET, SOCK_STREAM, 0);

    if (ListenSock < 0)
    {
        LogIt("build_server: socket()");
        return 0;
    }

#if defined(__MINGW32__) || defined(__CYGWIN__)
    if (setsockopt(ListenSock, SOL_SOCKET, SO_REUSEADDR, (const char *) &optval, sizeof(optval)) < 0)
#else
    if (setsockopt(ListenSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
#endif
    {
        LogIt("StartListener: setsockopt(SO_REUSEADDR)");
        return 0;
    }

    if (GlobalOptions.CustomBindAddress)
        listenerAddr.sin_addr.s_addr = inet_addr(GlobalOptions.bind_address);

    if (bind(ListenSock, (struct sockaddr *) &listenerAddr, sizeof(listenerAddr)) < 0)
    {
        LogIt("StartListener: bind()");
        return 0;
    }

    if (listen(ListenSock, 1) < 0)
    {
        LogIt("StartListener: listen()");
        return 0;
    }
    printf("\nListening...\n");
    while (1)
    {
#if defined(__MINGW32__) || defined(__CYGWIN__)
        int client_addr_size;
#else
        unsigned int client_addr_size;
#endif

        client_addr_size = sizeof(struct sockaddr_in);

        ClientSock = accept(ListenSock, (struct sockaddr *) &clientAddr, &client_addr_size);
        if (ClientSock < 0)
        {
            if (errno != EINTR)
                LogIt("wait_for_clients: accept()");
            return 1;
        }
        sprintf(TheMessage, "New connection: request from %s", inet_ntoa(clientAddr.sin_addr));
        LogIt(TheMessage);
        ThreadParam.ClientSock = ClientSock;
#if defined(__MINGW32__) || defined(__CYGWIN__)
        CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)ClientThread, (LPVOID)&ThreadParam, 0, &threadID);
#else
        pthread_create (&threadID, NULL, (void *) &ClientThread, (void *) &ThreadParam);
#endif

    }
    fclose(LogFileHandle);
    return 1;
} 

