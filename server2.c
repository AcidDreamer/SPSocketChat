#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <stdbool.h>

#define MAX_CLIENTS 4
#define DATA_LENGTH 2048
#define ERROR -1

//A checking mechanism for the empty server slots
static int flag[MAX_CLIENTS] ;
//Where the client socket descriptors will be stored
static int cl_sc[MAX_CLIENTS];
//client structure for the  clients
static struct sockaddr_in client;
//socket descriptor for the Server
static int server_sock_desc;
static int sockaddr_len = sizeof (struct sockaddr_in);

void Sent_to_self();
void Sent_to_all_others();
bool Check_if_empty();
void *server_to_client(void *);
void *server_accepting();

int main(int argc, char** argv) {
    //server structure
    struct sockaddr_in server;
    //both are of type sockaddr_in

    //socket descriptor for the first client
    int client_sock_desc;
    //socket descriptor for the second client
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
    puts("Server is online");
    pthread_t sAThread;
    pthread_create(&sAThread, NULL, server_accepting,NULL);
    for(;;){
        char command[50];
        char *terminate_cmd="//quit\n";
        fgets(command,50,stdin);
        if(strcmp(command,terminate_cmd) == 0){
            int imaquiter;
            for(imaquiter =0 ;imaquiter<MAX_CLIENTS;imaquiter++){
                if(flag[imaquiter]==1){
                    puts("Client socket disconnected");
                    close(cl_sc[imaquiter]);
                }
            }
            puts("Server Socket Terminated");
            close(server_sock_desc);
            puts("Server closed gracefully");
            pthread_kill(sAThread,2);
            return 1;
        }
        
    }
    printf("Hello world");
    return 0;
}

void *server_accepting(){
    int client_sock_desc;
    int secondFlag;
    //if the user is first or the second user to connect
    int whichOne[3];
     //0 every value;
    int counter;
    for(counter = 0; counter<MAX_CLIENTS;counter++){
        flag[counter]=0;
    }
    //Give a type of "id" to the user after accepting the connection
    while ((client_sock_desc = accept(server_sock_desc, (struct sockaddr *) &client, (socklen_t*) & sockaddr_len))) {
        for(counter=0;counter<MAX_CLIENTS;counter++){
            if (flag[counter] == 0) {
                //store the socket
                cl_sc[counter] = client_sock_desc;
                //set id
                whichOne[0] = counter;
                //raise the flag
                flag[counter] = 1;
                puts("Client accepted");
                //raise the second flag
                secondFlag=1;
                break;
            } 
        }
        if(secondFlag==0){
                //send messege informing the user
                char messege[] = "Connection limit exceeded ,try again later.\n";
                send(client_sock_desc, messege, sizeof (messege), 0);
                //close the connection
                close(client_sock_desc);
                //continue looping
                continue;
        }else{
            //create thread and pass cl_sc as arguement
            pthread_t sTcThread;
            pthread_create(&sTcThread, NULL, server_to_client, (void*) whichOne);
        }
        secondFlag=0;
    }
}
void *server_to_client(void *socket_desc) {
    //make the arguement readable 
    int *whichOneImported = (int *) socket_desc;
    int whichOne = whichOneImported[0];
    //one int for retrieved data and one for his socket
    int retrieve, socket = cl_sc[whichOne];

    //the actual data
    char data[DATA_LENGTH];
    //free the string
    memset(data, 0, DATA_LENGTH);
    //if the user is not alone in the server
    if (!Check_if_empty(whichOne)) {
        char messege[] = "Server ---> Another user entered the room.\n";
        Sent_to_all_others(whichOne, messege, sizeof (messege), 0);
    }
    for (;;) {
        //we free the string in everyloop
        memset(data, 0, DATA_LENGTH);
        //we retrieve the data
        retrieve = recvfrom(socket, data, DATA_LENGTH, 0, NULL, NULL);
        //if the user is alone in the server
        if (Check_if_empty(whichOne)) {
            char messege[] = "Server ---> You are alone in the room.\n";
            Sent_to_self(whichOne, messege, sizeof(messege), 0);
        } else {
            //If an error occured
            if (retrieve < 0) {
                printf("Error receiving data!\n");
                //if we retrieved the data succesfully
            } else if (retrieve > 0) {
                Sent_to_all_others(whichOne, data, DATA_LENGTH, 0);
                printf("server: ");
                fputs(data, stdout);
                printf("Sent from %d from socket %d \n",whichOne,cl_sc[whichOne]);
            //if the user disconnected
            } else if(retrieve==0) {
                close(cl_sc[whichOne]);
                puts("check1\n");
                //Inform the other user
                char messege[] = "\nServer ---> A user disconnected.\n";
                if(!Check_if_empty(whichOne))
                    Sent_to_all_others(whichOne, messege, sizeof (messege), 0);
                puts("check2\n");
                //inform the server
                puts("User disconnected!\n");
                //fix the flags
                flag[whichOne] = 0;
                //terminate the thread
                puts("check3\n");
                return NULL;
            }
        }
    }
}


//Send the messege to all other users ,without echoing
void Sent_to_all_others(int sendFrom,char messege[],int sizeOfIt){
    int counter;
    for(counter=0;counter<MAX_CLIENTS;counter++){
        if( (flag[counter] != 0) && sendFrom != counter ){
                send(cl_sc[counter], messege, sizeOfIt, 0);
        }
    }
}

//Check if the user is alone in the room
bool Check_if_empty(int sendFrom){
    int counter;
    for(counter=0;counter<MAX_CLIENTS;counter++){
        if(flag[counter] == 1 &&  counter != sendFrom){
            return false;
        }
    }
    return true;
}

//Send the messege to the user
void Sent_to_self(int sendFrom,char messege[],int sizeOfIt){
    send(cl_sc[sendFrom], messege, sizeOfIt, 0);

}
