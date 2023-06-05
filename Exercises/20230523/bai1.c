#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void remove_extra_spaces(char *str)
{
    int i, j;
    int space = 0;

    for (i = 0, j = 0; str[i] != '\0'; i++)
    {
        if (str[i] == ' ')
        {
            if (space == 0)
            {
                str[j++] = str[i];
                space = 1;
            }
        }
        else
        {
            str[j++] = str[i];
            space = 0;
        }
    }

    str[j] = '\0';
}

void capitalize_first_char(char *str)
{
    if (str[0] >= 'a' && str[0] <= 'z')
    {
        str[0] = str[0] - 32;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Sử dụng: %s <cổng>\n", argv[0]);
        return 1;
    }

    int server_socket, client_sockets[MAX_CLIENTS], max_clients = MAX_CLIENTS;
    struct sockaddr_in server_addr, client_addr;
    fd_set readfds;
    int max_fd, activity, i, valread, sd;
    int opt = 1;
    int addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Tạo socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Không thể tạo socket");
        return 1;
    }

    // Thiết lập địa chỉ và cổng cho socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    // Gắn socket với địa chỉ và cổng
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Lỗi khi gắn socket");
        return 1;
    }

    // Lắng nghe kết nối từ client
    if (listen(server_socket, 5) == -1)
    {
        perror("Lỗi khi lắng nghe kết nối");
        return 1;
    }

    // Khởi tạo mảng client_sockets
    for (i = 0; i < max_clients; i++)
    {
        client_sockets[i] = 0;
    }

    printf("Server đã sẵn sàng. Đang chờ kết nối từ client...\n");

    while (1)
    {
        // Xóa tập hợp readfds và thêm server socket vào
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_fd = server_socket;

        // Thêm các client socket vào tập hợp readfds
        for (i = 0; i < max_clients; i++)
        {
            sd = client_sockets[i];

            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }

            // Tìm giá trị lớn nhất cho max_fd
            if (sd > max_fd)
            {
                max_fd = sd;
            }
        }

        // Sử dụng hàm select để chờ sự kiện trên các socket
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity == -1)
        {
            perror("Lỗi khi sử dụng hàm select");
            return 1;
        }

        // Kiểm tra sự kiện trên server socket
        if (FD_ISSET(server_socket, &readfds))
        {
            int new_socket;

            // Chấp nhận kết nối mới từ client
            if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen)) == -1)
            {
                perror("Lỗi khi chấp nhận kết nối");
                return 1;
            }

            // Thêm kết nối mới vào mảng client_sockets
            for (i = 0; i < max_clients; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    printf("Client đã kết nối thành công: socket fd %d, IP: %s, Port: %d\n", new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    break;
                }
            }
        }

        // Kiểm tra sự kiện trên các client socket
        for (i = 0; i < max_clients; i++)
        {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds))
            {
                memset(buffer, 0, BUFFER_SIZE);

                // Đọc dữ liệu từ client
                valread = recv(sd, buffer, BUFFER_SIZE, 0);
                if (valread == -1)
                {
                    perror("Lỗi khi nhận dữ liệu từ client");
                    return 1;
                }

                // Kiểm tra xem client đã gửi lệnh "exit" hay chưa
                if (strcmp(buffer, "exit") == 0)
                {
                    // Gửi thông báo tạm biệt cho client
                    char goodbye[] = "Tạm biệt!";
                    if (send(sd, goodbye, strlen(goodbye), 0) == -1)
                    {
                        perror("Lỗi khi gửi dữ liệu cho client");
                        return 1;
                    }

                    // Đóng kết nối và đánh dấu socket là 0
                    getpeername(sd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
                    printf("Client đã ngắt kết nối: IP: %s, Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    close(sd);
                    client_sockets[i] = 0;
                }
                else
                {
                    // Xử lý chuỗi từ client
                    remove_extra_spaces(buffer);
                    capitalize_first_char(buffer);

                    // Gửi kết quả về cho client
                    if (send(sd, buffer, strlen(buffer), 0) == -1)
                    {
                        perror("Lỗi khi gửi dữ liệu cho client");
                        return 1;
                    }
                }
            }
        }
    }

    // Đóng các socket của client
    for (i = 0; i < max_clients; i++)
    {
        sd = client_sockets[i];

        if (sd > 0)
        {
            close(sd);
        }
    }

    // Đóng server socket
    close(server_socket);

    return 0;
}

