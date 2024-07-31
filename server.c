#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 3672
#define USERNAME "local"
#define PASSWORD "pass"
#define BUFFER_SIZE 1024

void DieWithError(const char *errmsg) {
	fprintf(stderr, "%s: %s\n", errmsg, strerror(errno));
	exit(EXIT_FAILURE);
}

void send_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    FILE *file = fopen(filename, "r");
    if (!file) {
        DieWithError("File doesn't exist");
    }

    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE-1, file)) > 0) {
    	buffer[bytes_read] = '\0';
        if (send(sockfd, buffer, bytes_read, 0) < 0) {
            fclose(file);
            DieWithError("Send_file failed");
        }
    }
    fclose(file);
}

void receive_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    FILE *file = fopen(filename, "w");  // Open in text mode
    if (!file) {
        send(sockfd, "ERROR", 5, 0);
        return;
    }
    if((send(sockfd, "OK", 2, 0)) < 0)
        DieWithError("Sending ok failed");
    int dataSize;
    while ((dataSize = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, dataSize - 1, file);  // Write without null terminator
        if (dataSize < BUFFER_SIZE) break;
    }
    fclose(file);
}

int authenticate(int sockfd) {
    char username[30], password[50], prompt[40];
    int recv_size;

    strcpy(prompt, "Enter username: ");
    if (send(sockfd, prompt, strlen(prompt), 0) < 0)
        DieWithError("Send username prompt failed");
    
    if ((recv_size = recv(sockfd, username, sizeof(username) - 1, 0)) < 0)
        DieWithError("Receiving username failed");
    username[recv_size] = '\0';

    strcpy(prompt, "Enter password: ");
    if (send(sockfd, prompt, strlen(prompt), 0) < 0)
        DieWithError("Send password prompt failed");
    
    if ((recv_size = recv(sockfd, password, sizeof(password) - 1, 0)) < 0)
        DieWithError("Receiving password failed");
    password[recv_size] = '\0';

    if (strcmp(username, USERNAME) == 0 && strcmp(password, PASSWORD) == 0) {
        strcpy(prompt, "Authentication Successful!\n");
        if (send(sockfd, prompt, strlen(prompt), 0) < 0)
            DieWithError("Send authentication success message failed");
        return 1;
    } else {    
        strcpy(prompt, "Authentication Error!\n");
        if (send(sockfd, prompt, strlen(prompt), 0) < 0)
            DieWithError("Send authentication error message failed");
        return 0;
    }
}


int main() {
	int sockfd_s, sockfd_c, recvSize=0;
	struct sockaddr_in addr_server, addr_client;
	socklen_t addr_client_len = 0; 
	char send_buffer[100] = "", recv_buffer[100] = "";

	if((sockfd_s = socket(AF_INET, SOCK_STREAM, 0))<0)
		DieWithError("Socket creation failed");

	memset(&addr_server, 0, sizeof(addr_server));
	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons(PORT);
	addr_server.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sockfd_s, (struct sockaddr*)&addr_server, sizeof(addr_server)) < 0)
		DieWithError("binding failed.");

	if(listen(sockfd_s, 2) < 0)
		DieWithError("Listen failed");  //Listening to 2 connections

	printf("Server is listening on the port %d\n", PORT);


	addr_client_len = sizeof(addr_client);

	if((sockfd_c = accept(sockfd_s, (struct sockaddr*)&addr_client, &addr_client_len)) < 0)
		DieWithError("Connection Accept failed");
	if(authenticate(sockfd_c)) {
		while(1) {
			memset(recv_buffer, 0, sizeof(recv_buffer));
			if((recvSize = recv(sockfd_c, recv_buffer, sizeof(recv_buffer)-1, 0)) <= 0) {
				if(recvSize == 0)
					printf("Client disconnected\n");
				else
					DieWithError("Receiving Failed");
				break;
			} recv_buffer[recvSize] = '\0';
			if(strcmp(recv_buffer, "UPLOAD") == 0) {
				memset(recv_buffer, 0, sizeof(recv_buffer));
				recv(sockfd_c, recv_buffer, sizeof(recv_buffer), 0);
				receive_file(sockfd_c, recv_buffer);
				printf("File uploaded: %s\n", recv_buffer);
			} else if(strcmp(recv_buffer, "DOWNLOAD") == 0) {
				memset(recv_buffer, 0, sizeof(recv_buffer));
				recv(sockfd_c, recv_buffer, sizeof(recv_buffer), 0);
				printf("Client requested to download file: %s\n", recv_buffer);
				send_file(sockfd_c, recv_buffer);
			} else if(strcmp(recv_buffer, "CLOSE") == 0) {
				break;
			} else if(strcmp(recv_buffer, "ECHO") == 0) {
				memset(recv_buffer, 0, sizeof(recv_buffer));
				if(recv(sockfd_c, recv_buffer, sizeof(recv_buffer), 0) < 0)
					DieWithError("Didn't receive ECHO");
				send(sockfd_c, recv_buffer, recvSize, 0);
				printf("Echoed: %s\n", recv_buffer);
			} else;
		}
	}
	close(sockfd_c); close(sockfd_s);
	return 0;

}

/* Simple code to make a listening TCP server */
