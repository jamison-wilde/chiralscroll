ChiralScroll

Implements chiral scrolling for Windows 10 similar to the Synaptics driver that used to work on Windows 7. Uses raw touchpad input, so it should work on any laptop or other touchpad device.


Usage:

To use, simply leave running in the background. ChiralScroll runs in the system tray.

To scroll vertically touch the right edge of the touchpad and drag up or down. To scroll horizontally touch the bottom edge of the touchpad and drag left or right. Once you start dragging you can continue to drag in circles to scroll coninuously.


Settings:

Right click the tray icon and select settings. You can change the scroll speed for both horizontal and vertical scrolling, and the size of the edge zones that start scrolling. Settings are saved in a settings.ini file in the same directory. To reverse the scrolling direction, use a negative scroll speed.

The settings window lists all touchpad devices connected to the system. Should you have more than one, you can set them independently. The dvice names may not be obvous, so you may need to experiment to determine which device has which name.


Building:

Build using Visual Studio 2022 (Debug|x64).

C++ dependencies (abseil, spdlog, wxwidgets; triplet x64-windows-static) are installed automatically by vcpkg in manifest mode. This requires a git clone of vcpkg with user-wide MSBuild integration:

    git clone https://github.com/microsoft/vcpkg C:\vcpkg
    C:\vcpkg\bootstrap-vcpkg.bat
    C:\vcpkg\vcpkg integrate install

The first build compiles wxWidgets from source and takes a while.

The settings dialog (src/SettingsDialog.cpp/.h) is hand-written and checked in; wxFormBuilder is no longer required to build. The original design document is kept at formbuilder/ChiralScroll.fbp for reference.

The released version is based on the Debug build, and this is the version I recommend building. The Release build seems to have an issue where SendInput is occasionally very slow, causing scrolling to freeze. (The Release configuration also still targets the v142 toolset.)