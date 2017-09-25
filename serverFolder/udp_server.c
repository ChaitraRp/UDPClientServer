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

//This function gives size of the file
long getFileSize(FILE *fp){
	fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
	return fileSize;
}

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
	int udpSocket;
	struct sockaddr_in sin, clientServer;
	int clientServerSize;
	int sentBytes, recvBytes;
	char recvBuffer[MAXBUFSIZE];
	char packetBuffer[MAXBUFSIZE];
	char command[100];
	long fileSize = 0, packetSize = 1024, packetCount = 0, remainingBytes = 0, fileSizeReceived = 0, fileSizeSent = 0;
	FILE *fp;
	char filename[100];
	char deleteCommand[50];
	int ack;
	
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	//******************************************************************************************************************************
	//This code populates the sockaddr_in struct with the information about our socket
	bzero(&sin,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[1]));
	sin.sin_addr.s_addr = INADDR_ANY;


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("Unable to create the socket");
	}


	//******************************************************************************************************************************
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
			//after receiving the command, zero the recvBuffer
			printf("\nCommand received: %s\n", command);
			bzero(recvBuffer,sizeof(recvBuffer));	
			
			
			//if client types EXIT
			if(strcmp(command, "exit\n") == 0){
				bzero(command,sizeof(command));
				printf("Goodbye!\n");
				close(udpSocket);
				exit(0);
			}
			
			
			//************************************************************************************************************************
			//if client types GET
			else if(strcmp(command, "get\n") == 0){
				//receive file name from client
				if((recvfrom(udpSocket, filename, sizeof(filename), 0, (struct sockaddr *)&clientServer, &clientServerSize)) < 0)
					printf("\nError in receiving file name!\n");
				printf("Filename: %s\n",filename);
				
				if(fp = fopen(filename, "rb")){
					printf("\nFile exists!\n");
					//get the size of the file
					fileSize = getFileSize(fp);
					printf("File Size: %ld KB\n", fileSize);
					
					//calculate the packetCount and remainingBytes
					packetCount = getPacketCount(fileSize,packetSize);
					remainingBytes = getRemainingBytes(fileSize,packetSize);
					printf("Number of packets: %ld \n", packetCount);
					printf("Number of remaining bytes: %ld \n", remainingBytes);
					fseek(fp, 0, SEEK_SET);
					
					//send the filesize to client
					if((sendto(udpSocket, &fileSize, sizeof(fileSize),0, (struct sockaddr *)&clientServer, clientServerSize)) < 0)
						printf("\nError sending file size to client!\n");
					
					bzero(recvBuffer,sizeof(recvBuffer));
					
					//initialize fileSizeSent
					fileSizeSent = 0;
					
					//create a loop to send the file packet by packet until the fileSizeSent == fileSize
					while(fileSizeSent < fileSize){
						if(packetCount > 0){
							if (fread(recvBuffer, sizeof(char), packetSize, fp) <= 0) {
								printf("\nCopy Error!\n");
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							else{				
								//send the packets to client
								if((sendto(udpSocket, recvBuffer, sizeof(recvBuffer),0, (struct sockaddr *)&clientServer, clientServerSize)) < 0){
									printf("\nError sending packets!\n");
									bzero(recvBuffer,sizeof(recvBuffer));
								}
								else{
									usleep(100);
									//receive positive ack from the client
									recvfrom(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&clientServer, &clientServerSize);
									printf("ack received: %d\n",ack);
									//if ack is negative, then resend the packet for which the ack was negative
									if(ack == 0)
										sendto(udpSocket, recvBuffer, sizeof(recvBuffer),0, (struct sockaddr *)&clientServer, clientServerSize);
								}
							}//end of else copy to buffer
						}//end of if(packetCount > 0)
						
					
						//handle the remainingBytes if packetCount == 0
						else if(!packetCount && remainingBytes){
							//read remainingBytes into buffer
							if (fread(recvBuffer, sizeof(char), remainingBytes, fp) <= 0) {
                        	  	printf("\nCopy Error!\n");
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							//send last bytes to client
							if (sendto(udpSocket,recvBuffer, remainingBytes, 0, (struct sockaddr *)&clientServer, clientServerSize) < 0){
        	                    printf("\nError in sending the last bytes");
                	            bzero(recvBuffer,sizeof(recvBuffer));
                        	}
							else{
								usleep(100);
								//receive positive ack from the client
								recvfrom(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&clientServer, &clientServerSize);
								printf("ack received: %d\n",ack);
								//if ack is negative, then resend the packet for which the ack was negative
								if(ack == 0)
									sendto(udpSocket, recvBuffer, remainingBytes,0, (struct sockaddr *)&clientServer, clientServerSize);
								
								//calculate the fileSizeSent so far
								fileSizeSent = fileSizeSent + remainingBytes;
								fclose(fp);
							}
						}
						//calculate the fileSizeSent so far
						fileSizeSent = fileSizeSent + packetSize;
						--packetCount;
					}//end of while fileSizeSent < fileSize
				}//end of if fileopen()
				else
					printf("\nFile does not exist! Please try again...\n");
			}//end of GET
			
			
			//************************************************************************************************************************
			//if client types PUT
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
					//calculate the packetCount
					packetCount = getPacketCount(fileSize,packetSize);
					remainingBytes = getRemainingBytes(fileSize,packetSize);
					printf("Number of packets: %ld \n", packetCount);
					printf("Number of remaining bytes: %ld \n", remainingBytes);
					fp = fopen(filename,"wb");
					
					bzero(recvBuffer,sizeof(recvBuffer));
					
					//create a loop to receive the file packet by packet until the fileSizeReceived == fileSize
					while(fileSizeReceived < fileSize){
						if(packetCount > 0){
							//Now receive the contents of the packet from client
							if(recvfrom(udpSocket, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&clientServer, &clientServerSize) < packetSize){
								printf("\nError in receiving packet content into the buffer!\n");
								//Copy contents of recvBuffer
								strcpy(packetBuffer,recvBuffer);
								//send acknowledgement = 0 (negative ack) if there is an error in transfer
								ack = 0;
								sendto(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&clientServer, clientServerSize);
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							
							else{
								//send positive ack if packet is successfully received
								ack = 1;
								sendto(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&clientServer, clientServerSize);
								if(strcmp(packetBuffer,recvBuffer) == 0)
									strcpy(recvBuffer,packetBuffer);
							}
							
							//write contents of buffer to file
							//printf("%s",recvBuffer);
							if (fwrite(recvBuffer, 1, packetSize, fp) < 0) {
								printf("\nError writing file\n");
								bzero(recvBuffer,sizeof(recvBuffer));
							}
						}//end of if(packetCount > 0)
						
						//handle the remainingBytes if packetCount == 0
						else if(!packetCount && remainingBytes){
							//receive remainingBytes
							if (recvfrom(udpSocket, recvBuffer, remainingBytes, 0, (struct sockaddr *)&clientServer, &clientServerSize) < remainingBytes) {
								printf("\nError in receiving last bytes\n");
								//if error in receiving send negative acknowledgement
								ack = 0;
								sendto(udpSocket, &ack, sizeof(ack), 0,  (struct sockaddr *)&clientServer, clientServerSize);
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							else {
								//if successfully then send  positive acknowledgement
								ack = 1;
								sendto(udpSocket, &ack, sizeof(ack), 0,  (struct sockaddr *)&clientServer, clientServerSize);
							}
                            
							//write contents of buffer to file put_result
							if (fwrite(recvBuffer, 1, remainingBytes, fp) < 0) {
								printf("\nError writing file\n");
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							//calculate the fileSizeReceived so far
							fileSizeReceived = fileSizeReceived + remainingBytes;
							printf("\nFile transfer complete.\n");
							fclose(fp);
						}
						
						//calculate the fileSizeReceived so far
						fileSizeReceived = fileSizeReceived + packetSize;
						packetCount--;
                        bzero(recvBuffer,sizeof(recvBuffer));
					}//end of inner while
				}
			}//end of PUT
			
			//***********************************************************************************************************************
			//if client types ls
			else if(strcmp(command, "ls\n") == 0){
				bzero(recvBuffer,sizeof(recvBuffer));
				system("ls > ls_output.txt");
				printf("\nls command executed and output stored in ls_output.txt\n");
				
				//read the file ls_output.txt and store it into buffer
				fp = fopen("ls_output.txt", "rb");
                fileSize = getFileSize(fp);
				printf("Filesize: %ld KB", fileSize);
                fseek(fp, 0, SEEK_SET);
				
				if (fread(recvBuffer, sizeof(char), fileSize, fp) <= 0){
					printf("\nCopy error!\n");
					bzero(recvBuffer,sizeof(recvBuffer));
				}
				else{
					if (sendto(udpSocket,recvBuffer,strlen(recvBuffer), 0, (struct sockaddr *)&clientServer, clientServerSize) < 0){
						printf("\nError in send to\n");
						bzero(recvBuffer,sizeof(recvBuffer));
					}
					else{
						printf("\nls output sent successfully\n");
						fclose(fp);
						bzero(recvBuffer,sizeof(recvBuffer));
					}
				}
			}//end of LS
			
			
			
			//***********************************************************************************************************************
			//if client types delete
			else if(strcmp(command, "delete\n") == 0){
				//receive file name from client
				if((recvfrom(udpSocket, filename, sizeof(filename), 0, (struct sockaddr *)&clientServer, &clientServerSize)) < 0)
					printf("\nError in receiving file name!\n");
				printf("Filename: %s\n",filename);
				
				//check if file exists
				if(!access(filename, F_OK )){
					printf("\nFile exists!\n");
					//command to delete the file
					if(!remove(filename)){
						char msg[] = "File successfully removed.";
						printf("\n%s\n", msg);
						sendto(udpSocket, msg, sizeof(msg),0, (struct sockaddr *)&clientServer, clientServerSize);
					}
					else{
						char msg[] = "Not able to delete the file!";
						printf("\n%s\n", msg);
						sendto(udpSocket, msg, sizeof(msg),0, (struct sockaddr *)&clientServer, clientServerSize);
					}
				}
				else{
					char msg[] = "File does not exist! Please try again...";
					printf("\n%s\n", msg);
					sendto(udpSocket, msg, sizeof(msg),0, (struct sockaddr *)&clientServer, clientServerSize);
				}
			}//end of delete
			
			
			
			//***********************************************************************************************************************
			//if client types wrong command
			else{
				if (sendto(udpSocket,command,strlen(command), 0, (struct sockaddr *)&clientServer, clientServerSize) < 0){
					printf("\nError in sendto. Did not understand the command\n");
					bzero(command,sizeof(command));
				}
				else{
					printf("\nInvalid command!\n");
					bzero(command,sizeof(command));
				}
			}
		}//end of receive command check else
	}//end of while
	printf("Closing the socket!\n");
	close(udpSocket);
}
