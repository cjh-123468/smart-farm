# main.py
import sys
import os
import json

# 한글 사용자 계정 경로에서 Qt 플랫폼 플러그인을 못 찾는 문제 회피용
# (C:\PyQtPlatforms는 실제 PyQt5 Qt5/plugins/platforms 폴더를 가리키는 영문 경로 junction)
if os.path.isdir(r"C:\PyQtPlatforms"):
    os.environ['QT_QPA_PLATFORM_PLUGIN_PATH'] = r"C:\PyQtPlatforms"

import serial
import numpy as np
import pandas as pd
import collections
import joblib
import torch
import pyqtgraph as pg
import cv2
import requests
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget,
                             QGridLayout, QVBoxLayout, QLabel, QPushButton, QFrame)
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QImage, QPixmap

try:
    from ai_model import LightWeightLSTM
except ImportError as e:
    LightWeightLSTM = None
    print("▶ ai_model.py를 찾을 수 없어 AI 기능 없이 실행합니다:", e)

class TeslaSmartFarm(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("QUAD-CORE SMART FARM OS")
        self.resize(1100, 800)
        
        self.ptc_status = False
        self.peltier_status = False

        # ==========================================
        # AI 세팅 파트
        # ==========================================
        self.feature_cols = [
            'sensor1_temp', 'sensor1_humidity', 'sensor2_temp', 'sensor2_humidity',
            'sensor3_temp', 'sensor3_humidity', 'sensor4_temp', 'sensor4_humidity', 'co2_ppm'
        ]
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.data_buffer = collections.deque(maxlen=40) # 10분 데이터 저장소
        self.ai_ready = False
        
        try:
            # 외부 파일(ai_model.py)에서 가져온 클래스로 모델 생성
            self.model = LightWeightLSTM(input_dim=len(self.feature_cols)).to(self.device)
            # Colab에서 다운받은 가중치 덮어씌우기
            self.model.load_state_dict(torch.load('smartfarm_lstm.pth', map_location=self.device, weights_only=True))
            self.model.eval() 
            
            # Colab에서 다운받은 스케일러 불러오기
            self.scaler = joblib.load('smartfarm_scaler.pkl')
            self.ai_ready = True
            print("▶ AI 시스템 로딩 완료!")
        except Exception as e:
            print("▶ AI 로딩 실패 (파일이 같은 폴더에 있는지 확인하세요):", e)

        # ==========================================
        # 시리얼 통신 및 UI 세팅 파트
        # ==========================================
        self.port = "COM3"
        self.baud = 9600
        
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
        except Exception as e:
            print("시리얼 연결 실패:", e)
            self.ser = None

        self.max_points = 50
        self.x = list(range(self.max_points))
        self.y_temp = [0.0] * self.max_points
        self.y_hum = [0.0] * self.max_points
        self.y_co2 = [0.0] * self.max_points

        # ==========================================
        # ESP32-CAM 세팅 파트
        # ==========================================
        self.cam_ip, self.led_intensity = self.load_camera_config()
        self.cam_stream_url = f"http://{self.cam_ip}:81/stream"
        self.cam_capture_url = f"http://{self.cam_ip}/capture"
        self.cam_control_url = f"http://{self.cam_ip}/control"
        self.snapshot_interval_ms = 10 * 60 * 1000  # 10분마다 자동 저장

        self.snapshot_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "camera_snapshots")
        os.makedirs(self.snapshot_dir, exist_ok=True)

        self.cam_cap = None
        self.init_camera_capture()

        self.setStyleSheet("background-color: #1a1a1a; color: white; font-family: 'Segoe UI', sans-serif;")

        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        grid = QGridLayout(main_widget)

        self.temp_graph, self.temp_line = self.create_tesla_graph("AVG TEMPERATURE", '#ff4d4d')
        self.hum_graph, self.hum_line = self.create_tesla_graph("AVG HUMIDITY", '#00d4ff')
        self.co2_graph, self.co2_line = self.create_tesla_graph("CO2 LEVEL", '#33ff77')

        control_panel = QVBoxLayout()
        
        self.prob_card = QFrame()
        self.prob_card.setStyleSheet("background-color: #262626; border-radius: 15px; padding: 20px;")
        card_layout = QVBoxLayout(self.prob_card)
        
        self.prob_title = QLabel("DEW POINT PROBABILITY")
        self.prob_title.setStyleSheet("font-size: 10pt; color: #888; font-weight: bold; background: transparent;")
        
        self.prob_label = QLabel("시스템 대기 중")
        self.prob_label.setStyleSheet("font-size: 25pt; font-weight: bold; color: #fff; background: transparent;")
        
        card_layout.addWidget(self.prob_title)
        card_layout.addWidget(self.prob_label)
        
        self.btn_ptc = self.create_tesla_button("HEATER (PTC) OFF", "#333")
        self.btn_ptc.clicked.connect(self.toggle_ptc)
        
        self.btn_peltier = self.create_tesla_button("COOLER (PELTIER) OFF", "#333")
        self.btn_peltier.clicked.connect(self.toggle_peltier)

        control_panel.addWidget(self.prob_card)
        control_panel.addSpacing(20)
        control_panel.addWidget(self.btn_ptc)
        control_panel.addWidget(self.btn_peltier)

        self.cam_frame = QFrame()
        self.cam_frame.setStyleSheet("background-color: #262626; border-radius: 15px; padding: 10px;")
        cam_layout = QVBoxLayout(self.cam_frame)

        self.cam_title = QLabel("ESP32-CAM LIVE VIEW")
        self.cam_title.setStyleSheet("font-size: 10pt; color: #888; font-weight: bold; background: transparent;")

        self.cam_view = QLabel("카메라 연결 대기 중...")
        self.cam_view.setFixedHeight(260)
        self.cam_view.setAlignment(Qt.AlignCenter)
        self.cam_view.setScaledContents(True)
        self.cam_view.setStyleSheet("background-color: #000; border-radius: 10px; color: #888;")

        cam_layout.addWidget(self.cam_title)
        cam_layout.addWidget(self.cam_view)

        grid.addWidget(self.temp_graph, 0, 0)
        grid.addWidget(self.hum_graph, 0, 1)
        grid.addWidget(self.co2_graph, 1, 0)
        grid.addLayout(control_panel, 1, 1)
        grid.addWidget(self.cam_frame, 2, 0, 1, 2)

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_data)
        self.timer.start(100)

        self.cam_timer = QTimer()
        self.cam_timer.timeout.connect(self.update_camera_frame)
        self.cam_timer.start(100)

        self.snapshot_timer = QTimer()
        self.snapshot_timer.timeout.connect(self.save_snapshot)
        self.snapshot_timer.start(self.snapshot_interval_ms)

    def create_tesla_graph(self, title, color):
        plot = pg.PlotWidget()
        plot.setBackground('#262626')
        plot.setTitle(title, color='#888', size='12pt')
        plot.showGrid(x=True, y=True, alpha=0.1)
        line = plot.plot(self.x, self.y_temp, pen=pg.mkPen(color, width=3))
        return plot, line

    def create_tesla_button(self, text, color):
        btn = QPushButton(text)
        btn.setStyleSheet(f"""
            QPushButton {{
                background-color: {color};
                border: 1px solid #444;
                border-radius: 10px;
                padding: 20px;
                font-size: 12pt;
                font-weight: bold;
                color: white;
            }}
            QPushButton:hover {{ background-color: #444; }}
        """)
        return btn

    def update_data(self):
        if self.ser and self.ser.in_waiting > 0:
            try:
                line = self.ser.readline().decode(errors="ignore").strip()
                if not line:
                    return
                    
                parts = line.split(",")
                
                if len(parts) == 9:
                    data = [float(p) for p in parts]
                    
                    # 1. 평균 계산 및 그래프 업데이트
                    avg_temp = (data[0] + data[2] + data[4] + data[6]) / 4.0
                    avg_hum = (data[1] + data[3] + data[5] + data[7]) / 4.0
                    co2_val = data[8]
                    
                    self.y_temp = self.y_temp[1:] + [avg_temp]
                    self.y_hum = self.y_hum[1:] + [avg_hum]
                    self.y_co2 = self.y_co2[1:] + [co2_val]
                    
                    self.temp_line.setData(self.x, self.y_temp)
                    self.hum_line.setData(self.x, self.y_hum)
                    self.co2_line.setData(self.x, self.y_co2)

                    # 2. 실시간 AI 추론 파트
                    self.data_buffer.append(data) 
                    
                    if not self.ai_ready:
                        self.prob_label.setText("AI 로드 실패")
                        self.prob_label.setStyleSheet("font-size: 25pt; font-weight: bold; color: red;")
                        return

                    if len(self.data_buffer) < 40:
                        self.prob_label.setText(f"데이터 수집 중\n({len(self.data_buffer)}/40)")
                        self.prob_label.setStyleSheet("font-size: 22pt; font-weight: bold; color: orange;")
                    else:
                        df_new = pd.DataFrame(list(self.data_buffer), columns=self.feature_cols)
                        scaled_data = self.scaler.transform(df_new)
                        tensor_data = torch.FloatTensor(scaled_data).unsqueeze(0).to(self.device)

                        with torch.no_grad():
                            outputs = self.model(tensor_data)
                            probs = torch.nn.functional.softmax(outputs, dim=1)
                            pred_class = torch.argmax(probs, dim=1).item()
                            confidence = probs[0][pred_class].item() * 100

                        risk_states = [("안정(safe)", "#33ff77"), ("주의(warning)", "#ffa500"), ("위험(danger)", "#ff4d4d")]
                        current_state, state_color = risk_states[pred_class]

                        self.prob_label.setText(f"{current_state}\n{confidence:.1f}%")
                        self.prob_label.setStyleSheet(f"font-size: 28pt; font-weight: bold; color: {state_color};")

            except Exception as e:
                pass 

    def load_camera_config(self):
        default_ip = "192.168.45.218"
        default_led = 150
        config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "camera_config.json")
        try:
            with open(config_path, "r", encoding="utf-8") as f:
                cfg = json.load(f)
            return cfg.get("cam_ip", default_ip), cfg.get("led_intensity", default_led)
        except Exception as e:
            print(f"카메라 설정 파일({config_path}) 로드 실패, 기본값 사용:", e)
            return default_ip, default_led

    def init_camera_capture(self):
        try:
            if self.cam_cap is not None:
                self.cam_cap.release()
            self.cam_cap = cv2.VideoCapture(self.cam_stream_url)
        except Exception as e:
            print("ESP32-CAM 스트림 연결 실패:", e)
            self.cam_cap = None

    def update_camera_frame(self):
        if self.cam_cap is None or not self.cam_cap.isOpened():
            self.cam_view.setText("카메라 연결 대기 중...")
            self.init_camera_capture()
            return

        ret, frame = self.cam_cap.read()
        if not ret:
            return

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb_frame.shape
        qimg = QImage(rgb_frame.data, w, h, ch * w, QImage.Format_RGB888)
        self.cam_view.setPixmap(QPixmap.fromImage(qimg))

    def save_snapshot(self):
        try:
            requests.get(self.cam_control_url, params={"var": "led_intensity", "val": self.led_intensity}, timeout=3)
            resp = requests.get(self.cam_capture_url, timeout=3)
            if resp.status_code == 200:
                filename = datetime.now().strftime("%Y%m%d_%H%M%S") + ".jpg"
                filepath = os.path.join(self.snapshot_dir, filename)
                with open(filepath, "wb") as f:
                    f.write(resp.content)
                print("▶ 스냅샷 저장:", filepath)
            else:
                print("스냅샷 요청 실패, 상태코드:", resp.status_code)
        except Exception as e:
            print("스냅샷 저장 실패:", e)

    def toggle_ptc(self):
        self.ptc_status = not self.ptc_status
        if self.ptc_status:
            self.btn_ptc.setText("HEATER (PTC) ON")
            self.btn_ptc.setStyleSheet("background-color: #ff4d4d; color: white; border-radius: 10px; padding: 20px; font-size: 12pt; font-weight: bold;")
        else:
            self.btn_ptc.setText("HEATER (PTC) OFF")
            self.btn_ptc.setStyleSheet("background-color: #333; color: #888; border-radius: 10px; padding: 20px; font-size: 12pt; font-weight: bold;")

    def toggle_peltier(self):
        self.peltier_status = not self.peltier_status
        if self.peltier_status:
            self.btn_peltier.setText("COOLER (PELTIER) ON")
            self.btn_peltier.setStyleSheet("background-color: #00d4ff; color: white; border-radius: 10px; padding: 20px; font-size: 12pt; font-weight: bold;")
        else:
            self.btn_peltier.setText("COOLER (PELTIER) OFF")
            self.btn_peltier.setStyleSheet("background-color: #333; color: #888; border-radius: 10px; padding: 20px; font-size: 12pt; font-weight: bold;")

    def closeEvent(self, event):
        if hasattr(self, 'ser') and self.ser and self.ser.is_open:
            self.ser.close()
        if hasattr(self, 'cam_cap') and self.cam_cap is not None:
            self.cam_cap.release()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = TeslaSmartFarm()
    window.show()
    sys.exit(app.exec_())