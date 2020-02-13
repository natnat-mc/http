#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include "http.h"
#include "config.h"

extern char** environ;

/**
 * represents a route
 */
typedef struct route_t route_t;
typedef struct route_t {
	char* route;
	routehandler_t handler;
	void* udata;
	route_t* next;
} route_t;

/**
 * the first route of the linked list
 */
static route_t* firstRoute;

/**
 * checks if both `a` and `b` are equal, ignoring case
 * @param a
 * @param b
 * @returns 1 if the strings are equal without regard to case, 0 otherwise
 */
static int loEq(char* a, char* b) {
	char ca=*a;
	char cb=*b;
	while(ca || cb) {
		if(ca>='a' && ca<='z') ca-='a'-'A';
		if(cb>='a' && cb<='z') cb-='a'-'A';
		if(ca!=cb) return 0;
		a++; b++;
		ca=*a; cb=*b;
	}
	return 1;
}

/**
 * checks if `str` starts with `src`, ignoring case
 * @param str
 * @param src
 * @returns 1 if `str` starts with `src` without regard to case, 0 otherwise
 */
static int loSw(char* str, char* src) {
	char cr=*str;
	char cc=*src;
	while(cc) {
		if(cr>='a' && cr<='z') cr-='a'-'A';
		if(cc>='a' && cc<='z') cc-='a'-'A';
		if(cr!=cc) return 0;
		cr=*(str++);
		cc=*(src++);
	}
	return 1;
}

/**
 * returns the value of a hex character, or 0 if it is invalid
 * @param h, a hex digit
 * @returns the value of the hex character
 */
static int hexval(char h) {
	if(h>='0' && h<='9') return h-'0';
	if(h>='a' && h<='f') return 10+h-'a';
	if(h>='A' && h<='F') return 10+h-'A';
	return 0;
}

/**
 * returns the hex digit corresponding to a number
 * @param v, an int between 0 and 15 inclusive
 * @returns a hex represntation of the number
 */
static char hexdigit(int v) {
	if(v<10) return '0'+v;
	return 'a'+v-10;
}

/**
 * returns the string name of a HTTP method
 * @param m
 * @returns a string representation of the method
 */
static char* methStr(method_t m) {
	switch(m) {
		case GET:
			return "GET";
		case HEAD:
			return "HEAD";
		case POST:
			return "POST";
		case PUT:
			return "PUT";
		case OPTIONS:
			return "OPTIONS";
		case DELETE:
			return "DELETE";
		case CONNECT:
			return "CONNECT";
		case TRACE:
			return "TRACE";
		case PATCH:
			return "PATCH";
	}
	return "[invalid]";
}

/**
 * returns the name of the numeric status
 * @param status
 * @returns the name of the numeric status*
 */
static char* statusName(int status) {
	switch(status) {
		case 200:
			return "OK";
		case 400:
			return "BAD REQUEST";
		case 403:
			return "FORBIDDEN";
		case 404:
			return "NOT FOUND";
		case 500:
			return "INTERNAL SERVER ERROR";
	}
	return "ERROR";
}

char* http_getHeader(headers_t* headers, char* name) {
	if(!headers) return NULL;

	header_t* header=headers->first;
	while(header) {
		if(loEq(header->name, name)) return header->value;
	}
	return NULL;
}

int http_setHeader(headers_t* headers, char* name, char* value) {
	if(!headers) return -1;

	char* dup=strdup(value);
	if(!dup) return -1;

	header_t* header=headers->first;
	while(header) {
		if(loEq(header->name, name)) {
			free(header->value);
			header->value=dup;
			return 0;
		}
		header=header->next;
	}

	header=headers->first;
	while(header && header->next) header=header->next;

	header_t* newHeader=malloc(sizeof(header_t));
	if(!newHeader) {
		free(dup);
		return -1;
	}
	char* dupName=strdup(name);
	if(!dupName) {
		free(dup);
		free(newHeader);
		return -1;
	}
	newHeader->name=dupName;
	newHeader->value=dup;
	newHeader->next=NULL;
	if(header) header->next=newHeader;
	else headers->first=newHeader;
	return 0;
}

