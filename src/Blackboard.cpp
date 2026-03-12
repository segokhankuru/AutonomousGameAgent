#include "Blackboard.h"

namespace Core {

// --- Setters (Write Mode) ---

void Blackboard::SetHPPercentage(float hp) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_hpPercentage = hp;
}

void Blackboard::SetMPPercentage(float mp) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_mpPercentage = mp;
}

void Blackboard::SetIsLootWindowOpen(bool isOpen) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_isLootWindowOpen = isOpen;
}

void Blackboard::SetHPBarROI(const ROI& roi) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_hpBarROI = roi;
}

void Blackboard::SetMPBarROI(const ROI& roi) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_mpBarROI = roi;
}


// --- Getters (Read Mode) ---

float Blackboard::GetHPPercentage() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_hpPercentage;
}

float Blackboard::GetMPPercentage() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_mpPercentage;
}

bool Blackboard::IsLootWindowOpen() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_isLootWindowOpen;
}

ROI Blackboard::GetHPBarROI() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_hpBarROI;
}

ROI Blackboard::GetMPBarROI() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_mpBarROI;
}

} // namespace Core
