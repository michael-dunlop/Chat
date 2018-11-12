/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:

**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include "dialog.h"
#include <time.h>
//#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <QString>

char * user;
int roomNumMess;
char * currentRoom;
char * password;
char * host = "pod1-5.cs.purdue.edu";
char * sport;
int port = 54321;
int hasJoinedRoom = 0;


#define MAX_MESSAGES 100

int open_client_socket(char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}

	return sock;
}

#define MAX_RESPONSE (10 * 1024)
int sendCommand(char *  host, int port, char * command, char * response) {
	
	int sock = open_client_socket( host, port);

	if (sock<0) {
		return 0;
	}

	// Send command
	write(sock, command, strlen(command));
	write(sock, "\r\n",2);

	//Print copy to stdout
	write(1, command, strlen(command));
	write(1, "\r\n",2);

	// Keep reading until connection is closed or MAX_REPONSE
	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}
	response[len]=0;

	printf("response:\n%s\n", response);

	close(sock);

	return 1;
}

//creates string to be sent in SendCommand, stores it in out
void getCom(char * com,  char * functionOut){
        int inLen = strlen(user) + strlen(password) + strlen(com) + 3;
        char out[inLen];
	int i = 0;

        for(i; i < strlen(com); i++){
                out[i] = *(com + i);
	}
	
	out[i] = ' ';
	i++;

        for(i; i < strlen(com) + strlen(user) + 1; i++){
                out[i] = *(user + i - strlen(com) - 1);
	}	

	out[i] = ' ';
	i++;

	for(i; i < inLen; i++){
                out[i] = *(password + i - strlen(user) - strlen(com) - 2);
	}

        out[inLen] = '\0';

	memcpy(functionOut, out, inLen);

	return;
}

//Overloaded GetCom for Args
void getCom(char * com, char * args, char * functionOut){
        int inLen = strlen(user) + strlen(password) + strlen(com) + strlen(args) + 3;
        char out[inLen];
	int i = 0;

        for(i; i < strlen(com); i++){
                out[i] = *(com + i);
	}
	
	out[i] = ' ';
	i++;

        for(i; i < strlen(com) + strlen(user) + 1; i++){
                out[i] = *(user + i - strlen(com) - 1);
	}	

	out[i] = ' ';
	i++;

        for(i; i < strlen(com) + strlen(user) + strlen(password) + 2; i++){
                out[i] = *(password + i - strlen(user) - strlen(com) - 2);
	}

	out[i] = ' ';
	i++;

        for(i; i < strlen(com) + strlen(user) + strlen(password) + strlen(args) + 3; i++){
                out[i] = *(args + i - strlen(user) - strlen(com) - strlen(password) -3);
	}

    out[strlen(com) + strlen(user) + strlen(password) + strlen(args) + 3] = '\0';

    memcpy(functionOut, out, strlen(out));

    functionOut[strlen(com) + strlen(user) + strlen(password) + strlen(args) + 3] = '\0';
}

int getNumMessages(){
    char response[MAX_RESPONSE];
    char * com = "GET-NUM-MESSAGES";
    char in[strlen(user) + strlen(password) + strlen(com) + strlen(currentRoom) + 3];
    getCom(com, currentRoom, in);
    sendCommand(host, port, in, response);

    return atoi(response);
}

void Dialog::getMessages(){

    if(messageCount >= roomNumMess) return;

    char response[MAX_RESPONSE];

    char * com = "GET-MESSAGES";
    char mcount[MAX_RESPONSE];

    sprintf(mcount, "%d", messageCount);
    strcat(mcount, " ");
    strcat(mcount, currentRoom);


    char in[strlen(user) + strlen(password) + strlen(com) + strlen(mcount)];

    getCom(com, mcount, in);

    sendCommand(host, port, in, response);

    if(strcmp(response, "NO-NEW-MESSAGES\r\n") == 0) return;
    if(strcmp(response, "ERROR (User not in room)\r\n") == 0) return;

    allMessages->append(response);
    messageCount = roomNumMess;


}

