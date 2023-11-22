# JXL Batch Converter
A simple GUI for libjxl binaries, written in C++ with Qt.

![Screnshot](simplelibjxlgui-v03.png)

(new in v0.3) Drag and drop files / folder directly!
![Screnshot-DragNDrop](simplelibjxlgui-v03-filelists.png)

Or I could say a glorified batch command wrapped in GUI... :3

Currently tested on Windows 10 and WSL2 Ubuntu.

Features:
- cjxl, djxl, cjpegli, and djpegli
- Basic encoding parameters GUI
- Custom flags
- Single / multiple file mode
- Folder with / without recursive mode
- Set per-image processing timeout
- Options saved on exit (config file will be placed in user home)

Get libjxl binaries at the [official project page](https://github.com/libjxl/libjxl)!
