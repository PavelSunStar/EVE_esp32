# EVE_esp32

High-performance ESP32 driver for Bridgetek EVE BT81x graphics controllers  
(BT815 / BT816 / BT817 – including VM816C50A modules).

Designed for high-speed SPI / Quad-SPI communication with DMA support and a clean layered architecture.

---

## ✨ Features

- SPI and Quad-SPI support
- DMA-accelerated RAM_G transfers
- Display List (DL) management
- Bitmap and sprite support
- Layered architecture (Bus → Core → GFX → Sprite)
- Optimized for ESP32-S3
- Arduino-compatible
