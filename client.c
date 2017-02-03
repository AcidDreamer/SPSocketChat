#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>

#include<pthread.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>

#define ERROR -1
#define DATA_LENGTH 2048
static int sock;
void *client_to_server();
int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    char message[DATA_LENGTH], server_reply[DATA_LENGTH];

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");

    //check arguement count
    if (argc != 2) {
        printf("The program is used as such.\n arg0 = program , arg1 = port\n");
        exit(ERROR);
    }
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));
    //initialize the data structure
    bzero(&server.sin_zero, 8); //zero padding to match a struct sockaddr

    //Connect to remote server
    if (connect(sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
    pthread_t cTsThread;
    pthread_create(&cTsThread, NULL, client_to_server, NULL);
    puts("Connected\n");

    //keep communicating with server
    while (1) {
        printf("Enter message : ");
        scanf("%s", message);

        //Send some data
        if (send(sock, message, strlen(message), 0) < 0) {
            puts("Send failed");
            return 1;
        }

        //Receive a reply from the server
        if (recv(sock, server_reply, 2000, 0) < 0) {
            puts("recv failed");
            break;
        }

        puts("Server reply :");
        puts(server_reply);
    }

    close(sock);
    return 0;
}

void *client_to_server() {
    int retrieve;
    //the actual data
    char data[DATA_LENGTH];
    //free the string
    memset(data, 0, DATA_LENGTH);
    for (;;) {
        //we free the string in everyloop
        memset(data, 0, DATA_LENGTH);
        //we retrieve the data
        retrieve = recvfrom(sock, data, DATA_LENGTH, 0, NULL, NULL);
        //If an error occured
        if (retrieve < 0) {
            printf("Error receiving data!\n");
            //if we retrieved the data succesfully
        } else if (retrieve > 0) {
            printf("server: ");
            fputs(data, stdout);
            //if the user disconnected
        } else {
            //inform the server
            puts("Server Dropped!\n");
            //terminate the thread
            int xyz = 2;
            pthread_exit(&xyz);
        }
    }
}

