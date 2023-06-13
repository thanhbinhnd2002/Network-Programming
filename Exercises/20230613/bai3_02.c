#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 2

// Struct chứa thông tin của mỗi client
typedef struct {
    int socket;
    struct sockaddr_in address;
} ClientInfo;

// Mảng chứa thông tin của các client
ClientInfo clients[MAX_CLIENTS];

// Biến đếm số lượng client hiện tại
volatile int client_count = 0;

// Khóa để đồng bộ hóa truy cập vào biến client_count
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hàm gửi tin nhắn từ client này đến client kia
void send_message(int from, int to, const char* message) {
    write(to, message, strlen(message));
}

// Hàm xử lý kết nối với mỗi client
void* handle_client(void* arg) {
    int client_index = *(int*)arg;
    int client_socket = clients[client_index].socket;

    while (1) {
        char message[1024];
        int read_size = read(client_socket, message, sizeof(message));

        if (read_size <= 0) {
            // Xử lý khi client ngắt kết nối
            pthread_mutex_lock(&client_mutex);
            client_count--;
            printf("Client disconnected: %s:%d\n",
                   inet_ntoa(clients[client_index].address.sin_addr),
                   ntohs(clients[client_index].address.sin_port));
            pthread_mutex_unlock(&client_mutex);
            break;
        }

        // Gửi tin nhắn từ client này đến client kia
        int to = (client_index + 1) % MAX_CLIENTS;
        send_message(client_socket, clients[to].socket, message);
    }

    close(client_socket);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    int server_socket;
    struct sockaddr_in server_address;
    pthread_t client_threads[MAX_CLIENTS];

    // Tạo socket server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Thiết lập thông tin địa chỉ server
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8000);

    // Gắn socket với địa chỉ server
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe các kết nối từ client
    if (listen(server_socket, 5) < 0) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8000\n");

    while (1) {
        // Chấp nhận kết nối từ client
        int client_socket = accept(server_socket, NULL, NULL);

        if (client_socket < 0) {
            perror("Error accepting client connection");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&client_mutex);

        if (client_count < MAX_CLIENTS) {
            // Lưu thông tin của client vào mảng clients
            clients[client_count].socket = client_socket;
            printf("Client connected: %s:%d\n",
                   inet_ntoa(clients[client_count].address.sin_addr),
                   ntohs(clients[client_count].address.sin_port));

            // Tạo luồng xử lý cho client
            int* client_index = malloc(sizeof(int));
            *client_index = client_count;
            pthread_create(&client_threads[client_count], NULL, handle_client, client_index);

            client_count++;
        } else {
            // Đạt số lượng client tối đa, từ chối kết nối
            const char* message = "Server is full. Please try again later.\n";
            write(client_socket, message, strlen(message));
            close(client_socket);
        }

        pthread_mutex_unlock(&client_mutex);
    }

    close(server_socket);
    pthread_mutex_destroy(&client_mutex);

    return 0;
}
