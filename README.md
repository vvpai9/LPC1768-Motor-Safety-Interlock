# Motor Safety Interlock with Multi-Sensor Check Using RTOS on LPC1768 (ARM Cortex-M3)

---

This repository contains the complete implementation of a real-time motor safety interlock system developed using the LPC1768 ARM Cortex-M3 microcontroller and RTX RTOS.  
The motor operates only under safe environmental conditions, monitored using ultrasonic, gas, and temperature sensors.

---

## Project Overview

The system continuously monitors:
- Obstacle distance (HC-SR04 Ultrasonic Sensor)
- Gas concentration (MQ-5 Gas Sensor)
- Temperature (LM35 Temperature Sensor)

If any unsafe condition is detected, the motor is stopped immediately.  
The project demonstrates RTOS-based task scheduling, synchronization, and inter-task communication using semaphores, event flags, and message queues.

! (https://github.com/vvpai9/LPC1768-Motor-Safety-Interlock/blob/main/Poster.png?raw=true)

! (https://github.com/vvpai9/LPC1768-Motor-Safety-Interlock/blob/main/Block_Diagram.png?raw=true)


---

## Software Requirements

- Windows OS (Windows 7/8/10/11 recommended)  
- Keil µVision 4  
- Flash Magic  
- USB-to-Serial driver (if required)  

---

## Hardware Requirements

- LPC1768 Development Board (ARM Cortex-M3)  
- HC-SR04 Ultrasonic Sensor  
- MQ-5 Gas Sensor  
- LM35 Temperature Sensor  
- DC Motor (30 or 60RPM)
- 2-Channel Relay Module or Motor Driver  
- External 12 V power supply for motor  
- Jumper wires  
- Breadboard
- External 5 V power supply for Cortex-M3
- USB to DB9 connector cable  

---

## Installation and Setup

### Installing Keil µVision 4

1. Download Keil µVision 4 installer
2. Follow the installation steps.  
3. Launch Keil µVision.  
4. Install device packs if prompted.  
5. Ensure NXP LPC1768 is available in the device list.

---

### Opening the Project in Keil

1. Open Keil µVision.  
2. Select Project → Open Project.  
3. Open project.uvproj.  
4. Verify the device is LPC1768 and the target is ARM Cortex-M3.  
5. Build the project using Project → Build Target.

---

### Generating the HEX File

1. Open Project → Options for Target.  
2. Go to the Output tab.  
3. Enable Create HEX File.  
4. Rebuild the project.  
5. The file project.hex will be generated.

---

## Flashing the Code Using Flash Magic

### Installing Flash Magic

1. Run Flash_Magic_Installer.exe.  
2. Complete the installation.  
3. Launch Flash Magic.

---

### Flashing the LPC1768 Board

1. Connect the LPC1768 board to the PC via USB or Serial.  
2. Put the board into ISP mode.  
3. Open Flash Magic and configure:  
   - Device: LPC1768  
   - COM Port: Select from Device Manager  
   - Baud Rate: 9600 or 115200  
   - Interface: UART  
4. Browse and select project.hex.  
5. Click Start.  
6. On successful flashing, reset the board.

---

## Sensor and Motor Connections

### LPC1768 Pin Mapping

| Component | LPC1768 Pin |
|----------|-------------|
| Ultrasonic TRIG | P0.26 |
| Ultrasonic ECHO | P0.25 |
| LM35 Output | P0.23 (AD0.0) |
| MQ-5 Output | P0.24 (AD0.1) |
| DC Motor Control | P2.0 – P2.1 |
| Ground | Common GND |

! (https://github.com/vvpai9/LPC1768-Motor-Safety-Interlock/blob/main/Interfacing_Diagram.png?raw=true)

! (https://github.com/vvpai9/LPC1768-Motor-Safety-Interlock/blob/main/Implementation.jpeg?raw=true)

---

### Power Connections

- LPC1768 Board: 5 V adapter  
- Sensors: 5 V from board  
- DC Motor: External 12 V supply  
- All grounds must be common  

Use a voltage divider for HC-SR04 ECHO pin to step down 5 V to 3.3 V.

---

## Software Architecture

### Tasks Implemented

- Sensor Task: Reads ultrasonic, gas, and temperature sensors  
- Motor Control Task: Implements safety interlock logic  
- Logger Task: Logs fault events using message queue  

---

### RTOS Objects Used

- Semaphore for shared sensor data protection  
- Event flags for task signaling  
- Message queue for fault logging  

---

## Working Principle

1. Sensors acquire environmental data.  
2. Data is stored in a shared structure.  
3. The motor task evaluates safety conditions.  
4. If any unsafe condition is detected, the motor is stopped.  
5. Fault events are logged.  

---

## Results

- Deterministic real-time response  
- Immediate motor shutdown on fault  
- Stable operation on hardware  
- Verified through live demonstration video  

---

## Demonstration

Refer to Working_Demo.mp4 for the hardware demonstration.

---

## Project Website

Google Sites Portfolio:  
https://sites.google.com/kletech.ac.in/os-esd

---

## References

- NXP LPC1768 User Manual  
- ARM Cortex-M3 Technical Reference Manual  
- Keil RTX Documentation  
- HC-SR04, MQ-5, and LM35 datasheets  

---

## Team

Team C-18  
Course: Operating Systems and Embedded Systems Design  
Project Guide: Prof. Basawaraj Patil  
