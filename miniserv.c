
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

const int MAX_SIE = 42*4096;

typedef struct clients{
    int id;
    char msg[11000];
}t_client;

t_client    clients[1024];

int max = 0, next_id = 0;

fd_set rfds, wfds, fds;

char *buffWrite = NULL, *buffRead = NULL;

void    fatal_error() {
    write (2, "Fatal error\n", 12);
    exit (1);
}

void send_all(int s) {
    for (int i = 0; i <= max; i++) {
        if (FD_ISSET(i, &rfds) && i != s)
            send(i, buffWrite, strlen(buffWrite), 0);
    }
}

int main( int ac, char **av) {
    if (ac != 2) {
        write (2, "Wrong number of arguments\n", 26);
        exit (1);
    }


    buffRead = malloc(MAX_SIE * sizeof(char*));
    buffWrite = malloc(MAX_SIE * sizeof(char*));
    bzero(&clients, sizeof(clients));
    FD_ZERO(&fds);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0)
        fatal_error();

    max = sockfd;
    FD_SET(sockfd, &fds);

    struct sockaddr_in servaddr;
    socklen_t len = sizeof(servaddr);
    servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(av[1]));

    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) < 0)
        fatal_error();
	if (listen(sockfd, 128) < 0)
        fatal_error();
    while (1) {
        wfds = rfds = fds;
        if (select(max + 1, &wfds, &rfds, NULL, NULL) < 0)
            continue;
        for (int s = 0; s <= max; s++) {
            if (FD_ISSET(s, &wfds) && s == sockfd) {
                int acc = accept(sockfd, (struct sockaddr *)&servaddr, &len);
                if (acc < 0)
                    continue;
                max = (acc > max) ? acc : max;
                clients[acc].id = next_id++;
                FD_SET(acc, &fds);
                sprintf(buffWrite, "server: client %d just arrived\n", clients[acc].id);
                send_all(acc);
                break;
            }
            if (FD_ISSET(s, &wfds) && s != sockfd) {
                int rec = recv(s, buffRead, 42*4096, 0);
                if (rec <= 0) {
                    sprintf(buffWrite, "server: client %d just left\n", clients[s].id);
                    send_all(s);
                    FD_CLR(s, &fds);
                    close(s);
                    break ;
                }
                else {
                    for (int i = 0, j = strlen(clients[s].msg); i < rec; i++,j++) {
                        clients[s].msg[j] = buffRead[i];
                        if (clients[s].msg[j] == '\n') {
                            clients[s].msg[j] = '\0';
                            sprintf(buffWrite, "client %d: %s\n", clients[s].id, clients[s].msg);
                            send_all(s);
                            bzero(&clients[s].msg, strlen(clients[s].msg));
                            j = -1;
                        }
                    }
                    break;
                }
            }
        }
    }
}