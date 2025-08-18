# CafeTrainControl (ATmega328P)

Control firmware for a café food-delivery train system.  
The project runs on ATmega328P and consists of a **Transmitter** (command device) and a **Receiver** (control device).  
Communication is performed over **UART**, so the Transmitter can theoretically control **any UART-compatible device**.  

> **Note:** This project is a successor to the [LocomotiveCafe system (ATmega2560)](https://github.com/mihan2007/CafeTrainAtmnega2560/releases/tag/LocomotiveCafe).  
> Compared to the previous implementation, the new firmware:  
> - Is designed for **8 tables** but can be easily extended to **any number of tables**  
> - Can be optimized for **custom delivery logic** (the previous project did not allow this flexibility)  
> - Retains the **same path logic** as in the original project  
> - Is simplified and ported to run on **ATmega328P** with modular Transmitter/Receiver roles  

> Latest release: see [Releases](../../releases) (e.g., **v1.1.0** – Safety & UI update).

---

## ✨ Features
- **8 programmable stop points (tables)** (extendable to unlimited tables)  
- Buttons: table selection, forward / backward, emergency stop  
- Smooth acceleration & braking via PWM  
- **Safety**: short-circuit protection, no-load (idle) protection  
- Live status on LCD (current mode / stop / alarms)  
- Robust UART command handling (interrupt-driven ring buffer)  
- Same PCB used for both Transmitter and Receiver (different set of components soldered)  

---

## 🔌 System architecture

The project is built around **UART communication** between two identical ATmega328P-based boards:

- **Transmitter (Master / Command device)**  
  - User-facing board with buttons for table selection, direction control (FWD/BWD), and emergency stop.  
  - Sends commands over UART to the Receiver.  
  - Uses the same PCB as the Receiver, but **some components are not soldered** (only those required for input and UART).  

- **Receiver (Slave / Control device)**  
  - Board responsible for motor driving, LED indicators, stop sensors, protections, and LCD display.  
  - Receives UART commands from the Transmitter and executes them in real time.  

Because communication is based on the **standard UART protocol**, the Transmitter can theoretically control **any UART-compatible device**, not only this custom Receiver firmware.

---

## 📂 Repository layout

    /Transmitter   # firmware for the Transmitter (command device)
    /Receiver      # firmware for the Receiver (control device)
    /docs          # circuit schematic and PCB layout
    /tools         # flashing programs

---


---

## 📐 Hardware design
The **circuit schematic and PCB layout** are located in the `/docs` folder.  
Both Transmitter and Receiver use the **same PCB design**:  
- On the **Transmitter**, some components are intentionally not soldered (only UART + buttons are used).  
- On the **Receiver**, all components are populated (motor driver, sensors, LCD, protections).  

**Main hardware features:**  
- MCU: **ATmega328P**  
- Motor driver (H-bridge) with current sense  
- **8 stop sensors (tables), 8 LEDs (table indicators)**  
- LCD 16x2 (HD44780-compatible)  
- Emergency stop button, FWD/BWD buttons  
- Power stage with short-circuit and idle protection  

---

## 🚀 Build & Flash

### Prereqs
- `avr-gcc`, `avr-libc`, `make`, `avrdude`  
- Board/Target: ATmega328P @ 16 MHz (adjust `F_CPU` if different)  

📦 In the `/tools` folder you can find:  
- **avrdude** (command-line flasher)  
- **avrdududeprog** (graphical interface for avrdude)  
- **Atmega328pb_flasher.exe** – small standalone utility for flashing ATmega328PB  

⚠️ Note:  
- For **ATmega328P**, you can use either `avrdude` or `avrdududeprog`.  
- For **ATmega328PB**, `avrdududeprog` will **not work**. Use either `avrdude` directly or the provided **Atmega328pb_flasher.exe** utility from `/tools`.  

### Build (CLI)
```bash
# From project root
make -C Transmitter
make -C Receiver
```
## 🧪 Runtime/UX

- Select a table → train moves, decelerates smoothly, stops at the corresponding sensor  
- LCD shows state: `IDLE / MOVING / BRAKING / E-STOP` with selected table  

### Protections
- **Short-circuit**:  
  Implemented using current sensors (e.g., **CJMCU-758**).  
  These sensors provide an **analog voltage output** proportional to the current flowing through the sensor.  
  When the current exceeds a safe threshold, the system triggers motor cut-off and displays an alert.  

- **No-load**:  
  If a **forward or backward command** is received but **no stop sensor is triggered within 10 minutes**,  
  the system automatically cuts off motor power to prevent unsafe idle operation.



