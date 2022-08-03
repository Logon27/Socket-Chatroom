#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>

struct msgData {
	char nickname[128];
	char msg[128];
};

int main(int argc, char* argv[])
{
	//structs for the message
	static const struct msgData EmptyStruct = {{0}};
	struct msgData message = {{0}};
	
	//socket information
    char* addr;
    char* nickname;
    int sock, port;
    struct sockaddr_in serveraddr;
	
	//bufferSize and select loop variable initialization
    int bufferSize = sizeof(struct msgData);
    int running = 1;
	fd_set readfds;

	//make sure all arguments are input
    if(argc != 4)
    {
        write(1, "Usage: cclient <address> <port> <nickname>\n", 43);
        exit(0);
    }

	//parse the arguments
    addr = argv[1];
    port = atoi(argv[2]);
    nickname = argv[3];
    if((strlen(nickname) + 1) > 128)
    {
        write(1, "Please enter a nickname shorter than 128 characters\n", 52);
        exit(0);
    }

	//establish the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = (short) AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(addr);
    serveraddr.sin_port = htons(port);

    if(connect(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
        write(1, "Failed to connect to server\n", 28);
        exit(0);
    }
    
	//print the nickname to the server
	message = EmptyStruct;
    sprintf(message.nickname, "%s", nickname);
    if(write(sock, &message, bufferSize) < 0)
    {
        write(1, "Failed to write nickname to server\n", 35);
        exit(0);
    }
	
	while(running)
    {
		//set all fds for our select statement
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(0, &readfds);
        if(select(sock+1, &readfds, 0, 0, 0) < 0)
        {
            write(1, "Error on select\n", 16);
        }

		//check for input from stdin
        if(FD_ISSET(0, &readfds))
        {
            char buf[128] = {0};
			char buf2[6] = {0};
            read(0, buf, 128);
			sprintf(buf2, "%.5s", buf);
            if(strcmp(buf2, "exit\n") == 0)
            {
                running = 0;
				//if exit was typed then quit
            }
			
			//make sure the message is shorter that 128 chars including null terminator
			if(buf[127] != 0)
			{
				write(1, "Message cannot be longer than 127 characters...\n", 48);
				//clear the stdin of leftover characters
				int c = buf[127];
				while (c != '\n' && c != EOF) { read(0,&c,1); }
				continue;
			}
			
			//write the message to the socket
			message = EmptyStruct;
			sprintf(message.msg, "%s", buf);
			//did less than or equal to 0 to include an error or a possible connection close
			if(write(sock, &message, bufferSize) <= 0)
			{
				write(1, "Failed to write msg to server\n", 35);
				exit(0);
			}
        }
        
		//check for messages from the socket
        if(FD_ISSET(sock, &readfds))
        {
			//read the message from the socket
			message = EmptyStruct;
			//did less than or equal to 0 to include an error or a possible connection close
			if(read(sock, &message, bufferSize) <= 0)
			{
				write(1, "Error when reading from socket\n", 31);
				write(1, "Connection possibly lost to server\n", 35);
				exit(0);
			}
			//write the message with accompanying nickname
			write(1,"<",1);
			write(1,message.nickname,strlen(message.nickname));
			write(1,"> ",2);
			write(1, message.msg, strlen(message.msg));
			if(strcmp(message.msg, "server is full\n") == 0)
			{
				running = 0;
			}
		}
	}
	//close the socket and exit
	write(1, "Closing Client...\n", 18);
    close(sock);
    exit(0);
}
