#include "TCPRequestChannel.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;


TCPRequestChannel::TCPRequestChannel (const std::string _ip_address, const std::string _port_no) {
    // recommended to use sockaddr_in struct which is a structure in the sock api
    // you can specify the port you want to bind to and when you call bind it will handle the connection to that address om that port for you
    // have to handle byte ordering have to convert between host byte order and network byte order there's a function to help do this <- inet_pton

    // if server (if ip address is empty)
    //     create a socket on the specified port 
    //         specify domain, type, and protocol
    //     bind the socket to an address you get from the machine (Can use getaddressinfo or some structs) sets up listening 
    //     mark socket as listening done with a call to listen which is one of the socket api functions
    if (_ip_address == "")
    {
        struct sockaddr_in server;
        socklen_t addr_size;
        int server_sock, bind_stat, listen_stat;
        //addr_size = sizeof(server);

        this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (this->sockfd < 0){
			perror ("Cannot create socket");	
			exit(-1);
		}

        // provide necessary machine info for sockaddr_in
        // address family, IPv4
        // IPv4 address, use current IPv4 address (INADDR_ANY)
        // connection port
        // convert short from host byte order to network byte order
        server.sin_family = AF_INET;
        server.sin_port = htons(stoi(_port_no)); // this line is the conversion
        server.sin_addr.s_addr = INADDR_ANY;
        
        
        int opt_val = 1;
        setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
        
        // bind
        bind_stat = bind(this->sockfd, (struct sockaddr *) &server, sizeof(server));
        if (bind_stat < 0)
        {
            close(this->sockfd);
            cout << _ip_address << " " << _port_no << endl;
			perror("server: bind");
			exit(-1);
        }
        
        
        //listen
        listen_stat = listen(this->sockfd, 10);

        if (listen_stat < 0)
        {
            perror("listen");
	        exit(1);
        }
        
        


    }
    
    // if client
    //     create a socket on the specified port 
    //         specify domain, type, and protocol
    //     connect to the socket to the IP address of the server
    else
    {
        struct sockaddr_in server_info;
        int client_sock, connect_stat;
       

        this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (this->sockfd < 0){
			perror ("Cannot create socket");	
			exit(-1);
		}

        server_info.sin_family = AF_INET;
        server_info.sin_port = htons(stoi(_port_no)); // this line is the conversion
        inet_pton(AF_INET, _ip_address.c_str(), &(server_info.sin_addr)); 
        //server_info.sin_addr.s_addr = inet_addr(_ip_address.c_str());// setting the ip address
       

        // connect

        connect_stat = connect(this->sockfd, (const struct sockaddr *) &server_info, sizeof(server_info));
        if (connect_stat < 0)
        {
            perror ("Cannot Connect");
			exit(-1);
        }
        
    }
}

TCPRequestChannel::TCPRequestChannel (int _sockfd) {
    // all you do is set a variable
    this->sockfd = _sockfd;
}

TCPRequestChannel::~TCPRequestChannel () {
    // close the sockfd -> close(this->sockfd)
    close(this->sockfd);
}

int TCPRequestChannel::accept_conn () {
    // declare sockaddr_storage <- used to establish the client connection 
    struct sockaddr_storage client_address;
    socklen_t addr_size = sizeof(client_address);
    int newfd;
    // implement the accept function in the socket api  - returns the sockfd of the client
    // - accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
    newfd = accept(this->sockfd, (struct sockaddr*) &client_address, &addr_size);
    if (newfd < 0) {
            cout << strerror(errno) << endl;
            
    }
    return newfd;
}

// read/write, or recv/send
int TCPRequestChannel::cread (void* msgbuf, int msgsize) {
    ssize_t no_bytes; // number of bytes to read
    // read from the socket - read(int fd, void* buf, size_t count)
    no_bytes = read(this->sockfd, msgbuf, msgsize);
    // return number of bytes read
    return no_bytes;

}

int TCPRequestChannel::cwrite (void* msgbuf, int msgsize) {
    ssize_t no_bytes; // number of bytes to read
    // write to the socket - write(int fd, const void* buf, size_t count)
    no_bytes = write(this->sockfd, msgbuf, msgsize);
    // return number of bytes written
    return no_bytes;
}

int TCPRequestChannel::getfd() {
    return this->sockfd;
}
