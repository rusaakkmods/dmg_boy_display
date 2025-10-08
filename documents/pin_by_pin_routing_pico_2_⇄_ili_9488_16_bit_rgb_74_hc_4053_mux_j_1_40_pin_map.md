# Pin‑by‑Pin Routing — Pico2 ⇄ ILI9488 (16‑bit RGB) + 74HC4053 mux

**Context**  
- Panel: XBY028MBO857‑0601AA0 (2.73″ 320×320) — **3SPI + 18RGB** FPC.  
- **J1 = 40‑pin FPC** (0.5 mm). Pinout per your sheet: 1=LEDA … 40=GND.  
- MCU: **Pico 2** (RP235x). **DMG capture uses GPIO0..GPIO4.**  
- We run **RGB565 (16‑bit)**, **DE‑only**, **PCLK≈7–8 MHz**.  
- **DB0..DB2 are shared** with 3‑wire SPI during init via **U3=74HC4053**.  

---

## A) J1 (FPC) ⇄ Pico2: exact pin map

### Power & backlight
- **J1‑1 (LED A)**  → **U2 PT4115 SW → L1 → LEDA**  
- **J1‑2 (LED K1)** → **U2 PT4115 CS (LED−)**  
- **J1‑3 (LED K2)** → **U2 PT4115 CS (LED−)** *(tie K1 & K2 together at CS)*  
- **J1‑4 (GND)** → **GND plane**  
- **J1‑5 (VCI / VDD)** ← **U1 3.3 V LDO** *(C2 10 µF at U1; C3/C4 0.1 µF near J1)*  

### Control & timing
- **J1‑6 (RESET, L‑active)** ← **GPIO21** *(add 10 kΩ pull‑up to 3V3)*  
- **J1‑7 (NC)** → **NC**  
- **J1‑8 (NC)** → **NC**  
- **J1‑9 (SDA)** ↔ **U3 1Y0** *(SPI side, see section C)*  
- **J1‑10 (SCK)** ↔ **U3 2Y0** *(SPI side)*  
- **J1‑11 (CS)** ↔ **U3 3Y0** *(SPI side)*  
- **J1‑12 (PCLK / DOTCLK)** ← **GPIO28** via **R2 = 33 Ω** *(resistor at MCU pin)*  
- **J1‑13 (DE)** ← **GPIO27** via **R3 = 33 Ω** *(at MCU pin)*  
- **J1‑14 (VSYNC)** → **NC** *(DE‑only)*  
- **J1‑15 (HSYNC)** → **NC** *(DE‑only)*  

### RGB data (16‑bit RGB565 on DB0..DB15; DB16/DB17 unused)
- **J1‑16 (DB0)**  ↔ **U3 1Y1** ⇄ **GPIO20** *(DB0 at run‑time; SPI during init)*  
- **J1‑17 (DB1)**  ↔ **U3 2Y1** ⇄ **GPIO19**  
- **J1‑18 (DB2)**  ↔ **U3 3Y1** ⇄ **GPIO18**  
- **J1‑19 (DB3)**  ← **GPIO17**  
- **J1‑20 (DB4)**  ← **GPIO16**  
- **J1‑21 (DB5)**  ← **GPIO15**  
- **J1‑22 (DB6)**  ← **GPIO14**  
- **J1‑23 (DB7)**  ← **GPIO13**  
- **J1‑24 (DB8)**  ← **GPIO12**  
- **J1‑25 (DB9)**  ← **GPIO11**  
- **J1‑26 (DB10)** ← **GPIO10**  
- **J1‑27 (DB11)** ← **GPIO9**   
- **J1‑28 (DB12)** ← **GPIO8**   
- **J1‑29 (DB13)** ← **GPIO7**   
- **J1‑30 (DB14)** ← **GPIO6**   
- **J1‑31 (DB15)** ← **GPIO5**   
- **J1‑32 (DB16)** → **NC** *(unused in RGB565)*  
- **J1‑33 (DB17)** → **NC** *(unused in RGB565)*  
- **J1‑34 (GND)** → **GND plane**  
- **J1‑35..39 (NC)** → **NC**  
- **J1‑40 (GND)** → **GND plane**  

**DMG capture header** (for reference): **GPIO0..GPIO4 → CLK, D0, D1, VS, HS** (exact order per your PIO).

---

## B) 74HC4053 (U3) — pin‑by‑pin wiring (TSSOP‑16)

**Goal:** During init, route **SPI (SDA/SCK/CS)** to the panel. During run, reconnect **GPIO20/19/18** as **DB0/DB1/DB2**. One **SEL** line flips all three.

**Power & control**
- **Pin 16 (VCC)** → 3.3 V, **0.1 µF** to GND at the pin.  
- **Pin 7 (VEE)** → GND *(single‑supply)*.  
- **Pin 8 (GND)** → GND plane.  
- **Pin 6 (\~E)** → GND *(enable always on).*  
- **Pins 11 (S1), 10 (S2), 9 (S3)** ← **GPIO26 (MUX_SEL)** *(tie all three together).*  
  - **Logic:** **SEL=0 → SPI path active (Y0)**; **SEL=1 → RGB path active (Y1)**.

