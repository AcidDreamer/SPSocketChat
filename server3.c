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
//Where the client socket descriptors will be stored
static int cl_sc[MAX_CLIENTS];
//client structure for the  clients of type sockaddr_in
static struct sockaddr_in client;
//socket descriptor for the Server
static int server_sock_desc;
//addrlen specifies the size, in bytes, of the address structure pointed to by addr
static int sockaddr_len = sizeof (struct sockaddr_in);
//a mutex to create a lock for the file resource
static  pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

void Sent_to_self();
void Sent_to_all_others();
bool Check_if_empty();
void *server_to_client(void *);
void *server_accepting();
bool create_or_write(char message[]);

int main(int argc, char** argv) {
    //server structure of type sockaddr_in
    struct sockaddr_in server;
    //socket descriptor for the client -->will be stored seperatly at cl_sc[]
    int client_sock_desc;
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
    
    puts("Server is online,at any moment press //quit to terminate it");
    pthread_t sAThread;
    pthread_create(&sAThread, NULL, server_accepting,NULL);
    for(;;){        //infinite loop
        char command[50];
        char *terminate_cmd="//quit\n";
        fgets(command,50,stdin);
        if(strcmp(command,terminate_cmd) == 0){
            int imaquiter;
            for(imaquiter =0 ;imaquiter<MAX_CLIENTS;imaquiter++){
                if(flag[imaquiter]==1){
                    if(close(cl_sc[imaquiter])==0){
                        puts("Client socket disconnected");
                    }else{
                        puts("Error terminating client connection");
                    }
                }
            }
            //we close down the server socket
            if(close(server_sock_desc)==0){puts("Server Socket Terminated");
            }else{puts("Error terminating server socket");}
            pthread_mutex_lock(&fileMutex); //we finally lock the mutex
            //we delete the log file
            if(remove(FILE_NAME)==0){puts("session-log successfully delete!");
            }else{puts("Error deleting session-log,probably there was no session-log.");}
            sleep(10);  //waiting for any awk packets
            puts("Server shut down gracefully");
            return 1;
        }
        
    }
    printf("Hello world");
    return 0;
}

void *server_accepting(){
    /*
    client_sock_desc --> socket descriptor provided after we accept a client
    secondFlag --> flag to indicate if there are any available space for a new client
    whichOne[0] --> contains the socket descriptor of the new client to pass to the server_to client thread
    counter --> just a generic counter used for fors
    */ 
    int client_sock_desc, secondFlag, whichOne[3], counter;
    //initializing flag[]
    for(counter = 0; counter<MAX_CLIENTS;counter++){
        flag[counter]=0;
    }
    puts("Accepting Thread initialized");
    
    //Give a type of "id" to the user after accepting the connection and initialize his thread
    while ((client_sock_desc = accept(server_sock_desc, (struct sockaddr *) &client, (socklen_t*) & sockaddr_len))) {
        //check if there is space for a new client 
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
        //if there is not ,inform and close the clients socket
        if(secondFlag==0){
                char messege[] = "Connection limit exceeded ,try again later.\n";
                send(client_sock_desc, messege, sizeof (messege), 0);
                close(client_sock_desc);
                continue;
        //else initialize his thread
        }else{
            pthread_t sTcThread;
            pthread_create(&sTcThread, NULL, server_to_client, (void*) whichOne);
        }
        //reset the flag
        secondFlag=0;
    }
}

void *server_to_client(void *socket_desc) {
    //type correct the argument thats passed
    int *whichOneImported = (int *) socket_desc;
    //get a local variable for the argument
    int whichOne = whichOneImported[0];
    //one int for retrieved data and one for the clients socket
    int retrieve, socket = cl_sc[whichOne];
    //the actual data
    char data[DATA_LENGTH];

    memset(data, 0, DATA_LENGTH);       //free the string
    
    //if the user is not alone in the server,the moment he connects
    if (!Check_if_empty(whichOne)) {
        char messege[] = "Server ---> Another user entered the room.\n";
        Sent_to_all_others(whichOne, messege, sizeof (messege), 0);
    }
    
    //infinite loop
    for (;;) {
        memset(data, 0, DATA_LENGTH);         //we free the string in everyloop
        retrieve = recvfrom(socket, data, DATA_LENGTH, 0, NULL, NULL);         //we retrieve the data
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
                //we lock and unlock the mutex to make sure 2 client access the file at the same time
                pthread_mutex_lock(&fileMutex);
                create_or_write(data);
                pthread_mutex_unlock(&fileMutex);
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

bool create_or_write(char message[]){
    FILE *f;
    f = fopen(FILE_NAME, "ab+");
    
    if (f == NULL){
        printf("Error opening file!\n");
        return false;
    }
    
    fprintf(f, "%s", message);
    fclose(f);

    return true;
}
