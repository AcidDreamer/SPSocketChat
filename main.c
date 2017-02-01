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


#define MAX_CLIENTS 2
#define DATA_LENGTH 2048
#define ERROR -1

void *connection_handler(void *);

int main(int argc, char** argv) {
    //server structure
    struct sockaddr_in server;
    //client structure for the first client
    struct sockaddr_in client0;
    //client structure for the second client
    struct sockaddr_in client1;
    //all three, are of type sockaddr_in

    //socket descriptor for the Server
    int server_sock_desc;
    //socket descriptor for the first client
    int client_sock_desc0;
    //socket descriptor for the second client
    int client_sock_desc1;
    //required for bind and accept
    int sockaddr_len = sizeof (struct sockaddr_in);
    //Number of bytes of a message
    int data_length;
    //The actual messege
    char data[DATA_LENGTH];

    //check arguement count
    if (argc != 2) {
        printf("The program is used as such.\n arg0 = program , arg1 = port\n");
        exit(ERROR);
    }

    //Initializing the socket
    if ((server_sock_desc = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        perror("Server socket :");
        exit(-1);
    };

    //setting up the server data structure
    server.sin_family = AF_INET;
    //set-up the port via argument
    server.sin_port = htons(atoi(argv[1]));
    //contains the IP address that will be associated with the socket
    server.sin_addr.s_addr = INADDR_ANY; //kernel listens to all interfaces,0.0.0.0
    //initialize the data structure
    bzero(&server.sin_zero, 8); //zero padding to match a struct sockaddr

    //bind the port providing server socket descriptor and the server structure
    if ((bind(server_sock_desc, (struct sockaddr *) &server, sockaddr_len)) == ERROR) {
        perror("Bind :");
        exit(-1);
    }

    //if bind was successful listen to the port ,up to MAX_CLIENTS at the same time 
    if ((listen(server_sock_desc, 1)) == ERROR) {
        perror("Listen :");
        exit(-1);
    }
    while ((client_sock_desc0 = accept(server_sock_desc, (struct sockaddr *) &client0, (socklen_t*) & sockaddr_len))) {
        puts("Client accepted");
        int data_length = 1;
        while (data_length) {
            data_length = recv(client_sock_desc0, data, DATA_LENGTH, 0);
            if (data_length) {
                send(client_sock_desc1, data, data_length, 0);
                data[data_length] = '\0';
                printf("Message from the other user : %s \n", data);
            }

        }
    }
    while ((client_sock_desc1 = accept(server_sock_desc, (struct sockaddr *) &client0, (socklen_t*) & sockaddr_len))) {
    }
        printf("Hello world");
        return 0;
    }

    void *connection_handler(void *socket_desc) {
    }



    //stolen memes
    /*
     void send_message(char *s, int uid){
            int i;
            for(i=0;i<MAX_CLIENTS;i++){
                    if(clients[i]){
                            if(clients[i]->uid != uid){
                                    write(clients[i]->connfd, s, strlen(s));
                            }
                    }
            }
    }

     * 
     * void strip_newline(char *s){
            while(*s != '\0'){
                    if(*s == '\r' || *s == '\n'){
     *s = '\0';
                    }
                    s++;
            }
    }

     */


    /*
    data_length = 1;
    while(data_length){
        data_length = recv(client_sock_desc0,data,DATA_LENGTH,0);
        if(data_length){
            send(client_sock_desc0,data,data_length,0);
            data[data_length] = '\0';
            printf("Sent messege : %s \n",data);
        }
    }
    printf("Client disconnected.\n");
    data_length = 1;
    while (data_length) {
        data_length = recv(client_sock_desc0, data, 6, 0);
        if (data_length) {
            data[data_length] = '\0';
        }
        int stcom = strcmp(data, "quit");
        printf("%s data length : %d \n", &data, &stcom);
        if (strcmp(data, "quit") == 0) {
            close(client_sock_desc0);
        }
    }
         
    while ((data_length = recv(client_sock_desc0, data, sizeof (data), 0)) > 0) {
        if (!strlen(data)) {
            continue;
        }
        data[data_length] = '\0';
        printf("%s ", &*data);
        if (strlen(" quit") == data_length && !strncmp(" quit", data, data_length)) {
            close(client_sock_desc1);
            puts("check 1");
        }
    }
     */

    /*
        while (1) {
            //we wait till the first client connects
            puts("Waiting for a client\n");
            if ((client_sock_desc0 = accept(server_sock_desc, (struct sockaddr *) &client0, &sockaddr_len)) == ERROR) {
                perror("Accept :");
                exit(-1);
            }
            //we inform the first client that he is alone 
            char first_user_message[] = "You are connected , please wait for the second user to connect!\n";
            data_length = strlen(first_user_message);
            send(client_sock_desc0, first_user_message, data_length, 0);

            //We wait for the second client
            puts("Waiting for a client\n");
            if ((client_sock_desc1 = accept(server_sock_desc, (struct sockaddr *) &client1, &sockaddr_len)) == ERROR) {
                perror("Accept :");
                exit(-1);
            }
            data_length = 1;
            while (data_length) {
                data_length = recv(client_sock_desc0, data, DATA_LENGTH, 0);
                if (data_length) {
                    send(client_sock_desc1, data, data_length, 0);
                    data[data_length] = '\0';
                    printf("Message from the other user : %s \n", data);
                }
            }

            puts("check2");
            close(client_sock_desc0);
            close(client_sock_desc1);
        }
     */