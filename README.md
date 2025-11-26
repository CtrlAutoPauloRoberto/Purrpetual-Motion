# 🐱 Bongo Cat Stealth Mouse Jiggler (ESP32)

A programmable, undetectable mouse jiggler that disguises itself as a cute desktop accessory. It displays an animated **Bongo Cat** GIF while subtly moving the mouse cursor to prevent computer sleep or "Away" status.

Unlike standard jigglers, this project uses a **human-like (mybe a Cat-like?) movement algorithm** and spoofs the Bluetooth ID of a legitimate Logitech mouse.

## ✨ Key Features

* **👻 Stealth Operation:** Identifies via Bluetooth Low Energy (BLE) as a **"Logitech MX Master 3"**.
* **🥁 Visual Feedback:** Plays a smooth Bongo Cat animation on the IPS display.
* **🧠 Humanized Physics:** Instead of robotic zig-zags, the cursor moves in random, smooth curves with variable speeds and pauses, mimicking human behavior.
* **🔵 Custom UI:** Features a custom-drawn, iOS-style Bluetooth status badge (Squircle shape) that updates in real-time.
* **🔄 Auto-Reconnect Watchdog:** Automatically reboots the MCU if the Bluetooth connection drops for more than 30 seconds, ensuring the device is always ready.
* **💡 PWM Brightness Control:** Optimized for **LilyGO T-Display S3** (Pin 38) for a subtle, low-light appearance.

## 🛠️ Hardware Requirements

* **LilyGO T-Display S3** (ESP32-S3 with built-in ST7789 display).
* USB-C Cable.
* (Optional) 3D Printed case for a desktop finish. (You can find one in to printables.com website, one like: https://www.printables.com/model/1476247-lilygo-t-display-s3-amoled-case) 

## 📦 Dependencies

This project is built using the Arduino framework. You will need the following libraries:

1. **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** (by Bodmer) - *Driver for the display.*
2. **[ESP32-BLE-Mouse](https://github.com/T-vK/ESP32-BLE-Mouse)** (by T-vK) - *Bluetooth HID emulation.*
3. **[AnimatedGIF](https://github.com/bitbank2/AnimatedGIF)** (by bitbank2) - *GIF decoding.*

## ⚙️ Configuration

You can tweak the behavior at the top of the `main.cpp` file:

### Movement Settings
```cpp
#define MAX_MOVE_RANGE 15       // Radius of movement (pixels)
#define MOUSE_SPEED_FACTOR 0.05 // Lower = Smoother/Slower
#define MIN_WAIT_TIME 2000      // Min pause between moves (ms)
#define MAX_WAIT_TIME 15000     // Max pause between moves (ms)
