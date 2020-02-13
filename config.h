#ifndef __CONFIG_H
#define __CONFIG_H

#ifndef LISTEN_PORT
#define LISTEN_PORT "8080"
#endif

#ifndef LISTEN_BACKLOG
#define LISTEN_BACKLOG 128
#endif

#ifndef SERVER_ROOT
#define SERVER_ROOT "public"
#endif

#ifndef SERVER_CGI_ROOT
#define SERVER_CGI_ROOT "cgi"
#endif

#ifndef SERVER_ERR
#define SERVER_ERR "errdocs/%d.html"
#endif
#ifndef SERVER_ERRMSG
#define SERVER_ERRMSG \
	"<!DOCTYPE html>\n" \
	"<html>\n" \
	"\t<head>\n" \
	"\t\t<meta charset=\"UTF-8\" />\n" \
	"\t\t<title>Error</title>\n" \
	"\t</head>\n" \
	"\t<body>\n" \
	"\t\t<h1>%d</h1>\n" \
	"\t\t<p>%s</p>\n" \
	"\t</body>\n" \
	"</html>\n"
#endif

#ifndef SERVER_INDEX
#define SERVER_INDEX "index.html"
#endif

#endif
