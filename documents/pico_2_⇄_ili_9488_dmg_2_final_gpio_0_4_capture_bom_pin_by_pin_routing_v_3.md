# Pico2 ⇄ ILI9488 — DMG 2× (Final, GPIO0–4 Capture)

**Goal:** Stream Game Boy DMG (160×144) → 2× (320×288) at ~60 Hz on a 2.73″ ILI9488. Use the panel’s **RGB (DPI)** for pixels and **3‑wire SPI** only for DCS init. One **Pico 2** handles **capture + 2× scale + RGB scan‑out**.

**Locked decisions**
- **Bus:** 16‑bit **RGB565** (DB15..DB0). DB16/DB17 unused.
- **Timing:** **DE‑only**, **PCLK ≈ 7–8 MHz**.
- **Capture reservation:** **GPIO0..GPIO4** (CLK, D0, D1, VS, HS).
- **SPI reuse:** **DB0..DB2** shared with SPI via **74HC4053** during init; then switched back to RGB for run.
- **Backlight:** PT4115 constant‑current ~**120 mA**, **always on** (DIM tied high).
- **MUX_SEL:** now on **GPIO22** (boot default = SPI path via 100 k pulldown). **GPIO26** freed for **ADC0 potentiometer**.

---

## Complete BOM (prototype)

| Ref | Part | Value / PN | Qty | Notes |
|---|---|---|---:|---|
| **J0** | MCU board | Raspberry Pi **Pico 2** | 1 | RP235x dev board |
| **J1** | FPC connector | 40‑pin, 0.5 mm | 1 | Matches XBY028MBO857‑0601AA0 panel |
| **U1** | LDO | 3.3 V (≥300 mA) e.g. MIC5219‑3.3 / AP2112‑3.3 | 1 | Panel VDD (VCI) |
| **C1** | LDO VIN cap | 10 µF ≥6.3 V X5R | 1 | At U1 VIN |
| **C2** | LDO VOUT cap | 10 µF ≥6.3 V X5R | 1 | At U1 VOUT |
| **C3, C4** | VDD decouplers | 0.1 µF 25 V X7R | 2 | Near J1 VCI pins |
| **C5–C8** | RGB group decouplers | 0.1 µF 25 V X7R | 4 | Near color byte clusters |
| **U2** | LED buck | **PT4115** | 1 | BL constant‑current driver |
| **C10** | PT4115 VIN cap | 10 µF ≥10 V | 1 | At U2 VIN |
| **C11** | PT4115 VIN small | 0.1 µF | 1 | At U2 VIN |
| **L1** | Inductor | **47 µH**, ≥0.3 A | 1 | Per PT4115 ref |
| **D1** | Schottky | **SS14** | 1 | PT4115 flyback |
| **R1a, R1b** | Sense resistors | **1R8 ∥ 1R5**, 1% | 2 | ≈ **0.82 Ω** → **~120 mA** (I≈0.1 V/Req) |
| **C9** | BL output cap | 4.7–10 µF 10 V | 1 | Optional LED ripple smoothing |
| **Rdim** | DIM pull‑up | **10 kΩ** | 1 | **PT4115 DIM → 3V3** (full‑on) |
| **U3** | Analog switch | **74HC4053** (TSSOP‑16) | 1 | SPI↔RGB mux for DB0..DB2 |
| **C_U3** | U3 decoupler | 0.1 µF | 1 | At U3 VCC |
| **R2** | Series | **33 Ω** | 1 | On **PCLK** (at MCU) |
| **R3** | Series | **33 Ω** | 1 | On **DE** (at MCU) |
| **R_MSBx** | Series (DNP pads) | **22 Ω** | 6 | On DB15, DB14, DB10, DB9, DB4, DB3 (optional SI) |
| **R_SCK** | Series (DNP pad) | **1 kΩ** | 1 | On **SCK** at U3 X‑side (if init noise) |
| **Rsel** | Pull‑down | **100 kΩ** | 1 | **GPIO22 (MUX_SEL)** → GND (boot to SPI path) |
| **D2** | TVS ESD | **PESD5V0S1BA** (SOD‑323) | 1 | **J1‑5 (VCI) → GND**; optional 2nd at LEDA |
| **P1** | Potentiometer | **B10K** linear | 1 | User control on ADC0 |
| **R_POT** | Series to ADC | **100 Ω** | 1 | Wiper → GPIO26 (ADC0) |
| **C_POT** | ADC filter | **100 nF** | 1 | GPIO26 → AGND |
| **J_POT** | 3‑pin header | — | 1 | 3V3 / ADC0 / AGND (optional) |
| **HW** | Standoffs | M2/M2.5 | 4 | Mounting / FPC relief |

