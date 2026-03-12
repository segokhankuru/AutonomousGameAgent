#include "Hardware.h"

namespace Hardware {

SerialPort::SerialPort(const std::string& portName)
    : m_running(false) {

    std::string targetPort = portName;
    if (targetPort.empty()) {
        std::cout << "Attempting to auto-detect Arduino COM port..." << std::endl;
        targetPort = AutoDetectArduinoPort();
    }

    if (!targetPort.empty()) {
        m_connected = Connect(targetPort);
        if (m_connected) {
            std::cout << "Successfully connected to Arduino on " << targetPort << std::endl;
            m_running = true;
            m_worker = std::thread(&SerialPort::WorkerThread, this);
        } else {
            std::cerr << "Failed to connect to Arduino on " << targetPort << std::endl;
        }
    } else {
        std::cerr << "Could not detect an Arduino COM port." << std::endl;
    }
}

SerialPort::~SerialPort() {
    m_running = false;
    m_cv.notify_one();
    if (m_worker.joinable()) {
        m_worker.join();
    }

    if (m_hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
    }
}

bool SerialPort::IsConnected() const {
    return m_connected;
}

void SerialPort::SendCommandAsync(const SerialCommand& cmd) {
    if (!m_connected) return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(cmd);
    }
    m_cv.notify_one();
}

void SerialPort::WorkerThread() {
    while (m_running) {
        SerialCommand cmd;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });

            if (!m_running && m_queue.empty()) {
                break;
            }

            cmd = m_queue.front();
            m_queue.pop();
        }

        // Send over Serial Port synchronously
        DWORD bytesWritten;
        if (!WriteFile(m_hSerial, &cmd, sizeof(SerialCommand), &bytesWritten, NULL)) {
            std::cerr << "Error writing to Serial Port!" << std::endl;
        }

        // Ensure data is sent
        FlushFileBuffers(m_hSerial);
    }
}

bool SerialPort::Connect(const std::string& portName) {
    std::string fullPortName = "\\\\.\\" + portName;

    m_hSerial = CreateFileA(fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (m_hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(m_hSerial, &dcbSerialParams)) {
        CloseHandle(m_hSerial);
        return false;
    }

    dcbSerialParams.BaudRate = CBR_115200; // 115200 baud
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(m_hSerial, &dcbSerialParams)) {
        CloseHandle(m_hSerial);
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(m_hSerial, &timeouts)) {
        CloseHandle(m_hSerial);
        return false;
    }

    return true;
}

std::string SerialPort::AutoDetectArduinoPort() {
    std::string detectedPort = "";
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT | DIGCF_PROFILE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        return detectedPort;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
        char buffer[256];
        if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            std::string hardwareID(buffer);

            // Check for typical Arduino Leonardo/Micro VID & PID (2341:8036)
            // Note: Users might use clones, so this may need tweaking if another device is used.
            if (hardwareID.find("VID_2341&PID_8036") != std::string::npos ||
                hardwareID.find("VID_2341&PID_0036") != std::string::npos) { // Uno/Leonardo

                HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
                if (hKey != INVALID_HANDLE_VALUE) {
                    char portName[256];
                    DWORD size = sizeof(portName);
                    if (RegQueryValueExA(hKey, "PortName", NULL, NULL, (LPBYTE)portName, &size) == ERROR_SUCCESS) {
                        detectedPort = portName;
                    }
                    RegCloseKey(hKey);
                }
                break; // Found it
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return detectedPort;
}

} // namespace Hardware
