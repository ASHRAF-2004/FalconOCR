#pragma once

#include <memory>
#include <string>

namespace falcon::app {

class MainWindow {
 public:
  MainWindow();
  ~MainWindow();

  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;

  bool Initialize();
  int RunMessageLoop();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

int RunGuiApp();

}  // namespace falcon::app
