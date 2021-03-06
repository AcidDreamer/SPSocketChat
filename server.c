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

void *server_to_client(void *);
static int flag[2] = {0, 0};
//Where the client socket descriptors will be stored
static int cl_sc[MAX_CLIENTS];

int main(int argc, char** argv) {
    //server structure
    struct sockaddr_in server;
    //client structure for the first client
    struct sockaddr_in client;
    //both are of type sockaddr_in
    //socket descriptor for the Server
    int server_sock_desc;

    //if the user is first or the second user to connect
    int whichOne[3];
    //socket descriptor for the first client
    int client_sock_desc;
    //socket descriptor for the second client
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
    //Give a type of "id" to the user after accepting the connection
    while ((client_sock_desc = accept(server_sock_desc, (struct sockaddr *) &client, (socklen_t*) & sockaddr_len))) {
        //for the first user
        if (flag[0] == 0) {
            //store the socket
            cl_sc[0] = client_sock_desc;
            //set id
            whichOne[0] = 0;
            //raise the flag
            flag[0] = 1;
            puts("Client accepted");
            //second user ,same procedure
        } else if (flag[1] == 0) {
            cl_sc[1] = client_sock_desc;
            whichOne[0] = 1;
            //raise the flag
            flag[1] = 1;
            puts("Client accepted");
        } else {
            //send messege informing the user
            char messege[] = "Connection limit exceeded ,try again later.\n";
            send(client_sock_desc, messege, sizeof (messege), 0);
            //close the connection
            close(client_sock_desc);
            //continue looping
            continue;
        }
        //create thread and pass cl_sc as arguement
        pthread_t sTcThread;
        pthread_create(&sTcThread, NULL, server_to_client, (void*) whichOne);

    }
    printf("Hello world");
    return 0;
}

void *server_to_client(void *socket_desc) {
    //make the arguement readable 
    int *whichOneImported = (int *) socket_desc;
    int whichOne = whichOneImported[0];
    //one int for retrieved data and one for his socket
    int retrieve, socket = cl_sc[whichOne];
    //chat buddy socket and his flag i
    int palsSocket, localFlag;

    //the actual data
    char data[DATA_LENGTH];
    //free the string
    memset(data, 0, DATA_LENGTH);
    //set accordingly before receiving the message indicating the other part of the flag
    if (whichOne == 0) {
        palsSocket = cl_sc[1];
        localFlag = 1;
    } else if (whichOne == 1) {
        palsSocket = cl_sc[0];
        localFlag = 0;
    }
    //if the user is not alone in the server
    if (flag[localFlag] == 1) {
        char messege[] = "Server ---> Another user entered the room.\n";
        send(palsSocket, messege, sizeof (messege), 0);
    }
    for (;;) {
        //we free the string in everyloop
        memset(data, 0, DATA_LENGTH);
        //we retrieve the data
        retrieve = recvfrom(socket, data, DATA_LENGTH, 0, NULL, NULL);
        //set accordingly before sending the message
        if (whichOne == 0) {
            palsSocket = cl_sc[1];
            localFlag = 1;
        } else if (whichOne == 1) {
            palsSocket = cl_sc[0];
            localFlag = 0;
        }
        //if the user is alone in the server
        if (flag[localFlag] != 1) {
            char messege[] = "Server ---> You are alone in the room.\n";
            send(socket, messege, sizeof (messege), 0);
        } else {
            //If an error occured
            if (retrieve < 0) {
                printf("Error receiving data!\n");
                //if we retrieved the data succesfully
            } else if (retrieve > 0) {
                send(palsSocket, data, DATA_LENGTH, 0);
                printf("server: ");
                fputs(data, stdout);
                //printf("\n");
                //if the user disconnected
            } else {
                //Inform the other user
                char messege[] = "\nServer ---> The other user disconnected.\n";
                send(palsSocket, messege, sizeof (messege), 0);
                //inform the server
                puts("User disconnected!\n");
                //fix the flags
                flag[whichOne] = 0;
                //terminate the thread
                int xyz = 2;
                pthread_exit(&xyz);
            }
        }
    }
}
