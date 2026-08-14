# pependencies: pynput pyserial

import sys
import time
import threading
import queue
import logging
from logging.handlers import RotatingFileHandler
from datetime import datetime
from pynput import keyboard
import serial
import serial.tools.list_ports

# config
SERIAL_PORT = "COM5" # should be the same as the one selected in arduino ide
BAUDRATE = 115200 # standard baud rate for microcontrollers, do NOT use arduino serial monitor while using this program, the arduino monitor will occupy the port and this program wont be able to connect, this also applies vice versa, using this program will prevent arduino serial monitor from working
LOGFILE = "esp32_comm.log" # for debugging
LOG_MAX_BYTES = 5 * 1024 * 1024 # 5 mb per log file
LOG_BACKUP_COUNT = 3 # keep max 3 log files
SERIAL_READ_TIMEOUT = 0.1 # seconds
RECONNECT_DELAY = 2.0 # seconds to wait before trying to reconnect if serial doesnt want to

# setup logger
logger = logging.getLogger("ESP32Bridge")
logger.setLevel(logging.DEBUG)
fmt = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s", "%Y-%m-%d %H:%M:%S")

# console handler
ch = logging.StreamHandler(sys.stdout)
ch.setLevel(logging.DEBUG)
ch.setFormatter(fmt)
logger.addHandler(ch)

# rotating file handler
fh = RotatingFileHandler(LOGFILE, maxBytes=LOG_MAX_BYTES, backupCount=LOG_BACKUP_COUNT, encoding="utf-8")
fh.setLevel(logging.DEBUG)
fh.setFormatter(fmt)
logger.addHandler(fh)

# thread safe queue for outgoing messages
out_queue = queue.Queue()

# flag to request thread shutdown
stop_event = threading.Event()

def find_serial_ports():
    # return serial port names incase the selected one wont connect
    # purely for debugging
    ports = serial.tools.list_ports.comports()
    return [p.device for p in ports]

# class for simplifying port communication
class SerialManager:
    def __init__(self, port, baudrate, read_timeout=0.1):
        self.port = port
        self.baudrate = baudrate
        self.read_timeout = read_timeout
        self.ser = None
        self.lock = threading.Lock()

    def connect(self):
        # try to connect to serial port
        with self.lock:
            if self.ser and self.ser.is_open:
                return
            logger.info(f"Attempting to open serial port {self.port} @ {self.baudrate}")
            self.ser = serial.Serial(self.port, self.baudrate, timeout=self.read_timeout)
            # small pause to allow device to reset/initialize
            time.sleep(0.2)
            logger.info("Serial port opened")

    def close(self):
        with self.lock:
            try:
                if self.ser and self.ser.is_open:
                    self.ser.close()
                    logger.info("Serial port closed")
            except Exception as e:
                logger.exception("Error closing serial: %s", e)

    def write_line(self, line: str):
        # write a line to the serial port
        with self.lock:
            if not (self.ser and self.ser.is_open):
                raise serial.SerialException("Serial port not open")
            data = (line + "\n").encode("utf-8")
            self.ser.write(data)
            self.ser.flush()

    def read_line(self):
        # read a line from serial (kind of blocks the current thread). rework this later
        with self.lock:
            if not (self.ser and self.ser.is_open):
                return None
            try:
                line = self.ser.readline()
                if not line:
                    return None
                # decode safely and remove whitespace
                try:
                    s = line.decode("utf-8", errors="replace").rstrip("\r\n")
                except Exception:
                    s = repr(line)
                return s
            except Exception as e:
                raise

