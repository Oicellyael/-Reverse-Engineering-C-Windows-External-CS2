# CS2 External Framework (DirectX 11 & ImGui)
![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat&logo=windows)

🗿Educational project for learning reverse engineering and Windows memory manipulation.🗿

https://youtu.be/UrZ4s53YZPU

A high-performance, multi-threaded external application for Counter-Strike 2, demonstrating advanced memory manipulation, real-time data visualization, and engine state tracking.

## 🛠 Technical Stack
* **Language:** C++.
* **Graphics:** DirectX 11 (Overlay Rendering).
* **UI Framework:** ImGui.
* **Memory Management:** WinAPI (Process IDs, Module Base fetching, RPM/WPM).
* **Image Processing:** `stb_image` for dynamic texture loading.

## 🚀 Key Features
* **Multi-Threaded Architecture:** Decoupled logic for rendering and data processing (Glow/Render threads) to ensure zero impact on game performance.
* **Dynamic Map Detection:** Implements complex pointer-chain traversal (`engine2.dll + 0x909CF8 -> 0x38 -> 0x1A8`) to identify current game maps in real-time.
* **Advanced 2D Radar:** Real-time enemy positioning using trigonometric coordinate transformation based on the local player's view angles[.
* **Recoil Control System (RCS):** Smooth view-angle compensation using `m_aimPunchAngle` and mathematical clamping to maintain human-like movement.
* **Glow Overlay:** External memory modification of the `m_Glow` structures for visual entity highlighting.
* **Configuration System:** Persistent settings management via `config.ini`[.

## 💻 Implementation Details
### Memory Safety & Performance
The project utilizes generic templates for memory I/O, ensuring type safety and reducing code redundancy.
```cpp
template <typename T>
inline T Read(uintptr_t address) {
    T value;
    ReadProcessMemory(process, (LPCVOID)address, &value, sizeof(T), NULL);
    return value;
}
```

### Visual Engine
Rendering is handled via a transparent, top-most Win32 window layered with a DX11 swapchain.Map textures are loaded directly into VRAM as Shader Resource Views (SRVs) for maximum efficiency.

## ⚠️ Disclaimer
This project is for **educational purposes only**. It was developed to explore Windows internals, memory management, and real-time graphics rendering.

---


