#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Optional depending on your use case
#include <Xinput.h>

// The ViGEm API
#include <ViGEm/Client.h>

// Link against SetupAPI
#pragma comment(lib, "setupapi.lib")

int main()
{
    std::cout << "Hello World!\n";
}
