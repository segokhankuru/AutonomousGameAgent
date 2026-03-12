#include <iostream>
#include <thread>
#include <chrono>

#include "Security.h"
#include "Hardware.h"
#include "Display.h"

int main() {
    try {
        std::cout << "--- Autonomous Game Agent Starting ---" << std::endl;

        // 1. Security Check: Ensure Mouse Acceleration is disabled
        std::cout << "\n[1] Performing Security Checks..." << std::endl;
        Security::MouseCheck::EnsureMouseAccelerationDisabled();
        std::cout << "Security Check Passed: Mouse Acceleration is disabled." << std::endl;

        // 2. Hardware Sync: Initialize Arduino Serial Port Communication
        std::cout << "\n[2] Initializing Hardware Sync (Arduino COM Port)..." << std::endl;
        Hardware::SerialPort serialPort; // Attempts to auto-detect

        if (serialPort.IsConnected()) {
            std::cout << "Sending test command to Arduino..." << std::endl;
            // Example command: MoveMouse (0), X=10, Y=20
            Hardware::SerialCommand testCmd = {0, 10, 20};
            serialPort.SendCommandAsync(testCmd);
        } else {
            std::cerr << "WARNING: Operating without hardware sync. Arduino not connected." << std::endl;
        }

        // 3. Display Capture: Initialize DXGI for monitor 0
        std::cout << "\n[3] Initializing Display Capture (DXGI)..." << std::endl;
        Display::ScreenCapture screenCapture(0);

        std::cout << "\nStarting main loop. Press Ctrl+C to exit." << std::endl;

        // Main Loop (Test capturing 10 frames)
        int framesCaptured = 0;
        while (framesCaptured < 10) {
            if (screenCapture.CaptureFrame()) {
                framesCaptured++;
                std::cout << "Captured frame " << framesCaptured << " ("
                          << screenCapture.GetWidth() << "x" << screenCapture.GetHeight() << ")" << std::endl;

                // Do AI processing / Template matching here in future sprints
            }

            // Sleep slightly to prevent maxing out CPU in this simple test loop
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS wait
        }

        std::cout << "\n--- Test completed successfully ---" << std::endl;

        // Wait for user input before closing
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();

    } catch (const std::exception& e) {
        std::cerr << "\nFATAL ERROR: " << e.what() << std::endl;

        // Wait for user input even on error so they can read it
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    return 0;
}
