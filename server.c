#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "config.h"
#include "server.h"

static void reapChild(int sig) {
	(void)(sig);
	int status=0;
	waitpid(-1, &status, WNOHANG);
}

int server_create(void) {
	int sockfd;
	struct addrinfo hints, *res;

	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET6; //IPv4 ou IPv6
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; //Permet d'associer la socket a toutes les adresses locales

	int err;
	if((err=getaddrinfo(NULL, LISTEN_PORT, &hints, &res))!=0) {
		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(err));
		freeaddrinfo(res);
		return -1;
	}
	if((sockfd=socket(res->ai_family, res->ai_socktype, res->ai_protocol))<0) {
		freeaddrinfo(res);
		perror("socket()");
		return -1;
	}
	if(bind(sockfd, res->ai_addr, res->ai_addrlen)<0) {
		freeaddrinfo(res);
		close(sockfd);
		perror("bind()");
		return -1;
	}
	if(listen(sockfd, LISTEN_BACKLOG)<0) {
		freeaddrinfo(res);
		close(sockfd);
		perror("listen()");
		return -1;
	}
	return sockfd;
}

void server_accept(int fd, sockaction_t action) {
	struct sigaction reapAction={.sa_handler=reapChild};
	sigaction(SIGCHLD, &reapAction, NULL);

	for(;;) {
		int sock=accept(fd, NULL, 0);
		if(sock<0 && errno==EINTR) continue;
		if(sock<0) {
			perror("accept()");
			exit(1);
		}
		pid_t pid=fork();
		while(pid<0 && errno==EAGAIN) pid=fork();
		if(pid<0) {
			close(sock);
			return;
		} else if(pid==0) {
			action(sock);
			close(sock);
			exit(0);
		} else {
			close(sock);
		}
	}
}
