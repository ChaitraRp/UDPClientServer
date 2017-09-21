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

#define MAXBUFSIZE 1024

//This function returns packets with fixed size
long getPacketCount(size_t fileSize, long packetSize){
	return(fileSize/packetSize);
}

//This function returns remaining number of bytes
long getRemainingBytes(size_t fileSize, long packetSize){
	return(fileSize%packetSize);
}

int main (int argc, char * argv[] )
{
	int udpSocket;                         //This will be our socket
	struct sockaddr_in sin, clientServer;     //"Internet socket address structure"
	unsigned int clientServerSize;            //length of the sockaddr_in structure
	int sentBytes, recvBytes;              //number of bytes we send and receive in the message
	char recvBuffer[MAXBUFSIZE];               //a buffer to store received message
	char packetBuffer[MAXBUFSIZE];
	char command[100];
	long fileSize, packetSize = 1024, packetCount, remainingBytes, fileSizeReceived = 0, fileSizeSent = 0;
	FILE *fp;
	char filename[100];
	uint32_t ack;
	
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
		bzero(command,sizeof(command));
		//receive command from client
		if((recvfrom(udpSocket, command, sizeof(command), 0,  (struct sockaddr *)&clientServer, &clientServerSize)) < 0){
			printf("\nError in receiving command from the client\n");
			bzero(command,sizeof(command));
		}
		else{
			printf("\nCommand received: %s\n", command);
			bzero(recvBuffer,sizeof(recvBuffer));	
			
			
			//if client says EXIT
			if(strcmp(command, "exit\n") == 0){
				bzero(command,sizeof(command));
				printf("Goodbye!\n");
				close(udpSocket);
				exit(0);
			}
			
			//if client says PUT
			else if(strcmp(command, "put\n") == 0){
				bzero(command,sizeof(command));
				
				//receive file name from client
				if((recvfrom(udpSocket, filename, sizeof(filename), 0, (struct sockaddr *)&clientServer, &clientServerSize)) < 0)
					printf("\nError in receiving file name!\n");
				printf("Filename: %s\n",filename);
				
				//receive filesize from client
				if((recvfrom(udpSocket, &fileSize, sizeof(fileSize), 0, (struct sockaddr *)&clientServer, &clientServerSize)) < 0)
					printf("\nError in receiving file size!\n");
				else{
					printf("File Size: %ld KB\n", fileSize);
					packetCount = getPacketCount(fileSize,packetSize);
					remainingBytes = getRemainingBytes(fileSize,packetSize);
					printf("Number of packets: %ld \n", packetCount);
					printf("Number of remaining bytes: %ld \n", remainingBytes);
					fp = fopen(filename,"wb");
					
					bzero(recvBuffer,sizeof(recvBuffer));
					
					while(fileSizeReceived < fileSize){
						if(packetCount > 0){
							//Now receive the contents of the packet from client
							if(recvfrom(udpSocket, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&clientServer, &clientServerSize) < packetSize){
								printf("\nError in receiving packet content into the buffer!\n");
								//Copy contents of recvBuffer
								strcpy(packetBuffer,recvBuffer);
								//send acknowledgement = 0 if packet transfer error
								ack = 0;
								sendto(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&clientServer, clientServerSize);
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							
							else{
								ack = 1;
								sendto(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&clientServer, clientServerSize);
								if(strcmp(packetBuffer,recvBuffer) == 0)
									strcpy(recvBuffer,packetBuffer);
							}
							
							//write contents of buffer to file put_result
							//printf("%s",recvBuffer);
							if (fwrite(recvBuffer, 1, packetSize, fp) < 0) {
								printf("\nError writing file\n");
								bzero(recvBuffer,sizeof(recvBuffer));
							}
						}//end of if(packetCount > 0)
						
						//handle the remainingBytes if packetCount == 0
						else if(!packetCount && remainingBytes){
							if (recvfrom(udpSocket, recvBuffer, remainingBytes, 0, (struct sockaddr *)&clientServer, &clientServerSize) < remainingBytes) {
								printf("\nError in receiving last bytes\n");
								ack = 0;
								sendto(udpSocket, &ack, sizeof(ack), 0,  (struct sockaddr *)&clientServer, clientServerSize);
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							else {
								ack = 1;
								sendto(udpSocket, &ack, sizeof(ack), 0,  (struct sockaddr *)&clientServer, clientServerSize);
							}
                            
							//write contents of buffer to file put_result
							if (fwrite(recvBuffer, 1, remainingBytes, fp) < 0) {
								printf("\nError writing file\n");
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							fileSizeReceived = fileSizeReceived + remainingBytes;
							printf("\nFile transfer complete.\n");
							fclose(fp);
						}
						fileSizeReceived = fileSizeReceived + packetSize;
						packetCount--;
                        bzero(recvBuffer,sizeof(recvBuffer));
					}//end of inner while
				}
			}//end of PUT
		}//end of receive command check else
	}//end of while
	printf("Closing the socket!\n");
	close(udpSocket);
}