int http_removeHeader(headers_t* headers, char* name) {
	if(!headers) return -1;

	header_t* header=headers->first;
	while(header) {
		if(loEq(header->name, name)) {
			if(headers->first==header) {
				headers->first=header->next;
			} else {
				header_t* prev=headers->first;
				while(prev->next!=header) prev=prev->next;
			}
			free(header->name);
			free(header->value);
			free(header);
			return 0;
		}
		header=header->next;
	}
	return 0;
}

int http_clearHeaders(headers_t* headers) {
	if(!headers) return -1;

	header_t* header=headers->first;
	while(header) {
		free(header->name);
		free(header->value);
		header_t* next=header->next;
		free(header);
		header=next;
	}
	headers->first=NULL;

	return 0;
}

char* http_urlencode(char* url) {
	char* buf=malloc(strlen(url)*3+1);
	if(!buf) return NULL;
	char* ptr=buf;
	char c;
	while((c=*(url++))) {
		if((c>='a' && c<='z') || (c>='A' || c<='Z')) {
			*(ptr++)=c;
		} else {
			*(ptr++)='%';
			*(ptr++)=hexdigit((c>>4)&0xf);
			*(ptr++)=hexdigit(c&0xf);
		}
	}
	*(ptr++)=0;

	return buf;
}

char* http_urldecode(char* url) {
	char* buf=malloc(strlen(url)+1);
	if(!buf) return NULL;
	char* ptr=buf;
	char c;
	while((c=*(url++))) {
		if(c=='%') {
			if(!(*url && *(url+1))) {
				free(buf);
				return NULL;
			}
			int val=hexval(*(url++))<<4;
			val|=hexval(*(url++));
			*(ptr++)=(char) val;
		} else {
			*(ptr++)=c;
		}
	}
	*(ptr++)=0;

	return buf;
}

res_t* http_createResponse(int fd) {
	res_t* res=malloc(sizeof(res_t));
	if(!res) return NULL;
	headers_t* headers=malloc(sizeof(headers_t));
	if(!headers) {
		free(res);
		return NULL;
	}
	res->status=200;
	res->fd=fd;
	res->headers=headers;
	res->state=0;
	return res;
}

req_t* http_parseRequest(int fd) {
	req_t* req=malloc(sizeof(req_t));
	if(!req) return NULL;
	headers_t* headers=malloc(sizeof(headers_t));
	if(!headers) {
		free(req);
		return NULL;
	}

	req->fd=fd;
	req->headers=headers;

	char buf[1024];
	int a=read(fd, buf, sizeof(buf));
	if(a<0) {
		free(req);
		free(headers);
		return NULL;
	}

	if(loSw(buf, "GET")) req->method=GET;
	else if(loSw(buf, "HEAD")) req->method=HEAD;
	else if(loSw(buf, "POST")) req->method=POST;
	else if(loSw(buf, "PUT")) req->method=PUT;
	else if(loSw(buf, "OPTIONS")) req->method=OPTIONS;
	else if(loSw(buf, "DELETE")) req->method=DELETE;
	else if(loSw(buf, "CONNECT")) req->method=CONNECT;
	else if(loSw(buf, "TRACE")) req->method=TRACE;
	else if(loSw(buf, "PATCH")) req->method=PATCH;
	else {
		free(req);
		free(headers);
		return NULL;
	}

	int urlpos=0;
	while(buf[urlpos]!=' ' && urlpos<a) urlpos++;
	if(urlpos==a) {
		free(req);
		free(headers);
		return NULL;
	}
	urlpos++;
	int urlendpos=urlpos+1;
	while(buf[urlendpos]!=' ' && urlendpos<a) urlendpos++;
	if(urlendpos==a) {
		free(req);
		free(headers);
		return NULL;
	}
	buf[urlendpos]=0;
	req->url=strdup(buf+urlpos);
	req->realurl=req->url;
	if(!req->url) {
		free(req);
		free(headers);
		return NULL;
	}

	//TODO parse headers

	return req;
}

