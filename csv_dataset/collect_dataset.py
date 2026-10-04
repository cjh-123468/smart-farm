import os
import csv
import json
import time
from datetime import datetime

import serial
import requests

# ===========================
# 설정
# ===========================
SERIAL_PORT = "COM3"
BAUD = 9600

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR, "..", "camera_config.json")

DEFAULT_CAM_IP = "192.168.45.218"
DEFAULT_LED_INTENSITY = 150


def load_camera_config():
    try:
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            cfg = json.load(f)
        return cfg.get("cam_ip", DEFAULT_CAM_IP), cfg.get("led_intensity", DEFAULT_LED_INTENSITY)
    except Exception as e:
        print(f"카메라 설정 파일({CONFIG_PATH}) 로드 실패, 기본값 사용:", e)
        return DEFAULT_CAM_IP, DEFAULT_LED_INTENSITY


CAM_IP, LED_INTENSITY = load_camera_config()
CAM_CAPTURE_URL = f"http://{CAM_IP}/capture"
CAM_CONTROL_URL = f"http://{CAM_IP}/control"

SENSOR_INTERVAL_SEC = 60    # 센서값 기록 주기
PHOTO_INTERVAL_SEC = 600    # 사진 촬영 주기
SENSOR_CSV = os.path.join(BASE_DIR, "dataset_sensor_log.csv")
PHOTO_DIR = os.path.join(BASE_DIR, "dataset_photos")
PHOTO_CSV = os.path.join(BASE_DIR, "dataset_photo_log.csv")

SENSOR_COLUMNS = [
    "timestamp",
    "sensor1_temp", "sensor1_humidity",
    "sensor2_temp", "sensor2_humidity",
    "sensor3_temp", "sensor3_humidity",
    "sensor4_temp", "sensor4_humidity",
    "co2_ppm",
]

os.makedirs(PHOTO_DIR, exist_ok=True)


def init_csv(path, columns):
    if not os.path.exists(path):
        with open(path, "w", newline="", encoding="utf-8-sig") as f:
            csv.writer(f).writerow(columns)


def append_csv(path, row):
    with open(path, "a", newline="", encoding="utf-8-sig") as f:
        csv.writer(f).writerow(row)


def read_latest_sensor_line(ser):
    """시리얼 버퍼에 쌓인 줄을 모두 읽고 마지막(가장 최신) 유효한 줄만 반환."""
    latest = None
    while ser.in_waiting > 0:
        line = ser.readline().decode(errors="ignore").strip()
        if line:
            latest = line
    return latest


def set_led_intensity(value):
    requests.get(CAM_CONTROL_URL, params={"var": "led_intensity", "val": value}, timeout=3)


def capture_photo():
    # ESP32-CAM은 /capture 요청 시 LED를 자동으로 켰다 끄는데(약 150ms),
    # 밝기(led_duty)가 기본 0이라 미리 한 번 밝기를 설정해둬야 실제로 빛이 난다.
    set_led_intensity(LED_INTENSITY)
    resp = requests.get(CAM_CAPTURE_URL, timeout=5)
    resp.raise_for_status()
    return resp.content


def main():
    init_csv(SENSOR_CSV, SENSOR_COLUMNS)
    init_csv(PHOTO_CSV, ["timestamp", "filename"])

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD, timeout=0.1)
    except Exception as e:
        print("시리얼 연결 실패:", e)
        return

    print("데이터 수집 시작... (Ctrl+C로 종료)")
    print(f"- 센서 로그: {SENSOR_CSV} ({SENSOR_INTERVAL_SEC}초 주기)")
    print(f"- 사진 로그: {PHOTO_CSV} / {PHOTO_DIR} ({PHOTO_INTERVAL_SEC}초 주기)")

    last_sensor_time = 0.0
    last_photo_time = 0.0

    try:
        while True:
            line = read_latest_sensor_line(ser)
            now = time.time()

            if line and (now - last_sensor_time >= SENSOR_INTERVAL_SEC):
                parts = line.split(",")
                if len(parts) == 9:
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    append_csv(SENSOR_CSV, [timestamp] + parts)
                    last_sensor_time = now
                    print(f"[센서] {timestamp} 기록")
                else:
                    print("[센서] 유효하지 않은 데이터, 스킵:", line)

            if now - last_photo_time >= PHOTO_INTERVAL_SEC:
                try:
                    photo_bytes = capture_photo()
                    filename = datetime.now().strftime("%Y%m%d_%H%M%S") + ".jpg"
                    filepath = os.path.join(PHOTO_DIR, filename)
                    with open(filepath, "wb") as f:
                        f.write(photo_bytes)
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    append_csv(PHOTO_CSV, [timestamp, filename])
                    last_photo_time = now
                    print(f"[사진] {filename} 저장")
                except Exception as e:
                    print("[사진] 촬영 실패, 다음 주기에 재시도:", e)
                    last_photo_time = now  # 실패해도 주기는 넘김 (연속 재시도 방지)

            time.sleep(1)

    except KeyboardInterrupt:
        print("데이터 수집 종료.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
