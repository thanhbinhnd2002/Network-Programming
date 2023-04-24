#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
int main()
{
struct sockaddr_in addr;
addr.sin_family = AF_INET;
    return 0;
}