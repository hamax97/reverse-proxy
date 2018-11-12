#ifndef HEADERS_H
#define HEADERS_H

/* Libraries needed */

#include <fstream> // ifstream
#include <exception>
#include <stdio.h> // printf() fprintf()
#include <stdlib.h> // strtol()

/* Networking, Signaling, Forking */

#include <string.h> // memset()
#include <sys/socket.h> // socket() bind() listen() accept() inet_aton() getaddrinfo()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // inet_pton()
#include <netdb.h> // getaddrinfo() freeaddrinfo()
#include <unistd.h> // close() fork()
#include <signal.h> // sigaction()
#include <sys/wait.h> // waitpid()
#include <sys/types.h> // getaddrinfo() waitpid() freeaddrinfo() fork()
#include <errno.h> // global variable errno

/* -------- MACROS -------- */

// Ports under 1024 are reserved, and you can use them only if you are ROOT
#define PORT "17777"

// Maximum waiting queue
#define BACKLOG 10

// Check /proc/sys/net/core/rmem_default for buffer values for recv()
// Check /proc/sys/net/core/wmem_default for buffer values for send()
#define MAX_MESSAGE_SIZE 5000

/* -------- END MACROS -------- */

/* -------- FUNCTION DECALARATION ------- */

/* Returns a pointer to the sin_addr structure containing the ip */
void *get_internet_address(struct sockaddr *sa);

/* Stores in 'number' the number in the message's JSON header */
void get_number_in_header(const char* message, int *number_of_chars);

/* -------- END FUNCTION DECALARATION ------- */

/* -------- FUNCTION DEFINITION ------- */

void
*get_internet_address(struct sockaddr *sa){

  // How these parallel structures work?
  // sockaddr and sockaddr_in
  if( (((struct sockaddr_in *)sa) -> sin_family) == AF_INET ) // IPv4
    return &(((struct sockaddr_in *)sa) -> sin_addr);

  return &(((struct sockaddr_in6*)sa) -> sin6_addr); // IPv6
  
}

/* -------- END FUNCTION DEFINITION ------- */

#endif