> If 0R82 is hard to source, **1R8 ∥ 1R5** gives ~0.82 Ω; **1R8 ∥ 1R8 ≈ 0.90 Ω (~111 mA)**; **1R8 ∥ 2R2 ≈ 0.99 Ω (~101 mA)**.

---

## Pico 2 ⇄ Panel (J1 40‑pin) — Final Pin Map
Panel: XBY028MBO857‑0601AA0. J1 numbering **1→40**.

### Power & Backlight
- **J1‑1 (LEDA)**  ← **PT4115 SW → L1 → LEDA**
- **J1‑2 (LEDK1)** → **PT4115 CS**
- **J1‑3 (LEDK2)** → **PT4115 CS** *(tie to LEDK1 at CS)*
- **J1‑4 (GND)**   → **GND plane**
- **J1‑5 (VCI/VDD)** ← **U1 3.3 V** *(D2 TVS here, C3/C4 nearby)*

### Control & Timing
- **J1‑6 (RESET L‑act)** ← **GPIO21** *(+10 kΩ pull‑up to 3V3)*
- **J1‑7 (NC)** → NC
- **J1‑8 (NC)** → NC
- **J1‑9 (SDA)** ↔ **U3 1Y0** *(SPI path)*
- **J1‑10 (SCK)** ↔ **U3 2Y0** *(SPI path; R_SCK pad optional)*
- **J1‑11 (CS)** ↔ **U3 3Y0** *(SPI path)*
- **J1‑12 (PCLK)** ← **GPIO28** via **R2 33 Ω**
- **J1‑13 (DE)**   ← **GPIO27** via **R3 33 Ω**
- **J1‑14 (VSYNC)** → NC (DE‑only)
- **J1‑15 (HSYNC)** → NC (DE‑only)

### RGB Data (RGB565)
- **J1‑16 (DB0)**  ↔ **U3 1Y1 ⇄ 1Z=GPIO20**
- **J1‑17 (DB1)**  ↔ **U3 2Y1 ⇄ 2Z=GPIO19**
- **J1‑18 (DB2)**  ↔ **U3 3Y1 ⇄ 3Z=GPIO18**
- **J1‑19 (DB3)**  ← **GPIO17** *(R_MSB pad on DB3)*
- **J1‑20 (DB4)**  ← **GPIO16** *(R_MSB pad on DB4)*
- **J1‑21 (DB5)**  ← **GPIO15**
- **J1‑22 (DB6)**  ← **GPIO14**
- **J1‑23 (DB7)**  ← **GPIO13**
- **J1‑24 (DB8)**  ← **GPIO12**
- **J1‑25 (DB9)**  ← **GPIO11** *(R_MSB pad on DB9)*
- **J1‑26 (DB10)** ← **GPIO10** *(R_MSB pad on DB10)*
- **J1‑27 (DB11)** ← **GPIO9**
- **J1‑28 (DB12)** ← **GPIO8**
- **J1‑29 (DB13)** ← **GPIO7**
- **J1‑30 (DB14)** ← **GPIO6** *(R_MSB pad on DB14)*
- **J1‑31 (DB15)** ← **GPIO5** *(R_MSB pad on DB15)*
- **J1‑32 (DB16)** → NC
- **J1‑33 (DB17)** → NC
- **J1‑34 (GND)**  → **GND plane**
- **J1‑35..39 (NC)** → NC
- **J1‑40 (GND)** → **GND plane**

