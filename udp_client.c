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
#include <errno.h>
#include <string.h>

#define MAXBUFSIZE 100

int main (int argc, char * argv[])
{

	int sentBytes, recvBytes;                             // number of bytes sent by sendto()
	int udpSocket;                               //this will be our socket
	char buffer[MAXBUFSIZE];

	struct sockaddr_in remoteServer;              //"Internet socket address structure"
	int remoteServerSize;
	int choice;

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	//----------------------------------------------------------------------------------------------------------------
	//Here we populate a sockaddr_in struct with information regarding where we'd like to send our packet
	bzero(&remoteServer,sizeof(remoteServer));               //zero the struct
	remoteServer.sin_family = AF_INET;                 //address family
	remoteServer.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remoteServer.sin_addr.s_addr = inet_addr(argv[1]); //sets server IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	//-----------------------------------------------------------------------------------------------------------------
	//sendto() sends immediately. It will report an error if the message fails to leave the computer. However, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	while(choice != 5){
		printf("\nMENU\n");
		printf("Enter 1 to GET the file\nEnter 2 to PUT the file\nEnter 3 to DELETE the file\nEnter 4 to LIST the file\nEnter 5 to EXIT from here\nEnter your choice: \n");
		scanf("%d", &choice);
		
		if(choice == 5){
			printf("Goodbye!\n");
			close(udpSocket);
			exit(0);
		}
		
		else if(choice == 4){
			printf("\nLS\n");
			
			char command[] = "hello";	
			sentBytes = sendto(udpSocket, command, strlen(command), 0, (struct sockaddr *)&remoteServer, sizeof(remoteServer));

			if (sentBytes < 0){
				printf("Error in sendto\n");
			}
		
			remoteServerSize = sizeof(remoteServer);
			bzero(buffer,sizeof(buffer));
			recvBytes = recvfrom(udpSocket, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remoteServer, &remoteServerSize);  

			printf("Server says %s\n", buffer);
		}
	}

	printf("Closing socket!\n");
	close(udpSocket);
}
