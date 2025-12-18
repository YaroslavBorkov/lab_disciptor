#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 64
#define BUF_SIZE 512

typedef struct {
    int sock;
    int id;
    char nickname[64];
    bool active;
} Client;

Client clients[MAX_CLIENTS];
int next_id = 1;

void broadcast(const char *msg, int except_sock)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sock != except_sock) {
            send(clients[i].sock, msg, strlen(msg), 0);
        }
    }
}

void remove_client(int index)
{
    printf("Client #%d disconnected\n", clients[index].id);

    close(clients[index].sock);
    clients[index].active = false;
    memset(clients[index].nickname, 0, sizeof(clients[index].nickname));
}

void handle_message(int i)
{
    char buffer[BUF_SIZE];

    ssize_t received = recv(clients[i].sock, buffer, sizeof(buffer) - 1, 0);

    if (received <= 0) {
        remove_client(i);
        return;
    }

    buffer[received] = '\0';

    // Получаем ник 
    if (strncmp(buffer, "NICK ", 5) == 0) { 

        strncpy(clients[i].nickname,
                buffer + 5,
                sizeof(clients[i].nickname) - 1);

        clients[i].nickname[strcspn(clients[i].nickname, "\r\n")] = '\0';

        printf("Client #%d set nickname: %s\n", clients[i].id, clients[i].nickname);

        char join_msg[128];
        snprintf(join_msg, sizeof(join_msg),
                 "[SYSTEM] %s joined the chat\n",
                 clients[i].nickname);

        broadcast(join_msg, -1);
        return;
    }

    //обычное сообщение 
    broadcast(buffer, -1);

    printf("%s", buffer);
}



int main()
{
    int server_socket;

    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Socket creation error\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // любой интерфейс
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind error\n");
        close(server_socket);
        return 1;
    }
    if (listen(server_socket, 10) < 0) {
        printf("Listen error\n");
        close(server_socket);
        return 1;
    }
    printf("Chat server running on port %d...\n", PORT);

    //главный цикл 
    while (1) {
        fd_set readfds; //хранит набор файловых дескрипторов
        // отслеживать дескрипторы готовые для чтения
        FD_ZERO(&readfds); //макрос очищает набор дескрипторов
        FD_SET(server_socket, &readfds); //добавление дескриптора в набор

        int max_fd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                FD_SET(clients[i].sock, &readfds);
                if (clients[i].sock > max_fd)
                    max_fd = clients[i].sock;
            }
        }
        max_fd++;

        int ready = select(max_fd, &readfds, NULL, NULL, NULL);
        if (ready < 0) {
            printf("Select error\n");
            break;
        }

        //новое подключение 
        if (FD_ISSET(server_socket, &readfds)) {
            int client_sock = accept(server_socket, NULL, NULL);
            if (client_sock < 0) printf("Accept error\n");
            else {
                bool added = false;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i].active) {

                        clients[i].active = true;
                        clients[i].sock   = client_sock;
                        clients[i].id     = next_id++;

                        char id_msg[64];
                        snprintf(id_msg, sizeof(id_msg),
                                 "ID %d\n", clients[i].id);

                        send(client_sock, id_msg, strlen(id_msg), 0);

                        printf("New client connected, ID #%d\n",
                               clients[i].id);



                        added = true;
                        break;
                    }
                }
            }
        }

        //сообщения от клиентов 
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && FD_ISSET(clients[i].sock, &readfds)) handle_message(i);
        }
    }


    close(server_socket);
    return 0;
}
