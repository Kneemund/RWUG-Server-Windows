#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <ViGEm/Client.h>

// linking options
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "ws2_32.lib")

#include "socket.h"
#include "gamepad.h"

#define PORT 4242

#define RWUG_IN_SIZE 58
#define RWUG_OUT_SIZE 4

#define RWUG_PLAY 0x01
#define RWUG_STOP 0x02

#define STICK_RADIUS 32767

SOCKADDR_IN gamepad_addr;
int gamepad_addr_size;
SOCKET udp_socket;

VOID CALLBACK input_callback(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, UCHAR LedNumber, LPVOID UserData) {
    char packet[RWUG_OUT_SIZE];

    UCHAR strength = max(LargeMotor, SmallMotor);
    if (strength > 0) {
        // PLAY
        packet[0] = RWUG_PLAY;
        packet[1] = strength;
        memset(&packet[2], 10, sizeof(unsigned short));
    } else {
        // STOP
        packet[0] = RWUG_STOP;
        packet[1] = 0;
        packet[2] = 0;
        packet[3] = 0;
    }

    sendto(udp_socket, packet, RWUG_OUT_SIZE, 0, (SOCKADDR*) &gamepad_addr, gamepad_addr_size);
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    // Initialize ViGEm
    const auto client = vigem_alloc();
    if (client == nullptr) {
        std::cerr << "Failed to allocate ViGEm driver connection." << std::endl;
        return -1;
    }

    const auto connect_error = vigem_connect(client);
    if (!VIGEM_SUCCESS(connect_error)) {
        std::cerr << "ViGEm Bus connection failed with error code: 0x" << std::hex << connect_error << std::endl;
        return -1;
    }

    const auto pad = vigem_target_x360_alloc();
    const auto pir = vigem_target_add(client, pad);

    if (!VIGEM_SUCCESS(pir)) {
        std::cerr << "Target plugin failed with error code: 0x" << std::hex << pir << std::endl;
        return -1;
    }

    const auto notification_error = vigem_target_x360_register_notification(client, pad, &input_callback, nullptr);
    if (!VIGEM_SUCCESS(notification_error)) {
        std::cerr << "Registering for notification failed with error code: 0x" << std::hex << notification_error << std::endl;
        return -1;
    }

    // Initialize Socket
    udp_socket = init_udp_socket(PORT);
    if (udp_socket < 0) return -1;

    memset(&gamepad_addr, 0, sizeof(gamepad_addr));
    gamepad_addr_size = sizeof(gamepad_addr);

    while (true) {
        char incoming_packet[RWUG_IN_SIZE];
        if (recvfrom(udp_socket, incoming_packet, RWUG_IN_SIZE, 0, (SOCKADDR*) &gamepad_addr, &gamepad_addr_size) > 0) {
            uint32_t hold;
            memcpy(&hold, &incoming_packet[38], sizeof(uint32_t));

            XUSB_REPORT report = {};

            for (int i = 0; i < GAMEPAD_BUTTON_DATA_LENGTH; ++i) {
                struct gamepad_button button = GAMEPAD_BUTTON_DATA[i];
                report.wButtons |= button.evdev_button_code * ((hold & button.button_code) != 0);
            }

            float stickLX, stickLY, stickRX, stickRY;
            memcpy(&stickLX, &incoming_packet[42], sizeof(float));
            memcpy(&stickLY, &incoming_packet[46], sizeof(float));
            memcpy(&stickRX, &incoming_packet[50], sizeof(float));
            memcpy(&stickRY, &incoming_packet[54], sizeof(float));

            report.sThumbLX = (SHORT) (stickLX * STICK_RADIUS);
            report.sThumbLY = (SHORT) (stickLY * STICK_RADIUS);
            report.sThumbRX = (SHORT) (stickRX * STICK_RADIUS);
            report.sThumbRY = (SHORT) (stickRY * STICK_RADIUS);

            report.bLeftTrigger = ((hold & GAMEPAD_BUTTON_ZL) != 0) * 255;
            report.bRightTrigger = ((hold & GAMEPAD_BUTTON_ZR) != 0) * 255;

            vigem_target_x360_update(client, pad, report);
        }
    }

    // Cleanup
    destroy_udp_socket(&udp_socket);

    vigem_target_remove(client, pad);
    vigem_target_free(pad);

    WSACleanup();
}
