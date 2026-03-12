#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <wrl/client.h> // For Microsoft::WRL::ComPtr

namespace Display {

class ScreenCapture {
public:
    // Initializes DXGI capture for the specified monitor index (0 = primary)
    ScreenCapture(UINT monitorIndex = 0);
    ~ScreenCapture();

    // Captures the current frame.
    // Returns true if a new frame was successfully captured.
    bool CaptureFrame();

    // Gets the dimensions of the captured screen.
    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }

    // Gets a pointer to the raw BGRA pixel data of the last captured frame.
    // Ensure you only access within [0, GetWidth() * GetHeight() * 4)
    const uint8_t* GetPixelData() const { return m_pixelData.data(); }

private:
    void Initialize(UINT monitorIndex);
    void Cleanup();

private:
    UINT m_width = 0;
    UINT m_height = 0;
    std::vector<uint8_t> m_pixelData;

    // DirectX / DXGI Interfaces
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_deskDupl;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_stagingTexture;
};

} // namespace Display
