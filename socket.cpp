#include <WinSock2.h>
#include <iostream>

SOCKET init_udp_socket(const unsigned short bind_port) {
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_socket == INVALID_SOCKET) return -1;

    SOCKADDR_IN bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(bind_port);
    bind_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket, (SOCKADDR*)&bind_addr, sizeof(bind_addr)) < 0) {
        closesocket(udp_socket);
        std::cerr << "Failed to bind socket." << std::endl;
        return -1;
    }

    std::cout << "UDP server listening on port " << bind_port << ".\n";
    return udp_socket;
}

void destroy_udp_socket(SOCKET* udp_socket) {
    if (*udp_socket >= 0) {
        closesocket(*udp_socket);
        *udp_socket = -1;
    }
}