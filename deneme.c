#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9000 // The port the server will listen on

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    // Clear address structure
    memset(&serv_addr, 0, sizeof(serv_addr));

    // Setup the host_addr structure for use in bind call
    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Automatically be filled with current host's IP address
    serv_addr.sin_port = htons(PORT);

    // Bind the socket to the current IP address on port, PORT
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // Listen for incoming connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while(1) { // Loop indefinitely to accept and process multiple connections
        printf("Waiting for a connection...\n");

        // Accept an incoming connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }

        printf("Connection accepted.\n");

        // Clear the buffer with zeros
        memset(buffer, 0, 1024);

        // Read the message from the client
        n = read(newsockfd, buffer, 1023);
        if (n < 0) {
            error("ERROR reading from socket");
        }
        printf("Here is the message: %s\n", buffer);

        // Respond to client
        n = write(newsockfd, "I got your message", 18);
        if (n < 0) {
            error("ERROR writing to socket");
        }

        // Close the connection socket
        close(newsockfd);
    }

    // Never reached due to the infinite loop
    // close(sockfd);

    return 0;
}
