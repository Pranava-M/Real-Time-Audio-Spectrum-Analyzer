# STM32F401 Real-Time Audio Spectrum Analyzer

A real-time audio spectrum analyzer built on the **STM32F401 "Black Pill"**. A MAX4466
microphone is sampled by the ADC at 16 kHz, a 512-point FFT splits the sound into
frequencies, and the result is drawn live as 32 bars on an ILI9341 TFT and streamed to a
phone over Bluetooth (HC-05).

Written in **C with CMSIS only (register level, no HAL)** for **Keil µVision 5**.
Embedded Systems case study project.

> **Course:** Embedded Systems

---

## Features

- **Live spectrum:** 32 logarithmic bars from 31 Hz to 8 kHz, with falling bars and peak-hold dots.
- **Peak frequency readout** with parabolic interpolation (finer than the 31.25 Hz FFT bin).
- **Oscilloscope mode:** 20 ms waveform view that locks onto a rising zero crossing.
- **Level meter** in % and dBFS, with a **CLIP** warning when the ADC saturates.
- **Bluetooth link** (HC-05): live report 4 times a second and remote control from a phone.
- **3 push buttons** on external interrupts: MODE, PAUSE, GRID.
- **4 status LEDs:** peak, heartbeat, grid, paused.
- **No HAL, no libraries.** Clock, GPIO, timer, ADC, SPI, UART and FFT are all written by hand.

## How it works

```
Sound → MAX4466 → PA1 (ADC1)
        TIM2 triggers ADC at 16 kHz → ADC interrupt stores 512 samples (32 ms)
        → remove DC → Hann window → 512-point FFT → 32 dB bars + peak frequency
        → SPI1 → ILI9341 TFT          → USART1 → HC-05 → phone
```

| Quantity | Value |
|---|---|
| CPU clock | 84 MHz (25 MHz crystal + PLL, falls back to internal HSI) |
| Sampling rate | 16 kHz (TIM2 trigger, so sample spacing is exact) |
| **Bandwidth** | **8 kHz** (Nyquist: 16 kHz / 2) |
| FFT size | 512 points |
| Frequency resolution | 31.25 Hz per bin |
| Block time | 32 ms |
| ADC | 12-bit, 21 MHz ADC clock |
| TFT SPI clock | 10.5 MHz |
| Bluetooth | 9600 baud, 8N1 |

## Hardware

| Part | Purpose |
|---|---|
| STM32F401 Black Pill (CCU6 or CEU6) | Microcontroller, Cortex-M4F at 84 MHz |
| MAX4466 microphone module | Sensor with adjustable-gain amplifier |
| ILI9341 2.4"/2.8" SPI TFT, 320×240 | Display |
| HC-05 Bluetooth module | Wireless link to a phone |
| 3 tactile buttons | MODE, PAUSE, GRID |
| 3 LEDs + 3 × 220 Ω resistors | Peak, heartbeat, grid |
| 100 nF and 10 µF capacitors | Supply decoupling |
| 830-point breadboard, jumper wires | Wiring |
| ST-Link V2 | Flashing and debugging |

## Wiring

Power the Black Pill from USB-C. Connect all grounds together.

**MAX4466**

| Module | Black Pill |
|---|---|
| VCC | 3V3 |
| GND | GND |
| OUT | PA1 |

**ILI9341 TFT (SPI1)**

| Module | Black Pill |
|---|---|
| VCC | 3V3 (use 5V if the screen stays white or dim) |
| GND | GND |
| CS | PA4 |
| RESET | PB1 |
| DC | PB0 |
| SDI (MOSI) | PA7 |
| SCK | PA5 |
| LED | 3V3 |
| SDO (MISO) | PA6 (optional) |

**HC-05 (USART1)**

| Module | Black Pill |
|---|---|
| VCC | 5V |
| GND | GND |
| TXD | PA10 |
| RXD | PA9 |
| STATE | PB15 (optional, shows "BT:LINK") |

TX and RX are crossed: module TXD goes to PA10, module RXD goes to PA9.

**Buttons** (one leg to the pin, other leg to GND, internal pull-ups are used)

| Button | Pin |
|---|---|
| MODE | PB12 |
| PAUSE | PB13 |
| GRID | PB14 |

**LEDs** (pin → 220 Ω → LED long leg, short leg → GND)

| LED | Pin | Meaning |
|---|---|---|
| Red (PEAK) | PB7 | Very loud sound or clipping |
| Green (HEARTBEAT) | PB8 | Blinks at 1 Hz while the firmware runs |
| Blue (GRID) | PB9 | Grid lines are on |
| On-board | PC13 | Display is paused |

**ST-Link V2:** SWDIO → DIO (PA13), SWCLK → SCK (PA14), GND → GND.

## Build and flash (Keil µVision 5)

1. In **Pack Installer**, install `Keil::STM32F4xx_DFP`.
2. **Project → New µVision Project**. Choose device **STM32F401CCUx** (or **CEUx** to match your chip).
3. In **Manage Run-Time Environment**, tick only **CMSIS → CORE** and **Device → Startup**. Do not add any HAL components.
4. Add the source file(s):
   - **Single file:** add `main.c` only.
   - **Multi-file:** add all `.c` files from `src/` and put `.\src` in the include path.
