#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Sử dụng: %s <địa chỉ IP> <cổng máy nhận> <cổng máy chờ>\n", argv[0]);
        return 1;
    }

    char *receiver_ip = argv[1];
    int receiver_port = atoi(argv[2]);
    int sender_port = atoi(argv[3]);

    int receiver_socket, sender_socket;
    struct sockaddr_in receiver_addr, sender_addr;
    fd_set readfds;
    int max_fd;
    int activity;
    char buffer[BUFFER_SIZE];

    // Tạo socket máy nhận
    if ((receiver_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Không thể tạo socket máy nhận");
        return 1;
    }

    // Thiết lập địa chỉ máy nhận
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    receiver_addr.sin_port = htons(receiver_port);

    // Gắn socket với địa chỉ máy nhận
    if (bind(receiver_socket, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) == -1)
    {
        perror("Lỗi khi gắn socket máy nhận");
        return 1;
    }

    // Tạo socket máy chờ
    if ((sender_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Không thể tạo socket máy chờ");
        return 1;
    }

    // Thiết lập địa chỉ máy chờ
    memset(&sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_port = htons(sender_port);
    if (inet_aton(receiver_ip, &sender_addr.sin_addr) == 0)
    {
        fprintf(stderr, "Địa chỉ IP không hợp lệ\n");
        return 1;
    }

    printf("Ứng dụng chat đã sẵn sàng. Nhập 'quit' để thoát.\n");

    while (1)
    {
        // Xóa tập hợp readfds và thêm các socket vào
        FD_ZERO(&readfds);
        FD_SET(0, &readfds); // stdin
        FD_SET(receiver_socket, &readfds);
        FD_SET(sender_socket, &readfds);

        // Tìm giá trị lớn nhất của các socket
        max_fd = (receiver_socket > sender_socket) ? receiver_socket : sender_socket;

        // Sử dụng hàm select để chờ sự kiện trên các socket
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity == -1)
        {
            perror("Lỗi khi sử dụng hàm select");
            return 1;
        }
        // Kiểm tra sự kiện trên socket nhận
        if (FD_ISSET(receiver_socket, &readfds))
        {
            struct sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);

            memset(buffer, 0, BUFFER_SIZE);

            // Nhận tin nhắn từ máy nhận
            ssize_t recv_len = recvfrom(receiver_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_len);
            if (recv_len == -1)
            {
                perror("Lỗi khi nhận tin nhắn từ máy nhận");
                return 1;
            }

            // Chuyển đổi địa chỉ IP từ định dạng network sang định dạng chuỗi
            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sender_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);

            printf("Tin nhắn từ [%s:%d]: %s\n", sender_ip, ntohs(sender_addr.sin_port), buffer);
        }

        // Kiểm tra sự kiện trên stdin
        if (FD_ISSET(0, &readfds))
        {
            // Đọc tin nhắn từ người dùng
            fgets(buffer, BUFFER_SIZE, stdin);

            // Loại bỏ ký tự xuống dòng
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n')
            {
                buffer[len - 1] = '\0';
            }

            // Kiểm tra nếu người dùng muốn thoát
            if (strcmp(buffer, "quit") == 0)
            {
                break;
            }

            // Gửi tin nhắn đến máy nhận
            ssize_t send_len = sendto(sender_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&sender_addr, sizeof(sender_addr));
            if (send_len == -1)
            {
                perror("Lỗi khi gửi tin nhắn đến máy nhận");
                return 1;
            }
        }
    }

    // Đóng socket
    close(receiver_socket);
    close(sender_socket);

    return 0;
}
