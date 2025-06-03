# Automatic Door Control System with People Counting and Smart Lighting

## Overview
This project implements an automatic door control system with integrated people counting and smart lighting features using the STM32F401RE microcontroller. The system can detect motion using PIR sensors, control the door's movement via a motor, and communicate with a LabVIEW application for monitoring and control purposes.

## Hardware Components
- STM32F401RE Nucleo board
- PIR motion sensors (HC-SR501)
- DC motor for door control
- Limit switches for detecting door position (open/closed)
- LEDs for lighting control
- UART communication for interfacing with LabVIEW

## Software Architecture

### Driver Layer
Low-level drivers that provide direct hardware abstraction:

1. **gpio_driver**: GPIO pin configuration and control
2. **rcc_config**: Clock configuration for peripherals
3. **systick_driver**: Time-keeping and delay functions
4. **exti_driver**: External interrupt handling for sensors
5. **pwm_driver**: PWM generation for motor control
6. **uart_driver**: UART communication handling

### Service Layer
Mid-level services that provide specific functionality:

1. **limit_switch_service**: Handles door position detection
2. **pir_sensor_service**: Processes motion detection with debouncing
3. **uart_protocol_service**: Implements the communication protocol with LabVIEW
4. **motor_control_service**: Controls the door motor

### Application Layer
The main application logic is in main.c and integrated with the above services.

## Communication Protocol

The system communicates with a LabVIEW application using a custom frame-based UART protocol with ACK/NACK mechanism for reliability.

### Frame Format
`[START][TYPE][ID][LENGTH][PAYLOAD][END]`

- **START**: Fixed byte (0x7E)
- **TYPE**: Frame type (commands, status updates, ACK/NACK)
- **ID**: Unique identifier for the frame
- **LENGTH**: Length of the payload (0-4 bytes)
- **PAYLOAD**: Data content (up to 4 bytes)
- **END**: Fixed byte (0x7F)

### Frame Types
- **Commands** (LabVIEW to STM32):
  - SET_MODE (0x10): Change door operation mode (Auto, Force Open, Force Close)
  - RESET_COUNT (0x11): Reset the people counter to 0

- **Status Updates** (STM32 to LabVIEW):
  - STATUS_UPDATE (0x20): Door state, light state, current mode, person count

- **Control Frames**:
  - ACK (0x30): Acknowledge receipt of a frame
  - NACK (0x31): Indicate failure to process a frame

### Reliability Features
- Frame ID tracking
- Acknowledgment system
- Retry mechanism
- Timeout handling
- Error detection

## Operation Modes

1. **Auto Mode**: Door opens automatically when motion is detected and closes after a delay. Lighting is controlled based on people count.

2. **Force Open**: Door remains open regardless of motion detection.

3. **Force Close**: Door remains closed regardless of motion detection.

## Door States
The system tracks and reports the following door states:
- **OPEN**: Door is fully open
- **OPENING**: Door is in the process of opening
- **CLOSE**: Door is fully closed
- **CLOSING**: Door is in the process of closing

## People Counting Logic
The system uses PIR sensors to detect motion and increment/decrement people count:
- When someone enters, count increases
- When someone exits, count decreases
- Count is used to control lighting (lights on when count > 0)
- The count can be reset via command from LabVIEW

## Project Structure

```
Core/
  ├── Inc/                      # Header files
  │   ├── main.h                # Main application header
  │   ├── stm32f4xx_hal_conf.h  # HAL configuration
  │   ├── stm32f4xx_it.h        # Interrupt handlers header
  │   ├── Drivers/              # Driver headers
  │   │   ├── exti_driver.h     # External interrupt driver
  │   │   ├── gpio_driver.h     # GPIO configuration driver
  │   │   ├── pwm_driver.h      # PWM for motor control
  │   │   ├── rcc_config.h      # Clock configuration
  │   │   ├── systick_driver.h  # System tick for timing
  │   │   └── uart_driver.h     # UART communication
  │   └── Services/             # Service layer headers
  │       ├── limit_switch_service.h  # Door position detection
  │       ├── pir_sensor_service.h    # Motion detection
  │       └── uart_protocol_service.h # UART protocol implementation
  │
  ├── Services/                 # Service implementations
  │   ├── motor_control_service.c  # Motor control implementation
  │   └── motor_control_service.h  # Motor control header
  │
  ├── Src/                      # Source files
  │   ├── main.c                # Main application
  │   ├── stm32f4xx_hal_msp.c   # HAL MSP initialization
  │   ├── stm32f4xx_it.c        # Interrupt handlers
  │   ├── syscalls.c            # System call implementations
  │   ├── sysmem.c              # System memory handlers
  │   ├── system_stm32f4xx.c    # System initialization
  │   ├── Drivers/              # Driver implementations
  │   │   ├── exti_driver.c     # External interrupt driver
  │   │   ├── gpio_driver.c     # GPIO configuration driver
  │   │   ├── pwm_driver.c      # PWM for motor control
  │   │   ├── rcc_config.c      # Clock configuration
  │   │   ├── systick_driver.c  # System tick for timing
  │   │   └── uart_driver.c     # UART communication
  │   └── Services/             # Service implementations
  │       ├── limit_switch_service.c  # Door position detection
  │       ├── pir__sensor_service.c   # Motion detection
  │       └── uart_protocol_service.c # UART protocol implementation
  │
  └── Startup/                  # Startup code
      └── startup_stm32f401retx.s  # Assembly startup file
```

## Key Features

1. **Reliable Communication**: Frame-based protocol with ACK/NACK mechanism ensures reliable communication with LabVIEW.

2. **Smart Door Control**: Automatic door control with support for multiple modes.

3. **People Counting**: PIR sensors track people entering and leaving.

4. **Position Tracking**: Limit switches detect the door's fully open and fully closed positions.

5. **Debouncing**: Hardware events are properly debounced for reliable operation.

6. **Safety Features**: Prevents door from hitting obstacles through limit switches.

7. **Flexible Speed Control**: Motor speed control via PWM for smooth operation.

8. **Resource Optimization**: Uses minimal resources while providing all required features.

9. **State Feedback**: Provides continuous feedback about door states, light states, and people count.

## Development Environment

- STM32CubeIDE
- Custom peripheral drivers (non-HAL)
- C programming language
- STM32F401RE Target