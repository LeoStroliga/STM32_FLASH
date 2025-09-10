import serial
import sqlite3
import time

# --- CONFIG ---
SERIAL_PORT = "COM7"
BAUDRATE = 115200
DB_FILE = "lora_data.db"
ACK_ADDRESS = 1   # LoRa address of the remote device
ACK_BYTE = "A"    # one character
# ----------------

# Initialize SQLite
conn = sqlite3.connect(DB_FILE)
cursor = conn.cursor()

cursor.execute("""
CREATE TABLE IF NOT EXISTS gps_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    longitude REAL NOT NULL,
    latitude REAL NOT NULL
)
""")
conn.commit()

def send_cmd(ser, cmd, wait=0.2):
    """Send AT command to LoRa module"""
    ser.write((cmd + "\r\n").encode("utf-8"))
    time.sleep(wait)
    resp = ser.read_all().decode(errors="ignore").strip()
    if resp:
        print(f"[LORA] {cmd} -> {resp}")
    return resp

def lora_init(ser):
    """Setup LoRa module at startup"""
    cmds = [
        "AT+RESET",
        "AT+MODE=0",
        "AT+ADDRESS=2",
        "AT+NETWORKID=5",
        "AT+BAND=868000000",
        "AT+PARAMETER=9,7,1,4"
    ]
    for c in cmds:
        send_cmd(ser, c)

def parse_lora_line(line: str):
    if not line.startswith("+RCV"):
        return None
    parts = line.split(",")
    if len(parts) < 5:
        return None
    try:
        src_addr = int(parts[0].split("=")[1])
        timestamp = parts[2].strip()
        lon = float(parts[3].strip())
        lat = float(parts[4].strip())
        return src_addr, timestamp, lon, lat
    except ValueError:
        return None

def send_ack(ser, dest_addr, data):
    cmd = f"AT+SEND={dest_addr},1,{data}"
    send_cmd(ser, cmd)

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
        print(f"[INFO] Opened {SERIAL_PORT} @ {BAUDRATE}")
    except serial.SerialException as e:
        print(f"[ERROR] {e}")
        return

    # Init LoRa module
    lora_init(ser)

    try:
        while True:
            raw_line = ser.readline()
            if not raw_line:
                continue

            try:
                line = raw_line.decode("utf-8", errors="ignore").strip()
            except UnicodeDecodeError:
                continue

            result = parse_lora_line(line)
            if result:
                src_addr, timestamp, lon, lat = result
                cursor.execute(
                    "INSERT INTO gps_data (timestamp, longitude, latitude) VALUES (?, ?, ?)",
                    (timestamp, lon, lat)
                )
                conn.commit()
                print(f"[OK] Stored → time={timestamp}, lon={lon}, lat={lat}")

                # Send ACK back to sender
                send_ack(ser, src_addr, ACK_BYTE)
            else:
                if line:
                    print(f"[SKIP] {line}")

    except KeyboardInterrupt:
        print("\n[INFO] Stopped.")
    finally:
        ser.close()
        conn.close()

if __name__ == "__main__":
    main()
