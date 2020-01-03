#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>

struct msgData {
	char nickname[128];
	char msg[128];
};

int main(int argc, char* argv[])
{
	//required structs for messages and connections
	static const struct msgData EmptyStruct = {{0}};
	struct msgData message = {{0}};
	struct sockaddr_in serveraddr, clientaddr;
	
	//buffers for sending / receiving messages
    int bufferSize = sizeof(struct msgData);
    char printBuffer[75] = {0};
	
	//listener / connection fds
    int listener, conn;
    socklen_t length;

	//for select and the while loop
    int running = 1;
    fd_set readfds;

	//store connected fds and nicknames
    int clientfds[5] = {0};
    char nicknames[5][128];
    memset(nicknames, 0, sizeof(nicknames[0][0]) * 5 * 128);
	
	//keep track of bytes read and written
	int bytesWritten = 0;
	int bytesRead = 0;
    
	//open the server socket and set it to listen on a random port.
    listener = socket( AF_INET, SOCK_STREAM, 0 );
    
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = (short) AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(0); /* bind() will give a unique port. */
    if(bind(listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        write(1, "Error binding to port\n", 22);
    }
    length = sizeof(serveraddr);
    getsockname(listener, (struct sockaddr *)&serveraddr, &length);

    sprintf(printBuffer, "Assigned port number %d\n", ntohs(serveraddr.sin_port));
    write(1, printBuffer, sizeof(printBuffer));
    
    if(listen(listener, 5) < 0)
    {
        write(1, "Error on listening\n", 19);
    }

    length = sizeof(clientaddr);
	
	//keep track of the number of connections
	int connections = 0;

    while(running)
    {
		//set all Fd's for select
        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        FD_SET(0, &readfds);
		
		connections = 0;
		for(int i = 0; i < 5; i++)
        {
            if(clientfds[i] != 0)
			{
				FD_SET(clientfds[i], &readfds);
				connections++;
			}
		}
		
		//add 1 for stdin as well as our connections
        if(select(listener+1+connections, &readfds, 0, 0, 0) < 0)
        {
            write(1, "Error on select\n", 16);
        }

		//if someone types "q" on the server it should just close
        if(FD_ISSET(0, &readfds))
        {
            char ch;
            read(0, &ch, 1);
            if(ch == 'q')
            {
                running = 0;
				break;
            }
        }
        
		//listen for new incoming connections
        if(FD_ISSET(listener, &readfds))
        {
            conn = accept(listener, (struct sockaddr *)&clientaddr, &length);
            if(conn < 0)
            {
                write(1, "Error on accepting\n", 19);
            }
			//accept the connection and read its nickname
            write(1,"Accepted Connection\n", 20);
			message = EmptyStruct;
			bytesRead = read(conn, &message, bufferSize);
            if(bytesRead < 0)
            {
                write(1, "Error on read()\n", 16);
            }
			write(1, "Nickname: ", 10);
			write(1, message.nickname, strlen(message.nickname));
			write(1, "\n", 1);
			//save the client fd and nickname
            for(int i = 0; i < 5; i++)
            {
                if(clientfds[i] == 0)
                {
                    clientfds[i] = conn;
                    strncpy(nicknames[i], message.nickname, 128);
                    break;
                }
                else if(i == 4 && clientfds != 0)
                {
					//return a message if the server is full
					message = EmptyStruct;
					sprintf(message.nickname, "server");
                    sprintf(message.msg, "server is full\n");
                    write(conn, &message, bufferSize);
                    close(conn);
                }
            }
        }

		//loop through all active connections
        for(int i = 0; i < 5; i++)
        {
            if(clientfds[i] != 0 && FD_ISSET(clientfds[i], &readfds))
            {
				//read the message received
				message = EmptyStruct;
				bytesRead = read(clientfds[i], &message, bufferSize);
                if(bytesRead < 0)
                {
                    write(1, "Error on read()\n", 16);
					//if two clients are connected then they are terminated randomly for some reason this is entered
					bzero(nicknames[i], 128);
                    close(clientfds[i]);
					clientfds[i] = 0;
					continue;
                }
				
				//bytesRead == 0 means the client most likely disconnected.
				//also identify if the client "exit"ed
				if(strcmp(message.msg,"exit\n") == 0 || bytesRead == 0)
                {
                    bzero(nicknames[i], 128);
                    close(clientfds[i]);
					clientfds[i] = 0;
					continue;
                }
				
				//send the message with nickname to all registered clients
				sprintf(message.nickname, nicknames[i]);
				for(int j = 0; j < 5; j++)
				{
					if(clientfds[j] != 0 && clientfds[i] != clientfds[j])
					{
						bytesWritten = write(clientfds[j], &message, bufferSize);
						//did less than or equal to 0 to include an error or a possible connection close
						if(bytesWritten <= 0)
						{
							write(1, "Error on write()\n", 16);
							//its possible this is entered on a random client disconnect
							bzero(nicknames[j], 128);
							close(clientfds[j]);
							clientfds[i] = 0;
							continue;
						}
					}
				}
				//print the received message
				write(1, "Received msg: ", 14);
				write(1, message.msg, strlen(message.msg)+1);
            }
        }
    }

	//close the socket and exit
    write(1, "Closing Server...\n", 18);
    close(listener);
    exit(0);
}
