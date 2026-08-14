# send_keys_to_esp32.py
# pip install pynput pyserial

import sys
import time
from pynput import keyboard
import serial

# ----- CONFIG -----
SERIAL_PORT = "COM4"      # <-- change this to your port, e.g. "/dev/ttyUSB0"
BAUDRATE = 115200
# ------------------

try:
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
except Exception as e:
    print("Could not open serial port:", e)
    sys.exit(1)

print(f"Opened serial {SERIAL_PORT} @ {BAUDRATE}. Listening for keyboard... (Ctrl+C to quit)")

def send_key_event(kind, keystr):
    # Format: <KIND>:<KEY>\n   kind = DOWN or UP or HOLD
    msg = f"{kind}:{keystr}\n".encode('utf-8')
    try:
        ser.write(msg)
    except Exception as e:
        print("Serial write failed:", e)

def on_press(k):
    try:
        keystr = k.char
    except AttributeError:
        keystr = str(k)  # e.g. Key.space, Key.up
    send_key_event("DOWN", keystr)

def on_release(k):
    try:
        keystr = k.char
    except AttributeError:
        keystr = str(k)
    send_key_event("UP", keystr)
    # Optional: stop on ESC
    # if k == keyboard.Key.esc:
    #     return False

listener = keyboard.Listener(on_press=on_press, on_release=on_release)
listener.start()

try:
    while True:
        time.sleep(0.1)
except KeyboardInterrupt:
    print("Exiting...")
finally:
    listener.stop()
    ser.close()
