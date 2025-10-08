# Pico2 → ILI9488 RGB (320×320) — DMG 2× Prototype (GPIO0–4 Capture)

**Goal:** Stream Game Boy DMG (160×144) → 2× (320×288) at ~60 Hz on a 2.73″ ILI9488 using the panel’s **18‑bit RGB (DPI)** for pixels and **3‑wire SPI** only for DCS init. One **Pico 2** handles **capture + scaling + RGB scan‑out**. This version reserves **GPIO0..GPIO4 for DMG capture** and starts the RGB bus at **GPIO5**.

---

## Summary
- Panel FPC exposes **3SPI + 18RGB + DCLK/DE/HS/VS + RESET + LEDA/LEDK + VDD/GND**.
- Run **RGB565 (16‑bit DPI)** with **DE‑only** timing at **PCLK ≈ 7–8 MHz**; active 320×288 centered in 320×320.
- To stay within Pico 2 header pins, reuse **DB0..DB2** as **3‑wire SPI** during init via **74HC4053**; switch back to RGB for run.
- Backlight: **PT4115** constant‑current ≈ **120 mA** into LEDA/LEDK; **PWM on DIM** (from Pico2) optional.

---

## Pin Budget (fits Pico 2)
- RGB565 data (16): **DB15..DB0**
- Timing (2): **DE, PCLK**
- SPI init (3): reuse **DB0..DB2** via 74HC4053 (needs **1 GPIO** for SEL)
- RESET (1): MCU pin
- BL PWM (1): MCU pin (optional; tie high for always‑on)
- DMG capture (5): **GPIO0..GPIO4**
- **Total = 16 + 2 + 1 + 1 + 5 + 1 = 26** ✅

---

## Complete BOM (prototype)

| Ref | Part | Value / PN | Qty | Notes |
|---|---|---|---:|---|
| **J0** | MCU board | Raspberry Pi **Pico 2** | 1 | RP235x dev board |
| **J1** | FPC connector | 40‑pin, 0.5 mm pitch | 1 | Mates with panel (top contact) |
| **U1** | LDO | 3.3 V (≥300 mA), MIC5219‑3.3 / AP2112‑3.3 | 1 | Panel VDD |
| **C1** | Bulk in | 10 µF ≥6.3 V X5R | 1 | Near U1 VIN |
| **C2** | Bulk out | 10 µF ≥6.3 V X5R | 1 | Near U1 VOUT |
| **C3–C8** | Decoupling | 0.1 µF 25 V X7R | 6 | Near panel VDD & RGB groups |
| **U2** | LED buck | **PT4115** | 1 | Backlight CC |
| **L1** | Inductor | 47 µH ≥0.3 A | 1 | Per PT4115 ref |
| **D1** | Schottky | SS14 | 1 | PT4115 flyback |
| **R1** | Sense | **1R8 ∥ 1R5 (parallel)**, 1% | 2 | ≈ **0.82 Ω** → **~120 mA** (I≈0.1 V/Req) |
| **C9** | BL cap | 4.7–10 µF 10 V | 1 | Optional ripple smoothing |
| **Rb** | Pull‑down | 100 kΩ | 1 | Pulls DIM low at reset (omit if tying DIM high) |
| **U3** | Analog switch | **74HC4053** (TSSOP‑16) | 1 | Reuse DB0..DB2 as SPI during init |
| **R2** | Series | **33 Ω** | 1 | On **PCLK** |
| **R3** | Series | **33 Ω** | 1 | On **DE** |
| **R4–R9** | Series (opt) | **22 Ω** | 6 | On RGB MSBs if ringing |
| **R10–R12** | Jumpers | **0 Ω** | 3 | SPI/RGB LSB debug straps |
| **R13** | Jumper | **0 Ω** | 1 | BL path reroute (optional) |
| **D2** | TVS ESD (opt) | **PESD5V0S1BA** (SOD‑323) | 1 | At **FPC VDD** (opt. also LEDA); avoid fast nets |
| **HW** | Standoffs | M2/M2.5 | 4 | Mounting / FPC relief |

**Sourcing note:** If 0R82 is scarce, **1R8 ∥ 1R5 ≈ 0.82 Ω** (best), or **1R8 ∥ 1R8 ≈ 0.90 Ω (~111 mA)**, **1R8 ∥ 2R2 ≈ 0.99 Ω (~101 mA)**.

---

## Pico 2 ↔ Panel FPC: Pin Map (FINAL)

**DMG capture (reserved):**
- **GPIO0..GPIO4** → DMG **CLK, D0, D1, VS, HS** (your order)

**RGB scan‑out (contiguous):**
- **DB15..DB0** ← **GPIO5..GPIO20**  
  (i.e. **GPIO5→DB15**, **GPIO6→DB14**, …, **GPIO20→DB0**)
- **DCLK (PCLK)** ← **GPIO28** via **33 Ω**
- **DE** ← **GPIO27** via **33 Ω**
- **HS/VS**: NC (DE‑only timing)

**Panel control & misc:**
- **RESET** ← **GPIO21** (active‑low)
- **BL_PWM (DIM)** ← **GPIO22**
- **MUX_SEL** (74HC4053) ← **GPIO26** (0=SPI init, 1=RGB run)

**SPI (init‑only) via U3 74HC4053 (reusing RGB LSBs):**
- **COM A/B/C** ← **GPIO20 / GPIO19 / GPIO18** (DB0/DB1/DB2)
- **X A/B/C → SDA / SCK / CS** (panel SPI)
- **Y A/B/C → DB0 / DB1 / DB2** (RGB path)

