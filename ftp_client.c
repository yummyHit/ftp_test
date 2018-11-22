/*FTP Client*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

/*for getting file size using stat()*/
#include <sys/stat.h>

/*for sendfile()*/
#include <sys/sendfile.h>

/*for O_RDONLY*/
#include <fcntl.h>

int main(int argc,char *argv[])
{
	struct sockaddr_in server;
	struct stat obj;
	int sock = 0, choice = 0;
	char buf[256] = {0,}, command[5] = {0,}, filename[128] = {0,}, *f = NULL;
	int k, size, status;
	int filehandle;

	if(argc != 3) {
		printf("Usage: %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		printf("socket creation failed");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_port = atoi(argv[2]);

	k = connect(sock,(struct sockaddr*)&server, sizeof(server));
	if(k == -1) {
		printf("Connect Error");
		exit(1);
	}

	int i = 1;
	while(1) {
		memset(buf, 0, sizeof(buf));
		memset(command, 0, sizeof(command));
		memset(filename, 0, sizeof(filename));

		printf("\nEnter a choice:\n1- get\n2- put\n3- pwd\n4- ls\n5- cd\n6- quit\n");
		scanf("%d", &choice);
		switch(choice) {
			case 1:
				printf("Enter filename to get: ");
				scanf("%s", filename);
				strncpy(buf, "get ", 4);
				strncat(buf, filename, strlen(filename));
				send(sock, buf, sizeof(buf), 0);
				recv(sock, &size, sizeof(int), 0);
				if(!size) {
					printf("No such file on the remote directory\n\n");
					break;
				}

				f = malloc(size);
				recv(sock, f, size, 0);
				while(1) {
					filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
					if(filehandle == -1) {
						sprintf(filename + strlen(filename), "%d", i);
					}
					else break;
				}

				write(filehandle, f, size);
				close(filehandle);
				if(f) {
					memset(f, 0, strlen(f));
					free(f);
				}

				memset(buf, 0, sizeof(buf));
				printf("\nget file contents...\n\n");
				strncpy(buf, "cat ", 4);
				strncat(buf, filename, strlen(filename));
				system(buf);
				break;
			case 2:
				printf("Enter filename to put to server: ");
				scanf("%s", filename);
				filehandle = open(filename, O_RDONLY);
				if(filehandle == -1) {
					printf("No such file on the local directory\n\n");
					break;
				}

				strncpy(buf, "put ", 4);
				strncat(buf, filename, strlen(filename));
				send(sock, buf, sizeof(buf), 0);
				stat(filename, &obj);
				size = obj.st_size;
				send(sock, &size, sizeof(int), 0);
				sendfile(sock, filehandle, NULL, size);
				recv(sock, &status, sizeof(int), 0);
				if(status)
					printf("File stored successfully\n");
				else
					printf("File failed to be stored to remote machine\n");
				break;
			case 3:
				strncpy(buf, "pwd", 3);
				send(sock, buf, sizeof(buf), 0);
				recv(sock, buf, sizeof(buf), 0);
				printf("The path of the remote directory is: %s\n", buf);
				break;
			case 4:
				strncpy(buf, "ls", 2);
				send(sock, buf, sizeof(buf), 0);
				recv(sock, &size, sizeof(int), 0);
				f = malloc(size);
				recv(sock, f, size, 0);
				filehandle = creat("temp.txt", O_WRONLY);
				write(filehandle, f, size);
				close(filehandle);
				if(f) {
					memset(f, 0, strlen(f));
					free(f);
				}

				printf("The remote directory listing is as follows:\n");
				system("cat temp.txt; rm temp.txt");
				break;
			case 5:
				strncpy(buf, "cd ", 3);
				printf("Enter the path to change the remote directory: ");
				scanf("%s", buf + 3);
				send(sock, buf, sizeof(buf), 0);
				recv(sock, &status, sizeof(int), 0);
				if(status)
					printf("Remote directory successfully changed\n");
				else
					printf("Remote directory failed to change\n");
				break;
			case 6:
				strncpy(buf, "quit", 4);
				send(sock, buf, sizeof(buf), 0);
				recv(sock, &status, sizeof(buf), 0);
				if(status) {
					printf("Server closed\nQuitting..\n");
					exit(0);
				}
				printf("Server failed to close connection\n");
		}
	}
}
