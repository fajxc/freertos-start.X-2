# FreeRTOS Countdown Timer

A FreeRTOS-based countdown timer for PIC24 microcontrollers. It supports time entry over UART, LED feedback, potentiometer-controlled brightness, and pause/resume control.

## Overview

The system runs a simple state machine that controls a countdown timer. Time is entered through a UART terminal, buttons control the flow, and LEDs provide visual feedback. FreeRTOS handles tasks, timing, and concurrency.

**Target:** PIC24 (dsPIC33/PIC24 family)  
**RTOS:** FreeRTOS

## Hardware Requirements

### Microcontroller
- PIC24 or dsPIC33
- 16 MHz input (configured to 96 MHz PLL)

### Peripherals

**Push Buttons:**
- PB1: Enter time mode
- PB2: Used with PB3 to start
- PB3: Pause, resume, abort

**LEDs:**
- LED0 (RB5): Completion
- LED1 (RB6): Countdown blink
- LED2 (RB7): PWM brightness

**Other:**
- Potentiometer: AN5 (RA3)
- UART2: For all terminal IO

### Pin Mapping

All pin definitions are in `hw_config.h`:
- LED0: RB5
- LED1: RB6
- LED2: RB7
- ADC: AN5 (RA3)

## Features

- State-based operation
- UART time entry in MM:SS
- One-second accurate countdown
- LED animations for every state
- Software PWM for LED2
- ADC brightness control
- Pause/resume
- Abort via long press
- Optional info display (ADC + duty cycle)
- LED2 mode toggle (solid or blink)
- Backspace support

## State Machine

### WAITING
- LED2 pulses
- PB1 starts time entry
- Terminal shows a welcome message

### ENTER_TIME
- User enters MM:SS over UART
- Input is echoed
- Validates minutes and seconds
- Enter stores time
- PB2+PB3 long press clears input

### READY
- LEDs off
- PB2+PB3 click starts the countdown
- PB2+PB3 long press clears the stored time

### COUNTDOWN
- LED1 blinks
- LED2 brightness controlled by ADC
- PB3 click pauses
- PB3 long press aborts
- 'i' toggles extended info
- 'b' toggles LED2 mode
- Terminal overwrites the same line with remaining time

### PAUSED
- LEDs freeze
- ADC still updates LED2 brightness
- Same controls as COUNTDOWN

### FINISHED
- LED0 and LED1 alternate for 5 seconds
- LED2 stays solid
- Shows completion message
- Auto-returns to WAITING

## Building

### Requirements
- MPLAB X
- XC16
- FreeRTOS source

### Using Make
```bash
make clean
make build
```

### Using MPLAB X
1. Open project
2. Select default config
3. Build (F11)

### Output Files
- Debug: `dist/default/debug/*.elf`
- Production: `dist/default/production/*.hex`

## Usage

### Startup
1. Flash the hex file
2. Open UART2 at 115200 baud
3. Power the board

### Starting a Timer
1. Ensure system is in WAITING (LED2 pulsing)
2. PB1 enters time input
3. Enter MM:SS
4. Press Enter
5. PB2+PB3 starts the countdown

### During Countdown
- **Pause:** PB3 click
- **Resume:** PB3 click
- **Abort:** PB3 long press
- **Change brightness:** turn potentiometer
- **Toggle info:** press 'i'
- **Toggle LED2 mode:** press 'b'

### Completion
- LEDs go into the finish pattern
- System resets after 5 seconds

### Terminal Commands

| Key | Function |
|-----|----------|
| MM:SS | Enter time |
| Enter | Submit |
| Backspace | Remove last char |
| i | Show/hide ADC + duty cycle |
| b | Toggle LED2 mode |

### Button Summary

| Action | Buttons | Function |
|--------|---------|----------|
| Enter time | PB1 | Start input |
| Start countdown | PB2 + PB3 | Begin countdown |
| Clear time | PB2 + PB3 (hold) | Reset input |
| Pause/Resume | PB3 | Toggle pause |
| Abort | PB3 (hold) | Stop immediately |

## Project Structure

```
freertos-start.X 2/
├── main.c
├── app.h
├── hw_config.h
├── FreeRTOSConfig.h
│
├── adc.c / adc.h
├── buttons.c / buttons.h
├── pwm.c / pwm.h
├── uart.c / uart.h
│
├── FreeRTOS/
│   ├── include/
│   ├── portable/
│   └── *.c
│
├── build/
├── dist/
└── nbproject/
```

### Key Files
- `main.c`: Tasks and state machine
- `app.h`: Global definitions, RTOS objects
- `hw_config.h`: Pin definitions and hardware macros
- `adc.c`: AN5 sampling and conversion
- `pwm.c`: Software PWM
- `buttons.c`: Debouncing, click/long-press detection

## Technical Details

### FreeRTOS
- Max priority: 3
- Tick rate: 1 kHz
- Static allocation (heap_1)
- PIC24/dsPIC33 port

### Task Priorities
- **3:** PWM
- **2:** Countdown, Buttons
- **1:** Time input, Waiting
- **0:** ADC, Idle

### Timing
- Countdown resolution: 1 s
- LED1 blink: 1 Hz
- LED2 PWM: defined in pwm.c
- Debounce: ~50 ms
- Long press: >1 s
- Finish sequence: 5 s

### ADC
- 10-bit
- AN5
- Manual sample/convert
- ~20 microsecond conversion time

### PWM
- Software-driven
- Duty cycle mapped from ADC
- Used for pulsing and brightness

### Inter-Task Communication
- **Queues:** Button events, UART RX
- **Semaphores:** start signals
- **Mutexes:** UART printing, state, countdown value

### Memory
- All tasks have fixed stack sizes
- No dynamic allocation during runtime

## Troubleshooting

### Countdown not starting
- Time not validated
- PB2+PB3 not pressed together
- Not in READY state

### LED2 not reacting
- Check ADC wiring
- Check AN5 config
- Press 'i' to verify ADC values

### UART issues
- Confirm 115200 baud
- Check TX/RX pins
- Terminal must send CR+LF

### Buttons not responding
- Check debounce logic
- Verify pin setup
- Ensure button task is active
