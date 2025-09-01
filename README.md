# Smart Queue Management System  
_Project by Team 11_

## Details about the project
The project’s goal is to create an **IoT-based smart queue management system** that uses sensor data analysis to recommend the optimal queue for each customer.  
Unlike random assignment, the system considers factors such as:  
- The **service rate** of each employee (e.g., service may slow down after breaks).  
- The **expected time per customer** (e.g., special cases may take longer).  

In case of a sensor/network failure, the system continues operating using a randomized distribution, based on previously collected hourly statistics.

## Features
- **Sensors at queue entry and exit** – detect arrival and completion of customers.  
- **Real-time queue analysis** – calculate expected waiting times.  
- **Customer interfaces** (alternative implementations we explored):  
  1. **Simple Display** – shows recommended queue number, number of people before the customer, and estimated waiting time.  
  2. **Web Interface** – customers scan a QR code and view live queue status and recommended queue.  
- **Robust fallback** – if the system cannot access live data, it switches to randomized distribution.  

## Hardware
- **ESP32 Devkit V1** microcontroller.  
- **IR / ultrasonic sensors** at queue entry and exit points.  
- **Display / simple digital board** for customer feedback.  
- **Optional**: QR code leading to a lightweight web interface.  

## Software
- **ESP32 firmware** – written in C++ (Arduino framework).  
- **Queue logic** – calculates average waiting times and distributes customers accordingly.  
- **Data handling** – works with live sensor data, but also keeps hourly statistics for fallback.  
- **UI options** – simple display board or a web-based interface.  

## Folder description
* **ESP32**: source code for the ESP side (firmware).  
* **Documentation**: wiring diagram + operating instructions.  
* **Unit Tests**: tests for individual hardware components (sensors, displays).  
* **flutter_app**: (optional) client application code.  

## ESP32 SDK version used in this project
- ESP32 Arduino Core v2.0.17  

## Arduino/ESP32 libraries used in this project
* WiFi – version XXXX  
* HTTPClient – version XXXX  
* (add sensor-specific libraries, e.g. Ultrasonic, IRRemote, etc.)  

## Connection diagram
(Insert diagram here – showing ESP32, sensors, display/QR code integration)

## System diagram
```mermaid
flowchart TD
    A[Customer Arrival] --> B[Entry Sensor Detects]
    B --> C[ESP32 Updates Queue State]
    C --> D[Calculate Waiting Times]
    D --> E[Display Board: Recommended Queue]
    E --> G[Customer Joins Queue]
    G --> H[Exit Sensor Detects Completion]
    H --> C
