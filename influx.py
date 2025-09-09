
import serial
import time
from datetime import datetime
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

# --- CONFIG ---
SERIAL_PORT = "COM7"      # Windows COM port
BAUDRATE = 115200
INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "JuJKyCftZy25lQBEAm-uKdOaaugTspaJoafF_jMTOhQh5LN6Bmto2HA_4z396UtYj0YydK3grsKhnZ8fhZXo2w=="
INFLUX_ORG = "projekt"
INFLUX_BUCKET = "lora_data"
# ----------------

def connect_influx():
    client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
    write_api = client.write_api(write_options=SYNCHRONOUS)
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
    try:
        text = line.decode("utf-8", errors="replace").strip()
        if not text.startswith("+RCV="):
            return

        # Example: +RCV=1,40,2025-09-08T15:00:08Z,16.283333,43.533333,-52,54
        parts = text.split(',')
        if len(parts) < 5:
            return

        timestamp_str = parts[2]
        lon = float(parts[3])
        lat = float(parts[4])

        try:
            timestamp_dt = datetime.strptime(timestamp_str, "%Y-%m-%dT%H:%M:%SZ")
        except ValueError:
            timestamp_dt = datetime.utcnow()

        point = Point("lora_gps") \
            .field("lat", lat) \
            .field("lon", lon) \
            .time(timestamp_dt, WritePrecision.S)

        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
        print(f"[OK] Written → time={timestamp_dt.isoformat()}Z, lon={lon}, lat={lat}")

    except Exception as e:
        print(f"[ERROR] Failed to process line: {e}")

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
