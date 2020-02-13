#ifndef _CGI_H
#define _CGI_H

#include "http.h"

/**
 * handles a request and response using a PHP CGI
 * @param req
 * @param res
 * @param data, a fd to the CGI directory
 */
int cgi_php(req_t* req, res_t* res, void* data);

#endif
