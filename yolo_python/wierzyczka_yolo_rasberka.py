import os
import time
from ultralytics import YOLO
from picamera2 import Picamera2
import cv2

# --- KONFIGURACJA ---
frameWidth = 1280
frameHeight = 720
targetfps = 20
imgsz = 640
FIFO_PATH = "/tmp/yolo_fifo"

# Tworzenie potoku FIFO jeśli nie istnieje
if not os.path.exists(FIFO_PATH):
    os.mkfifo(FIFO_PATH)

# Parametry czasowe
interval = 1.0 / targetfps
next_t = time.perf_counter()

# Model
model = YOLO("/home/pi/yolo_wierzyczka/best.pt")

# Kamera
picam2 = Picamera2()
config = picam2.create_preview_configuration(
    main={"size": (frameWidth, frameHeight), "format": "RGB888"}
)
picam2.configure(config)
picam2.start()

active_classes = None
mode_text = "Wyszukiwanie: wszystkie"
targetX = frameWidth // 2
targetY = frameHeight // 2

# Otwieramy FIFO w trybie zapisu (nieblokującym)
# Uwaga: Program w C musi być odpalony, żeby Python mógł pisać do FIFO bez błędów
try:
    fifo_fd = os.open(FIFO_PATH, os.O_WRONLY | os.O_NONBLOCK)
except OSError:
    print("Błąd: Nie można otworzyć FIFO. Uruchom najpierw program w C!")
    fifo_fd = None

try:
    while True:
        now = time.perf_counter()

        if now >= next_t:
            frame = picam2.capture_array()
            
            # Detekcja
            results = model(frame, imgsz=imgsz, classes=active_classes, verbose=False, conf=0.7)
            
            display = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
            cv2.circle(display, (targetX, targetY), 8, (0, 0, 255), -1)

            boxes = results[0].boxes
            status = 0
            err_x = 0.0
            err_y = 0.0

            if len(boxes) > 0:
                # WYBÓR CELU: Wybieramy ten z najwyższym confidence
                best_box = max(boxes, key=lambda x: x.conf[0])
                
                status = 1
                x1, y1, x2, y2 = map(int, best_box.xyxy[0])
                cx = (x1 + x2) // 2
                cy = (y1 + y2) // 2

                # --- NORMALIZACJA (-1.0 do 1.0) ---
                # Wyliczamy błąd względem środka i dzielimy przez połowę szerokości
                err_x = (cx - targetX) / (frameWidth / 2)
                err_y = (targetY - cy) / (frameHeight / 2) # Y odwrócone, żeby 'w górę' było dodatnie

                # Rysowanie
                cv2.rectangle(display, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.circle(display, (cx, cy), 4, (0, 255, 0), -1)

            # --- WYSYŁKA DO C PRZEZ FIFO ---
            if fifo_fd is not None:
                try:
                    # Format: status,err_x,err_y\n
                    data = f"{status},{err_x:.3f},{err_y:.3f}\n"
                    os.write(fifo_fd, data.encode())
                except OSError:
                    # Jeśli program w C się zamknął, próbujemy otworzyć ponownie
                    pass

            # Wyświetlanie UI
            cv2.rectangle(display, (10, 10), (600, 70), (0, 0, 0), -1)
            cv2.putText(display, mode_text, (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2)
            cv2.imshow("YOLO Detection", display)

            next_t += interval
        else:
            time.sleep(0.001)

        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'): break
        elif key == ord('a'): active_classes, mode_text = None, "Wszystkie klasy"
        elif key == ord('0'): active_classes, mode_text = [0], "Niebieski"
        elif key == ord('1'): active_classes, mode_text = [1], "Różowy"
        elif key == ord('2'): active_classes, mode_text = [2], "Zielony"
        elif key == ord('3'): active_classes, mode_text = [3], "Żółty"

finally:
    if fifo_fd: os.close(fifo_fd)
    cv2.destroyAllWindows()
    picam2.stop()
    picam2.close()