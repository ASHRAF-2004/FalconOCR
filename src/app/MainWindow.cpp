#include "falcon/app/MainWindow.h"

#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "falcon/core/Image.h"
#include "falcon/core/Raster.h"
#include "falcon/core/GlyphDB.h"
#include "falcon/ocr/OcrTypes.h"
#include "falcon/ocr/Pipeline.h"
#include "falcon/util/String.h"
#endif

namespace falcon::app {

namespace {

#ifdef _WIN32
constexpr wchar_t kWindowClassName[] = L"FalconOCR.MainWindow";
constexpr wchar_t kWindowTitle[] = L"FalconOCR Preview";

enum MenuId : UINT {
  kMenuFileOpen = 1,
  kMenuFileExit = 2,
  kMenuOcrRun = 10,
};

std::wstring Utf8ToWide(const std::string& text) {
  if (text.empty()) {
    return std::wstring();
  }

  const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if (required <= 0) {
    return std::wstring();
  }

  std::wstring wide(static_cast<std::size_t>(required - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), required);
  return wide;
}
#endif

}  // namespace

struct MainWindow::Impl {
#ifdef _WIN32
  HINSTANCE instance{};
  HWND hwnd{};
  HMENU menu{};
  falcon::core::Raster raster;
  std::vector<uint32_t> dib;
  std::filesystem::path opened_path;

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    Impl* impl = nullptr;

    if (msg == WM_NCCREATE) {
      auto* create_struct = reinterpret_cast<LPCREATESTRUCT>(l_param);
      impl = static_cast<Impl*>(create_struct->lpCreateParams);
      impl->hwnd = hwnd;
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
      return DefWindowProcW(hwnd, msg, w_param, l_param);
    }

    impl = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!impl) {
      return DefWindowProcW(hwnd, msg, w_param, l_param);
    }

    switch (msg) {
      case WM_CREATE:
        impl->OnCreate();
        return 0;
      case WM_COMMAND:
        impl->OnCommand(LOWORD(w_param));
        return 0;
      case WM_PAINT:
        impl->OnPaint();
        return 0;
      case WM_DESTROY:
        impl->OnDestroy();
        return 0;
      default:
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
  }

  bool Initialize() {
    instance = GetModuleHandleW(nullptr);

    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &Impl::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassW(&wc)) {
      MessageBoxW(nullptr, L"Failed to register FalconOCR window class.", kWindowTitle, MB_ICONERROR);
      return false;
    }

    hwnd = CreateWindowExW(0, kWindowClassName, kWindowTitle, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, nullptr, nullptr, instance, this);

    if (!hwnd) {
      MessageBoxW(nullptr, L"Failed to create FalconOCR main window.", kWindowTitle, MB_ICONERROR);
      return false;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    return true;
  }

