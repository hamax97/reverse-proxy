/* All the needed libraries, macros, helper functions are included here */
#include "../include/headers.h"

/* Function to obtain the available addresses and store them in 'result' */
void obtain_available_addresses(struct addrinfo *&result);

/* Create a socket and bind it to an available address in result */
void socket_and_bind(int &server_socket_fd, struct addrinfo *result,
		     int &web_server_fd1, int &web_server_fd2);

/* Function that initializes the signal handling */
void signal_handling();

/* Signal handler function */
void signal_handler(int signal);

/* Funtion to receive a message in client_socket_fd */
void receive_request(int client_socket_fd, char* message, size_t& bytes_read,
		     char* client_ip_address);

/* Function that sends a request to a web server and waits for the response */
void
process_request(int web_server_fd, char* request, size_t request_size,
		char* response, size_t response_size, bool& close_connection,
		int target_server, int client_socket_fd);

/* Create connection to the web servers */
void connect_to_web_servers(int web_server_fd1, int web_server_fd2);

/* Function to reconnect to one of the web servers */
void reconnect(int web_server_fd, int port);

int
main(int argc, char* argv[]){
  
  // getaddrinfo() ----------------

  struct addrinfo *result;
  obtain_available_addresses(result);
  
  // END getaddrinfo() ------------

  // socket() and bind() -------------

  // This server socket file descriptor
  
  int server_socket_fd, web_server_fd1, web_server_fd2;
  socket_and_bind(server_socket_fd, result, web_server_fd1, web_server_fd2);
  
  // END socket() and bind() -------------

  // listen() ------------------
  
  // Tells the OS to listen in this socket
  // 100: backlog: how many to enqueue
  if(listen(server_socket_fd, BACKLOG) == -1){
    perror("listen()");
    exit(1);
  }
  printf("Socket listening on port %s!\n", SERVER_PORT);

  // END listen() ------------------

  // SIGNAL Handling -----------------------

  signal_handling();
  
  // END SIGNAL Handling -------------------

  int target_server = 1; // indicates the target web server
  
  // accept() and recv() -------------
  while(1){

    // check web servers connection status
    switch(CONNECTION_STATUS){
    case 1: // connection to web server 1 is down
      reconnect(web_server_fd1, WEB_SERVER_PORT1);
      break;
    case 2: // connection to web server 2 is down
      reconnect(web_server_fd2, WEB_SERVER_PORT2);
      break;
    default:
      ; // everything is ok
    }

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

      while(true){ // This infinite while loop is to keep alive the connection
	// recv(): Receiving message from client ------

	char request[MAX_MESSAGE_SIZE];
	size_t request_size; // the request size will be stored here
	receive_request(client_socket_fd, request, request_size, address);

	// END recv() --------------------------------

	// Process request -----------------------

	char response[MAX_MESSAGE_SIZE];
	size_t response_size;
	bool close_connection = false;
      
	if(target_server == 1){
	  process_request(web_server_fd1, request, request_size,
			  response, response_size, close_connection, target_server, client_socket_fd);
	}else{
	  process_request(web_server_fd2, request, request_size,
			  response, response_size, close_connection, target_server, client_socket_fd);
	}
      
	// END Process request -----------------------

	// Send response to client -------------------

	//if( CONNECTION_STATUS != 0 ){ //something went wrong

	//char internal_server_error[] =
	//  {"Internal server error 500"};
	  
	//}else{ // Everything is ok
	  
	  //}
	
	// END Send response to client -------------------------

	if(close_connection){
	  close(client_socket_fd);
	  printf("CHILD: Connection from %s closed!\n", address);
	  exit(0);
	}
      }
    }
    // parent

    // Switch the target server
    if(target_server == 1)
      target_server = 2;
    else
      target_server = 1;
    
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
process_request(int web_server_fd, char* request, size_t request_size,
		char* response, size_t response_size, bool& close_connection,
		int target_server, int client_socket_fd){


  // THIS WAY OF READING THE REQUEST GENERATES PROBLEMS, because of the \r
  // Process request
  std::istringstream req(request);
  std::string line;
  std::string final_request;

  std::getline(req, line);
  printf("First line %s\n", line.c_str());
  final_request.append(line); //Request type

  std::getline(req, line);
  
  if(target_server == 1)
    final_request.append("Host: 127.0.0.1:6001\r\n");
  else
    final_request.append("Host: 127.0.0.1:6002\r\n");

  
  while(std::getline(req, line)){
    final_request.append(line);
  }

  final_request.append("\r\n");

  // Connect first

  struct sockaddr_in webserver_addr;
  
  webserver_addr.sin_family = AF_INET;
  if(target_server == 1){
    webserver_addr.sin_port = htons(WEB_SERVER_PORT1);
  }else{
    webserver_addr.sin_port = htons(WEB_SERVER_PORT2);
  }
  char address[] = {"127.0.0.1"};
  inet_pton(AF_INET, address, &webserver_addr.sin_addr.s_addr);
  
  if( connect(web_server_fd, (struct sockaddr *) &webserver_addr, sizeof(webserver_addr)) != 0){
    perror("connect()");
  }
  
  // Send the request

    char final_request1[] = {"GET / HTTP/1.1\r\nHost:127.0.0.1:6001\r\nUser-Agent: Mozilla/5.0\r\n\r\n"};
  
    char final_request2[] = {"GET / HTTP/1.1\r\nHost:127.0.0.1:6002\r\nUser-Agent: Mozilla/5.0\r\n\r\n"};
  
  //if( send(web_server_fd, final_request.c_str(), request_size, 0) == -1 ){
  if(target_server == 1){
    if( send(web_server_fd, final_request1, sizeof(final_request2), 0) == -1 ){
      perror("send()");
      
      if( errno == ECONNRESET )
	CONNECTION_STATUS = target_server;
      
      close_connection = true;
      return;
    }
  }else{
    if( send(web_server_fd, final_request2, sizeof(final_request2), 0) == -1 ){
      perror("send()");
      
      if( errno == ECONNRESET )
	CONNECTION_STATUS = target_server;

      close(web_server_fd);
      
      close_connection = true;
      return;
    }    
  }
  printf("CHILD:\n  - Sending request to server %d.\n  - Request:\n%s\n",
	 target_server, final_request2);
  
  // Wait for the response
  response_size = recv(web_server_fd, response, MAX_MESSAGE_SIZE-1, MSG_WAITALL);

  printf("CHILD:\nresponse: \n%s\n", response);

  if(response_size == 0){ // one of the web servers closed the connection

    CONNECTION_STATUS = target_server;
    
  }else if(response_size == -1){

    perror("send()");
    
    if( errno == ECONNRESET )
      CONNECTION_STATUS = target_server;
    
    close_connection = true;
    return;
    
  }

  send(client_socket_fd, response, response_size, 0);

  // Process request and response and look for the connection header
  close_connection = true;
}

void
obtain_available_addresses(struct addrinfo *&result){
  // Holds the status that getaddrinfo() returns
  int gai_status;
  // hints used by getaddrinfo
  struct addrinfo hints;

  // Listening socket
  
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

  // Listening socket
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
receive_request(int client_socket_fd, char* message, size_t& bytes_read,
		char* client_ip_address){

  // MAX_MESSAGE_SIZE - 1: avoids writing to the null byte character
  // MSG_WAITALL: Waits untill all the data is received
  if( (bytes_read = recv(client_socket_fd, message, MAX_MESSAGE_SIZE - 1, 0)) == -1 ){
	  
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

void
reconnect(int web_server_fd, int port){
  struct sockaddr_in webserver_addr;

  webserver_addr.sin_family = AF_INET;
  webserver_addr.sin_port = htons(port);
  char address[] = {"127.0.0.1"};
  inet_pton(AF_INET, address, &webserver_addr.sin_addr.s_addr);

  if( connect(web_server_fd, (struct sockaddr *) &webserver_addr, sizeof(webserver_addr)) != 0){
    perror("connect()");
  }
}

/* ---------- END MAIN FUNCTIONS ---------- */