5. **Options for Target (Alt+F7)**
   - Target: Xtal = **25.0 MHz**, Floating Point Hardware = **Single Precision**, ARM Compiler V6.
   - C/C++: optimisation **-O1** or **-O2**; leave "Define" empty (add `STM32F401xC` only if Keil asks for a device).
   - Debug: **ST-Link Debugger**, port SW. Flash Download: tick **Reset and Run**.
6. **Build (F7)**, then **Download (F8)**.

Build size is about 20 KB flash and 17 KB RAM.

## Usage

On power-up a splash screen shows the clock source for 1.5 s, then the analyzer starts.
Play music or a tone near the microphone and the bars move.

| Control | Action |
|---|---|
| MODE button or `M` | Switch between bars and waveform |
| PAUSE button or `P` | Freeze or resume the display |
| GRID button or `G` | Grid lines on or off |
| `+` / `-` (Bluetooth) | Display gain up or down in 6 dB steps (−12 to +24 dB) |
| `?` (Bluetooth) | Show help |

**Bluetooth:** pair with **HC-05** (PIN `1234` or `0000`), open a serial terminal app
(for example *Serial Bluetooth Terminal*) at **9600 baud**. You receive 4 lines a second:

```
F=0440Hz L=063% B=00012357899875321000001233210000
```

`F` is the dominant frequency, `L` the level in %, and `B` the 32 bars as digits 0–9
(left = low frequency). `CLIP` is appended when the ADC saturates.

## Interrupts (NVIC)

| Priority | Handler | Purpose |
|---|---|---|
| 1 | `ADC_IRQHandler` | Stores one audio sample per conversion |
| 2 | `SysTick_Handler` | 1 ms time base |
| 3 | `USART1_IRQHandler` | Bluetooth receive and transmit (ring buffers) |
| 4 | `EXTI15_10_IRQHandler` | Button presses (debounced in the main loop) |

## Repository layout

```
.
├── README.md
├── main.c                  single-file version (paste into Keil)
├── src/                    multi-file version of the same firmware
│   ├── board.h             pin map and constants
│   ├── bsp.c / bsp.h       clock, SysTick, GPIO, LEDs, buttons, TIM2 + ADC1
│   ├── uart_bt.c / .h      HC-05 driver (USART1, ring buffers)
│   ├── ili9341.c / lcd.h   TFT driver (SPI1)
│   ├── gfx.c / .h, font5x7.c / .h   rectangles and text
│   ├── spectrum.c / .h     DC removal, window, FFT, bars, peak frequency
│   ├── ui.c / .h           screen layout and drawing
│   └── main.c              start-up and main loop
└── docs/
    ├── CODE_EXPLAINED.md   detailed explanation of the code
    └── images/             add your photos, screenshots and wiring diagram here
```

## Case-study requirements

| Requirement | Where it is met |
|---|---|
| Wireless protocol | Bluetooth, HC-05 on USART1 |
| ADC / GPIO | ADC1 reads the microphone; GPIO drives LEDs and reads buttons |
| NVIC interrupts | ADC, SysTick, USART1 and EXTI with set priorities |
| Display interface | ILI9341 TFT |
| Peripheral protocol | SPI (TFT) and UART (HC-05) |
| IDE and library | Keil µVision 5, CMSIS |

## Troubleshooting

| Problem | Fix |
|---|---|
| `cannot open source input file stm32f4xx.h` | Open Manage Run-Time Environment and tick CMSIS → CORE and Device → Startup |
| "Please select first the target STM32F4xx device" | Add `STM32F401xC` (or `STM32F401xE`) to C/C++ → Define |
| White screen | Check SCK, MOSI, DC, RESET, CS wires; try the TFT VCC on 5V |
| Picture upside down or mirrored | In the TFT driver, change `LCD_MADCTL` from `0x28` to `0xE8` |
| Red and blue swapped | Change `LCD_MADCTL` from `0x28` to `0x20` |
| Garbled picture | Shorten the SPI wires |
| No Bluetooth text | Swap TXD and RXD; set the terminal to 9600 baud |
| Bars never move | Check MAX4466 OUT → PA1 and its power; turn the gain trimmer clockwise |
| Bars always at the top, CLIP shown | Turn the trimmer the other way, or send `-` |
| On-board LED blinks very fast | HardFault: recheck Floating Point = Single Precision |

## Project status

The firmware compiles cleanly with `arm-none-eabi-gcc` and the FFT and drawing code were
checked against a PC test harness. Add your own measured results here, for example:

| Test tone | Displayed peak |
|---|---|
| 440 Hz | `<your reading>` |
| 1000 Hz | `<your reading>` |

## Possible improvements

- More bars or a longer FFT for finer resolution.
- A phone app that draws the spectrum from the Bluetooth stream.
- A noise-gate mode so the bars react only to real sound.
- DMA for the ADC and the SPI transfers, to free the CPU further.

## License

Add a license of your choice (for example MIT) as `LICENSE` in the repository root.
