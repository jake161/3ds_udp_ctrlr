# 3DS UDP Controller

The goal of this project is to send button inputs from the 3DS via UDP to some arbitrary endpoint. Then companion software will do some translation.

## Installation
1. Add the `udp_ctrlr.3dsx` file to the `/3ds` directory on your homebrewed 3DS's SD card.
2. That's it.

## Usage
1. Ensure your 3DS is connected to WiFi.
2. Start UDP Ctrlr via the homebrew menu. This will automatically start a server on your 3DS's ip at port 5000.
3. See this link for [vgamepad specific setup](https://github.com/yannbouteiller/vgamepad/blob/main/readme/linux.md)
4. Run the `udp_ctrlr.py` companion script:

    ```sh
    python udp_ctrlr.py <YOUR 3DS IP> 5000
    ```
    This will start the connection and create an emulated gamepad which mirrors the 3DS's inputs.