def serial_reader_thread(serial_mgr: SerialManager):
    # read forever from serial and log
    while not stop_event.is_set():
        try:
            # connect if connection is not open
            if not (serial_mgr.ser and serial_mgr.ser.is_open):
                try:
                    serial_mgr.connect()
                except Exception as e:
                    logger.warning("Could not open serial port (%s). Available ports: %s. Retrying in %.1f s. Error: %s",
                                   serial_mgr.port, find_serial_ports(), RECONNECT_DELAY, e)
                    time.sleep(RECONNECT_DELAY)
                    continue

            line = serial_mgr.read_line()
            if line is not None:
                logger.info(f"< ESP32: {line}")
            else:
                # found nothing
                # sleep to prevent it from hogging resources
                time.sleep(0.01)
        except serial.SerialException as e:
            logger.error("Serial exception: %s", e)
            serial_mgr.close()
            time.sleep(RECONNECT_DELAY)
        except Exception as e:
            logger.exception("Unexpected error in serial reader thread: %s", e)
            time.sleep(1)

def serial_writer_thread(serial_mgr: SerialManager):
    # write messages to serial from out_queue and log whatever is sent
    while not stop_event.is_set():
        try:
            try:
                msg = out_queue.get(timeout=0.1)  # block briefly
            except queue.Empty:
                continue

            # try to write, and reconnect if it fails
            wrote = False
            attempts = 0
            while not wrote and attempts < 3 and not stop_event.is_set():
                # attempt a write
                attempts += 1
                try:
                    # connect if connection is not open
                    if not (serial_mgr.ser and serial_mgr.ser.is_open):
                        serial_mgr.connect()
                    # write message
                    serial_mgr.write_line(msg)
                    # log whatever was sent
                    logger.info(f"> Sent: {msg}")
                    wrote = True
                except serial.SerialException as e:
                    logger.warning("Write failed (attempt %d): %s. Reconnecting...", attempts, e)
                    serial_mgr.close()
                    time.sleep(RECONNECT_DELAY)
                except Exception as e:
                    logger.exception("Unexpected error while writing to serial: %s", e)
                    time.sleep(1)
            if not wrote:
                logger.error("Failed to send message after retries: %s", msg)
        except Exception as e:
            logger.exception("Unexpected error in serial writer thread: %s", e)
            time.sleep(1)


# keyboard handling
def normalize_key(key):
    # convert pynput key states to string representation
    try:
        # if key represents a char (e.g. a, b, c) then use the "char" property
        if hasattr(key, "char") and key.char is not None:
            return key.char
        else:
            return str(key) # for special chars (e.g. "Key.space")
    except Exception:
        # something failed, just fallback to str(key) for simplicity
        # maybe log error or something
        return str(key)

def on_press(k):
    keystr = normalize_key(k)

    # maybe add whitelist / blacklist for keys
    # e.g. blacklisting control or tab which arent really used

    msg = f"DOWN:{keystr}"
    out_queue.put(msg)

def on_release(k):
    keystr = normalize_key(k)
    msg = f"UP:{keystr}"
    out_queue.put(msg)
    
    # maybe add stop on a certain key press (e.g. ESC or a combination like CTRL + ESC) to stop this program

def main():
    # show simple start info
    logger.info("Starting keyboard->ESP32 bridge")
    logger.info(f"Configured serial port: {SERIAL_PORT} @ {BAUDRATE}")
    logger.info(f"Available ports: {find_serial_ports()}")

    # initialize serial communication
    serial_mgr = SerialManager(SERIAL_PORT, BAUDRATE, read_timeout=SERIAL_READ_TIMEOUT)

    # Start serial reader/writer threads
    reader = threading.Thread(target=serial_reader_thread, args=(serial_mgr,), daemon=True)
    writer = threading.Thread(target=serial_writer_thread, args=(serial_mgr,), daemon=True)
    reader.start()
    writer.start()

    # start listener (threaded)
    listener = keyboard.Listener(on_press=on_press, on_release=on_release)
    listener.start()
    logger.info("Keyboard listener started")

    try:
        while not stop_event.is_set():
            time.sleep(0.2)
    except KeyboardInterrupt:
        logger.info("KeyboardInterrupt received. Shutting down")
        stop_event.set()
    finally:
        # cleanup
        listener.stop()
        logger.info("Stopping threads...")

        # give threads time to finish
        time.sleep(0.3)
        serial_mgr.close()
        logger.info("Shutdown complete")

if __name__ == "__main__":
    main()
