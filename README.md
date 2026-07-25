# dotdeskop_editor

A sleek, fast, and modern viewer and manager for Linux `.desktop` files built with C++ and Qt. 
It features a beautifully customized **Catppuccin Mocha** inspired user interface, fluid animations, and quick contextual actions for managing and launching your system applications.

## ✨ Features

- **Dual Viewing Modes**: Seamlessly toggle between a spacious Grid view and a detailed List view.
- **Real-time Search**: Instantly filter through your applications by name, keywords, or execution commands.
- **Quick Context Menu**: Click the options button (⋮) on any application card to:
  - Launch the application directly.
  - Open the underlying `.desktop` file in your default text editor.
  - Copy the application's name, execution command, or file path to your clipboard.
- **Modern UI Elements**: Enjoy interactive visual feedback with custom ripple effect buttons and a dynamic, mouse-tracking radial gradient glow on the main window.
- **Broad Directory Support**: Automatically scans standard system paths, user local paths, Flatpak exports, and Snap directories to find all installed `.desktop` entries.

## 🛠️ Prerequisites

Ensure you have the following dependencies installed on your system:
- **CMake**
- **Make**
- **Qt5** or **Qt6** development libraries (Core, Gui, Widgets)
- A C++ compiler (GCC or Clang)

*Example for Arch Linux:*
```bash
sudo pacman -S base-devel cmake qt5-base
```

## 🚀 Build Instructions

Building the project is straightforward. Navigate to the root directory of the project in your terminal and run the following command:

```bash
cmake . && make
```

## 💻 Usage

After a successful compilation, you can run the application directly from the build directory:

```bash
./dotdeskop_editor
```