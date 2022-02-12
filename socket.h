#pragma once

#include <WinSock2.h>

SOCKET init_udp_socket(const unsigned short bind_port);
void destroy_udp_socket(SOCKET* udp_socket);