void Dialog::getUsersInRoom(){

    char resp[MAX_RESPONSE];
    char * com = "GET-USERS-IN-ROOM";
    char * args = currentRoom;
    char in[strlen(user) + strlen(password) + strlen(com) + strlen(args)];
    getCom(com, args, in);
    sendCommand(host, port, in, resp);
    usersList->addItem(resp);

}

void Dialog::leaveRoom(){

    char resp[MAX_RESPONSE];
    char * com = "LEAVE-ROOM";
    char * args = currentRoom;
    char in[strlen(user) + strlen(password) + strlen(com) + strlen(args)];
    getCom(com, args, in);
    sendCommand(host, port, in, resp);

}

void Dialog::enterRoom(){
    if(hasJoinedRoom != 0){
        leaveRoom();
    }

    if(currentRoom != NULL){
        char response2[MAX_RESPONSE];
        char * com2 = "SEND-MESSAGE";
        QString qsArgs = "has left the room";
        qsArgs.prepend(" ");
        qsArgs.prepend(currentRoom);
        QByteArray baArgs = qsArgs.toLatin1();
        const char * constArgs = baArgs.data();
        char * args2 = new char[baArgs.size()];
        strcpy(args2, constArgs);
        char in2[strlen(user) + strlen(password) + strlen(com2) + strlen(args2) + 3];
        getCom(com2, args2, in2);
        sendCommand(host, port, in2, response2);
    }

    char response[MAX_RESPONSE];
    char * com = "ENTER-ROOM";

    bool ok;
    QString qRName = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                          tr("Room to Join:"), QLineEdit::Normal,
                          QDir::home().dirName(), &ok);

    QByteArray baRName = qRName.toLatin1();
    const char * constRName = baRName.data();
    char * rName = new char[baRName.size()];
    strcpy(rName, constRName);
    char in[strlen(user) + strlen(password) + strlen(com) + strlen(rName) + 3];
    getCom(com, rName, in);
    sendCommand(host, port, in, response);

    if(strcmp(response, "ERROR (No room)\r\n") == 0){
        QMessageBox roomExistsMessage;
        roomExistsMessage.setText("This room doesn't exist! Try again.");
        roomExistsMessage.exec();
        return;
    }


    allMessages->clear();
    usersList->clear();


    currentRoom = rName;

    messageCount = 0;

    //getMessages();
    //getUsersInRoom();


    char response2[MAX_RESPONSE];

    char * com2 = "SEND-MESSAGE";
    QString qsArgs = "has entered the room";
    qsArgs.prepend(" ");
    qsArgs.prepend(currentRoom);
    QByteArray baArgs = qsArgs.toLatin1();
    const char * constArgs = baArgs.data();
    char * args2 = new char[baArgs.size()];
    strcpy(args2, constArgs);
    char in2[strlen(user) + strlen(password) + strlen(com2) + strlen(args2) + 3];
    getCom(com2, args2, in2);
    sendCommand(host, port, in2, response2);




  //  messageCount = roomNumMess;

    for(int i = 0; i < roomsList->count(); i++){
        if(roomsList->item(i)->text() == QString(currentRoom)){
            roomsList->item(i)->setSelected(true);\
            break;
        }
    }

}

void Dialog::makeNewRoom(){

    if(hasJoinedRoom != 0){
        leaveRoom();
    }

    char response[MAX_RESPONSE];
    char * com = "CREATE-ROOM";

    bool ok;
    QString qRName = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                          tr("Room Name"), QLineEdit::Normal,
                          QDir::home().dirName(), &ok);

    QByteArray baRName = qRName.toLatin1();
    const char * constRName = baRName.data();
    char * rName = new char[baRName.size()];
    strcpy(rName, constRName);
    char in[strlen(user) + strlen(password) + strlen(com) + strlen(rName) + 3];
    getCom(com, rName, in);
    sendCommand(host, port, in, response);

    if(strcmp(response, "NO\r\n") == 0){
        QMessageBox roomExistsMessage;
        roomExistsMessage.setText("This room already exists! Try again.");
        roomExistsMessage.exec();
        return;
    }

    if(currentRoom != NULL){
        char response2[MAX_RESPONSE];
        char * com2 = "SEND-MESSAGE";
        char * args2 =  " has left the room ";
        strcat(args2, currentRoom);
        char in2[strlen(user) + strlen(password) + strlen(com2) + strlen(args2) + 3];
        getCom(com2, args2, in2);
        sendCommand(host, port, in2, response2);
    }

    currentRoom = rName;
    allMessages->clear();
    usersList->clear();
    messageCount = 0;


    QListWidgetItem * myRoom = new QListWidgetItem(QString(rName), roomsList, 0);
    for(int i = 0; i < roomsList->count(); i++){
        roomsList->item(i)->setSelected(false);
    }
    myRoom->setSelected(true);


    com = "ENTER-ROOM";
    char in2[strlen(user) + strlen(password) + strlen(com) + strlen(rName) + 3];
    getCom(com, rName, in2);
    sendCommand(host, port, in2, response);
    getUsersInRoom();
}
	
