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

/*
void storeMD5(char[] filename2){
	char md5Command[] = "md5sum ";
	strcat(md5Command,filename2);
	strcat(md5Command," > md5.txt");
	system(md5Command);
	
	if(fp = fopen("md5.txt", "r")){
	bzero(buffer,sizeof(buffer));
	fgets(buffer, 300, fp);
	printf("MD5: %s", buffer);
	bzero(buffer,sizeof(buffer));
	}
}*/

int main (int argc, char * argv[])
{

	int sentBytes, recvBytes;
	int udpSocket;
	char buffer[MAXBUFSIZE];
	char recvBuffer[MAXBUFSIZE];
	char packetBuffer[MAXBUFSIZE];
	struct sockaddr_in remoteServer;
	int remoteServerSize;
	int choice;
	char filename[100];
	FILE *fp;
	long fileSize = 0, packetSize = 1024, packetCount = 0, remainingBytes = 0, fileSizeSent = 0, fileSizeReceived = 0;
	char command[100];
	int ack = 1;

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	//*****************************************************************************************************************************
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
	
	remoteServerSize = sizeof(remoteServer);

	//*****************************************************************************************************************************
	//sendto() sends immediately. It will report an error if the message fails to leave the computer. However, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	while(strcmp(command, "exit\n") != 0){
		printf("\nMENU\n");
		printf("Type get to get the file\nType put to send the file\nType delete to delete the file\nType ls to list the files in directory\nType exit to exit from here\nEnter here: \n");
		fgets(command, 200, stdin);
		
		//send the command to the server
		if((sendto(udpSocket, command, sizeof(command), 0, (struct sockaddr *)&remoteServer, remoteServerSize)) < 0){
			printf("\nError in sending command to the server!\n");
			bzero(command,sizeof(command));
        }
		
		else{		
			//Client exits
			if(strcmp(command, "exit\n") == 0){
				printf("Goodbye!\n");
				close(udpSocket);
				exit(0);
			}
			
			
			//*****************************************************************************************************************************
			//GET command
			else if(strcmp(command, "get\n") == 0){
				printf("\nGET\n");
				printf("Please enter the filename to fetch from the server: ");
				scanf("%s", filename);
				printf("Filename: %s", filename);
				
				//send the filename to server
				if((sendto(udpSocket, filename, sizeof(filename),0, (struct sockaddr *)&remoteServer, remoteServerSize)) < 0)
					printf("\nError sending file name to server!\n");
				
				//receive filesize from client
				if((recvfrom(udpSocket, &fileSize, sizeof(fileSize), 0, (struct sockaddr *)&remoteServer, &remoteServerSize)) < 0)
					printf("\nError in receiving file size!\n");
				else{
					printf("File Size: %ld KB\n", fileSize);
					//calculate the number of packets
					packetCount = getPacketCount(fileSize,packetSize);
					remainingBytes = getRemainingBytes(fileSize,packetSize);
					printf("Number of packets: %ld \n", packetCount);
					printf("Number of remaining bytes: %ld \n", remainingBytes);
					fp = fopen(filename,"wb");
					
					bzero(recvBuffer,sizeof(recvBuffer));
					
					//create a loop to receive the file packet by packet until the fileSizeReceived == fileSize
					while(afileSizeReceived < fileSize){
						if(packetCount > 0){
							//Now receive the contents of the packet from server
							if(recvfrom(udpSocket, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&remoteServer, &remoteServerSize) < packetSize){
								printf("\nError in receiving packet content into the buffer!\n");
								//Copy contents of recvBuffer
								strcpy(packetBuffer,recvBuffer);
								//send acknowledgement = 0 if packet transfer error
								ack = 0;
								sendto(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&remoteServer, remoteServerSize);
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							
							else{
								//send positive acknowledgement if transfer is successful
								ack = 1;
								sendto(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&remoteServer, remoteServerSize);
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
							//receive the remainingBytes
							if (recvfrom(udpSocket, recvBuffer, remainingBytes, 0, (struct sockaddr *)&remoteServer, &remoteServerSize) < remainingBytes) {
								printf("\nError in receiving last bytes\n");
								//if error in receiving last bytes send negative acknowledgement
								ack = 0;
								sendto(udpSocket, &ack, sizeof(ack), 0,  (struct sockaddr *)&remoteServer, remoteServerSize);
								bzero(recvBuffer,sizeof(recvBuffer));
							}
							else {
								//if no error in receiving last bytes send positive acknowledgement
								ack = 1;
								sendto(udpSocket, &ack, sizeof(ack), 0,  (struct sockaddr *)&remoteServer, remoteServerSize);
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
				
			}//end of GET
			
			
			//*************************************************************************************************************************
			//PUT command
			else if(strcmp(command, "put\n") == 0){
				int count = 0;
				printf("\nPUT\n");
				printf("Please enter the filename to send to server: ");
				scanf("%s", filename);
				printf("Filename: %s", filename);
				
				//send the filename to server
				if((sendto(udpSocket, filename, sizeof(filename),0, (struct sockaddr *)&remoteServer, remoteServerSize)) < 0)
					printf("\nError sending file name to server!\n");
				
				//open the file
				if(fp = fopen(filename, "rb")){
					printf("\nFile exists!\n");
					fileSize = getFileSize(fp);
					printf("File Size: %ld KB\n", fileSize);
					//calculate the number of packets
					packetCount = getPacketCount(fileSize,packetSize);
					remainingBytes = getRemainingBytes(fileSize,packetSize);
					printf("Number of packets: %ld \n", packetCount);
					printf("Number of remaining bytes: %ld \n", remainingBytes);
					fseek(fp, 0, SEEK_SET);
					
					//send the filesize to server
					if((sendto(udpSocket, &fileSize, sizeof(fileSize),0, (struct sockaddr *)&remoteServer, remoteServerSize)) < 0)
						printf("\nError sending file size to server!\n");
					
					bzero(buffer,sizeof(buffer));
					
					//re-initialize fileSizeSent
					fileSizeSent = 0;
					
					//create a loop to send the file packet by packet until the fileSizeSent == fileSize
					while(fileSizeSent < fileSize){
						if(packetCount > 0){
							if (fread(buffer, sizeof(char), packetSize, fp) <= 0) {
								printf("\nCopy Error!\n");
								bzero(buffer,sizeof(buffer));
							}
							else{				
								//send the packets to server
								if((sendto(udpSocket, buffer, sizeof(buffer),0, (struct sockaddr *)&remoteServer, remoteServerSize)) < 0){
									printf("\nError sending packets!\n");
									bzero(buffer,sizeof(buffer));
								}
								else{
									usleep(100);
									//receive the ack from the sever
									recvfrom(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&remoteServer, &remoteServerSize);
									//printf("ack received: %d\n",ack);
									printf("%d\n", count);
									count++;
									
									//if ack received is negative, then resend the last packet
									if(ack == 0)
										sendto(udpSocket, buffer, sizeof(buffer),0, (struct sockaddr *)&remoteServer, remoteServerSize);
								}
							}//end of else copy to buffer
						}//end of if(packetCount > 0)
						
					
						//handle the remainingBytes if packetCount == 0
						else if(!packetCount && remainingBytes){
							//read remainingBytes into buffer
							if (fread(buffer, sizeof(char), remainingBytes, fp) <= 0) {
                        	  	printf("\nCopy Error!\n");
								bzero(buffer,sizeof(buffer));
							}
							//send last bytes to server
							if (sendto(udpSocket,buffer, remainingBytes, 0, (struct sockaddr *)&remoteServer, remoteServerSize) < 0){
        	                    printf("\nError in sending the last bytes");
                	            bzero(buffer,sizeof(buffer));
                        	}
							else{
								usleep(100);
								//receive the acknowledgement number
								recvfrom(udpSocket, &ack, sizeof(ack), 0, (struct sockaddr *)&remoteServer, &remoteServerSize);
								printf("%d\n", count);
									count++;
									
								//if acknowledgement is 0, resend the last set of bytes
								if(ack == 0)
									sendto(udpSocket, buffer, remainingBytes,0, (struct sockaddr *)&remoteServer, remoteServerSize);
								
								//calculate the fileSizeSent so far
								fileSizeSent = fileSizeSent + remainingBytes;
								fclose(fp);
								bzero(buffer,sizeof(buffer));
							}
						}
						
						//calculate the fileSizeSent so far
						fileSizeSent = fileSizeSent + packetSize;
						--packetCount;
					}//end of while fileSizeSent < fileSize
				}//end of if fileopen()
				else{
					printf("\nFile does not exist! Please try again...\n");
					bzero(buffer,sizeof(buffer));
				}
			}//end of PUT
			
			
			//*************************************************************************************************************************
			//LS command
			else if(strcmp(command, "ls\n") == 0){
				bzero(recvBuffer,sizeof(recvBuffer));
				bzero(command,sizeof(command));
				if(recvfrom(udpSocket, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&remoteServer, &remoteServerSize) < 0){
					printf("\nError receiving the output of ls\n");
					bzero(recvBuffer,sizeof(recvBuffer));
				}
				else{
					printf("\nls output:\n%s\n", recvBuffer);
					bzero(recvBuffer,sizeof(recvBuffer));
				}
			}//end of LS
			
			
			//*************************************************************************************************************************
			//DELETE command
			else if(strcmp(command, "delete\n") == 0){
				bzero(recvBuffer,sizeof(recvBuffer));
				bzero(command,sizeof(command));
				printf("Please enter the filename to delete: ");
				scanf("%s", filename);
				
				//send the filename to server
				if((sendto(udpSocket, filename, sizeof(filename),0, (struct sockaddr *)&remoteServer, remoteServerSize)) < 0)
					printf("\nError sending file name to server!\n");
				
				else{
					recvfrom(udpSocket, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&remoteServer, &remoteServerSize);
					printf("Server says: %s", recvBuffer);
				}
			}
			
			
			//***********************************************************************************************************************
			//if client types wrong command
			else{
				if(recvfrom(udpSocket, recvBuffer, sizeof(recvBuffer), 0,(struct sockaddr *)&remoteServer, &remoteServerSize)){
					printf("Server says: %s", recvBuffer);
					bzero(recvBuffer,sizeof(recvBuffer));
				}
				else{
					printf("\nDid not receive a reply from the server.\n");
					bzero(recvBuffer,sizeof(recvBuffer));
				}
			}
		}//end of else of sendto of command
	}//end of while

	printf("Closing socket!\n");
	close(udpSocket);
}
