# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ATmega328P firmware for a café food-delivery train system. Two separate MCU projects:
- **Receiver** — installed on the train controller: drives motors, relay switching, reads sensors, overload protection
- **Transmitter** — handheld control panel: buttons, LCD, sends UART commands to Receiver

## Build Environment

**IDE**: Atmel Studio 7 (`.cppproj` files). No Makefile-based build — open the solution in Atmel Studio and use Build → Build Solution (F7). Flash via Atmel Studio's device programming tool (ISP or JTAG).

The `Debug/` folder contains build artifacts (`.elf`, `.hex`, `.lss`, `.map`). The `.hex` is flashed to the MCU. The `.lss` (disassembly) is useful for debugging when source-level info is unavailable.

F_CPU = 16 MHz for both MCUs.

## Hardware Architecture

### Two-Path Train System

- **PATH1**: tables 1–4, kitchen return sensor set A
- **PATH2**: tables 5–9, kitchen return sensor set B

Table routing uses 4× 74HC595 shift registers (bit-banged via PD2/PD3/PD4). Route bitmasks (`ROUTE_TO_TABLE_1..9`) are defined in `Receiver/include/config.h`. The route is activated one bit at a time non-blocking in `railroad_control.c:activate_route_non_blocking()`.

Sensors are read via one 74HC165 shift register (PD5/PD6/PD7). Each path has 4 sensors: TABLE_STOP, KITCHEN_STOP, TABLE_SLOW, KITCHEN_SLOW.

### Key GPIO (Receiver)

| Pin | Role |
|-----|------|
| PB0 (`RAIL_POWER_ENABLE`) | Shared main power rail for both paths |
| PB1 (`PWM_PATH1_PIN`) | Timer1 OC1A — PATH1 PWM output |
| PB2 (`PWM_PATH2_PIN`) | Timer1 OC1B — PATH2 PWM output |
| PB3 (`PWM_PATH2_SWITCH_PIN`) | Gates PATH2 H-bridge into PWM mode |
| PB4 (`PWM_PATH1_SWITCH_PIN`) | Gates PATH1 H-bridge into PWM mode; **also required by PATH2** during route setup and acceleration |
| PB5 (`REVERS_PIN`) | Relay — PATH1 direction |
| PC2 (`PATH2_REVERS_PIN`) | Relay — PATH2 direction |
| PC3 (`PATH2_RAIL_POWER_ENABLE`) | PATH2 direct DC power (active in MOVING state) |

### PWM Ramp (non-blocking)

Timer1 is Fast PWM 8-bit (mode 5). Both channels (OC1A, OC1B) are independent.

`startPWMUpForPath(path)` begins a non-blocking ramp from duty 0 → 255. `processPWMUp()` (called every main loop iteration) advances duty by 1 each `PWM_DELAY=50` ticks of `rail_switch_step_counter` (1 ms/tick from Timer0 CTC ISR). Full ramp ≈ 12.75 s.

When the ramp finishes: `PWM_PATH1_SWITCH_PIN` (or `PATH2_SWITCH_PIN`) is **cleared** and PWM output is disconnected — the motor runs on direct DC from `RAIL_POWER_ENABLE` / `PATH2_RAIL_POWER_ENABLE`.

`updatePathModesAfterPwm()` (main.cpp) transitions `pathMode[path]` from `PATH_MODE_ACCELERATION` → `PATH_MODE_MOVING` and enables `PATH2_RAIL_POWER_ENABLE` for PATH2.

### Shared Power Rail Invariant

`PWM_PATH1_SWITCH_PIN` must stay HIGH whenever PATH2 is active (route setup, acceleration, or moving), because PATH2 uses the same power gate. `shouldKeepSharedPathPowerForPath2()` in `loco_control.c` checks this and is called every time PATH1 is stopped. Never clear `PWM_PATH1_SWITCH_PIN` while PATH2 is active.

## UART Protocol

9600 baud, half-duplex. Packet format (6 bytes):

```
[STX=0x02] [CMD] [TABLE_ID] [DATA] [CRC8(CMD,TABLE_ID,DATA)] [ETX=0x03]
```

ACK from Receiver: `send_command(ACK=0x40, original_cmd, seq)`.

### Critical: ATmega328P UART has only a 2-byte hardware receive buffer (UDR0 + shift register).

**Any `_delay_ms()` longer than ~1–2 ms inside the main loop risks UART buffer overrun (DOR flag)**. When the buffer overflows, bytes are silently dropped. Reassembled fragments can occasionally pass CRC8 (probability 1/256) and execute as valid commands — including `CMD_STOP` (0x30) — stopping both paths unexpectedly.