void Dialog::sendAction()
{
	char response[MAX_RESPONSE];

	char * com = "SEND-MESSAGE";
    QString qsArgs = inputMessage->toPlainText();
    qsArgs.prepend(" ");
    qsArgs.prepend(currentRoom);
    QByteArray baArgs = qsArgs.toLatin1();
	const char * constArgs = baArgs.data();
	char * args = new char[baArgs.size()];
	strcpy(args, constArgs);
    char in[strlen(user) + strlen(password) + strlen(com) + strlen(args) + 3];
	getCom(com, args, in);
	sendCommand(host, port, in, response);

    if(strcmp(response, "ERROR (No such room)\r\n") == 0){
        QMessageBox roomExistsMessage;
        roomExistsMessage.setText("You aren't in a room! Try again.");
        roomExistsMessage.exec();
        return;

    }

    char toSend[2];
    sprintf(toSend, "%d", messageCount);
    strcat(toSend, " ");
    strcat(toSend, user);
    QString qToSend = inputMessage->toPlainText();
    QByteArray baToSend = qToSend.toLatin1();
    const char * constToSend = baToSend.data();
    char * inmess = new char[baToSend.size()];
    strcpy(inmess, constToSend);
    strcat(toSend, " ");
    strcat(toSend, inmess);

    allMessages->append(toSend);
    inputMessage->clear();
    messageCount++;

    printf("Send Button\n");
}

void Dialog::newUserAction()
{
  char resp[MAX_RESPONSE];
  allMessages->clear();

    do {
	bool ok;
	QString utext = QInputDialog::getText(this, tr("QInputDialog::getText()"),
					      tr("User name:"), QLineEdit::Normal,
					      QDir::home().dirName(), &ok);
	QString ptext = QInputDialog::getText(this, tr("QInputDialog::getText()"),
					      tr("Password:"), QLineEdit::Normal,
                                              QDir::home().dirName(), &ok);

        QByteArray baUser = utext.toLatin1();
        QByteArray baPass = ptext.toLatin1();
        const char * constUser = baUser.data();
        const char * constPass = baPass.data();
        user = new char[baUser.size()];
        password = new char[baPass.size()];
        strcpy(user, constUser);
        strcpy(password, constPass);

	char * com = "ADD-USER";
	char in[strlen(user) + strlen(password) + strlen(com)];
	getCom(com, in);

        sendCommand(host, port, in, resp);
	
        if(strcmp("OK\r\n", resp) != 0){
		QMessageBox userExistsMessage;
		userExistsMessage.setText("This username is already taken! Try again.");
		userExistsMessage.exec();
        } else {

            break;
        }
       printf("New User Button\n");
     } while (true);



}





void Dialog::getRooms(){
    if(currentRoom == NULL) return;
    char resp[MAX_RESPONSE];
    char * com = "GET-ROOMS";
    char * args = currentRoom;
    char in[strlen(user) + strlen(password) + strlen(com)];
    getCom(com, in);
    sendCommand(host, port, in, resp);
    roomsList->addItem(resp);
}

