#How to run the code:

There are two folders:
-	serverFolder
-	clientFolder

serverFolder has udp_server.c and a Makefile
clientFolder has udp_client.c and a Makefile

To run: type make on command line from both serverFolder and clientFolder
To clean the object file: run make clean on command line from both serverFolder and clientFolder

--------------------------------------------------------------------------------------------------------------------------------

#Reliability using Positive/Negative Ack
-	The size of the file to be fetched from the remote server or transferred to the remote server is calculated. The packetSize is set and number of packets is calculated and last remaining bytes is calculated.
-	A while loop runs over the fileSize until the fileSizeTransfered is equal to the fileSize
-	Transfer happens packet by packet and for every packet sent to the remote server, the remote server sends an acknowledgement.
-	The ack value is an integer value and there are two types: 0 (negative) and 1 (positive)
-	The client server sends a 0 if there is an error in packet transfer and the remote server will resend that particular packet for which it received the negative ack 
-	If the remote server receives a positive ack, then it knows that the client has received the last sent packet successfully. So it will move ahead and send the next packet.

----------------------------------------------------------------------------------------------------------------------------------

#Observations

Test files used:
File name	File size (in MB)
6mb.jpg		6.3
15mb.jpg	15.4
30mb.pdf	31.6
50mb.pdf	51.2
80mb.pdf	79.5
100mb.pdf	101.7

Client and Server on same machine
All the four commands: get, put, ls, delete works normally for all file sizes. Tested up to 100mb.
Packet size: 1024 and 10000
File transfer is successful.
MD5 sum matches

Client and Server on remote machines
All 4 commands work normally for file sizes of 6mb.
Packet Size: 1024
File transfer successful
MD5 sum matches

-	put and get works normally for file size of 6mb. 
-	put and get command Does not work for file size > 15mb.
-	ls and delete works normally on remote server
