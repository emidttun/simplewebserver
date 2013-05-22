#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <sys/socket.h>
#include <netinet/in.h>


const char Message[] = "Hello and welcome to my webserver!";

int main(void)
{
    int listenSocket = 0;
    int retVal = 0;
    struct sockaddr_in server;

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == listenSocket) {
        fprintf(stderr, "Could not create socket.\n");
        exit(1);
    } else {
        fprintf(stderr, "Socket created.\n");
    }

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
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

    for (;;) {
        struct sockaddr_in clientName = {0};
        int connectionSocket = 0;
        int clientNameLength = sizeof(clientName);

        connectionSocket = accept(
                    listenSocket,
                    (struct sockaddr*) &clientName,
                    &clientNameLength);

        if (-1 == connectionSocket) {
            fprintf(stderr, "Could not accept connection.\n");
            close(listenSocket);
            exit(1);
        } else {
            fprintf(stderr, "Connection established.\n");
        }

        write(connectionSocket, Message, sizeof(Message));
        close(connectionSocket);

        fprintf(stderr, "Connection closed.\n");
    }


    close(listenSocket);
    printf("Goodbye!\n");
    return 0;
}

