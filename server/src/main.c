#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080

int main( void )
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    char buffer[1024];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    printf("Server socket created.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; //
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("Bind success.\n");

    listen(server_socket, 5) ;
    printf("Server listening on port %d...\n", PORT);

    while (1) 
    {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        printf("Client connected.\n");

        memset(buffer, 0, sizeof(buffer));
        ssize_t received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (received <= 0) {
            printf("Client disconnected or error.\n");
            close(client_socket);
            continue;
        }

        buffer[received] = '\0';
        printf("Received from client: %s\n", buffer);

        const char *answer = "pong";
        send(client_socket, answer, strlen(answer), 0);
        printf("Sent to client: %s\n", answer);

        close(client_socket);
    }
    printf("Closing connection...\n");
    close(server_socket);
    return 0;
}