**Rule: never use blocking delays in the main execution path.** Use `rail_switch_step_counter` tick comparison for non-blocking waits, as the route-setup and PWM-ramp code already does.

Known acceptable blocking delays (short enough not to overflow at 9600 baud):
- `_delay_ms(20)` — used after `send_command` for signal propagation
- `_delay_ms(80)` — relay contact settle in `MoveLocoBackward` (borderline; do not increase)

### send_command_with_ack bug

`Transmitter/uart.c:send_command_with_ack()` has `(void)data` — the `data` parameter is ignored and `seq` is sent as the DATA byte instead. The ACK protocol still works because the Receiver echoes `packet.param` (which IS the seq). **Do not "fix" this without updating the ACK check in the Transmitter.**

## Path Mode State Machine

```
PATH_MODE_STOP (0)
  → PATH_MODE_ROUTE_SETUP (1)   [forward command, route activating]
  → PATH_MODE_ACCELERATION (2)  [PWM ramp running]
  → PATH_MODE_MOVING (3)        [direct DC, ramp complete]
  → PATH_MODE_STOP              [arrival sensor / stop command]
```

Backward movement skips ROUTE_SETUP: `MoveLocoBackward()` calls `LocoStopPath()`, switches the direction relay (80 ms settle delay), then calls `PowerSupplyOnPath()` + `startPWMUpForPath()` → goes directly to ACCELERATION.

## Overload Protection (protection.c)

EMA filter (α = 1/8) on ADC channel 0 (current sense). State machine:
`OL_ARMED → OL_ACTIVE → OL_DIAG_START → OL_DIAG_PULSE → OL_LATCHED`

Threshold is stored in EEPROM addr 2 as `threshold/10` (e.g. 90 = 900 ADC). At runtime, `runtimeThresholdStored` caches this value. Call `overload_update_threshold(storedVal)` after changing the EEPROM value — it also resets the EMA filter (`filtInit = 0`).

`overload_enter_active()` calls `LocoStop()` (stops both paths) and sends `CMD_OVER_LOAD_STOP` 3 times.

## EEPROM Layout (Receiver)

| Address | Content |
|---------|---------|
| 0 | PATH1 slow-mode PWM duty (raw OCR1A value) |
| 1 | PATH2 slow-mode PWM duty |
| 2 | Overload threshold ÷ 10 |

`0xFF` = unset → use compile-time default.

## Service Menu

Accessed from Transmitter by holding both STOP buttons. Items:

| ID | Item |
|----|------|
| 1 | Sensor states (live read from 74HC165) |
| 2 | Overload threshold (displayed as ADC value = stored × 10) |
| 3 | PATH1 slow-mode PWM duty |
| 4 | PATH2 slow-mode PWM duty |

`CMD_MENU_SET` uses `packet.table_id = menuItem` (not path number) so the Receiver can distinguish threshold (item 2) from PWM items (3, 4). The Receiver's handler switches on `packet.table_id`.

## Receiver Main Loop Structure

```c
while (1) {
    checkSensorsState();               // reads 74HC165, calls handleSensorEvent
    check_and_send_overload_stop();    // ADC read + EMA + state machine
    overload_led_sync();
    checkLocoMovementTimeout();        // 300 s watchdog → LocoStop

    UART_Packet pkt = UART_receive_full_packet();  // returns invalid if no STX
    process_packet(pkt);               // dispatches CMD_* handlers

    processPWMUp();                    // advances ramp by one step if due
    updatePathModesAfterPwm();         // ACCELERATION → MOVING transition

    update_adc_display_if_due();       // ADC display throttle

    if (routeSetupInProgress) {
        activate_route_non_blocking(SelectedTable);
        startPendingRouteSetupIfReady();
    }
}
```

## Common Pitfalls

- **`LocoStopTable(0)` or `LocoStopTable(>9)` calls `LocoStop()` (both paths)**. Always validate table index before passing it.
- **`save_pending_packet` in Transmitter holds only one packet.** A second non-ACK packet arriving during `send_command_with_ack` overwrites the first.
- **`handleSensorEvent` has a `_delay_ms(20)` inside** — unavoidable, keep other delays out of sensor paths.
- **Watchdog is NOT enabled** on either MCU. Unexplained resets are BOD (brownout) from power supply transients during relay switching, not watchdog.
