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
    struct sockaddr_in server;                                  //server structure for the server of type sockaddr_in
    char message[DATA_LENGTH-48], server_reply[DATA_LENGTH];    //messages received & send
    int for_compare;                                            //used for strcmp
    char quit_msg[DATA_LENGTH] = "//quit\n";                    //used for strcmp
     
    if (argc != 3) {            //check arguement count
        printf("The program is used as such.\n arg0 = program , arg1 = port , arg2 = username\n");
        exit(ERROR);
    }
    
    printf("Welcome,%s.Have fun chatting ,press //quit to exit any time you want!.\n",argv[2]);

    sock = socket(AF_INET, SOCK_STREAM, 0);             //Create socket
    if (sock == -1) {                                   //error checking
        printf("Could not create socket");
    }
    puts("Socket created");
    server.sin_addr.s_addr = inet_addr("127.0.0.1");    //set address
    server.sin_family = AF_INET;                        //IPv4
    server.sin_port = htons(atoi(argv[1]));             //set port according to the arguement
    //initialize the data structure
    bzero(&server.sin_zero, 8);                         //zero padding to match a struct sockaddr

    //Connect to remote server
    if (connect(sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
        perror("connect failed. Error");
        exit(ERROR);
    }
    puts("Connected\n");
    pthread_t cTsThread;
    pthread_create(&cTsThread, NULL, client_to_server, NULL);           // creating a thread to handle server messages

    for(;;) {                       //keep communicating with server
        memset(message, 0, DATA_LENGTH);    //in everyloop "clear" the string
        fgets(message,DATA_LENGTH, stdin);  //get message from standard input
        if((for_compare = (strcmp(message,quit_msg))) == 0){    //check if the user wants to quit
            puts("You have exited!");
            pthread_kill(cTsThread, 2);                         //quit
            return 1;
        }
        if((strcmp(message,"\n")==0) || (strlen(message)<=2)){      //check if input is empty or too short
            puts("Input cannot be empty and messages cannot be too short");
            continue;
        }
        /*
        allocate space for a compined message and create a message type of
        username->message
        */
        char *actual_msg = (char *)malloc(2500);
        strcpy(actual_msg,argv[2]);
        strcat(actual_msg,"->");
        strcat(actual_msg,message);
        if (send(sock, actual_msg,strlen(actual_msg), 0) < 0) {                 //in case sending a message fails
            puts("Send failed");
            continue;
        }
    }
    return 0;
}

//thread responsible for capturing everything the server sends to the client
void *client_to_server() {
    int retrieve;                                   //data we retrieve
    char data[DATA_LENGTH];                         //the actual data
    memset(data, 0, DATA_LENGTH);                   //free the string
    for (;;) {
        memset(data, 0, DATA_LENGTH);              //we free the string in everyloop
        retrieve = recvfrom(sock, data, DATA_LENGTH, 0, NULL, NULL);         //we retrieve the data
        if (retrieve < 0) {                                                   //If an error occured
            printf("Error receiving data!\n");
        } else if (retrieve > 0) {                                            //if we retrieved the data succesfully
            fputs(data, stdout);
        } else {                                                            //if the server died
            puts("Server Dropped!\n");
            int xyz = 2;                                                   //terminate the thread
            exit(0);    
            pthread_exit(&xyz);
        }
    }
}
