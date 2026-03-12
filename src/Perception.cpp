#include "Perception.h"
#include <iostream>

namespace Perception {

VisionModule::VisionModule(std::shared_ptr<Core::Blackboard> blackboard)
    : m_blackboard(std::move(blackboard)) {
}

bool VisionModule::CalibrateHPBar(const cv::Mat& fullScreen, const std::string& templatePath) {
    Core::ROI foundRoi;
    if (FindTemplate(fullScreen, templatePath, foundRoi, 0.8)) {
        m_blackboard->SetHPBarROI(foundRoi);
        std::cout << "[Perception] HP Bar Calibrated. ROI: ("
                  << foundRoi.x << "," << foundRoi.y << ") "
                  << foundRoi.width << "x" << foundRoi.height << std::endl;
        return true;
    }
    std::cerr << "[Perception] Failed to calibrate HP Bar. Template not found." << std::endl;
    return false;
}

bool VisionModule::CalibrateMPBar(const cv::Mat& fullScreen, const std::string& templatePath) {
    Core::ROI foundRoi;
    if (FindTemplate(fullScreen, templatePath, foundRoi, 0.8)) {
        m_blackboard->SetMPBarROI(foundRoi);
        std::cout << "[Perception] MP Bar Calibrated. ROI: ("
                  << foundRoi.x << "," << foundRoi.y << ") "
                  << foundRoi.width << "x" << foundRoi.height << std::endl;
        return true;
    }
    std::cerr << "[Perception] Failed to calibrate MP Bar. Template not found." << std::endl;
    return false;
}

bool VisionModule::FindTemplate(const cv::Mat& src, const std::string& templatePath, Core::ROI& outROI, double threshold) {
    if (src.empty()) return false;

    // Read the template image
    cv::Mat templ = cv::imread(templatePath, cv::IMREAD_COLOR);
    if (templ.empty()) {
        std::cerr << "FindTemplate Error: Could not load template " << templatePath << std::endl;
        return false;
    }

    // Convert both images to BGR (DXGI captures typically BGRA or similar)
    // We assume the source is 4 channels (BGRA), we must strip alpha for matchTemplate
    cv::Mat srcBgr;
    if (src.channels() == 4) {
        cv::cvtColor(src, srcBgr, cv::COLOR_BGRA2BGR);
    } else {
        srcBgr = src;
    }

    // Result matrix for Template Matching
    cv::Mat result;
    int result_cols = srcBgr.cols - templ.cols + 1;
    int result_rows = srcBgr.rows - templ.rows + 1;

    if (result_cols <= 0 || result_rows <= 0) {
        std::cerr << "FindTemplate Error: Template image is larger than the source." << std::endl;
        return false;
    }

    result.create(result_rows, result_cols, CV_32FC1);

    // Perform Template Matching using TM_CCOEFF_NORMED
    cv::matchTemplate(srcBgr, templ, result, cv::TM_CCOEFF_NORMED);

    // Locate the best match
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

    if (maxVal >= threshold) {
        outROI.x = maxLoc.x;
        outROI.y = maxLoc.y;
        outROI.width = templ.cols;
        outROI.height = templ.rows;
        return true;
    }

    return false;
}

float VisionModule::CalculatePercentageByColor(const cv::Mat& roiImage, const cv::Scalar& lowerBound, const cv::Scalar& upperBound) {
    if (roiImage.empty()) return 100.0f;

    // Convert from BGRA/BGR to HSV for better color segmentation
    cv::Mat hsvImage;
    if (roiImage.channels() == 4) {
        cv::cvtColor(roiImage, hsvImage, cv::COLOR_BGRA2BGR); // First drop alpha
        cv::cvtColor(hsvImage, hsvImage, cv::COLOR_BGR2HSV);
    } else {
        cv::cvtColor(roiImage, hsvImage, cv::COLOR_BGR2HSV);
    }

    cv::Mat mask;
    cv::inRange(hsvImage, lowerBound, upperBound, mask);

    // Calculate the ratio of white pixels (matching color) vs total pixels
    int totalPixels = mask.rows * mask.cols;
    if (totalPixels == 0) return 100.0f;

    int coloredPixels = cv::countNonZero(mask);
    return (static_cast<float>(coloredPixels) / totalPixels) * 100.0f;
}

void VisionModule::UpdateHPPercentage(const cv::Mat& fullScreen) {
    Core::ROI hpROI = m_blackboard->GetHPBarROI();

    // Check if calibration has happened
    if (!hpROI.IsValid()) return;

    // Ensure ROI is within image bounds
    if (hpROI.x < 0 || hpROI.y < 0 ||
        hpROI.x + hpROI.width > fullScreen.cols ||
        hpROI.y + hpROI.height > fullScreen.rows) return;

    // Extract ROI
    cv::Rect rect(hpROI.x, hpROI.y, hpROI.width, hpROI.height);
    cv::Mat barImage = fullScreen(rect);

    // Red color in HSV (hue wraps around so we usually need two ranges, but for simplicity we'll use one broad range if applicable,
    // or two masks. Here we'll use a standard red range combining both ends of hue)
    cv::Mat mask1, mask2, combinedMask;
    cv::Mat hsvImage;
    if (barImage.channels() == 4) {
        cv::cvtColor(barImage, hsvImage, cv::COLOR_BGRA2BGR);
        cv::cvtColor(hsvImage, hsvImage, cv::COLOR_BGR2HSV);
    } else {
        cv::cvtColor(barImage, hsvImage, cv::COLOR_BGR2HSV);
    }

    // OpenCV Hue is 0-180. Red is 0-10 and 160-180.
    cv::inRange(hsvImage, cv::Scalar(0, 70, 50), cv::Scalar(10, 255, 255), mask1);
    cv::inRange(hsvImage, cv::Scalar(160, 70, 50), cv::Scalar(180, 255, 255), mask2);
    cv::bitwise_or(mask1, mask2, combinedMask);

    int totalPixels = combinedMask.rows * combinedMask.cols;
    int coloredPixels = cv::countNonZero(combinedMask);
    float hpPct = (totalPixels > 0) ? (static_cast<float>(coloredPixels) / totalPixels) * 100.0f : 100.0f;

    m_blackboard->SetHPPercentage(hpPct);
}

void VisionModule::UpdateMPPercentage(const cv::Mat& fullScreen) {
    Core::ROI mpROI = m_blackboard->GetMPBarROI();

    // Check if calibration has happened
    if (!mpROI.IsValid()) return;

    // Ensure ROI is within image bounds
    if (mpROI.x < 0 || mpROI.y < 0 ||
        mpROI.x + mpROI.width > fullScreen.cols ||
        mpROI.y + mpROI.height > fullScreen.rows) return;

    // Extract ROI
    cv::Rect rect(mpROI.x, mpROI.y, mpROI.width, mpROI.height);
    cv::Mat barImage = fullScreen(rect);

    // Blue color in HSV (approx 100 to 130)
    cv::Scalar lowerBlue(100, 150, 0);
    cv::Scalar upperBlue(140, 255, 255);

    float mpPct = CalculatePercentageByColor(barImage, lowerBlue, upperBlue);
    m_blackboard->SetMPPercentage(mpPct);
}

} // namespace Perception
