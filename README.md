# Smart Farm

STM32 온습도·CO2 센서 + ESP32-CAM 카메라를 이용한 스마트팜 모니터링 / 데이터 수집 프로젝트.

## 구성

| 영역 | 설명 |
|---|---|
| **STM32F103 (DHT11_four_sensor)** | DHT11 온습도 센서 4개 + CM1107N CO2 센서를 읽어 USART로 1초마다 전송 |
| **ESP32-CAM (AI Thinker)** | Wi-Fi 기반 실시간 영상 스트리밍 + 사진 캡처, 야간 촬영용 플래시 LED 제어 |
| **SmartFarm_OS** | PyQt5 기반 실시간 대시보드 (온습도/CO2 그래프, 카메라 라이브 뷰, 자동 스냅샷, AI 이슬점 위험도 예측) |
| **csv_dataset** | 센서값(1분 주기) + 사진(10분 주기, 결로 확인용)을 쌍으로 모으는 데이터셋 수집 스크립트 |

## 빠른 시작

전체 설치/배선/트러블슈팅 순서는 **[SETUP_GUIDE.txt](./SETUP_GUIDE.txt)**에 자세히 정리되어 있습니다.

```bash
pip install -r requirements.txt

# 대시보드 실행
cd SmartFarm_OS
python main.py

# 데이터셋 수집 (대시보드와 동시 실행 불가 - 같은 COM 포트 사용)
cd csv_dataset
python collect_dataset.py
```

실행 전 `camera_config.json`에서 ESP32-CAM의 IP와 플래시 LED 밝기를 환경에 맞게 설정하세요.

```json
{
  "cam_ip": "192.168.x.x",
  "led_intensity": 150
}
```

## 폴더 구조

```
project/
  camera_config.json          # ESP32-CAM IP / LED 밝기 공통 설정
  requirements.txt
  SETUP_GUIDE.txt             # 처음부터 다시 세팅할 때 참고하는 전체 가이드

  dht11_sensor (1)/DHT11_four_sensor/   # STM32CubeIDE 펌웨어 프로젝트

  esp32cam_webserver/
    CameraWebServer_AI_THINKER/         # ESP32-CAM에 실제로 올린 스케치 (공식 예제 기반)

  SmartFarm_OS/
    main.py                             # 실시간 대시보드

  csv_dataset/
    collect_dataset.py                  # 센서+사진 데이터셋 수집
    dht11_dataset.py                    # (구) 센서값만 로깅하는 간단 스크립트
```

## 참고

- `ai_model.py`, `smartfarm_lstm.pth`, `smartfarm_scaler.pkl`은 저장소에 포함되어 있지 않습니다 (AI 이슬점 예측용 모델 파일). 없어도 대시보드는 정상 동작하며 AI 패널만 비활성화됩니다.
- ESP32-CAM Wi-Fi 정보는 `esp32cam_webserver/CameraWebServer_AI_THINKER/CameraWebServer.ino` 상단에 직접 입력해야 합니다 (플레이스홀더로 되어 있음).
