import serial
import time
from datetime import datetime
from influxdb_client import InfluxDBClient, Point, WritePrecision

# --- CONFIG ---
SERIAL_PORT = "COM7"      # Windows COM port
BAUDRATE = 115200
INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "QimX0B_5GNzG-CD-WLIhHvRm6Z_N8pOvXDgsbCt0ZfHyNcIm479sFLOqADpkyWlmBzmij0iIyhnwKcAyDMecAA=="
INFLUX_ORG = "projekt"
INFLUX_BUCKET = "lora_data"
# ----------------

def connect_influx():
    client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
    write_api = client.write_api(write_options=None)
    print("[INFO] Connected to InfluxDB")
    return client, write_api

def connect_serial():
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
            print(f"[INFO] Listening on {SERIAL_PORT} @ {BAUDRATE} bps")
            return ser
        except serial.SerialException as e:
            print(f"[WARN] Could not open serial port {SERIAL_PORT}, retrying in 5s... ({e})")
            time.sleep(5)

def process_line(line, write_api):
    text = line.decode("utf-8", errors="replace").strip()
    if not text:
        return

    parts = text.split(',')
    if len(parts) != 3:
        print(f"[WARN] Invalid CSV format: {text}")
        return

    timestamp_str, lon_str, lat_str = parts

    try:
        ts = datetime.fromisoformat(timestamp_str.replace("Z", "+00:00"))  # UTC
        lon = float(lon_str)
        lat = float(lat_str)
    except ValueError as e:
        print(f"[WARN] Value error: {e}, line: {text}")
        return

    point = Point("lora_gps") \
        .field("lon", lon) \
        .field("lat", lat) \
        .time(ts, WritePrecision.NS)

    write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
    print(f"[{ts.isoformat()}] Written: {text}")

def main():
    client, write_api = connect_influx()
    ser = connect_serial()

    try:
        while True:
            try:
                line = ser.readline()
                if line:
                    process_line(line, write_api)
            except serial.SerialException:
                print("[WARN] Serial connection lost, reconnecting...")
                ser.close()
                ser = connect_serial()
    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
    finally:
        ser.close()
        client.close()

if __name__ == "__main__":
    main()
