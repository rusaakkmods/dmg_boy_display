# Routing Guide — Pico2 ⇄ ILI9488 (16‑bit RGB) — DMG 2× v1

This is the detailed, do‑this‑then‑that routing plan for the **Pico 2 → ILI9488** 16‑bit RGB prototype (DMG 2× @ ~60 Hz), with **GPIO0..GPIO4 reserved for capture** and the RGB bus starting at **GPIO5**. It assumes the **pin map in your latest build**:

- **DMG capture:** `GPIO0..GPIO4` → `CLK, D0, D1, VS, HS`
- **RGB16 (RGB565):** `GPIO5..GPIO20` → `DB15..DB0`
- **PCLK:** `GPIO28 → DCLK` (33 Ω at MCU)
- **DE:** `GPIO27 → DE` (33 Ω at MCU)
- **RESET:** `GPIO21 → RESET`
- **BL_PWM:** `GPIO22 → PT4115 DIM` (tie high via 10 k if full‑on)
- **MUX SEL:** `GPIO26 → 74HC4053 SEL`
- **SPI (init‑only) over RGB LSBs via 74HC4053:**
  - `GPIO20/19/18` ↔ `DB0/DB1/DB2` (U3 COM A/B/C)
  - U3 **X** → `SDA/SCK/CS` (panel SPI), **Y** → `DB0/DB1/DB2` (RGB path)

---

## 1) Board stack & design rules (KiCad‑style)
- **Layers:** 2‑layer is OK; 4‑layer preferred if you can (L1=signals, L2=GND, L3=3V3/5V + short tracks, L4=signals). If 2‑layer, dedicate as much uninterrupted **GND plane** as possible on the back.
- **Clearances:** 6 mil/6 mil (0.152/0.152 mm) min is fine; 8/8 mil if your fab is cheap.
- **Trace widths:**
  - **Power (5 V, LED loop):** 30–50 mil short runs; 12–20 mil to PT4115 sense path.
  - **3V3 VDD to panel:** 20 mil main, 10 mil branches.
  - **RGB/PCLK/DE:** 6–8 mil.
  - **DMG capture:** 6–8 mil, keep together.
- **Vias:** 0.3/0.6 mm (12/24 mil) typical. Drop **ground stitching vias** every ~15–20 mm around high‑edge nets (PCLK/DE) and around the LED buck island.

---

## 2) Placement order (don’t skip this)
1) **J1 (40‑pin FPC)** on a board edge; give 2–3 mm keep‑out for the cable bend.
2) **Pico 2 module** so that **GPIO5..GPIO22** can run straight toward J1 without dog‑legs.
3) **U3 (74HC4053)** close to **J1**, between DB0..DB2 and the SPI pads; rotate so COM pins face the Pico side.
4) **U1 (3V3 LDO)** near **J1 VDD** pins; place **C1/C2** right at VIN/VOUT.
5) **U2 (PT4115)**, **L1**, **D1**, **R1** clustered as a tight buck island; route LEDA/LEDK to J1 with very short wires. Keep this island **away from RGB/PCLK** by at least 5–8 mm and not under the FPC.
6) Leave space for **D2 (PESD5V0S1BA)** right at **J1 VDD**; optional second TVS footprint at **LEDA**.
7) Sprinkle **C3–C8 (0.1 µF)** decouplers: one pair near J1 VDD pins, and one near each color byte group (DB[5:0] blue, DB[11:6] green, DB[17:12] red).

---

## 3) Net classes & assignments
- **RGB_BUS:** `DB15..DB0` (GPIO5..GPIO20). Priority‑route as a bundle.
- **TIMING:** `PCLK (GPIO28)`, `DE (GPIO27)` — shortest, cleanest, 33 Ω series **at MCU pins**.
- **SPI_INIT:** `SDA/SCK/CS` ↔ 74HC4053 X‑side; `DB0..DB2` ↔ Y‑side; `COM` ↔ GPIO20/19/18.
- **POWER_3V3:** U1 → J1 VDD; star off into local decouplers.
- **LED_BUCK:** PT4115 island nets (`VIN`, `SW`, `CS`, `DIM`) segregated with a small keep‑out from logic.
- **CAPTURE:** `GPIO0..GPIO4` → DMG signals routed together, modest length, avoid PCLK crossovers.

---

## 4) Routing order (top‑down)
1) **PCLK first**: from **GPIO28** → **R2 = 33 Ω** (placed within 2–3 mm of the Pico pin) → direct to **J1 DCLK**. Shortest path; avoid vias; keep ≥0.5 mm away from LED buck copper.
2) **DE next**: from **GPIO27** → **R3 = 33 Ω** at the pin → **J1 DE**. Keep parallel to PCLK but with a little spacing (0.5–1 mm).
3) **RGB bundle**: route **GPIO5..GPIO20 → DB15..DB0** as a **tight, parallel bus** on the same layer as PCLK/DE. Try to keep **skew < 5 mm** across all 16 lines. Small dog‑legs are fine; avoid via‑hopping.
4) **SPI‑init mux**: drop **U3** near J1 so **DB0..DB2 ↔ U3 Y‑side** runs are 5–10 mm; **U3 X‑side ↔ SDA/SCK/CS** runs also 5–10 mm. **GPIO20/19/18 ↔ U3 COM** can be longer; keep them neat.
5) **RESET & BL**: route **GPIO21 → RESET** and **GPIO22 → DIM**. Keep them away from the LED **SW** node; add a ground guard trace if you can.
6) **3V3 power**: **U1 VOUT → J1 VDD** with a short, fat trace; plant **C2** right at U1, and **C3/C4** right by J1 VDD pins.
7) **LED buck island**: place **U2–L1–D1–R1** tight; the **SW→L1→LEDA** path is a **small loop**. Keep a **solid ground** under U2 and the sense resistor leg (**CS→R1→GND**).
8) **TVS**: place **PESD5V0S1BA** with a **stub < 2 mm** off **J1 VDD**, and a **wide, direct via** into the ground plane next to its cathode pad. Optional duplicate at **LEDA**.
9) **Capture nets**: route **GPIO0..GPIO4** together, modest length; avoid coupling to PCLK. If they must cross, cross **orthogonally** and add a GND via nearby.

