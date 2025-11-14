import vgamepad as vg
import socket
import sys
import json

keys_names = [
    "KEY_A", "KEY_B", "KEY_SELECT", "KEY_START",
    "KEY_DRIGHT", "KEY_DLEFT", "KEY_DUP", "KEY_DDOWN",
    "KEY_R", "KEY_L", "KEY_X", "KEY_Y",
    "", "", "KEY_ZL", "KEY_ZR",
    "", "", "", "",
    "KEY_TOUCH", "", "", "",
    "KEY_CSTICK_RIGHT", "KEY_CSTICK_LEFT", "KEY_CSTICK_UP", "KEY_CSTICK_DOWN",
    "KEY_CPAD_RIGHT", "KEY_CPAD_LEFT", "KEY_CPAD_UP", "KEY_CPAD_DOWN"
]

DS_to_DS4 = {
    "KEY_A": vg.DS4_BUTTONS.DS4_BUTTON_CIRCLE,
    "KEY_B": vg.DS4_BUTTONS.DS4_BUTTON_CROSS,
    "KEY_X": vg.DS4_BUTTONS.DS4_BUTTON_SQUARE,
    "KEY_Y": vg.DS4_BUTTONS.DS4_BUTTON_TRIANGLE,
    "KEY_L": vg.DS4_BUTTONS.DS4_BUTTON_SHOULDER_LEFT,
    "KEY_R": vg.DS4_BUTTONS.DS4_BUTTON_SHOULDER_RIGHT,
    "KEY_ZL": vg.DS4_BUTTONS.DS4_BUTTON_TRIGGER_LEFT,
    "KEY_ZR": vg.DS4_BUTTONS.DS4_BUTTON_TRIGGER_RIGHT,
    "KEY_SELECT": vg.DS4_BUTTONS.DS4_BUTTON_SHARE,
    "KEY_START": vg.DS4_BUTTONS.DS4_BUTTON_OPTIONS,
    "KEY_DRIGHT": vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_EAST,
    "KEY_DLEFT": vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_WEST,
    "KEY_DUP": vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_NORTH,
    "KEY_DDOWN": vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_SOUTH,
}

button_states = {name: "up" for name in keys_names if name}

prev_kHeld = 0

def update_controller_state(ctrl, gamepad):
    for n, name in enumerate(keys_names):
        if not name or name not in DS_to_DS4:
            continue

        mask = 1 << n
        state = button_states[name]

        is_down = bool(ctrl["kDown"] & mask)
        is_held = bool(ctrl["kHeld"] & mask)
        is_up   = bool(ctrl["kUp"] & mask)

        ds4_button = DS_to_DS4[name]

        # Press
        if is_down and state != "down":
            button_states[name] = "down"
            gamepad.press_button(button=ds4_button)
            print(f"{name} pressed → DS4 {ds4_button}")

        # Hold
        if is_held and state != "held":
            button_states[name] = "held"
            print(f"{name} held")

        # Release
        if is_up and state != "up":
            button_states[name] = "up"
            gamepad.release_button(button=ds4_button)
            print(f"{name} released → DS4 {ds4_button}")
    up = bool(ctrl["kHeld"] & (1 << keys_names.index("KEY_DUP")))
    down = bool(ctrl["kHeld"] & (1 << keys_names.index("KEY_DDOWN")))
    left = bool(ctrl["kHeld"] & (1 << keys_names.index("KEY_DLEFT")))
    right = bool(ctrl["kHeld"] & (1 << keys_names.index("KEY_DRIGHT")))

    # Map to DS4 eight-way directions
    
    if up and left:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_NORTHWEST
    elif up and right:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_NORTHEAST
    elif down and left:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_SOUTHWEST
    elif down and right:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_SOUTHEAST
    elif up:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_NORTH
    elif down:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_SOUTH
    elif left:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_WEST
    elif right:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_EAST
    else:
        dpad_dir = vg.DS4_DPAD_DIRECTIONS.DS4_BUTTON_DPAD_NONE


    left_trigger_value  = 255 if (ctrl["kHeld"] & (1 << keys_names.index("KEY_ZL"))) else 0
    right_trigger_value = 255 if (ctrl["kHeld"] & (1 << keys_names.index("KEY_ZR"))) else 0
    gamepad.left_trigger(value=left_trigger_value)
    gamepad.right_trigger(value=right_trigger_value)

    # Left Joystick

    lx = max(0, min(255, ctrl["dx"] + 128))
    ly = max(0, min(255, -ctrl["dy"] + 128))
    gamepad.left_joystick(x_value=lx, y_value=ly)

    # Right Joystick

    cx = 128
    cy = 128
    if ctrl["kHeld"] & (1 << keys_names.index("KEY_CSTICK_LEFT")):
        cx = 0
    elif ctrl["kHeld"] & (1 << keys_names.index("KEY_CSTICK_RIGHT")):
        cx = 255

    if ctrl["kHeld"] & (1 << keys_names.index("KEY_CSTICK_UP")):
        cy = 0
    elif ctrl["kHeld"] & (1 << keys_names.index("KEY_CSTICK_DOWN")):
        cy = 255

    gamepad.right_joystick(x_value=cx, y_value=cy)
    gamepad.directional_pad(direction=dpad_dir)
    gamepad.update()

def main():
    IP = sys.argv[1]
    PORT = int(sys.argv[2])
    CON_MSG = "howdy"
    gamepad = vg.VDS4Gamepad()
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.sendto(CON_MSG.encode(),(IP,PORT))

    while(True):

        data,addr = sock.recvfrom(1024)
        data_s = data.decode().strip()
        ctrl = json.loads(data_s)
        update_controller_state(ctrl,gamepad)


if __name__ == "__main__":
    main()