**Channel 1 (DB0 ↔ SDA)**
- **Pin 14 (1Z)** ↔ **GPIO20** *(DB0 at run‑time)*  
- **Pin 13 (1Y1)** ↔ **J1‑16 (DB0)**  
- **Pin 12 (1Y0)** ↔ **J1‑9 (SDA)**

**Channel 2 (DB1 ↔ SCK)**
- **Pin 15 (2Z)** ↔ **GPIO19** *(DB1 at run‑time)*  
- **Pin 1  (2Y1)** ↔ **J1‑17 (DB1)**  
- **Pin 2  (2Y0)** ↔ **J1‑10 (SCK)**

**Channel 3 (DB2 ↔ CS)**
- **Pin 4  (3Z)** ↔ **GPIO18** *(DB2 at run‑time)*  
- **Pin 3  (3Y1)** ↔ **J1‑18 (DB2)**  
- **Pin 5  (3Y0)** ↔ **J1‑11 (CS)**

*Tip:* If SCK edges look spicy during init, add **1 kΩ** in series at **pin 2 (2Y0)**.

---

## C) Pin‑by‑pin routing list (line‑by‑line)

### 1. Power U1 (3.3 V LDO)
- **U1 VIN → 5V_IN**  
- **U1 VOUT → J1‑5 (VCI/VDD)**  
- **U1 VOUT → C2 (10 µF) → GND** *(place tight)*  
- **U1 VIN → C1 (10 µF) → GND** *(place tight)*

### 2. Backlight U2 (PT4115)
- **U2 VIN ← 5V_IN**  
- **U2 SW → L1 (47 µH) → J1‑1 (LEDA)**  
- **D1 SS14: cathode → U2 SW, anode → GND**  
- **J1‑2 (LEDK1), J1‑3 (LEDK2) → U2 CS → R1 (1R8 ∥ 1R5) → GND** *(≈120 mA)*  
- **U2 DIM ← GPIO22 (BL_PWM)** *(or tie high via 10 kΩ for full‑on)*

### 3. Timing & control
- **GPIO28 → R2 (33 Ω) → J1‑12 (PCLK)**  
- **GPIO27 → R3 (33 Ω) → J1‑13 (DE)**  
- **GPIO21 → J1‑6 (RESET)** *(+10 kΩ pull‑up to 3V3)*

### 4. RGB16 data bus
- **GPIO5  → J1‑31 (DB15)**  
- **GPIO6  → J1‑30 (DB14)**  
- **GPIO7  → J1‑29 (DB13)**  
- **GPIO8  → J1‑28 (DB12)**  
- **GPIO9  → J1‑27 (DB11)**  
- **GPIO10 → J1‑26 (DB10)**  
- **GPIO11 → J1‑25 (DB9)**   
- **GPIO12 → J1‑24 (DB8)**   
- **GPIO13 → J1‑23 (DB7)**   
- **GPIO14 → J1‑22 (DB6)**   
- **GPIO15 → J1‑21 (DB5)**   
- **GPIO16 → J1‑20 (DB4)**   
- **GPIO17 → J1‑19 (DB3)**   
- **GPIO18 ↔ U3 3Z  → (Y1) J1‑18 (DB2)** *(muxed)*  
- **GPIO19 ↔ U3 2Z  → (Y1) J1‑17 (DB1)** *(muxed)*  
- **GPIO20 ↔ U3 1Z  → (Y1) J1‑16 (DB0)** *(muxed)*  
- **J1‑32 (DB16)** → **NC**  
- **J1‑33 (DB17)** → **NC**

### 5. SPI (init only) path (through U3)
- **J1‑9  (SDA) ↔ U3 1Y0 ←→ 1Z (GPIO20)**  
- **J1‑10 (SCK) ↔ U3 2Y0 ←→ 2Z (GPIO19)**  
- **J1‑11 (CS)  ↔ U3 3Y0 ←→ 3Z (GPIO18)**  
- **GPIO26 → U3 S1/S2/S3 (SEL)**; **U3 \~E → GND**; **U3 VCC=3V3; VEE=GND; GND=GND**

### 6. Grounds & ESD
- **J1‑4, J1‑34, J1‑40 → GND plane**  
- **D2 (PESD5V0S1BA) anode → GND; cathode → J1‑5 (VDD) via <2 mm stub**  
- *(Optional)* **Second D2** at **J1‑1 (LEDA)** to GND

### 7. DMG capture (for completeness)
- **GPIO0..GPIO4 → DMG CLK, D0, D1, VS, HS** *(your existing header; route together)*

---

## D) Notes & bring‑up
- **MUX default:** Add 100 kΩ pull‑down on **GPIO26** so **SEL=0 → SPI path** at power‑up.  
- **Init:** `SLPOUT (0x11)` → 120 ms → `COLMOD=0x55` (RGB565) → `MADCTL` → `DISPON (0x29)` → **SEL=1** then start PIO stream.  
- **SI:** Keep **R2/R3** within 2–3 mm of MCU pins; keep **U3** close to J1 so SPI/RGB stubs are ≤10 mm.  
- **Skew:** Try to keep **RGB bus skew ≤5 mm**; PCLK shortest.

*End.*

