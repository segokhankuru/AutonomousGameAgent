#pragma once

#include <shared_mutex>
#include <mutex>
#include <vector>
#include <optional>
#include <opencv2/opencv.hpp>

namespace Core {

// Represents a bounded Region of Interest (e.g. for HP Bar, MP Bar, or specific windows)
struct ROI {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool IsValid() const {
        return width > 0 && height > 0;
    }
};

// Central data hub for the agent.
// Uses std::shared_mutex to allow multiple readers (Logic/AI) and single writers (Perception).
class Blackboard {
public:
    Blackboard() = default;
    ~Blackboard() = default;

    // --- State setters ---
    void SetHPPercentage(float hp);
    void SetMPPercentage(float mp);
    void SetIsLootWindowOpen(bool isOpen);
    void SetHPBarROI(const ROI& roi);
    void SetMPBarROI(const ROI& roi);

    // --- State getters ---
    float GetHPPercentage() const;
    float GetMPPercentage() const;
    bool IsLootWindowOpen() const;
    ROI GetHPBarROI() const;
    ROI GetMPBarROI() const;

private:
    // Read/Write Mutex
    mutable std::shared_mutex m_mutex;

    // Agent specific perception state
    float m_hpPercentage = 100.0f;
    float m_mpPercentage = 100.0f;
    bool m_isLootWindowOpen = false;

    // Dynamically calibrated ROIs
    ROI m_hpBarROI;
    ROI m_mpBarROI;
};

} // namespace Core
