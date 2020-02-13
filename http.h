#ifndef _HTTP_H
#define _HTTP_H

#include <stddef.h>

/**
 * represents the recognized HTTP verbs
 */
typedef enum {
	GET, HEAD, POST, PUT, OPTIONS, DELETE, CONNECT, TRACE, PATCH
} method_t;

/**
 * represents a HTTP header
 */
typedef struct header_t header_t;
typedef struct header_t {
	char* name;
	char* value;
	header_t* next;
} header_t;

/**
 * represents a dictionnary of HTTP headers
 * this dictionnary uses case-insensitive keys, and is backed by a linked list
 */
typedef struct {
	header_t* first;
} headers_t;

/**
 * gets the value of a header
 * @param headers
 * @param name, not NULL
 * @returns the value of the header if it is found, NULL otherwise
 * @remark the returned value is not a copy and altering it is undefined behavior
 * @remark if the value associated with the given key is removed or changed, this reference becomes invalid; it is your responsability to ensure that this doesn't cause problems
 */
char* http_getHeader(headers_t* headers, char* name);

/**
 * sets the value of a header
 * @param headers
 * @param name, not NULL
 * @param value, not NULL
 * @returns 0 on success, -1 on error
 * @remark this sets the value to a copy of the value argument, not to the argument itself. This copy is managed internally
 */
int http_setHeader(headers_t* headers, char* name, char* value);

/**
 * removes a header from the dict
 * @param headers
 * @param name, not NULL
 * @returns 0 on success, -1 on error
 * @remark the only way this can fail is if the header dict is NULL
 */
int http_removeHeader(headers_t* headers, char* name);

/**
 * clears the header dict
 * @param headers
 * @returns 0 on success, -1 on error
 * @remark the only way this can fail is if the header dict is NULL
 */
int http_clearHeaders(headers_t* headers);

/**
 * represents a HTTP request as seen by the server
 */
typedef struct {
	method_t method;
	int fd;
	char* url;
	char* realurl;
	headers_t* headers;
} req_t;

/**
 * reads and parses a HTTP request from the given file descriptor
 * @param fd, a file descriptor open for reading
 * @returns a request object if the request could be parsed, NULL otherwise
 */
req_t* http_parseRequest(int fd);

/**
 * represents a HTTP response as seen by the server
 */
typedef struct {
	int status;
	int fd;
	int state;
	headers_t* headers;
} res_t;

/**
 * creates a HTTP response for the given file descriptor
 * @param fd, a file descriptor open for writing
 * @returns a response object
 */
res_t* http_createResponse(int fd);

/**
 * represents a HTTP route handler
 */
typedef int(*routehandler_t)(req_t*, res_t*, void*);

/**
 * adds a route handler to the HTTP server, giving it a user value
 * @param route, the base URI of the route
 * @param handler, not NULL
 * @param udata
 * @returns 0 on success, -1 on failure
 */
int http_addroute(char* route, routehandler_t handler, void* udata);

/**
 * encodes a URL component
 * @param str, not NULL
 * @returns the URL encoded equivalent of the input string
 * @remark it is your responsability to `free` the returned string
 */
char* http_urlencode(char* str);

/**
 * decodes a URL component
 * @param str, a URL encoded string
 * @returns the decoded equivalent of the input string
 * @remark if the string isn't a valid URL encoded string, this function is guaranteed not to crash, but may return incoherent results or NULL
 * @remark it is your responsability to `free` the returned string
 */
char* http_urldecode(char* str);

/**
 * handles a client socket
 * @param fd, a socket fd
 */
void http_server(int fd);

/**
 * serves a directory statically
 * @param req
 * @param res
 * @param fdp, fd to a directory, but cast as a `void*`
 * @returns 0 on success, -1 on failure
 */
int http_static(req_t* req, res_t* res, void* fdp);

/**
 * pumps the given fd into the response, closes the fd and ends the response
 * @param res, not yet ended
 * @param fd, open for reading
 */
void http_res_pipe(res_t* res, int fd);

/**
 * ends the response
 * @param res, not yet ended
 */
void http_res_endv(res_t* res);

/**
 * writes the given message to the response
 * @param res, not yet ended
 * @param data, zero-terminated
 */
void http_res_send(res_t* res, char* data);

/**
 * writes the given message to the response
 * @param res, not yet ended
 * @param data
 * @param len
 */
void http_res_sendl(res_t* res, char* data, size_t len);


/**
 * writes the given message to the response and ends it
 * @param res, not yet ended
 * @param data, zero-terminated
 */
void http_res_end(res_t* res, char* data);

/**
 * writes the given message to the response and ends it
 * @param res, not yet ended
 * @param data
 * @param len
 */
void http_res_endl(res_t* res, char* data, size_t len);

/**
 * sends the headers of the response
 * @param res, headers not yet sent
 */
void http_res_sendHeaders(res_t* res);

/**
 * sends the default error page for the given error
 * @param res
 * @param status, a valid HTTP status code
 */
void http_res_error(res_t* res, int status);

#endif
