#include "Display.h"
#include <cstring>

namespace Display {

ScreenCapture::ScreenCapture(UINT monitorIndex) {
    Initialize(monitorIndex);
}

ScreenCapture::~ScreenCapture() {
    Cleanup();
}

void ScreenCapture::Initialize(UINT monitorIndex) {
    HRESULT hr = S_OK;

    // Create D3D11 Device
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        1,
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        &featureLevel,
        &m_d3dContext
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create D3D11 device.");
    }

    // Get DXGI Device
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to obtain IDXGIDevice.");
    }

    // Get DXGI Adapter
    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to obtain IDXGIAdapter.");
    }

    // Get DXGI Output for the specified monitor index
    Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
    hr = dxgiAdapter->EnumOutputs(monitorIndex, &dxgiOutput);
    if (hr == DXGI_ERROR_NOT_FOUND) {
        throw std::runtime_error("Monitor index not found.");
    } else if (FAILED(hr)) {
        throw std::runtime_error("Failed to enumerate DXGI output.");
    }

    // Get DXGI Output1
    Microsoft::WRL::ComPtr<IDXGIOutput1> dxgiOutput1;
    hr = dxgiOutput.As(&dxgiOutput1);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to obtain IDXGIOutput1.");
    }

    // Duplicate Output
    hr = dxgiOutput1->DuplicateOutput(m_d3dDevice.Get(), &m_deskDupl);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to duplicate output. Are you running in a Remote Desktop session?");
    }

    // Get output description
    DXGI_OUTDUPL_DESC duplDesc;
    m_deskDupl->GetDesc(&duplDesc);

    m_width = duplDesc.ModeDesc.Width;
    m_height = duplDesc.ModeDesc.Height;

    // Create a staging texture to map to CPU memory
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = m_width;
    stagingDesc.Height = m_height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    hr = m_d3dDevice->CreateTexture2D(&stagingDesc, nullptr, &m_stagingTexture);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create staging texture.");
    }

    // Pre-allocate vector for pixel data
    m_pixelData.resize(m_width * m_height * 4);

    std::cout << "DXGI Capture Initialized for monitor " << monitorIndex
              << " (" << m_width << "x" << m_height << ")" << std::endl;
}

void ScreenCapture::Cleanup() {
    m_deskDupl.Reset();
    m_stagingTexture.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();
}

bool ScreenCapture::CaptureFrame() {
    if (!m_deskDupl) return false;

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    Microsoft::WRL::ComPtr<IDXGIResource> desktopResource;
    HRESULT hr = m_deskDupl->AcquireNextFrame(
        10, // Wait up to 10ms for a new frame
        &frameInfo,
        &desktopResource
    );

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // Timeout expected when screen is static
        return false;
    } else if (FAILED(hr)) {
        // Other failures (e.g., lost desktop access like pressing Ctrl+Alt+Del)
        // A robust implementation would re-initialize the Duplication API here.
        std::cerr << "Warning: AcquireNextFrame failed with HR: " << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Only process if we actually got a new image
    if (frameInfo.LastPresentTime.QuadPart == 0) {
        m_deskDupl->ReleaseFrame();
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> desktopTexture;
    hr = desktopResource.As(&desktopTexture);
    if (FAILED(hr)) {
        std::cerr << "Failed to get Texture2D from DXGIResource." << std::endl;
        m_deskDupl->ReleaseFrame();
        return false;
    }

    // Copy from GPU (desktopTexture) to CPU-accessible staging texture (m_stagingTexture)
    m_d3dContext->CopyResource(m_stagingTexture.Get(), desktopTexture.Get());

    // Map staging texture memory to CPU
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = m_d3dContext->Map(m_stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        const uint8_t* sourceData = static_cast<const uint8_t*>(mapped.pData);
        uint8_t* destData = m_pixelData.data();

        // Copy row by row to handle Pitch padding correctly
        for (UINT y = 0; y < m_height; ++y) {
            std::memcpy(
                destData + (y * m_width * 4),
                sourceData + (y * mapped.RowPitch),
                m_width * 4
            );
        }

        m_d3dContext->Unmap(m_stagingTexture.Get(), 0);
    } else {
        std::cerr << "Failed to map staging texture." << std::endl;
    }

    m_deskDupl->ReleaseFrame();
    return true;
}

} // namespace Display