int http_addroute(char* route, routehandler_t handler, void* udata) {
	route=strdup(route);
	if(!route) return -1;

	route_t* r=malloc(sizeof(route_t));
	if(!r) {
		free(route);
		return -1;
	}

	r->handler=handler;
	r->udata=udata;
	r->route=strdup(route);
	if(!r->route) {
		free(route);
		free(r);
		return -1;
	}

	if(!firstRoute) {
		firstRoute=r;
		return 0;
	}

	if(strcmp(firstRoute->route, route)<0) {
		r->next=firstRoute;
		firstRoute=r;
		return 0;
	}

	route_t* prev=firstRoute;
	while(1) {
		if(!prev->next) {
			prev->next=r;
			break;
		} else if(strcmp(prev->next->route, route)<0) {
			r->next=prev->next;
			prev->next=r;
			break;
		} else {
			prev=prev->next;
		}
	}

	return 0;
}

#define HTTP_RES_HEADERSSENT 0x1
#define HTTP_RES_ENDED 0x2

void http_server(int fd) {

	// create the default response
	res_t* res=http_createResponse(fd);
	if(!res) {
		fprintf(stderr, "Failed to create response");
		close(fd);
		exit(1);
	}
	http_setHeader(res->headers, "Server", "Custom HTTP");
	http_setHeader(res->headers, "Connection", "close");

	// parse the request
	req_t* req=http_parseRequest(fd);
	if(!req) {
		http_res_error(res, 400);
		return;
	}

	// handle routing
	for(route_t* route=firstRoute; route; route=route->next) {
		if(strstr(req->url, route->route)==req->url) {
			if(strcmp(route->route, "/")) {
				req->url=req->realurl+strlen(route->route);
			}
			int err=route->handler(req, res, route->udata);
			if(!err) break;
		}
	}

	// make sure everything is routed
	if(!(res->state&HTTP_RES_ENDED)) {
		http_res_error(res, 404);
		fprintf(stderr, "not handled\n");
	}

	// log some stuff
	{
		char addrBuf[1024];
		socklen_t addrLen=sizeof(addrBuf);
		struct sockaddr* addr=(struct sockaddr*) addrBuf;
		char strBuf[1024]="";

		if(!getpeername(fd, addr, &addrLen)) {
			const void* rst=(addr->sa_family==AF_INET6)?
				inet_ntop(AF_INET6, &((struct sockaddr_in6*) addr)->sin6_addr, strBuf, addrLen)
			:
				inet_ntop(AF_INET, &((struct sockaddr_in*) addr)->sin_addr, strBuf, addrLen);
			if(!rst) {
				strcpy(strBuf, "<can't decode>");
			}
		} else {
			strcpy(strBuf, "<can't read>");
		}
		printf("%s [%d] %s %s\n", methStr(req->method), res->status, req->realurl, strBuf);
	}

}

int http_static(req_t* req, res_t* res, void* fdp) {
	int dfd=(int) (ptrdiff_t) fdp;

	char* path=req->url;
	{ // sanitize path
		if(*path=='/') path++;
		if(strstr(path, "..")) {
			res->status=400;
			http_res_end(res, "<code>..</code> found in URL");
			return 0;
		}
	}
	if(!*path) path=".";

	int fd=openat(dfd, path, O_RDONLY);

	if(fd>=0) { // handle directories
		struct stat statbuf;
		if(!fstat(fd, &statbuf)) {
			if(statbuf.st_mode&S_IFDIR) {
				int fd2=openat(fd, SERVER_INDEX, O_RDONLY);
				close(fd);
				fd=fd2;
			}
		}
	}

	if(fd<0) { // deliver error document appropriately
		switch(errno) {
			case EACCES:
				http_res_error(res, 403);
				return 0;
			case ENOENT:
				http_res_error(res, 404);
				return 0;
			default:
				perror("open()");
				http_res_error(res, 500);
		}
		return 0;
	}

	do { // detect mime type
		char buf[1024];
		char path[1024];
		sprintf(buf, "/proc/%d/fd/%d", getpid(), fd);
		ssize_t t;
		if((t=readlink(buf, path, sizeof(path)-1))<0) break;
		path[t]=0;
		int pipefd[2];
		if(pipe(pipefd)) break;
		pid_t pid=fork();
		while(pid<0 && errno==EAGAIN) pid=fork();
		if(pid<0) break;

		if(pid==0) { // invoke `file` in the pipe
			close(pipefd[0]);
			dup2(pipefd[1], 1);
			char* argv[]={
				"env",
				"--",
				"file",
				"-b",
				"--mime-type",
				path,
				NULL
			};
			execve("/usr/bin/env", argv, environ);
			close(pipefd[1]);
			exit(1);
		} else { // read the results of `file`
			close(pipefd[1]);
			wait(NULL);
			int a=read(pipefd[0], buf, sizeof(buf)-1);
			if(a<=0) break;
			buf[a]=0;
			char* p=buf;
			while(*p) {
				if(*p=='\n' || *p=='\r') {
					*p=0;
					break;
				}
				p++;
			}
			http_setHeader(res->headers, "Content-Type", buf);
		}
	} while(0);

	res->status=200;
	http_res_pipe(res, fd);
	return 0;
}

