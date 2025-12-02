#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_IP   "127.0.0.1"
#define PORT 8080

int main( void )
{
    int socket; 

    struct sockaddr_in server_addr;
    
    printf("Hello from client!\n");
    return 0;
}