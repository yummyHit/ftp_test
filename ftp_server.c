/*FTP server*/
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

int main(int argc, char *argv[]) {
	struct sockaddr_in server, client;
	struct stat obj;
	char buf[256] = {0,}, command[5] = {0,}, filename[128] = {0,};
	int sock1 = 0, sock2 = 0, k = 0, i = 0, size = 0, len = 0, c = 0, filehandle = 0;

	if(argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	sock1 = socket(AF_INET, SOCK_STREAM, 0);
	if(sock1 == -1) {
		fprintf(stderr, "Socket creation failed\n");
		exit(1);
	}

	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = atoi(argv[1]);

	k = bind(sock1, (struct sockaddr*)&server, sizeof(server));
	if(k == -1) {
		fprintf(stderr, "Binding error\n");
		exit(1);
	}

	k = listen(sock1, 1);
	if(k == -1) {
		fprintf(stderr, "Listen failed\n");
		exit(1);
	}

	len = sizeof(client);
	sock2 = accept(sock1, (struct sockaddr*)&client, &len);

	printf("Client Accept... \nClient IP: %s\n", inet_ntoa(client.sin_addr));
	i = 1;
	while(1) {
		memset(buf, 0, sizeof(buf));
		memset(command, 0, sizeof(command));
		memset(filename, 0, sizeof(filename));

		recv(sock2, buf, sizeof(buf), 0);
		strncpy(command, buf, sizeof(command));
		if(!strncmp(command, "ls", 2)) {
			system("ls | grep -v \"temp.txt\" >temp.txt");
			i = 0;
			stat("temp.txt", &obj);
			size = obj.st_size;
			send(sock2, &size, sizeof(int), 0);
			filehandle = open("temp.txt", O_RDONLY);
			sendfile(sock2, filehandle, NULL, size);
		} else if(!strncmp(command, "get", 3)) {
			strncpy(filename, buf + 4, sizeof(filename));
			printf("get a file... \n");
			stat(filename, &obj);
			filehandle = open(filename, O_RDONLY);
			size = obj.st_size;
			if(filehandle == -1)
				size = 0;
			send(sock2, &size, sizeof(int), 0);
			if(size)
				sendfile(sock2, filehandle, NULL, size);
			printf("success give a %s file!\n", filename);
		} else if(!strncmp(command, "put", 3)) {
			int c = 0;
			char *f = NULL;
			strncpy(filename, buf + 4, sizeof(filename));
			printf("put a file... \n");
			recv(sock2, &size, sizeof(int), 0);
			i = 1;
			while(1) {
				filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
				if(filehandle == -1)
					sprintf(filename + strlen(filename), "%d", i);
				else
					break;
			}

			f = malloc(size);
			recv(sock2, f, size, 0);
			c = write(filehandle, f, size);
			close(filehandle);
			send(sock2, &c, sizeof(int), 0);
			if(f) {
				memset(f, 0, strlen(f));
				free(f);
			}
			printf("success save a %s file!\n", filename);
		} else if(!strncmp(command, "pwd", 3)) {
			system("pwd >temp.txt");
			i = 0;
			FILE *f = fopen("temp.txt","r");
			while(!feof(f))
				buf[i++] = fgetc(f);
			buf[i-1] = '\0';
			fclose(f);
			send(sock2, buf, sizeof(buf), 0);
			remove("temp.txt");
		} else if(!strncmp(command, "cd", 2)) {
			if(chdir(buf+3) == 0)
				c = 1;
			else
				c = 0;
			send(sock2, &c, sizeof(int), 0);
		}

		else if(!strcmp(command, "bye") || !strcmp(command, "quit")) {
			printf("FTP server quitting..\n");
			i = 1;
			send(sock2, &i, sizeof(int), 0);
			remove("temp.txt");
			exit(0);
		}
	}
	return 0;
}
