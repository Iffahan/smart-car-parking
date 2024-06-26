﻿# Smart Car Parking System

This project is a part of the Embedded System class at Prince of Songkla University, Computer Engineering. It demonstrates the integration of Arduino and ODROID platforms to create a smart car parking solution.


## Project Overview

The Smart Car Parking System utilizes an Arduino UNO R3 and an ODROID-C4 to create an efficient and secure parking experience. By interfacing these devices and leveraging the LINE Messaging API, users can reserve parking spots, receive a unique password, and access their spot through an automated barrier system.

## Features

- **Online Reservation**: Reserve parking spots via LINE OA using the LINE Messaging API.
- **Password Protected Access**: Receive a unique password upon reservation.
- **Automated Barrier Control**: Secure and automated access with a servo-controlled barrier.
- **Serial Communication**: Robust communication between Arduino UNO R3 and ODROID-C4.
- **Enhanced Security**: Additional security features for safe and reliable access.
- **Flexible Options**: Configurable settings for different requirements.

## Components

- Arduino UNO R3
- ODROID-C4
- Servo Motor (for the barrier)
- LINE Messaging API
- Serial Communication Modules
- Miscellaneous electronics (wires, resistors, etc.)

## System Architecture

1. **User Reservation**: Users reserve a parking spot using the LINE OA via the LINE Messaging API.
2. **Password Generation**: The system generates and sends a unique password to the user.
3. **Serial Communication**: Arduino UNO R3 and ODROID-C4 communicate via serial to manage parking access.
4. **Automated Barrier**: Upon entering the password, the servo motor (barrier) is activated to grant access.
5. **Security Features**: Additional security measures ensure safe and authorized access.

## Setup Instructions

### Hardware Setup

1. Connect the Arduino UNO R3 to the ODROID-C4 using the serial communication pins.
2. Connect the servo motor to the Arduino UNO R3.
3. Ensure all components are securely connected and powered.

### Software Setup

1. Clone the repository:
    ```bash
    git clone https://github.com/yourusername/smart-car-parking.git
    ```
2. Install the required libraries for Arduino and ODROID.
3. Upload the Arduino code to the Arduino UNO R3.
4. Set up the ODROID-C4 with the necessary scripts to handle the LINE Messaging API.

## Usage

1. **Reserve a Parking Spot**: Use LINE OA to reserve a parking spot and receive a unique password.
2. **Enter the Parking Area**: Upon arrival, enter the received password to open the automated barrier.
3. **Access Granted**: The servo motor will activate, opening the barrier for authorized access.

## Contributing

Contributions are welcome! Please fork this repository and submit a pull request for any improvements or bug fixes.
