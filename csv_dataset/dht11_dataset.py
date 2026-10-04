import serial
import csv
from datetime import datetime
import os

print("현재 저장 위치:", os.getcwd())

PORT = "COM3"
BAUD = 9600

CSV_FILE = r"C:\Users\samsung\Desktop\4-1\졸프\dht11_log_4sensor_co2.csv"

ser = serial.Serial(PORT, BAUD, timeout=1)

with open(CSV_FILE, "w", newline="", encoding="utf-8-sig") as f:
    writer = csv.writer(f)

    writer.writerow([
        "timestamp",
        "sensor1_temp",
        "sensor1_humidity",
        "sensor2_temp",
        "sensor2_humidity",
        "sensor3_temp",
        "sensor3_humidity",
        "sensor4_temp",
        "sensor4_humidity",
        "co2_ppm"
    ])

    print("Logging started... Ctrl+C to stop")
    print("CSV 저장 파일:", CSV_FILE)

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()

            if not line:
                continue

            print(line)

            parts = line.split(",")

            if len(parts) == 9:
                now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                writer.writerow([now] + parts)
                f.flush()
            else:
                print("Invalid data:", line)
                print("받은 데이터 개수:", len(parts))

    except KeyboardInterrupt:
        print("Logging stopped.")

ser.close()