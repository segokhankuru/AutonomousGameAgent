cmake_minimum_required(VERSION 3.20)
project(AutonomousGameAgent VERSION 1.0.0 LANGUAGES CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find necessary Windows libraries for Sprint 1
# d3d11.lib, dxgi.lib, user32.lib, setupapi.lib (for COM port detection)
link_libraries(d3d11 dxgi user32 setupapi)

# ------------------------------------------------------------------------------
# Future Sprints Preparations (Commented out for now)
# ------------------------------------------------------------------------------

# --- OpenCV ---
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_libraries(${OpenCV_LIBS})

# --- ONNX Runtime ---
# find_package(onnxruntime REQUIRED)
# include_directories(${ONNXRUNTIME_INCLUDE_DIRS})
# link_libraries(${ONNXRUNTIME_LIBRARIES})

# --- Tesseract ---
# find_package(Tesseract REQUIRED)
# include_directories(${Tesseract_INCLUDE_DIRS})
# link_libraries(${Tesseract_LIBRARIES})

# ------------------------------------------------------------------------------

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Target link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 dxgi user32 setupapi ${OpenCV_LIBS})
