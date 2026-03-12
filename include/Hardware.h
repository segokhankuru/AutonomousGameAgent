#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <setupapi.h>
#include <devguid.h>

namespace Hardware {

// --- 5-Byte Command Struct (Little-Endian, 1-byte aligned) ---
#pragma pack(push, 1)
struct SerialCommand {
    uint8_t command;   // 0=MoveMouse, 1=ClickMouse, 2=KeyPress, etc.
    uint16_t param1;   // X coordinate or Key Code
    uint16_t param2;   // Y coordinate or Delay (ms)
};
#pragma pack(pop)

class SerialPort {
public:
    // Initialize serial port. If portName is empty, it attempts to auto-detect Arduino.
    SerialPort(const std::string& portName = "");
    ~SerialPort();

    // Sends a command asynchronously (pushes to queue, returns immediately).
    void SendCommandAsync(const SerialCommand& cmd);

    // Checks if the serial connection is successfully established.
    bool IsConnected() const;

private:
    std::string AutoDetectArduinoPort();
    bool Connect(const std::string& portName);
    void WorkerThread();

private:
    HANDLE m_hSerial = INVALID_HANDLE_VALUE;
    bool m_connected = false;

    // Threading mechanisms
    std::thread m_worker;
    std::atomic<bool> m_running;
    std::queue<SerialCommand> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

} // namespace Hardware
