/*
 * The maximum message size 
 * is 2048 bytes.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

int help_menu() {
    printf("*\n");
    printf("Available commands :\n");
    printf("/help : display this menu\n");
    printf("/dir : list the current directory used by the server\n");
    printf("/ip : get the server's public IP address\n");
    printf("/reboot : reboot the server\n");
    printf("/quit : kill the current connection\n");
    printf("To send a message, input just the string you want to send\n");
    printf("*\n");
    return 0;
}

int quit(int *should_stop) {
    *should_stop = 1;
    return 0;
}

int receive_msg(int fd) {
    char buffer[2048] = { 0 };
    int ret;
	
    ret = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (ret == -1) {
        perror("Error while receiving");
    } else {
        printf("%s\n", buffer);
    }

    return 0;
}

int read_input(int fd, char *buffer, int *pos, int buffer_size, int *should_stop) {
    int ret;
    char key;

    ret = read(STDIN_FILENO, &key, sizeof(key));
    if (ret != sizeof(key)) {
        perror("Error while reading key");
        return -1;
    }

    if (key == '\n') {
        if (strcmp(buffer, "/help") == 0) {
            ret = help_menu();
        } else if (strcmp(buffer, "/quit") == 0) {
            ret = quit(should_stop);
        } else {	
            ret = send(fd, buffer, strlen(buffer), 0);
            if (ret == -1) {
                perror("Error while sending message");
            }
        }
        
        memset(buffer, 0, buffer_size);
        *pos = 0;

        return ret;
    }

    if (*pos < buffer_size - 1) {
        buffer[*pos] = key;
        *pos += 1;
    }

    return 0;    
}

int main() {
    struct pollfd fds[2];
    int ret;
    int pos = 0;
    char buffer[2048] = { 0 };
    int should_stop = 0;
    char *username;

    username = (char *)malloc(512);

    printf("Enter username to use [max char. 2048]: ");
    scanf("%s", username);

    if (sizeof(username) < 512) {
        printf("Username too big, exiting.\n");
        exit(EXIT_FAILURE);
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
	address.sin_family = AF_INET;		  // struct to define type,
	address.sin_addr.s_addr = INADDR_ANY; // address and port to use
	address.sin_port = htons(1234);

    if (connect(fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("An error occured, cannot connect\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected\n");

    send(fd, username, strlen(username), 0);

    while (!should_stop) {
        memset(fds, 0, sizeof(fds));
        
        fds[0].fd = fd;
        fds[0].events = POLLIN;

        fds[1].fd = STDIN_FILENO;
        fds[1].events = POLLIN;

        ret = poll(fds, 2, -1);
        if (ret == -1) {
            perror("Error while polling");
            break;
        }

        if (fds[0].revents & POLLIN) {
            // read msg on socket
            receive_msg(fd);
        }

        if (fds[1].revents & POLLIN) {
            // read keyboard input
            read_input(fd, buffer, &pos, sizeof(buffer), &should_stop);
        }
    }

    printf("Connection closed\n");

    free(username);
    
    shutdown(fd, 2);

    return 0;
}
