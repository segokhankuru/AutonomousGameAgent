#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <opencv2/opencv.hpp>

#include "Security.h"
#include "Hardware.h"
#include "Display.h"
#include "Blackboard.h"
#include "Perception.h"

// Helper to convert DXGI raw buffer to cv::Mat
cv::Mat DXGIToMat(const Display::ScreenCapture& capture) {
    if (capture.GetWidth() == 0 || capture.GetHeight() == 0) return cv::Mat();
    // DXGI output is BGRA 4 channels
    return cv::Mat(capture.GetHeight(), capture.GetWidth(), CV_8UC4, (void*)capture.GetPixelData()).clone();
}

std::atomic<bool> g_running(true);

void PerceptionWorker(std::shared_ptr<Core::Blackboard> blackboard, Display::ScreenCapture* capture) {
    Perception::VisionModule vision(blackboard);

    // In a real scenario, you'd provide paths to template images for initial calibration
    // e.g., "hp_bar_template.png". For this example, we'll assume calibration is manual or skipped
    // until images are provided.

    while (g_running) {
        if (capture->CaptureFrame()) {
            cv::Mat frame = DXGIToMat(*capture);
            if (!frame.empty()) {
                // Once calibrated, continuously update HP and MP %
                vision.UpdateHPPercentage(frame);
                vision.UpdateMPPercentage(frame);

                // std::cout << "HP: " << blackboard->GetHPPercentage()
                //           << "% | MP: " << blackboard->GetMPPercentage() << "%" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

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

        // 3. Blackboard & Display Initialization
        std::cout << "\n[3] Initializing Central Blackboard and Display Capture (DXGI)..." << std::endl;
        auto blackboard = std::make_shared<Core::Blackboard>();
        Display::ScreenCapture screenCapture(0);

        // 4. Perception Engine (Threaded)
        std::cout << "\n[4] Starting Perception Background Worker (AI Vision)..." << std::endl;
        // Mock calibration to allow testing color thresholding immediately.
        // In reality, this would be `vision.CalibrateHPBar(..., "hp_template.png")`
        blackboard->SetHPBarROI({0, 0, 100, 20});
        blackboard->SetMPBarROI({0, 25, 100, 20});

        std::thread perceptionThread(PerceptionWorker, blackboard, &screenCapture);

        std::cout << "\n--- Autonomous Game Agent Running ---" << std::endl;
        std::cout << "Background Perception Thread is capturing and analyzing frames." << std::endl;
        std::cout << "Press Enter to stop execution..." << std::endl;

        std::cin.get();
        std::cout << "Stopping Agent..." << std::endl;

        g_running = false;
        if (perceptionThread.joinable()) {
            perceptionThread.join();
        }

        std::cout << "--- Agent Stopped Safely ---" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nFATAL ERROR: " << e.what() << std::endl;

        // Wait for user input even on error so they can read it
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    return 0;
}