**Power & backlight:**
- **VDD (panel)** ← U1 3.3 V (C2/C3 local)
- **LEDA/LEDK** ← U2 PT4115 loop (see routing)

**DPI color depth:** Prefer **16‑bit** via `COLMOD=0x55` (DB15..DB0 = RGB565). If you hard‑wire 18‑bit, duplicate MSBs and use `COLMOD=0x66`.

---

## Routing (step‑by‑step)

### 1) Power U1 (3.3 V LDO)
- **5V_IN → U1 VIN**
- **U1 VOUT (3.3 V) → J1 VDD**
- **U1 VOUT → C2 (10 µF) → GND**; **U1 VIN → C1 (10 µF) → GND** (tight)
- Short star‑ground near U1/J1

### 2) Backlight U2 (PT4115)
- **5V_IN → U2 VIN** (short)
- **U2 SW → L1 (47 µH) → J1 LEDA**
- **D1 (SS14): cathode → SW, anode → GND**
- **J1 LEDK → U2 CS**; **R1 (1R8 ∥ 1R5 ≈ 0.82 Ω) → GND** (≈120 mA)
- **DIM** ← **GPIO22** (PWM). For full‑on builds, tie DIM high via 10 k and free GPIO22
- Optional **C9 4.7–10 µF** LEDA→GND
- Keep buck loop compact; route away from PCLK/DE/data

### 3) RGB Bus (PIO scan‑out)
- Route **GPIO5..GPIO20 → DB15..DB0** as a tight bundle; keep lengths similar
- **PCLK (GPIO28)** shortest; **33 Ω** series at MCU
- **DE (GPIO27)** short; **33 Ω** series at MCU
- If ringing at edges, add **22 Ω** on color MSBs

### 4) SPI‑init Multiplex (U3 74HC4053)
- Place **U3** close to **J1**; short stubs to **SDA/SCK/CS** and **DB0..DB2**
- **GPIO20/19/18 → U3 COM A/B/C**
- **U3 X‑side → SDA/SCK/CS**; **U3 Y‑side → DB0/DB1/DB2**
- **GPIO26 → U3 SEL**; **INH** low (enable); decouple U3 with 100 nF

### 5) RESET & TVS
- **RESET (GPIO21)** → panel RESET (add 10 kΩ pull‑up to 3.3 V if the glass requires)
- **TVS (PESD5V0S1BA) @ FPC VDD:** right at J1 VDD with a **short stub** and **wide ground via** into plane
- **Optional TVS on LEDA:** second PESD5V0S1BA near LEDA
- Avoid TVS on **PCLK/DE/RGB**; if required, use **low‑cap <5 pF** parts and keep 33 Ω on PCLK/DE
- Test pads: **TP_VDD, TP_GND, TP_PCLK, TP_DE, TP_BL (GPIO22), TP_MUX_SEL**

### 6) DMG Capture
- Keep **GPIO0..GPIO4** short/clean to the capture header; avoid crossing PCLK

---

## Bring‑Up Checklist
1) **Power‑up:** 3.3 V stable → wait ≥200 ms → **SPI init** → `DISPON` → enable **backlight**
2) **SPI init (SEL=0 → SPI path):**
   - **RESET** low 10 ms → high
   - `SLPOUT (0x11)`; delay ~120 ms
   - `COLMOD (0x3A, 0x55)` for RGB565 (or `0x66` for RGB666)
   - `MADCTL (0x36, …)` orientation; porch/scan if needed
   - `DISPON (0x29)`
3) **Switch to RGB:** set **SEL=1**; start PIO scan‑out at **~7–8 MHz PCLK**
4) **Stream:** capture DMG lines → **2× X** expand into line buffer → **Y‑double** by repeating lines → center in 320×320

---

## Quick Nets (copy/paste)
- **Pico2 → Panel**
  - `GPIO5..GPIO20 → DB15..DB0`
  - `GPIO28 → DCLK` (33 Ω)
  - `GPIO27 → DE` (33 Ω)
  - `GPIO21 → RESET`
  - `GPIO22 → BL_PWM → U2 DIM`
  - `GPIO26 → U3 SEL`
- **U3 (74HC4053)**
  - `COM A/B/C ← GPIO20/19/18`
  - `X A/B/C → SDA/SCK/CS`
  - `Y A/B/C → DB0/DB1/DB2`
- **Power**
  - `5V_IN → U1 VIN`, `U1 VOUT(3V3) → J1 VDD`
  - `5V_IN → U2 VIN`, `U2 SW → L1 → J1 LEDA`, `J1 LEDK → U2 CS → (R1 1R8 ∥ 1R5) → GND`, `D1: cathode→SW, anode→GND`

---

## RP2350B (Production) — Cleanups & Headroom
- **Drop U3 (74HC4053)**; dedicate **3 SPI pins** to panel **SDA/SCK/CS**.
- **Enable HS/VS** to run **HV+DE** timing; optionally add **TE**.
- Keep **BL PWM** on a dedicated pin (or tie full‑on).
- Same **PESD5V0S1BA** at **VDD** (optionally LEDA); consider low‑cap TVS on **PCLK** only if environment requires it.
- Keep RGB bus on contiguous pins of the RP2350B for clean length control.

**End v2.** Ready for schematic/routing; shout if you want a KiCad netlist block generated from this map.