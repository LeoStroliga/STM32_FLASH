import serial
import sqlite3

# --- CONFIG ---
SERIAL_PORT = "COM7"      # Change if needed
BAUDRATE = 115200
DB_FILE = "lora_data.db"
# ----------------

# Initialize SQLite
conn = sqlite3.connect(DB_FILE)
cursor = conn.cursor()

# Create table if not exists
cursor.execute("""
CREATE TABLE IF NOT EXISTS gps_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    longitude REAL NOT NULL,
    latitude REAL NOT NULL
)
""")
conn.commit()

def parse_lora_line(line: str):
    """
    Parse LoRa CSV line:
    +RCV=1,40,2025-09-08T15:00:08Z,16.283333,43.533333,-52,54
    Returns: (timestamp, lon, lat) or None if invalid
    """
    if not line.startswith("+RCV"):
        return None

    parts = line.split(",")
    if len(parts) < 5:
        return None

    try:
        timestamp = parts[2].strip()
        lon = float(parts[3].strip())
        lat = float(parts[4].strip())
        return timestamp, lon, lat
    except ValueError:
        return None

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
        print(f"[INFO] Listening on {SERIAL_PORT} @ {BAUDRATE} bps")
    except serial.SerialException as e:
        print(f"[ERROR] Could not open serial port: {e}")
        return

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
                timestamp, lon, lat = result
                cursor.execute(
                    "INSERT INTO gps_data (timestamp, longitude, latitude) VALUES (?, ?, ?)",
                    (timestamp, lon, lat)
                )
                conn.commit()
                print(f"[OK] Stored → time={timestamp}, lon={lon}, lat={lat}")
            else:
                print(f"[SKIP] Invalid line: {line}")

    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    finally:
        ser.close()
        conn.close()

if __name__ == "__main__":
    main()
