# GameGirl

## Intro
I create this gamegirl for my nieces & for my personnal pleasure

## What

## Features

- STM32F429ZIT6
- 2.4" QVGA TFT LCD 
- Joystick
- 2 button 
- 1 start button
- 1 buzzer
- EPROM (see if that neccesary)
- 3D printed case
- Lithium-ion
- USB type C charge 

## Display pipeline

The framebuffer (240×320, RGB565, 150 KB) lives in the external SDRAM at `0xD0000000` (FMC bank 2).

```
          writes                        reads (DMA)
  CPU  ────────────►  SDRAM  ◄────────────  LTDC  ──── parallel RGB ───►  ILI9341
                  (0xD0000000)                        (pixels, ~60 Hz)    (screen)
        via FMC                via FMC
                                            SPI5 ──── config only ─────►  ILI9341
```

- **CPU** draws pixels into the framebuffer in SDRAM, through the FMC.
- **LTDC** has its own DMA: it continuously reads the SDRAM line by line, with no CPU involvement.
- It pushes the pixels to the screen over the **parallel RGB bus** (R/G/B, HSYNC, VSYNC, DOTCLK).
- **ILI9341** just displays what it receives; it never reads memory itself.
- **SPI5** is only used once in `ili9341_Init()` to configure the panel (RGB mode, orientation…). No pixel goes through it.

CPU and LTDC share the FMC bandwidth. The LTDC needs about 9 MB/s (240 × 320 × 2 bytes × 60 Hz) out of ~128 MB/s theoretical (16-bit SDRAM @ 64 MHz), so plenty is left for the CPU.

Because the LTDC reads continuously while the CPU writes into the same buffer, a single buffer can cause tearing. The SDRAM leaves room for double buffering (2 × 150 KB).