void Dialog::timerAction()
{
  if(currentRoom == NULL) return;
  roomNumMess = getNumMessages();
  getMessages();
  roomsList->clear();
  getRooms();
  usersList->clear();
  getUsersInRoom();


//    sprintf(message,"Timer Refresh New message %d", messageCount);

}

Dialog::Dialog()
{
    createMenu();

    QVBoxLayout *mainLayout = new QVBoxLayout;

    // Rooms List
    QVBoxLayout * roomsLayout = new QVBoxLayout();
    QLabel * roomsLabel = new QLabel("Rooms");
    roomsList = new QListWidget();
    roomsLayout->addWidget(roomsLabel);
    roomsLayout->addWidget(roomsList);

    // Users List
    QVBoxLayout * usersLayout = new QVBoxLayout();
    QLabel * usersLabel = new QLabel("Users");
    usersList = new QListWidget();
    usersLayout->addWidget(usersLabel);
    usersLayout->addWidget(usersList);

    // Layout for rooms and users
    QHBoxLayout *layoutRoomsUsers = new QHBoxLayout;
    layoutRoomsUsers->addLayout(roomsLayout);
    layoutRoomsUsers->addLayout(usersLayout);

    // Textbox for all messages
    QVBoxLayout * allMessagesLayout = new QVBoxLayout();
    QLabel * allMessagesLabel = new QLabel("Messages");
    allMessages = new QTextEdit;
    allMessagesLayout->addWidget(allMessagesLabel);
    allMessagesLayout->addWidget(allMessages);

    // Textbox for input message
    QVBoxLayout * inputMessagesLayout = new QVBoxLayout();
    QLabel * inputMessagesLabel = new QLabel("Type your message:");
    inputMessage = new QTextEdit;
    inputMessagesLayout->addWidget(inputMessagesLabel);
    inputMessagesLayout->addWidget(inputMessage);

    // Send and new account buttons
    QHBoxLayout *layoutButtons = new QHBoxLayout;
    QPushButton * sendButton = new QPushButton("Send");
    QPushButton * newUserButton = new QPushButton("New Account");
    QPushButton * newRoomButton = new QPushButton("New Room");
    QPushButton * enterRoomButton = new QPushButton("Join Room");
    layoutButtons->addWidget(sendButton);
    layoutButtons->addWidget(newUserButton);
    layoutButtons->addWidget(newRoomButton);
    layoutButtons->addWidget(enterRoomButton);

    // Setup actions for buttons
    connect(sendButton, SIGNAL (released()), this, SLOT (sendAction()));
    connect(newUserButton, SIGNAL (released()), this, SLOT (newUserAction()));
    connect(newRoomButton, SIGNAL (released()), this, SLOT (makeNewRoom()));
    connect(enterRoomButton, SIGNAL (released()), this, SLOT (enterRoom()));

    // Add all widgets to window
    mainLayout->addLayout(layoutRoomsUsers);
    mainLayout->addLayout(allMessagesLayout);
    mainLayout->addLayout(inputMessagesLayout);
    mainLayout->addLayout(layoutButtons);

    // Populate rooms
/*    for (int i = 0; i < 20; i++) {
        char s[50];
        sprintf(s,"Room %d", i);
        roomsList->addItem(s);
    }

    // Populate users
    for (int i = 0; i < 20; i++) {
        char s[50];
        sprintf(s,"User %d", i);
        usersList->addItem(s);
    }

    for (int i = 0; i < 20; i++) {
        char s[50];
        sprintf(s,"Message %d", i);
        allMessages->append(s);
    }
*/
    // Add layout to main window
    setLayout(mainLayout);

    setWindowTitle(tr("CS240 IRC Client"));
    //timer->setInterval(5000);

    messageCount = 0;

    timer = new QTimer(this);
    connect(timer, SIGNAL (timeout()), this, SLOT (timerAction()));
    timer->start(5000);

}


void Dialog::createMenu()

{
    menuBar = new QMenuBar;
    fileMenu = new QMenu(tr("&File"), this);
    exitAction = fileMenu->addAction(tr("E&xit"));
    menuBar->addMenu(fileMenu);

    connect(exitAction, SIGNAL(triggered()), this, SLOT(accept()));
}
