/* All the needed libraries, macros, helper functions are included here */
#include "../include/headers.h"

/* Function to obtain the available addresses and store them in 'result' */
void obtain_available_addresses(struct addrinfo *&result);

/* Create a socket and bind it to an available address in result */
void socket_and_bind(int &server_socket_fd, struct addrinfo *result);

/* Function that initializes the signal handling */
void signal_handling();

/* Signal handler function */
void signal_handler(int signal);

/* Funtion to receive a message in client_socket_fd */
void receive_request(int client_socket_fd, char* message, char* client_ip_address);



int
main(int argc, char* argv[]){
  
  // getaddrinfo() ----------------

  struct addrinfo *result;
  obtain_available_addresses(result);
  
  // END getaddrinfo() ------------

  // socket() and bind() -------------

  // This server socket file descriptor
  
  int server_socket_fd, web_server_fd1, web_server_fd2;
  socket_and_bind(server_socket_fd, result, web_server_fd1,
		  web_server_fd2);
  
  // END socket() and bind() -------------

  // listen() ------------------
  
  // Tells the OS to listen in this socket
  // 100: backlog: how many to enqueue
  if(listen(server_socket_fd, BACKLOG) == -1){
    perror("listen()");
    exit(1);
  }
  printf("Socket listening!\n");

  // END listen() ------------------

  // SIGNAL Handling -----------------------

  signal_handling();
  
  // END SIGNAL Handling -------------------

  // connect() -----------------------------
  // open connection to web servers

  connect_to_web_servers(web_server_fd1, web_server_fd2);
  
  // END connect() -------------------------
  
  // accept() and recv() -------------
  while(1){

    printf("Waiting for connections...\n");
    
    // Receiving connection from client

    // sockaddr_storage can hold both IPv4 or IPv6 addresses
    struct sockaddr_storage client_addr;
    int client_socket_fd;
    int addr_size = sizeof(client_addr);

    // accept() -----------------
    
    //Accepts the connection and puts the address of the client in client_addr
    client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client_addr, (socklen_t *) &addr_size);
    
    if( client_socket_fd == -1 ){
      perror("accept()");
      continue;
    }

    char address[INET6_ADDRSTRLEN];
    inet_ntop( client_addr.ss_family,
	       get_internet_address( (struct sockaddr*) &client_addr), address, sizeof(address) );
    printf("accept(): Got connection from: %s \n", address);

    // END accept() -------------------

    // fork(): child process  -----------------

    int child_pid;

    if( (child_pid = fork()) < 0 )
      
      perror("fork()");
    
    else if( child_pid == 0){ // child

      printf("I AM THE CHILD\n");
    
      // recv(): Receiving message from client ------

      char request[MAX_MESSAGE_SIZE];
      receive_request(client_socket_fd, request, address);

      // END recv() --------------------------------

      // Process request -----------------------


      
      
      printf("Request:\n%s\n", request);
      
      // END Process request -----------------------
      
      close(client_socket_fd);
      printf("CHILD: Connection from %s closed!\n", address);
      exit(0);
      
    }
    // parent
    
    // END fork() --------------------------------

  }
  // END accept() and recv() -------------
  
  close(server_socket_fd);
  close(web_server_fd1);
  close(web_server_fd2);

  return 0;
}

/* ---------- MAIN FUNCTIONS ---------- */

void
connect_to_web_servers(){
  struct sockaddr_in web_server_address1, web_server_address2;
  // Internet address
  web_server_address1.sin_family = AF_INET;
  web_server_address2.sin_family = AF_INET;
  // Port
  web_server_address1.sin_port = htons(CLIENT_PORT1);
  web_server_address2.sin_port = htons(CLIENT_PORT2);
  // IP address
  web_server_address1.sin_addr.s_addr =
    
  
}

void
process_request(char* request){
  // Connect to the IP and port
  //   1. using the struct sockaddr_in
  //   2. Using connect

  int 

  struct sockaddr_in 
  
  // Send the request
  // Wait for the response
}

