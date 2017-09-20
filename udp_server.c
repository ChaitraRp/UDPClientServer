#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define MAXBUFSIZE 1000000

int main (int argc, char * argv[] )
{
	int udpSocket;                         //This will be our socket
	struct sockaddr_in sin, clientServer;     //"Internet socket address structure"
	unsigned int clientServerSize;            //length of the sockaddr_in structure
	int sentBytes, recvBytes;              //number of bytes we send and receive in the message
	char recvBuffer[MAXBUFSIZE];               //a buffer to store received message
	
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	//------------------------------------------------------------------------------------------------------------
	//This code populates the sockaddr_in struct with the information about our socket
	bzero(&sin,sizeof(sin));                  //zero the struct
	sin.sin_family = AF_INET;                   //get server machine ip
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("Unable to create the socket");
	}


	//-------------------------------------------------------------------------------------------------------------
	//Once we've created a socket, we must bind that socket to the local address and port we've supplied in the sockaddr_in struct
	if (bind(udpSocket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("Unable to bind socket\n");
	}

	clientServerSize = sizeof(clientServer);

	while (1) {
		bzero(recvBuffer,sizeof(recvBuffer));
		recvBytes = recvfrom(udpSocket, recvBuffer, MAXBUFSIZE, 0, (struct sockaddr *)&clientServer, &clientServerSize);
		if (recvBytes < 0)
			printf("Error in receiving the message.\n");

		printf("The client says %s\n", recvBuffer);

		char msg[] = "received";
		sentBytes = sendto(udpSocket, msg, strlen(msg), 0, (struct sockaddr *)&clientServer, clientServerSize);
		if (sentBytes < 0){
			printf("Error sending message.\n");
		}
		bzero(recvBuffer,sizeof(recvBuffer));
	}
	printf("Closing the socket!\n");
	close(udpSocket);
}
