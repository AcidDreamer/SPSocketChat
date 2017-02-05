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

#define MAX_CLIENTS 2
#define DATA_LENGTH 2048
#define ERROR -1
#define FILE_NAME "session-log.txt"

//A checking mechanism for the empty server slots --> flag[x]==0 then its empty or if flag[x] ==1 the spot is habitted
static int flag[MAX_CLIENTS] ;  
static int cl_sc[MAX_CLIENTS];                  //Where the client socket descriptors will be stored
static struct sockaddr_in client;               //client structure for the  clients of type sockaddr_in
static int server_sock_desc;                    //socket descriptor for the Server
static int sockaddr_len = sizeof(struct sockaddr_in);   //addrlen specifies the size, in bytes, of the address structure pointed to by addr
static  pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;          //a mutex to create a lock for the file resource

void Sent_to_self();
void Sent_to_all_others();
bool Check_if_empty();
void *server_to_client(void *);
void *server_accepting();
bool create_or_write(char message[]);
void read_all_messages(int sendFrom);

int main(int argc, char** argv) {
    struct sockaddr_in server;      //server structure of type sockaddr_in
    int client_sock_desc;           //socket descriptor for the client -->will be stored seperatly at cl_sc[]
    int data_length;                //Number of bytes of a message
    char data[DATA_LENGTH];         //The actual messege

    //check arguement count
    if (argc != 2) {
        printf("The program is used as such.\n arg0 = program , arg1 = port\n");
        exit(ERROR);
    }

    
    //Initializing the socket
    if ((server_sock_desc = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        perror("Server socket :");
        exit(-1);
    }
    //Indicates that the rules used in validating addresses supplied in a bind(2) call should allow reuse of local addresses
    //For AF_INET sockets this means that a socket may bind, except when  there is an active listening socket bound to the address.
    int enable = 1;
    if (setsockopt(server_sock_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");
        
    //setting up the server data structure
    server.sin_family = AF_INET; //IPv4     
    server.sin_port = htons(atoi(argv[1]));     //set-up the port via argument
    //contains the IP address that will be associated with the socket
    server.sin_addr.s_addr = INADDR_ANY; //kernel listens to all interfaces,0.0.0.0
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
    
    puts("Server is online,at any moment press //quit to terminate it");
    pthread_t sAThread;                                                 
    pthread_create(&sAThread, NULL, server_accepting,NULL);             //create a thread responsible for thread accepting requests
    for(;;){        //infinite loop
        char command[50];
        char *terminate_cmd="//quit\n";
        fgets(command,50,stdin);
        if(strcmp(command,terminate_cmd) == 0){                         //if the user types //quit
            int imaquiter;
            for(imaquiter =0 ;imaquiter<MAX_CLIENTS;imaquiter++){       //we close all server socket 1 by 1
                if(flag[imaquiter]==1){
                    if(close(cl_sc[imaquiter])==0){
                        puts("Client socket disconnected");
                    }else{
                        puts("Error terminating client connection");
                    }
                }
            }
            if(close(server_sock_desc)==0){                //we close down the server socket
                puts("Server Socket Terminated");
            }else{
                puts("Error terminating server socket");
            }
            pthread_mutex_lock(&fileMutex);                 //we finally lock the mutex
            if(remove(FILE_NAME)==0){                       //we delete the log file
                puts("session-log successfully delete!");
            }else{
                puts("Error deleting session-log,probably there was no session-log.");
            }
            sleep(3);  //waiting for any awk packets
            puts("Server shut down gracefully");
            return 1;
        }
        
    }
    return 0;
}

//thread responsible for handling accept
void *server_accepting(){
    /*
    client_sock_desc --> socket descriptor provided after we accept a client
    secondFlag --> flag to indicate if there are any available space for a new client
    whichOne[0] --> contains the socket descriptor of the new client to pass to the server_to client thread
    counter --> just a generic counter used for fors
    */ 
    int client_sock_desc, secondFlag, whichOne[3], counter;
    
    for(counter = 0; counter<MAX_CLIENTS;counter++){        //initializing flag[]
        flag[counter]=0;
    }
    puts("Accepting Thread initialized");
    
    //Give a type of "id" to the user after accepting the connection and initialize his thread
    while ((client_sock_desc = accept(server_sock_desc, (struct sockaddr *) &client, (socklen_t*) & sockaddr_len))) {
        for(counter=0;counter<MAX_CLIENTS;counter++){               //check if there is space for a new client 
            if (flag[counter] == 0) {
                cl_sc[counter] = client_sock_desc;                  //store the socket
                whichOne[0] = counter;          //set id
                flag[counter] = 1;              //raise the flag
                puts("Client accepted");
                secondFlag=1;                   //raise the second flag
                break;  
            } 
        }
        if(secondFlag==0){                      //if there is not ,inform and close the clients socket
                char messege[] = "Connection limit exceeded ,try again later.\n";
                send(client_sock_desc, messege, sizeof (messege), 0);
                close(client_sock_desc);
                continue;
        }else{                                  //else initialize his thread

            pthread_t sTcThread;
            pthread_create(&sTcThread, NULL, server_to_client, (void*) whichOne);
        }
        secondFlag=0;                           //reset the flag
    }
}

//thread responsible for one client 
void *server_to_client(void *socket_desc) {
    int *whichOneImported = (int *) socket_desc;    //type correct the argument thats passed
    int whichOne = whichOneImported[0];             //get a local variable for the argument
    int retrieve, socket = cl_sc[whichOne];         //one int for retrieved data and one for the clients socket
    char data[DATA_LENGTH];                          //the actual data
    read_all_messages(whichOne);
    memset(data, 0, DATA_LENGTH);                    //free the string
    
    if (!Check_if_empty(whichOne)) {                //if the user is not alone in the server,the moment he connects
        char messege[] = "Server ---> Another user entered the room.\n";
        Sent_to_all_others(whichOne, messege, sizeof (messege), 0);
    }
    
    for (;;) {          //infinite loop
        memset(data, 0, DATA_LENGTH);         //we free the string in everyloop
        retrieve = recvfrom(socket, data, DATA_LENGTH, 0, NULL, NULL);         //we retrieve the data
        if (Check_if_empty(whichOne)) {                             //if the user is alone in the server
            char messege[] = "Server ---> You are alone in the room.\n"; 
            Sent_to_self(whichOne, messege, sizeof(messege), 0);    //inform him
        } else {
            if (retrieve < 0) {                                     //If an error occured
                printf("Error receiving data!\n");
            } else if (retrieve > 0) {                              //if we retrieved the data succesfully
                create_or_write(data);                              //use data retrieved to expand or create the session-log.txt
                Sent_to_all_others(whichOne, data, DATA_LENGTH, 0); //send the message to all other users
            } else if(retrieve==0) {                                 //if the user disconnected
                close(cl_sc[whichOne]);
                sleep(2);
                char messege[] = "\nServer ---> A user disconnected.\n";                        //Inform the other user
                if(!Check_if_empty(whichOne))
                    Sent_to_all_others(whichOne, messege, sizeof (messege), 0);
                puts("User disconnected!\n");       //inform the server
                flag[whichOne] = 0;                  //fix the flags
                return NULL;                         //terminate the thread
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
    sleep(1);
    send(cl_sc[sendFrom], messege, sizeOfIt, 0);

}

//creates or appends to the current file the users message
bool create_or_write(char message[]){
    FILE *f;                            //create a pointer to a file
    pthread_mutex_lock(&fileMutex);     //lock the file mutex
    f = fopen(FILE_NAME, "ab+");        //point it to the file
    if (f == NULL){                     //if the file couldn't be read or created
        printf("Error opening file!\n");
        pthread_mutex_unlock(&fileMutex);
        return false;
    }
    fprintf(f, "%s", message);          //print in the file the string provided
    fclose(f);
    pthread_mutex_unlock(&fileMutex);   //unlock the mutex
    return true;
}

//reads all the messages a user has missed and sents the log back to him
void read_all_messages(int sendFrom){
    char * buffer = 0;
    long length;
    pthread_mutex_lock(&fileMutex);      //we lock the file resource
    FILE * f = fopen (FILE_NAME, "rb");  //give the file a pointer
    
    if(f){
        fseek (f, 0, SEEK_END);         //Find the end of the file
        length = ftell (f);             //get the end as a number
        fseek (f, 0, SEEK_SET);         //set the file position indicator for the stream pointed to by stream.  
        buffer = malloc(length);        //create memory for the buffer
        if(buffer){                     //if the buffer isn't empty
            fread (buffer, 1, length, f);   //read and store in the buffer
        }
        fclose (f);                     //close the file
        pthread_mutex_unlock(&fileMutex);   //unlock the mutex
        if(buffer){     //if the buffer isn't empty
            char inf_message_st[] = "Server-->Previous conversations start here\n";
            Sent_to_self(sendFrom,inf_message_st,sizeof(inf_message_st));           //inform of any previous messages
            Sent_to_self(sendFrom,buffer,length);                                   //send the contexts of the buffer
            char inf_message_end[] = "Server-->Previous conversations end here\n";
            Sent_to_self(sendFrom,inf_message_end,sizeof(inf_message_end));         //inform that previous messages end here
        }
    }else if(f==NULL){
      char no_conv[] = "Server-->No conversations above\n";                         //inform the user that he didn't miss anything
      Sent_to_self(sendFrom,no_conv,sizeof(no_conv));
      pthread_mutex_unlock(&fileMutex);                                             //unlock the file resource
    }
}
