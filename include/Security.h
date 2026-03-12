#pragma once

#include <stdexcept>
#include <string>
#include <windows.h>
#include <iostream>

namespace Security {

class MouseCheck {
public:
    // Checks if Windows "Enhance pointer precision" (Mouse Acceleration) is enabled.
    // If enabled, throws an exception and stops execution.
    static void EnsureMouseAccelerationDisabled() {
        int mouseParams[3] = {0};

        // Retrieve mouse parameters.
        // mouseParams[0] = threshold 1
        // mouseParams[1] = threshold 2
        // mouseParams[2] = acceleration enabled (non-zero if enabled)
        if (SystemParametersInfoA(SPI_GETMOUSE, 0, &mouseParams, 0)) {
            if (mouseParams[2] != 0) {
                std::string errorMsg =
                    "CRITICAL SECURITY ERROR: Mouse Acceleration (Enhance pointer precision) is ENABLED!\n"
                    "Please disable it in Windows Settings -> Mouse Properties -> Pointer Options to ensure accurate absolute/relative coordinate targeting.\n"
                    "Execution halted.";
                std::cerr << errorMsg << std::endl;
                throw std::runtime_error(errorMsg);
            }
        } else {
            std::cerr << "WARNING: Failed to retrieve mouse parameters using SystemParametersInfo." << std::endl;
        }
    }
};

} // namespace Security
