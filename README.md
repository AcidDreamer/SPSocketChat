# SPSocketChat
<b>serverFinal.c</b> and <b>clientFinal.c</b> are the final results of the project ,the rest of the files are just for a brief revision.<br>
server.c    --> Threads are only created for the client, no option for gracefully closing the server.<br>
server2.c   --> Threads are now cerated for accept() and for receiving client requests, server can close gracefully.<br>
server3.c   --> An early emplimentation of session-log.txt ,no acceptable fault tolerance<br><br><br>

client.c    --> Thread for receiving and sending messages , no means for the client to disconnect.<br>


# Server
Compile the server.c with gcc as such<br>
gcc serverFinal.c -o server -lpthread<br>
Run as <br>
./server [port-number] ,
*without the [ ]<br>
# Server Configuration
Go into serverFinal.c ,there you can change the adress the server is listening to , the number of MAX_CLIENTS and the DATA_LENGTH.<br>
# Client
Compile the client.c with gcc as such<br>
gcc clientFinal.c -o client -lpthread<br>
Run as <br>
./client [port-number] [username] ,
*without the [ ]<br>
Run the client after you have launched the server.Preferably launch two clients to test the communication , enjoy!
