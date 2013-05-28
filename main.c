#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFERSIZE 1024

const char notImplemented[] = "HTTP/1.1 501 Not Implemented\r\n";
const char ok[] = "HTTP/1.1 200 OK\r\n\r\n";

void sigChildHandler(int sig)
{
    while (0 < waitpid(-1, NULL, WNOHANG));
}


int main(void)
{
    int listenSocket = 0;
    int retVal = 0;
    int optVal = 0;
    struct sockaddr_in server;

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

    for (;;) {
        struct sockaddr_in clientName = {0};
        int connectionSocket = 0;
        int clientNameLength = 0;
        int pid = 0;
        int file = 0;
        int readCnt = 0;
        int writeCnt = 0;
        char* pBuffer;
        char buffer[BUFFERSIZE];

        clientNameLength = sizeof(clientName);

        connectionSocket = accept(
                    listenSocket,
                    (struct sockaddr*) &clientName,
                    &clientNameLength);

        if (0 == (pid = fork())) {
            fprintf(stderr, "Child process %i created.\n", getpid());
            close(listenSocket);

            if (-1 == connectionSocket) {
                fprintf(stderr, "Could not accept connection.\n");
                close(listenSocket);
                exit(1);
            } else {
                fprintf(stderr, "Connection established.\n");
            }

            readCnt = read(connectionSocket, buffer, BUFFERSIZE);

            if (0 < readCnt) {
                fprintf(stderr, "Received: %s", buffer);

                if (0 != strstr(buffer, "GET")) {
                    file = open("testdata.txt", O_RDONLY);

                    if (-1 == file) {
                        fprintf(stderr, "Could not open the file.\n");
                        close(connectionSocket);
                        continue;
                    }

                    write (connectionSocket, ok, sizeof(ok) - 1);

                    while (0 < (readCnt = read(file, buffer, BUFFERSIZE))) {
                        writeCnt = 0;
                        pBuffer = buffer;

                        while (writeCnt < readCnt) {
                            readCnt -= writeCnt;
                            pBuffer += writeCnt;
                            writeCnt = write(connectionSocket, pBuffer, readCnt);

                            if (-1 == writeCnt) {
                                fprintf(stderr, "Could not write to the client.\n");
                                close(connectionSocket);
                                continue;
                            }
                        }
                    }


                    close(file);
                } else {
                    write(connectionSocket, notImplemented, sizeof(notImplemented) - 1);
                }
            }
            close(connectionSocket);
            fprintf(stderr, "Connection closed.\n");
            exit(0);
        }
        close(connectionSocket);
    }


    close(listenSocket);
    printf("Goodbye!\n");
    return 0;
}

