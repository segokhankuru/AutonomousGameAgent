#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>
#include <vector>

#include "Blackboard.h"

namespace Perception {

// Handles finding static HUD elements initially (Calibration via Template Matching)
// and dynamically processing changes inside ROIs (Color Thresholding).
class VisionModule {
public:
    VisionModule(std::shared_ptr<Core::Blackboard> blackboard);
    ~VisionModule() = default;

    // Phase 1: Initial Calibration
    // Loads an image template and finds it on the provided full screen capture.
    // If found, updates the Blackboard's specific ROI and returns true.
    bool CalibrateHPBar(const cv::Mat& fullScreen, const std::string& templatePath);
    bool CalibrateMPBar(const cv::Mat& fullScreen, const std::string& templatePath);

    // Phase 2: Dynamic Execution
    // Takes the full screen image and calculates HP percentage based on the known ROI
    // Uses simple red color thresholding. Updates Blackboard directly.
    void UpdateHPPercentage(const cv::Mat& fullScreen);

    // Takes the full screen image and calculates MP percentage based on the known ROI
    // Uses simple blue color thresholding. Updates Blackboard directly.
    void UpdateMPPercentage(const cv::Mat& fullScreen);

private:
    // Helper function to find a template inside a source image and return its ROI.
    bool FindTemplate(const cv::Mat& src, const std::string& templatePath, Core::ROI& outROI, double threshold = 0.8);

    // Helper to calculate fill percentage by color thresholding
    float CalculatePercentageByColor(const cv::Mat& roiImage, const cv::Scalar& lowerBound, const cv::Scalar& upperBound);

private:
    std::shared_ptr<Core::Blackboard> m_blackboard;
};

} // namespace Perception
