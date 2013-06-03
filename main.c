#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#define BUFFERSIZE 1024

const char responseOk[] =             "HTTP/1.1 200 OK\r\n\r\n";
const char responseNotFound[] =       "HTTP/1.1 404 Not Found\r\n\r\n";
const char responseNotImplemented[] = "HTTP/1.1 501 Not Implemented\r\n\r\n";


int transferToSocket(int outSock, char *fileName);

void sigChildHandler(int sig)
{
    while (0 < waitpid(-1, NULL, WNOHANG));
}

int main(int argc, char *argv[])
{
    int listenSocket = 0;
    int retVal = 0;
    int optVal = 0;
    struct sockaddr_in server;
    char requestBuffer[BUFFERSIZE];

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == listenSocket) {
        fprintf(stderr, "Could not create socket.\n");
        exit(1);
    } else {
        fprintf(stderr, "Socket created.\n");
    }

    /* To avoidning address in use error */
    retVal = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));

    if (-1 == retVal) {
        fprintf(stderr, "Could not set socket option.\n");
        exit(1);
    }

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8081);

    retVal = bind(listenSocket, (struct sockaddr*) &server, sizeof(server));

    if (0 == retVal) {
        fprintf(stderr, "Bind complete.\n");
    } else {
        fprintf(stderr, "Bind failed.\n");
        close(listenSocket);
        exit(1);
    }

    retVal = listen(listenSocket, 5);

    if (-1 == retVal) {
        fprintf(stderr, "Could not listen to socket.\n");
        close(listenSocket);
        exit(1);
    }

    signal(SIGCHLD, sigChildHandler);

    if (1 < argc && 0 == chdir(argv[1])) {
        fprintf(stderr, "\nSetting working directory to %s", argv[1]);
    }

    for (;;) {
        struct sockaddr_in clientName = {0};
        int connectionSocket = 0;
        int clientNameLength = 0;
        int pid = 0;

        clientNameLength = sizeof(clientName);

        connectionSocket = accept(
                    listenSocket,
                    (struct sockaddr*) &clientName,
                    &clientNameLength);

        if (0 == (pid = fork())) {
            int readCnt = 0;

            fprintf(stderr, "Child process %i created.\n", getpid());
            close(listenSocket);

            if (-1 == connectionSocket) {
                fprintf(stderr, "Could not accept connection.\n");
                close(listenSocket);
                exit(1);
            } else {
                fprintf(stderr, "Connection established.\n");
            }

            readCnt = read(connectionSocket, requestBuffer, BUFFERSIZE);

            if (0 < readCnt) {
                fprintf(stderr, "Received: %s", requestBuffer);

                if (0 != strstr(requestBuffer, "GET")) {
                    char fileName[256];
                    char *strFileStart;
                    char *strFileStop;

                    int fileAccess = 0;

                    /* Getting the filename for the GET request:
                     * "GET /<filename> HTTP/1.1CRFL"
                     * Searching for first '/' and consequtive space
                     */
                    strFileStart = strchr(requestBuffer, '/');
                    strFileStop = strchr(strFileStart, ' ');
                    strncpy(fileName, strFileStart + 1, strFileStop - strFileStart - 1);
                    fileName[strFileStop - strFileStart - 1] = '\0';

                    /* using "index.html" as filename if none is given */
                    if (0 == strcmp("", fileName)) {
                        strcpy(fileName, "index.html");
                    }

                    fileAccess = access(fileName, R_OK);

                    fprintf(stderr, "\nAccess of file %s returned %d.", fileName, fileAccess);

                    if (0 == fileAccess) {
                        if (0 != transferToSocket(connectionSocket, fileName)) {
                            close(connectionSocket);
                            continue;
                        }
                    } else {
                        write(connectionSocket, responseNotFound, sizeof(responseNotImplemented) - 1);
                    }
                } else {
                    write(connectionSocket, responseNotImplemented, sizeof(responseNotImplemented) - 1);
                }
            }
            close(connectionSocket);
            fprintf(stderr, "Connection closed.\n");
            exit(0);
        }

        /* Code running in parent process */
        close(connectionSocket);
    }

    close(listenSocket);
    return 0;
}


int transferToSocket(int outSock, char *fileName)
{
    int file = 0;
    int readCnt = 0;
    int writeCnt = 0;
    char* pBuffer;
    char buffer[BUFFERSIZE];

    file = open(fileName, O_RDONLY);

    if (-1 == file) {
        fprintf(stderr, "Could not open the file.\n");
        return -1;
    }

    write(outSock, responseOk, sizeof(responseOk) - 1);

    while (0 < (readCnt = read(file, buffer, BUFFERSIZE))) {
        writeCnt = 0;
        pBuffer = buffer;

        while (writeCnt < readCnt) {
            readCnt -= writeCnt;
            pBuffer += writeCnt;
            writeCnt = write(outSock, pBuffer, readCnt);

            if (-1 == writeCnt) {
                fprintf(stderr, "Could not write to the client.\n");
                close(file);
                return -1;
            }
        }
    }

    close(file);
    return 0;
}



