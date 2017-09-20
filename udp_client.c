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

long getFileSize(FILE *fp){
	fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
	return fileSize;
}

// This functions returns packets with fixed size
long getPacketCount(size_t fileSize, long packetSize){
	return(fileSize/packetSize);
}

// This functions returns packets with fixed size
long getRemainingBytes(size_t fileSize, long packetSize){
	return(fileSize%packetSize);
}

int main (int argc, char * argv[])
{

	int sentBytes, recvBytes;                             // number of bytes sent by sendto()
	int udpSocket;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	char recvBuffer[MAXBUFSIZE];
	struct sockaddr_in remoteServer;              //"Internet socket address structure"
	int remoteServerSize;
	int choice;
	char *filename = malloc(50);
	FILE *fp;
	uint32_t fSize;
	long fileSize, packetSize = 1024, packetCount, remainingBytes;

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
		
		//Client exits
		if(choice == 5){
			printf("Goodbye!\n");
			close(udpSocket);
			exit(0);
		}
		
		//PUT command
		else if(choice == 2){
			printf("\nPUT\n");
			printf("Please enter the filename to send to server: ");
			scanf("%s", filename);
			printf("Filename: %s", filename);
			
			if(fp = fopen(filename, "rb")){
				printf("\nFile exists!\n");
				fileSize = getFileSize(fp);
				printf("File Size: %ld KB\n", fileSize);
				fSize = htonl(fileSize);
				packetCount = getPacketCount(fileSize,packetSize);
				remainingBytes = getRemainingBytes(fileSize,packetSize);
				printf("Number of packets: %ld \n", packetCount);
				printf("Number of packets: %ld \n", remainingBytes);
				fseek(fp, 0, SEEK_SET);
				if (fread(buffer, sizeof(char), packetSize, fp) <= 0) {
					printf("\nCopy Error!\n");
                    bzero(buffer,sizeof(buffer));
				}
        		else{
					printf("\nCopy to buffer Successful!\n");
					sentBytes = sendto(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&remoteServer, sizeof(remoteServer));
					if (sentBytes < 0){
						printf("\nError sending file to server!\n");
						bzero(buffer,sizeof(buffer));
                    }
					else{
						recvBytes = recvfrom(udpSocket, recvBuffer, MAXBUFSIZE, 0, (struct sockaddr *)&remoteServer, &remoteServerSize);
						if(recvBytes < 0)
							printf("Error in recvfrom\n");
						else
							printf("Server says %s\n", recvBuffer);
					}
				}
			}
			else{
				printf("\nFile does not exist! Please try again...\n");
			}
		}
		
		//LS command
		else if(choice == 4){
			printf("\nLS\n");
			
			char command[] = "hello";
			remoteServerSize = sizeof(remoteServer);
			sentBytes = sendto(udpSocket, command, strlen(command), 0, (struct sockaddr *)&remoteServer, remoteServerSize);

			if (sentBytes < 0){
				printf("Error in sendto\n");
			}
		
			bzero(buffer,sizeof(buffer));
			recvBytes = recvfrom(udpSocket, recvBuffer, MAXBUFSIZE, 0, (struct sockaddr *)&remoteServer, &remoteServerSize);
			if(recvBytes < 0)
				printf("Error in recvfrom\n");
			else
				printf("Server says %s\n", recvBuffer);
		}
	}

	printf("Closing socket!\n");
	close(udpSocket);
}
