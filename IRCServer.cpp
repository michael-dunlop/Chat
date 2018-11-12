
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string>

#include "IRCServer.h"


//Get Next Word Function
char word[200];

char * nextword(char * str, int start) {
  int c;
  int i = start;

  while ((c = str[i]) != '\0') {
	if(c == ' ' || c == '\t' || c == '\n' || c == '\r'){
		if ((i - start) > 0){
			word[i - start] = '\0';
			return word;
		}
	} else {
		word[i - start] = c;
		i++;
	}
  }
  if((i - start) > 0){
	word[i - start] = '\0';
	return word;
  }

  return NULL;
} //END nextword word function

char * nextword(FILE * fd){
  int c;
  int i = 0;

  while ((c = fgetc(fd)) != EOF){
	if(c == ' ' || c == '\t' || c == '\n' || c == '\r'){
		if(i > 0){
			word[i] = '\0';
			return word;
		}
	} else {
		word[i] = c;
		i++;
	}
  }
  if(i > 0){
	word[i] = '\0';
	return word;
  }

  return NULL;
} //END nextword file function


typedef struct {
	char * username;
	char * password;
	int rooms[100];
} User;

typedef struct {
	char * users[100];
	char * roomName;
	char * messages[100];
	int numUsers;
	int numMessages;
} Room;

int QueueLength = 5;
int userCount = 0;
int roomCount = 0;
const char * myRoomName = (char *) malloc (200 * sizeof(char));
const char * myMessage = (char *) malloc (1000 * sizeof(char));
Room * Rooms;
User * Users;

//test

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

//	printf("The commandLine has the following format:\n");
//	printf("COMMAND <user> <password> <arguments>. See below.\n");
//	printf("You need to separate the commandLine into those components\n");
//	printf("For now, command, user, and password are hardwired.\n");

	int indexOfInput = 0;

	const char * command = strdup(nextword(commandLine, 0));	
	indexOfInput += strlen(command) + 1;
	const char * user = strdup(nextword(commandLine, indexOfInput));
	indexOfInput += strlen(user) + 1;
	const char * password = strdup(nextword(commandLine, indexOfInput));
	indexOfInput += strlen(password) + 1;
	
	const char * args;
	if(strlen(commandLine) > indexOfInput + 1){
		args = strdup(commandLine + indexOfInput);

		myRoomName = strdup(nextword(commandLine, indexOfInput));
		indexOfInput += strlen(myRoomName) + 1;

		if(strlen(commandLine) > indexOfInput){
			myMessage = strdup(commandLine + indexOfInput);
		} else {
			myMessage = "";
		}
	} else {
		args = "";
		myRoomName = "";
		myMessage = "";
	}
	
	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);
	
	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-NUM-MESSAGES")) {
		getNumMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ROOMS")) {
		getRooms(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

void
IRCServer::getNumMessages(int fd, const char * user, const char * password, const char * args)
{

	int roomIndex = -1;

	for(int i = 0; i < roomCount; i++){
		if(strcmp(Rooms[i].roomName, myRoomName) == 0){
			roomIndex = i;
			break;
		}
	}

	char * out = (char *) malloc (5 * sizeof(char));
	sprintf(out, "%d", Rooms[roomIndex].numMessages);
	write(fd, out, strlen(out));

}

void
IRCServer::getRooms(int fd, const char * user, const char * password, const char* args)
{
	char * out = (char *) malloc (1000 * sizeof(char));
	for(int i = 0; i < roomCount; i++){
		strcat(out, Rooms[i].roomName);
		strcat(out, "\t\n");
	}

	write(fd, out, strlen(out));
}

void
IRCServer::initialize()
{
	userCount = 0;
	roomCount = 0;
	Users = (User * ) malloc (1000 * sizeof(User));
	Rooms = (Room * ) malloc (100 * sizeof(Room));

	// Open password file
	FILE * file = fopen("passwords.txt", "a+");

	// Initialize users in room
	char * n = (char *) malloc (100 * sizeof(char));
	int i = 0;
	while((n = nextword(file)) != NULL) {
		if(i == 0){
			Users[userCount].username = strdup(n);
			i = 1;
			continue;
		} else if (i == 1) {
			Users[userCount].password = strdup(n);
			i = 0;
			userCount++;
			continue;
		}
	}

	// Initalize message list
	for(int j = 0; j < 100; j++){
		for(int k = 0; k < 100; k++){
			Rooms[j].messages[k] = (char *) malloc (1000 * sizeof(char));
		}
	}
}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password
	for (int i = 0; i < userCount; i++) {
		if(strcmp(Users[i].username, user) == 0){
			if(strcmp(Users[i].password, password) == 0){
				return true;
			} else {
				return false;
			}
		}
	}
	return false;
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	// Here add a new user. For now always return OK.
	for (int i = 0; i < userCount; i++){
		if(strcmp(Users[i].username, user) == 0){
			return;
		}
	}

	Users[userCount].username = strdup(user);
	Users[userCount].password = strdup(password);
	userCount++;

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
	return;		
}

