#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>п

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int connectToServer(int *sock, struct sockaddr_in *server)
{
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0)
    {
        fprintf(stderr, "socket error (code %d)\n", errno);
        return -1;
    }
    server->sin_family = AF_INET;
    server->sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server->sin_addr)) <= 0)
    {
        fprintf(stderr, "inet_pton error (code %d)\n", errno);
        close(*sock);
        return -1;
    }
    if (connect(*sock, (struct sockaddr *)server, sizeof(*server)) < 0)
    {
        fprintf(stderr, "connect error (code %d)\n", errno);
        close(*sock);
        return -1;
    }
    printf("Connected to server %s:%d\n", SERVER_IP, PORT);
    return 0;
}

int receive_client_id(int sock)
{
    char buffer[64];
    // ожидаем от сервера строку вида ID 5\n */
    ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0)
    {
        fprintf(stderr, "Couldnt receive ID from server.\n");
        return -1;
    }
    buffer[received] = '\0';

    int id = -1;
    sscanf(buffer, "ID %d", &id);

    printf("Client ID assigned: %d\n", id);
    return id;
}

int send_nickname_to_server(int sock, const char *nickname)
{
    char buffer[128];
    int n = snprintf(buffer, sizeof(buffer), "NICK %s\n", nickname);
    ssize_t sent = send(sock, buffer, n, 0);
    return 0;
}

// Основной цикл чата
void chat_loop(int sock, const char *nickname)
{
    char sendbuf[512];
    char recvbuf[512];

    while (1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // ввод с клавиатуры
        FD_SET(sock, &readfds);         // данные от сервера

        int maxfd = (sock > STDIN_FILENO ? sock : STDIN_FILENO) + 1;

        int ready = select(maxfd, &readfds, NULL, NULL, NULL);
        if (ready < 0)
        {
            fprintf(stderr, "select error (code %d)\n", errno);
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            printf("\033[1;36m[%s]MSG\033[0m: ", nickname);
            fflush(stdout);

            if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL)
            {
                printf("Input error. Exiting chat...\n");
                break;
            }
            sendbuf[strcspn(sendbuf, "\r\n")] = '\0';

            // Команда выхода
            if (strcmp(sendbuf, "/quit") == 0)
            {
                printf("Shutting down this client...\n");
                break;
            }
            char msg[600];
            int n = snprintf(msg, sizeof(msg), "%s: %s\n", nickname, sendbuf);

            ssize_t sent = send(sock, msg, n, 0); // отправка
            if (sent != n)
            {
                fprintf(stderr, "Error while sending message (maybe lost connection).\n");
                break;
            }
        }

        if (FD_ISSET(sock, &readfds))
        {
            ssize_t received = recv(sock, recvbuf, sizeof(recvbuf) - 1, 0); // получение от сервера
            if (received < 0)
            {
                fprintf(stderr, "Error while receiving from server (code %d)\n", errno);
                break;
            }

            recvbuf[received] = '\0';

            printf("%s", recvbuf);
            fflush(stdout); // моменталбный вывод
        }
    }
}

int main(void)
{
    int my_socket = -1;
    struct sockaddr_in server_addr;
    int my_id = -1;

    size_t retry_count = 0;
    bool connected = false;
    char my_nickname[64];
    while (!connected) // цикл входа в чат
    {
        if (connectToServer(&my_socket, &server_addr) == 0)
        {
            my_id = receive_client_id(my_socket);
            if (my_id < 0)
            {
                close(my_socket);
                my_socket = -1;
            }
            else
            {
                // Вводим никнейм
                printf("Enter your nickname: ");
                my_nickname[strcspn(my_nickname, "\r\n")] = '\0';
                connected = true;
                break;
            }
        }

        retry_count++;
        printf("Retrying connection to server... (attempt %zu)\n", retry_count);

        if (retry_count % 10 == 0)
        {
            char choice_line[16];
            char choice = 0;

            printf("Are you sure you wanna wait? (y/n): ");
            fflush(stdout);

            if (fgets(choice_line, sizeof(choice_line), stdin) != NULL)
            { // прекращение попыток
                if (sscanf(choice_line, " %c", &choice) == 1)
                {
                    if (choice == 'n' || choice == 'N')
                    {
                        printf("Shutting down this client...\n");
                        sleep(1);
                        if (my_socket >= 0)
                            close(my_socket);
                        return 0;
                    }
                }
            }
        }

        sleep(1);
    }
    printf("You are connected as client ID #%d, name: [%s]\n", my_id, my_nickname);

    // Основной цикл чата
    chat_loop(my_socket, my_nickname);

    if (my_socket >= 0)
        close(my_socket);
    return 0;
}