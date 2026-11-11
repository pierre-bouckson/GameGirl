# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

KiCad 9 project for the GameGirl handheld console PCB (`PCB.kicad_pro`, `PCB.kicad_sch`, `PCB.kicad_pcb`). The firmware lives in `../firmware/` (see `../firmware/CLAUDE.md`) and currently runs on an STM32F429I-DISC1 dev board. This board replaces it, so it must reproduce the parts of the DISC1 the firmware relies on. Reference schematic of the DISC1: `../docs/STM32F429I-DISC1/mb1075-f429i-c01_schematic.pdf`; MCU datasheet in the same folder; panel datasheet `../docs/ILI9341-Screen.pdf`.

Target hardware (from `../README.md`): STM32F429ZIT6 (LQFP144), 2.4" QVGA ILI9341 TFT, analog joystick, 2 buttons + 1 start button, buzzer, optional EEPROM, Li-ion battery with USB-C charging, 3D-printed case.

## Pin assignment comes from the firmware

`../firmware/GameGirl.ioc` (CubeMX) is the source of truth for MCU pins. Keep the schematic consistent with it, or change the `.ioc` together with the schematic:
- **FMC → external SDRAM** (38 pins, bank 2 at `0xD0000000`, 16-bit): holds the double framebuffer, so the SDRAM is mandatory.
- **LTDC → ILI9341 parallel RGB** (20 pins: RGB, HSYNC, VSYNC, DOTCLK, DE) for pixels; **SPI5** (+ CS/WRX GPIOs) only configures the panel.
- **Joystick**: ADC1 channel 5 = PA5 (X), channel 13 = PC3 (Y).
- **Main button**: PA0 on EXTI0.
- **USART1** (115200, `printf` output): expose it on a header/test points, since there is no ST-LINK VCP on this board.
- **I2C3** is configured (touch controller on the DISC1).
- Game best scores are stored in internal flash sectors 22/23, so the external EEPROM is optional.

Get exact pin names with `grep -E '^P[A-I][0-9]+.*\.Signal' ../firmware/GameGirl.ioc`.

## Libraries

All project libraries are project-relative (`${KIPRJMOD}`), declared in `sym-lib-table` / `fp-lib-table`:
- `JLCPCB-*` symbols + `PCM_JLCPCB` footprints (`libs/JLCPCB/`): JLCPCB basic/preferred parts (CDFER library). Prefer these to keep assembly cheap.
- `EasyEDA` (`libs/EasyEDA/`): parts imported by LCSC number. Add parts with:
  ```sh
  ./libs/add_lcsc.sh C2040 C25804 ...   # easyeda2kicad: symbol + footprint + 3D into libs/EasyEDA
  ```

## Commands

```sh
kicad-cli sch erc PCB.kicad_sch                      # electrical rules check
kicad-cli pcb drc PCB.kicad_pcb                      # design rules check
kicad-cli sch export pdf PCB.kicad_sch -o PCB.pdf    # schematic PDF
kicad-cli pcb export gerbers PCB.kicad_pcb -o gerbers/
kicad-cli pcb export drill PCB.kicad_pcb -o gerbers/
```

KiCad files are S-expressions; if editing them as text, KiCad (eeschema/pcbnew) must be closed (`~*.lck` exists while a file is open) or it will overwrite the changes. `PCB-backups/`, `*.lck` and `*.kicad_prl` are local KiCad state, not design files.

## Conventions

- Comments and the user's messages are in French; commit messages in English.
- Work happens on feature branches merged into `main` through GitHub PRs.
