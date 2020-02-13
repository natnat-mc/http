#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cgi.h"
#include "http.h"

extern char** environ;


static int readline(int fd, char* buf, int len) {
	while(len) {
		int a=read(fd, buf, 1);
		if(a<0) return 0;
		else if(a==0) break;
		if(*buf!='\r') buf++;
		if(*buf=='\n') break;
		len--;
	}
	*buf=0;
	return 1;
}

int cgi_php(req_t* req, res_t* res, void* data) {
	int dfd=(int) (intptr_t) data;

	// parse the url and querystring
	char urlbuf[1024];
	strncpy(urlbuf, req->url, sizeof(urlbuf)-1);
	char* url=urlbuf;
	char* qs=strrchr(req->url, '?');
	if(qs) {
		url[qs-req->url]=0;
		qs++;
	}
	if(*url=='/') url++;
	if(strstr(url, "..")) {
		http_res_error(res, 400);
		return 0;
	}

	// open the cgi file
	int fd=openat(dfd, url, O_RDONLY);
	if(fd<0) {
		switch(errno) {
			case ENOENT:
				res->status=404;
				return 1;
			case EACCES:
				http_res_error(res, 403);
				return 0;
			default:
				perror("openat()");
				http_res_error(res, 500);
				return 0;
		}
	}
	char buf[1024];
	char path[1024];
	sprintf(buf, "/proc/%d/fd/%d", getpid(), fd);
	ssize_t t;
	if((t=readlink(buf, path, sizeof(path)-1))<0) {
		perror("readlink()");
		http_res_error(res, 500);
		return 0;
	}
	path[t]=0;
	close(fd);

	// make a pipe
	int pipefd[2];
	if(pipe(pipefd)) {
		perror("pipe()");
		http_res_error(res, 500);
		return 0;
	}

	// call php-cgi
	pid_t pid=fork();
	if(pid<0 && errno==EAGAIN) pid=fork();
	if(pid<0) {
		perror("fork()");
		http_res_error(res, 500);
		return 0;
	} else if(pid==0) {
		close(pipefd[0]);
		dup2(pipefd[1], 1);
		char* argv[]={
			"env",
			"php-cgi",
			path,
			"--",
			qs,
			NULL
		};
		execve("/usr/bin/env", argv, environ);
		exit(0);
	} else {
		close(pipefd[1]);
		wait(NULL);

		char line[10240];
		while(readline(pipefd[0], line, sizeof(line)-1)) {
			if(!*line) break;
			char* delim=strstr(line, ": ");
			*delim=0;
			char* value=delim+2;
			http_setHeader(res->headers, line, value);
		}
		http_res_pipe(res, pipefd[0]);
		return 0;
	}
}