---

## 5) Special notes per block
### 5.1 Pixel clock & timing
- **Series resistors** R2/R3 go **at the MCU pins**; if you move them to the far end you won’t tame the driver edge.
- Keep **PCLK** 2–3 mm shorter than the average RGB line if possible; the panel samples on DOTCLK and has generous margins at ~8 MHz, but shorter clock is cheap insurance.
- DE‑only timing: don’t route HS/VS now; leave test pads for future if you want to try HV+DE.

### 5.2 RGB data bus
- Group by color if it helps your head (DB12..DB17=Red, DB6..DB11=Green, DB0..DB5=Blue). The glass maps 6:6:6; you are feeding 5:6:5 (DB16/17 unused).
- **Skew target** across the 16 lines: **≤5 mm** (at 8 MHz your bit window is ~125 ns; this is very generous, but uniform lengths help SI and future overclocks).
- If you see ringing or overshoot on scope, add **22 Ω** to the 2–3 MSB lines in each color group closest to the MCU.

### 5.3 74HC4053 SPI/RGB mux
- Power U3 from **3.3 V**; decouple with **100 nF** at VCC.
- Tie **INH** low (enable). **SEL** from **GPIO26**; add a 100 k pull‑down so default is **SPI path** at power‑up.
- Keep **X‑side (SPI)** and **Y‑side (RGB)** traces short (≤10 mm). COM runs to the Pico can be longer.
- If you see any ghosting during init, add **1 k series** on **SCK** only at the U3 X‑side input.

### 5.4 PT4115 backlight
- Keep the **SW→L1→LEDA** and **D1→GND** loops small; don’t run logic traces under them.
- **R1**: use **two resistors** in parallel (1R8 ∥ 1R5) right at the **CS** pin return to **GND**. The sense node should be short and quiet; avoid sharing that ground with PCLK/DE returns.
- **DIM**: from **GPIO22**, route away from SW; add **Rb = 100 kΩ** to **GND** so BL is off until firmware enables it (remove Rb if tying DIM high for always‑on).

### 5.5 Power & ESD
- **U1 LDO**: keep **C1 (VIN)** and **C2 (VOUT)** tight to the pins. Run **VOUT** straight to J1 VDD, then branch.
- **TVS (PESD5V0S1BA)**: don’t put it on PCLK/DE/data; if you must protect those in a harsh env, use **low‑cap (<5 pF)** parts and keep their stubs microscopic.

---

## 6) Keep‑outs & ground strategy
- Reserve a **5–8 mm keep‑out** around the **LED buck** island; no high‑impedance or high‑edge nets should pass through.
- Fill the back with **solid GND**; stitch with vias near:
  - PCLK and DE every ~15–20 mm
  - The FPC connector pad rows
  - The boundaries of the buck island
- For 2‑layer builds, run short GND jumpers (vias) beside any signal layer changes.

---

## 7) Test/Debug pads (recommended)
- **TP_PCLK** (near J1 DCLK), **TP_DE**, one **TP** on a mid‑bus RGB line (e.g., DB8), **TP_BL** (GPIO22), **TP_RESET** (RESET net), **TP_MUX_SEL**.
- **TP_VDD** and **TP_GND** near J1 for easy scope reference.
- Optional: pads on **SDA/SCK/CS** at the FPC side to sniff init.

---

## 8) DFM & assembly notes
- Put reference designators **outside** the FPC footprint shadow.
- Add **mechanical keep‑out** around J1 for the cable latch.
- Panel mounting holes/standoffs aligned to relieve FPC strain.
- Silk arrows on DB15 and DB0 ends of the bus to avoid mirroring mistakes.
- Consider adding **0 Ω rework pads** to reassign a couple of RGB lines if hand‑assembly flips a pair.

---

## 9) Pre‑fabrication checklist
- [ ] PCLK series **33 Ω** placed within 2–3 mm of **GPIO28**.
- [ ] DE series **33 Ω** placed within 2–3 mm of **GPIO27**.
- [ ] RGB bus routed **GPIO5..GPIO20 → DB15..DB0**, skew ≤5 mm, no stubs.
- [ ] U3 placed near J1; **DB0..DB2** and **SDA/SCK/CS** runs ≤10 mm; COM to Pico neat.
- [ ] **TVS PESD5V0S1BA** at **J1 VDD** with short stub + ground via.
- [ ] PT4115 island tight; **SW loop** small; **CS sense** quiet; **R1 (1R8 ∥ 1R5)** fitted.
- [ ] No logic over the buck island; ground plane intact under RGB/PCLK.
- [ ] Decouplers: C3–C8 placed (one near each color byte group + VDD cluster).
- [ ] Test pads present and reachable.

---

## 10) If/when you jump to RP2350B (production)
- Drop the **74HC4053**; dedicate **SDA/SCK/CS** to the panel and add **HS/VS** to run **HV+DE**.
- Keep the **PCLK shortest**, same series placement, and keep the RGB bus on one side of the MCU to simplify length control.

**End v1.** Ping me if you want a KiCad **.kicad_pcb** net‑class preset or a little placement sketch on a 2‑layer stack.