### 74HC4053 (U3) — Pin‑by‑Pin
- **VCC (16)** → **3V3**; **GND (8)** → GND; **VEE (7)** → GND; **\~E (6)** → GND
- **S1 (11), S2 (10), S3 (9)** ← **GPIO22 (MUX_SEL)** + **Rsel=100 kΩ → GND**
- **Ch‑1 (DB0/SDA):** **1Z (14) ↔ GPIO20**, **1Y1 (13) ↔ J1‑16 (DB0)**, **1Y0 (12) ↔ J1‑9 (SDA)**
- **Ch‑2 (DB1/SCK):** **2Z (15) ↔ GPIO19**, **1 (2Y1) ↔ J1‑17 (DB1)**, **2 (2Y0) ↔ J1‑10 (SCK)** *(R_SCK pad here)*
- **Ch‑3 (DB2/CS):** **3Z (4) ↔ GPIO18**, **3Y1 (3) ↔ J1‑18 (DB2)**, **3Y0 (5) ↔ J1‑11 (CS)**

### Potentiometer on ADC0
- **GPIO26 (ADC0)** ← **R_POT (100 Ω)** ← **P1 wiper**
- **GPIO26 → C_POT (100 nF) → AGND** (local)
- **P1 CW** → **3V3**; **P1 CCW** → **AGND** (tie AGND to GND at MCU)
- *(Optional)* **J_POT** header: 3V3 / ADC0 / AGND

### DMG Capture (reserved)
- **GPIO0..GPIO4** → **CLK, D0, D1, VS, HS** (your capture PIO order)

---

## Routing Notes (concise)
1) **PCLK/DE first:** series **33 Ω** resistors **at MCU pins**; shortest path to J1; keep ≥0.5 mm from PT4115 island.
2) **RGB bus:** route **GPIO5..GPIO20 → DB15..DB0** as a tight, parallel bundle; target skew ≤5 mm; add the **22 Ω pads** on DB15,14,10,9,4,3 near MCU (DNP unless needed).
3) **U3 placement:** near J1 so **DB0..DB2** and **SDA/SCK/CS** stubs ≤10 mm; COM runs to Pico can be longer.
4) **PT4115 island:** keep SW→L1→LEDA loop tight; CS sense node short to **R1** and **GND**; add **C10 (10 µF)** + **C11 (0.1 µF)** at VIN.
5) **TVS:** **PESD5V0S1BA** from **J1‑5 (VCI)** to GND using <2 mm stub + wide GND via; optional second at **J1‑1 (LEDA)**.
6) **Ground:** solid plane; stitch vias near FPC, PCLK/DE every ~15–20 mm, around buck island.
7) **Analog:** keep pot traces away from PCLK/DE and PT4115 SW; put **C_POT** at the MCU pin.

---

## Bring‑Up (display)
1) **SEL default:** thanks to **Rsel 100 kΩ**, **GPIO22=0 → SPI path** at boot.
2) **Reset:** drive **RESET low 10 ms → high**.
3) **DCS init:** `SLPOUT (0x11)` → 120 ms → `COLMOD (0x3A, 0x55)` → `MADCTL` → `DISPON (0x29)`.
4) **Run:** set **GPIO22=1** (U3 to RGB), start PIO stream at **~7–8 MHz** PCLK.

---

## RP2350B (production heads‑up)
- Drop U3; dedicate **SDA/SCK/CS**; add **HS/VS** (HV+DE). Keep 33 Ω on PCLK/DE; same TVS; optional TE.

**End v3 — ready for schematic + layout.**