void
obtain_available_addresses(struct addrinfo *&result){
  // Holds the status that getaddrinfo() returns
  int gai_status;
  // hints used by getaddrinfo
  struct addrinfo hints;

  memset( &hints, 0, sizeof(hints) ); // make sure that hints is empty
  /* AI_PASSIVE: address for a socket that will be bind()ed, that will listen
     on all interfaces (INADDR_ANY, IN6ADDR_ANY_INIT), NOTE: this only if parameter
     node in getaddrinfo = NULL */
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
  hints.ai_socktype = SOCK_STREAM; // tcp

  // result (linked list) where the addresses will be saved
  if( (gai_status = getaddrinfo(NULL, SERVER_PORT, &hints, &result)) != 0 ){
    fprintf(stderr, "getaddrinfo() error: %s", gai_strerror(gai_status));
    exit(1);
  }
  
}

void
socket_and_bind(int &server_socket_fd, struct addrinfo *result,
		int &web_server_fd1, int &web_server_fd2){

  // Try each address until bind() returns success
  struct addrinfo *it;
  for(it = result; it != NULL; it = it -> ai_next){

    // 0: any protocol
    server_socket_fd = socket(it -> ai_family, it -> ai_socktype, 0);

    // If there was an error opening the socket, try with another addr
    if( server_socket_fd == -1 )
      continue;

    // If the port is being used by another socket, you can share the same port
    // setsockopt(): set socket option
    // SOL_SOCKET: level in the protocol stack where the option is gonna be readed
    // SO_REUSEADDR: option
    int yes = 1;
    if( setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ){
      perror("setsockopt()");
      exit(1);
    }

    if( bind(server_socket_fd, it -> ai_addr, it -> ai_addrlen) == 0 )
      break; // success bind()ing

    // socket() worked, but bind() didn't, so close it
    close(server_socket_fd); 
    
  }

  // release dinamically alocated linked list
  freeaddrinfo( result );

  // bind() didn't work in any address 
  if(it == NULL){
    fprintf(stderr, "Could not bind()\n");
    exit(1);
  }

  // Open sockets that will connect to the web servers
  web_server_fd1 = socket(AF_INET, SOCK_STREAM, 0);
  web_server_fd2 = socket(AF_INET, SOCK_STREAM, 0);
}

void signal_handling(){
  
  // Contains information about which signals handle and how
  struct sigaction signal_action;
  signal_action.sa_handler = signal_handler; // how to handle the signal
  // signals to block, that is,there may be some code in the handler that
  // shouldn't be interrupted by a signal.
  // Set it to empty
  sigemptyset( &signal_action.sa_mask );
  // System calls interrupted by this signal will be restarted
  signal_action.sa_flags = SA_RESTART;

  if( sigaction(SIGCHLD, &signal_action, NULL) == -1 ){
    perror("sigaction()");
    exit(1);
  }
  
}

void
signal_handler(int signal){

  // waitpid() might change errno value that might be in use by
  // another function
  int errno_temp = errno;

  // waitpid(): waits for a child process to terminate and reap it
  // if you are here is because a child has already termianted, so
  // it will return immediately
  while( waitpid(-1, NULL, WNOHANG) > 0 );

  errno = errno_temp;

}

void
receive_request(int client_socket_fd, char* message, char* client_ip_address){
  int bytes_read;

  // MAX_MESSAGE_SIZE - 1: avoids writing to the null byte character
  // MSG_WAITALL: Waits untill all the data is received
  if( (bytes_read = recv(client_socket_fd, message, MAX_MESSAGE_SIZE - 1, MSG_WAITALL)) == -1 ){
	  
    perror("recv()");
    close(client_socket_fd);
    printf("CHILD: Connection from %s closed!\n", client_ip_address);
    exit(0);
	  
  }else if( bytes_read == 0 ){
    // no message and peer closed the connection
    close(client_socket_fd);
    printf("CHILD: Connection from %s closed!\n", client_ip_address);
    exit(0);
  }

  message[bytes_read] = '\0'; // null byte to mark the end of the message
	
  // Message completely received
  printf("CHILD: Message from %s completely received\n", client_ip_address);
  
}

/* ---------- END MAIN FUNCTIONS ---------- */

