# SPSocketChat
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