void
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args){
	if(!checkPassword(fd, user, password)){
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	for(int i = 0; i < roomCount; i++){
		if(strcmp(Rooms[i].roomName, myRoomName) == 0){
			//room already exists
			const char * msg = "NO\r\n";
			write(fd, msg, strlen(msg));
			return;
		}
	}

	Rooms[roomCount].roomName = strdup(myRoomName);
	roomCount++;
	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
}


void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	if(!checkPassword(fd, user, password)){
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	for(int i = 0; i < roomCount; i++){
		if(strcmp(Rooms[i].roomName, myRoomName) == 0){
			for(int j = 0; j < Rooms[i].numUsers; j++){
				if(strcmp(user, Rooms[i].users[j]) == 0){
					//User is already in room, do nothing
					const char * msg = "OK\r\n";
					write(fd, msg, strlen(msg));
					return;
				}
			}
			//User is not in room, add user
			Rooms[i].users[Rooms[i].numUsers] = strdup(user);
			Rooms[i].numUsers++;
			const char * msg = "OK\r\n";
			write(fd, msg, strlen(msg));
			return;
		}
	}

	//Room not found
	const char * msg = "ERROR (No room)\r\n";
	write(fd, msg, strlen(msg));
	return;
}	

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	if(!checkPassword(fd, user, password)){
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	int userIndex = -1; //index of the user to be removed
	int roomIndex = 0; //index of the room the user is in

	for(int i = 0; i < roomCount; i++){
		if(strcmp(Rooms[i].roomName, myRoomName) == 0){
			roomIndex = i;
			for(int j = 0; j < Rooms[i].numUsers; j++){
				if(strcmp(user, Rooms[i].users[j]) == 0){
					userIndex = j;
					break;
				}
			}
			//User not found
			if(userIndex != -1) break;
			const char * msg = "ERROR (No user in room)\r\n";
			write(fd, msg, strlen(msg));
			return;
		}
	}

	//user is found in the proper room, remove user
	for(int i = userIndex; i < Rooms[roomIndex].numUsers; i++){
		Rooms[roomIndex].users[i] = Rooms[roomIndex].users[i+1];
	}

	delete [] Rooms[roomIndex].users[Rooms[roomIndex].numUsers];

	Rooms[roomIndex].numUsers--;
	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
	
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	if(!checkPassword(fd, user, password)){
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	int roomIndex = -1;

	for(int i = 0; i < roomCount; i++){
		if(strcmp(Rooms[i].roomName, myRoomName) == 0){
			roomIndex = i;
			break;
		}
	}

	if(roomIndex == -1){
		const char * msg = "ERROR (No such room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	int userIndex = -1;

	for(int i = 0; i < Rooms[roomIndex].numUsers; i++){
		if(strcmp(Rooms[roomIndex].users[i], user) == 0){
			userIndex = i;
			break;
		}
	}

	if(userIndex == -1){
		const char * msg = "ERROR (user not in room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	char * out = (char *) malloc (1000 * sizeof(char));

	if(Rooms[roomIndex].numMessages >= 100){
		for(int i = 0; i < 100; i++){
			Rooms[roomIndex].messages[i] = Rooms[roomIndex].messages[i+1];
		}
		Rooms[roomIndex].numMessages = 99;
	}

	strcpy(out, user);
	strcat(out, " ");
	strcat(out, myMessage);
	Rooms[roomIndex].messages[Rooms[roomIndex].numMessages] = strdup(out);
	Rooms[roomIndex].numMessages++;
	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	if(!checkPassword(fd, user, password)){
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

//switch RoomName and Message, create startIndex from "RoomName"
	int startIndex = atoi(myRoomName);
	myRoomName = strdup(myMessage);
	
	int rIndex = -1;

	for(int i = 0; i < roomCount; i++){
		if(strcmp(Rooms[i].roomName, myRoomName) == 0){
			rIndex = i;
			break;
		}
	}

	int uIndex = -1;

	for(int i = 0; i < Rooms[rIndex].numUsers; i++){
		if(strcmp(Rooms[rIndex].users[i], user) == 0){
			uIndex = i;
			break;
		}
	}

	if(uIndex == -1){
		const char * msg = "ERROR (User not in room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	if(Rooms[rIndex].numMessages < startIndex) {
		const char * msg = "NO-NEW-MESSAGES\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	char * out = (char *) malloc (1000 * sizeof(char));

	for(int i = startIndex; i < Rooms[rIndex].numMessages; i++){
		sprintf(out, "%d", i);
		strcat(out, " ");
		strcat(out, Rooms[rIndex].messages[i]);
		strcat(out, "\r\n");
		write(fd, out, strlen(out));
	}
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	if(!checkPassword(fd, user, password)){
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	
	int roomIndex = -1;

	for(int i = 0; i < roomCount; i++){
		if(strcmp(Rooms[i].roomName, myRoomName) == 0){
			roomIndex = i;
			break;
		}
	}

	if(roomIndex == -1){
		const char * msg = "ERROR (No such room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	//Sort Usernames
	char * temp = (char *) malloc (100 * sizeof(char));
	for(int i = 0; i < Rooms[roomIndex].numUsers - 1; i++){
		for(int j = i + 1; j < Rooms[roomIndex].numUsers; j++){
			if(strcmp(Rooms[roomIndex].users[i], Rooms[roomIndex].users[j]) > 0){
				temp = strdup(Rooms[roomIndex].users[i]);
				Rooms[roomIndex].users[i] = strdup(Rooms[roomIndex].users[j]);
				Rooms[roomIndex].users[j] = strdup(temp);
			}
		}
	}
	
	char * out = (char *) malloc (100 * sizeof(char));
	for(int i = 0; i < Rooms[roomIndex].numUsers; i++){
		strcpy(out, Rooms[roomIndex].users[i]);
		strcat(out, "\r\n");
		write(fd, out, strlen(out));
//		printf("%s\n", Rooms[roomIndex].users[i]);
	}
	strcpy(out, "\r\n");
	write(fd, out, strlen(out));
}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	if(!checkPassword(fd, user, password)){
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}

	char * tempUser = (char *) malloc (100 * sizeof(char));
	char * tempPass = (char *) malloc (100 * sizeof(char));

	for(int i = 0; i < userCount - 1; i++){
		for(int j = i + 1; j < userCount; j++){
			if(strcmp(Users[i].username, Users[j].username) > 0) {
				tempUser = strdup(Users[i].username);
				tempPass = strdup(Users[i].password);
				Users[i].username = strdup(Users[j].username);
				Users[i].password = strdup(Users[j].password);
				Users[j].username = strdup(tempUser);
				Users[j].password = strdup(tempPass);
			}
		}
	}

	char * out = (char *) malloc (100 * sizeof(char));
	for(int i = 0; i < userCount; i++) {
		strcpy(out, Users[i].username);
		strcat(out, "\r\n");
		write(fd, out, strlen(out));
	}
	strcpy(out, "\r\n");
	write(fd, out, strlen(out));
}