void http_res_endv(res_t* res) {
	if(res->state&HTTP_RES_ENDED) return;
	http_res_sendHeaders(res);
	res->state|=HTTP_RES_ENDED;
}

void http_res_pipe(res_t* res, int fd) {
	if(res->state&HTTP_RES_ENDED) return;
	http_res_sendHeaders(res);
	char buf[1024];
	int a;
	while((a=read(fd, buf, sizeof(buf)))>0) write(res->fd, buf, a);
	close(fd);
	res->state|=HTTP_RES_ENDED;
}
void http_res_end(res_t* res, char* data) {
	if(res->state&HTTP_RES_ENDED) return;
	http_res_sendHeaders(res);
	size_t len=strlen(data);
	while(len) {
		int a=write(res->fd, data, len);
		if(a<=0) break;
		data+=a;
		len-=a;
	}
	res->state|=HTTP_RES_ENDED;
}
void http_res_endl(res_t* res, char* data, size_t len) {
	if(res->state&HTTP_RES_ENDED) return;
	http_res_sendHeaders(res);
	while(len) {
		int a=write(res->fd, data, len);
		if(a<=0) break;
		data+=a;
		len-=a;
	}
	res->state|=HTTP_RES_ENDED;
}

void http_res_send(res_t* res, char* data) {
	if(res->state&HTTP_RES_ENDED) return;
	http_res_sendHeaders(res);
	size_t len=strlen(data);
	while(len) {
		int a=write(res->fd, data, len);
		if(a<=0) break;
		data+=a;
		len-=a;
	}
}
void http_res_sendl(res_t* res, char* data, size_t len) {
	if(res->state&HTTP_RES_ENDED) return;
	http_res_sendHeaders(res);
	while(len) {
		int a=write(res->fd, data, len);
		if(a<=0) break;
		data+=a;
		len-=a;
	}
}

void http_res_sendHeaders(res_t* res) {
	if(res->state&HTTP_RES_HEADERSSENT) return;
	res->state|=HTTP_RES_HEADERSSENT;

	char buf[10240];
	sprintf(buf, "HTTP/1.1 %d %s\r\n", res->status, statusName(res->status));
	write(res->fd, buf, strlen(buf));

	for(header_t* header=res->headers->first; header; header=header->next) {
		sprintf(buf, "%s: %s\r\n", header->name, header->value);
		write(res->fd, buf, strlen(buf));
	}

	sprintf(buf, "\r\n");
	write(res->fd, buf, strlen(buf));
}

void http_res_error(res_t* res, int status) {
	if(res->state&HTTP_RES_ENDED) return;
	res->status=status;
	http_res_sendHeaders(res);

	char buf[1024];
	sprintf(buf, SERVER_ERR, status);
	int fd=open(buf, O_RDONLY);
	if(fd>=0) {
		http_res_pipe(res, fd);
	} else {
		sprintf(buf, SERVER_ERRMSG, status, statusName(status));
		http_res_end(res, buf);
	}
}
