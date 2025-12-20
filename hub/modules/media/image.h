#pragma once
// =============================================================================
// IMAGE.H - Image Processing Module
// =============================================================================
// Basic image operations and screen capture using Windows GDI.
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>


namespace Hub::Media {

// =============================================================================
// IMAGE DATA
// =============================================================================
struct ImageData {
  int width = 0;
  int height = 0;
  int channels = 4; // RGBA
  std::vector<uint8_t> pixels;

  bool isValid() const { return width > 0 && height > 0 && !pixels.empty(); }
  size_t size() const { return pixels.size(); }
};

// =============================================================================
// SCREEN CAPTURE
// =============================================================================
inline ImageData captureScreen() {
  ImageData img;

  // Get screen dimensions
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  // Create DC and bitmap
  HDC hScreenDC = GetDC(nullptr);
  HDC hMemDC = CreateCompatibleDC(hScreenDC);
  HBITMAP hBitmap =
      CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);

  SelectObject(hMemDC, hBitmap);

  // Capture screen
  BitBlt(hMemDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

  // Get bitmap info
  BITMAPINFOHEADER bi;
  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = screenWidth;
  bi.biHeight = -screenHeight; // Top-down
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  // Allocate buffer
  img.width = screenWidth;
  img.height = screenHeight;
  img.channels = 4;
  img.pixels.resize(screenWidth * screenHeight * 4);

  // Get pixels
  GetDIBits(hMemDC, hBitmap, 0, screenHeight, img.pixels.data(),
            reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

  // Cleanup
  DeleteObject(hBitmap);
  DeleteDC(hMemDC);
  ReleaseDC(nullptr, hScreenDC);

  return img;
}

// =============================================================================
// CAPTURE WINDOW
// =============================================================================
inline ImageData captureWindow(HWND hwnd) {
  ImageData img;

  RECT rect;
  GetClientRect(hwnd, &rect);
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;

  if (width <= 0 || height <= 0)
    return img;

  HDC hWindowDC = GetDC(hwnd);
  HDC hMemDC = CreateCompatibleDC(hWindowDC);
  HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC, width, height);

  SelectObject(hMemDC, hBitmap);
  BitBlt(hMemDC, 0, 0, width, height, hWindowDC, 0, 0, SRCCOPY);

  BITMAPINFOHEADER bi;
  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = width;
  bi.biHeight = -height;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  img.width = width;
  img.height = height;
  img.channels = 4;
  img.pixels.resize(width * height * 4);

  GetDIBits(hMemDC, hBitmap, 0, height, img.pixels.data(),
            reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

  DeleteObject(hBitmap);
  DeleteDC(hMemDC);
  ReleaseDC(hwnd, hWindowDC);

  return img;
}

// =============================================================================
// SAVE AS BMP
// =============================================================================
inline bool saveBMP(const ImageData &img, const std::string &filepath) {
  if (!img.isValid())
    return false;

  std::ofstream file(filepath, std::ios::binary);
  if (!file)
    return false;

  // BMP header
  BITMAPFILEHEADER fh;
  BITMAPINFOHEADER ih;

  int rowSize = ((img.width * 3 + 3) / 4) * 4; // Row must be 4-byte aligned
  int imageSize = rowSize * img.height;

  fh.bfType = 0x4D42; // "BM"
  fh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;
  fh.bfReserved1 = 0;
  fh.bfReserved2 = 0;
  fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

  ih.biSize = sizeof(BITMAPINFOHEADER);
  ih.biWidth = img.width;
  ih.biHeight = img.height; // Bottom-up for BMP
  ih.biPlanes = 1;
  ih.biBitCount = 24;
  ih.biCompression = BI_RGB;
  ih.biSizeImage = imageSize;
  ih.biXPelsPerMeter = 0;
  ih.biYPelsPerMeter = 0;
  ih.biClrUsed = 0;
  ih.biClrImportant = 0;

  file.write(reinterpret_cast<char *>(&fh), sizeof(fh));
  file.write(reinterpret_cast<char *>(&ih), sizeof(ih));

  // Write pixels (BGR, bottom-up)
  std::vector<uint8_t> row(rowSize, 0);
  for (int y = img.height - 1; y >= 0; y--) {
    for (int x = 0; x < img.width; x++) {
      int srcIdx = (y * img.width + x) * 4;
      int dstIdx = x * 3;
      row[dstIdx + 0] = img.pixels[srcIdx + 0]; // B
      row[dstIdx + 1] = img.pixels[srcIdx + 1]; // G
      row[dstIdx + 2] = img.pixels[srcIdx + 2]; // R
    }
    file.write(reinterpret_cast<char *>(row.data()), rowSize);
  }

  return true;
}

} // namespace Hub::Media

// =============================================================================
// C API FOR FFI
// =============================================================================
extern "C" {
__declspec(dllexport) int hub_screen_width();
__declspec(dllexport) int hub_screen_height();
__declspec(dllexport) int hub_screen_capture(const char *filepath);
}