  int RunMessageLoop() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
  }

  void OnCreate() {
    HMENU file_menu = CreateMenu();
    AppendMenuW(file_menu, MF_STRING, kMenuFileOpen, L"&Open…");
    AppendMenuW(file_menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file_menu, MF_STRING, kMenuFileExit, L"E&xit");

    HMENU ocr_menu = CreateMenu();
    AppendMenuW(ocr_menu, MF_STRING, kMenuOcrRun, L"&Run OCR");

    menu = CreateMenu();
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(file_menu), L"&File");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(ocr_menu), L"&OCR");

    SetMenu(hwnd, menu);
  }

  void OnCommand(UINT command_id) {
    switch (command_id) {
      case kMenuFileOpen:
        OnOpenFile();
        break;
      case kMenuFileExit:
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        break;
      case kMenuOcrRun:
        OnRunOcr();
        break;
      default:
        break;
    }
  }

  void OnDestroy() {
    if (menu) {
      DestroyMenu(menu);
      menu = nullptr;
    }
    PostQuitMessage(0);
  }

  void OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT client_rect{};
    GetClientRect(hwnd, &client_rect);
    const int client_width = client_rect.right - client_rect.left;
    const int client_height = client_rect.bottom - client_rect.top;

    if (raster.Empty() || dib.empty()) {
      const wchar_t* message = L"File → Open to load an image and run OCR.";
      DrawTextW(hdc, message, -1, &client_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
      const int src_width = raster.width;
      const int src_height = raster.height;
      if (src_width > 0 && src_height > 0 && client_width > 0 && client_height > 0) {
        const double scale_x = static_cast<double>(client_width) / src_width;
        const double scale_y = static_cast<double>(client_height) / src_height;
        const double scale = std::min(scale_x, scale_y);
        const int draw_width = std::max(1, static_cast<int>(src_width * scale));
        const int draw_height = std::max(1, static_cast<int>(src_height * scale));
        const int offset_x = (client_width - draw_width) / 2;
        const int offset_y = (client_height - draw_height) / 2;

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = src_width;
        bmi.bmiHeader.biHeight = -src_height;  // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        StretchDIBits(hdc, offset_x, offset_y, draw_width, draw_height, 0, 0, src_width, src_height,
                      dib.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
      }
    }

    EndPaint(hwnd, &ps);
  }

  void OnOpenFile() {
    wchar_t file_buffer[MAX_PATH] = {0};

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Images (*.bmp;*.pgm;*.ppm)\0*.bmp;*.pgm;*.ppm\0All Files\0*.*\0";
    ofn.lpstrFile = file_buffer;
    ofn.nMaxFile = static_cast<DWORD>(std::size(file_buffer));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&ofn)) {
      return;
    }

    try {
      opened_path = std::filesystem::path(file_buffer);
      raster = falcon::core::LoadImage(opened_path);
      BuildDibFromRaster();

      std::wstring title = kWindowTitle;
      title += L" - ";
      title += opened_path.filename().wstring();
      SetWindowTextW(hwnd, title.c_str());

      InvalidateRect(hwnd, nullptr, TRUE);
    } catch (const std::exception& ex) {
      const std::wstring message = Utf8ToWide(ex.what());
      MessageBoxW(hwnd, message.c_str(), kWindowTitle, MB_ICONERROR | MB_OK);
    }
  }

  void OnRunOcr() {
    if (raster.Empty()) {
      MessageBoxW(hwnd, L"Load an image before running OCR.", kWindowTitle, MB_ICONINFORMATION | MB_OK);
      return;
    }

    try {
      falcon::ocr::OcrOptions options;
      const auto page = falcon::ocr::RunOcr(raster, options);
      const auto languages = falcon::core::DiscoverLanguagePacks();

      std::u32string text32;
      for (const auto& line : page.lines) {
        for (const auto& ch : line.characters) {
          text32.push_back(ch.classification.codepoint);
        }
        text32.push_back(U'\n');
      }

      auto utf8 = falcon::util::ToUtf8(text32);
      if (utf8.empty()) {
        utf8 = "(No text recognized)";
      }

      std::string header = "Languages: ";
      if (languages.empty()) {
        header += "(builtin ASCII fallback)";
      } else {
        for (std::size_t i = 0; i < languages.size(); ++i) {
          header += languages[i];
          if (i + 1 < languages.size()) {
            header += ", ";
          }
        }
      }
      header += "\n\n";
      const std::wstring wide = Utf8ToWide(header + utf8);
      MessageBoxW(hwnd, wide.c_str(), L"OCR Result", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& ex) {
      const std::wstring message = Utf8ToWide(ex.what());
      MessageBoxW(hwnd, message.c_str(), kWindowTitle, MB_ICONERROR | MB_OK);
    }
  }

  void BuildDibFromRaster() {
    if (raster.Empty()) {
      dib.clear();
      return;
    }

    const std::size_t total_pixels = static_cast<std::size_t>(raster.width) * raster.height;
    dib.resize(total_pixels);

    for (std::size_t index = 0; index < total_pixels; ++index) {
      const uint8_t gray = raster.pixels[index];
      dib[index] = (0xFFu << 24) | (static_cast<uint32_t>(gray) << 16) |
                   (static_cast<uint32_t>(gray) << 8) | static_cast<uint32_t>(gray);
    }
  }
#else
  bool Initialize() {
    std::cout << "FalconOCR was built without the Win32 GUI. Use the CLI mode on non-Windows platforms." << std::endl;
    return true;
  }

  int RunMessageLoop() {
    std::cout << "GUI is only available on Windows. Exiting." << std::endl;
    return 0;
  }
#endif
};

MainWindow::MainWindow() : impl_(std::make_unique<Impl>()) {}
MainWindow::~MainWindow() = default;

bool MainWindow::Initialize() { return impl_->Initialize(); }

int MainWindow::RunMessageLoop() { return impl_->RunMessageLoop(); }

int RunGuiApp() {
  MainWindow window;
  if (!window.Initialize()) {
    return -1;
  }
  return window.RunMessageLoop();
}

}  // namespace falcon::app
