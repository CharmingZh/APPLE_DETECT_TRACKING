#ifndef SIMPLE_SERIAL_H
#define SIMPLE_SERIAL_H

#include <windows.h>
#include <string>
#include <iostream>

class SimpleSerial {
private:
    HANDLE hSerial;
    bool connected;

public:
    SimpleSerial(const std::string& portName, DWORD baudRate = 9600) : connected(false), hSerial(nullptr) {
        hSerial = CreateFile(portName.c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

        if (hSerial == INVALID_HANDLE_VALUE) {
            std::cerr << "Error: Could not open serial port " << portName << std::endl;
            return;
        }

        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "Error: Could not get serial port state" << std::endl;
            CloseHandle(hSerial);
            return;
        }

        dcbSerialParams.BaudRate = baudRate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            std::cerr << "Error: Could not set serial port state" << std::endl;
            CloseHandle(hSerial);
            return;
        }

        COMMTIMEOUTS timeouts = {0};
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        
        if (!SetCommTimeouts(hSerial, &timeouts)) {
             std::cerr << "Error: Could not set serial port timeouts" << std::endl;
             CloseHandle(hSerial);
             return;
        }

        connected = true;
        std::cout << "Successfully connected to " << portName << std::endl;
    }

    ~SimpleSerial() {
        if (connected) {
            CloseHandle(hSerial);
            connected = false;
        }
    }

    bool write(const std::string& data) {
        if (!connected) return false;
        DWORD bytes_written;
        return WriteFile(hSerial, data.c_str(), data.length(), &bytes_written, NULL);
    }

    bool isConnected() const {
        return connected;
    }
};

#endif //SIMPLE_SERIAL_H