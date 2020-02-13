#ifndef __SERVER_H
#define __SERVER_H

/**
 * creates a server socket, `listen()`s on it and returns it
 * @returns the fd of the server socket
 */
int server_create(void);

/**
 * represents an action to be ran when the server gets a connection
 */
typedef void(*sockaction_t)(int);

/**
 * accepts clients from a server socket and open a new process for each of them
 * @param fd, a server socket fd
 * @param action
 * @remark this blocks the main thread and only returns on error
 * @remark this also masks SIGCHLD
 */
void server_accept(int fd, sockaction_t action);

#endif
