#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "server.h"
#include "http.h"
#include "config.h"
#include "cgi.h"

int tagadatsointsoin(req_t* req, res_t* res, void* data) {
	(void)(req);
	(void)(data);

	char buf[]="TAGADA TSOIN TSOIN";
	char* ptr=buf;
	int orig=strlen(ptr);
	int len=orig;
	int off=0;

	http_setHeader(res->headers, "Content-Type", "text/plain");

	while(len>=orig/2) {
		http_res_sendl(res, ptr-off, len);
		http_res_send(res, "\n");
		*(ptr++)=' ';
		len--;
		off++;
	}

	http_res_endv(res);
	return 0;
}

int main(void) {
	int fd=server_create();
	if(fd<0) {
		fprintf(stderr, "Failed to create server\n");
		return 1;
	}

	int dir=open(SERVER_ROOT, O_RDONLY);
	if(dir<0) {
		perror("open()");
		return 1;
	}

	int cgidir=open(SERVER_CGI_ROOT, O_RDONLY);
	if(cgidir<0) {
		perror("open()");
		return 1;
	}

	http_addroute("/", http_static, (void*) (intptr_t) dir);
	http_addroute("/cgi", cgi_php, (void*) (intptr_t) cgidir);
	http_addroute("/tagadatsointsoin", tagadatsointsoin, NULL);
	server_accept(fd, http_server);
}
