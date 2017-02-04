#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>

#define ERROR -1
#define DATA_LENGTH 2048
static int sock;

void *client_to_server();

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    char message[DATA_LENGTH-48], server_reply[DATA_LENGTH];
    char *username;
    int for_compare;
    char quit_msg[DATA_LENGTH] = "//quit\n";
     

    //check arguement count
    if (argc != 3) {
        printf("The program is used as such.\n arg0 = program , arg1 = port , arg2 = username\n");
        exit(ERROR);
    }
    
    username= argv[2];
    printf("Welcome,%s.Have fun chatting ,press //quit to exit any time you want!.\n",username);

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");
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
    puts("Connected\n");
    pthread_t cTsThread;
    pthread_create(&cTsThread, NULL, client_to_server, NULL);

    //keep communicating with server
    for(;;) {
        memset(message, 0, DATA_LENGTH);
        fgets (message,DATA_LENGTH, stdin);
        //Send some data
        if((for_compare = (strcmp(message,quit_msg))) == 0){
            puts("You have exited!");
            pthread_kill(cTsThread, 2);
            return 1;
        }
        char *str = (char *)malloc(2500);
        strcpy(str,argv[2]);
        strcat(str,"->");
        strcat(str,message);
        if (send(sock, str,strlen(str), 0) < 0) {
            puts("Send failed");
            return ERROR;
        }
    }
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
            fputs(data, stdout);
        } else {
            //inform the server
            puts("Server Dropped!\n");
            //terminate the thread
            int xyz = 2;
            exit(0);
            pthread_exit(&xyz);
        }
    }
}
