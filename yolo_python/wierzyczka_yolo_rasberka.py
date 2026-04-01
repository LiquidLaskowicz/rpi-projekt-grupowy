from ultralytics import YOLO
from picamera2 import Picamera2
import cv2
import time

frameWidth = 1280
frameHeight = 720
targetfps = 20
imgsz = 640

interval = 1.0 / targetfps
next_t = time.perf_counter()

model = YOLO("/home/pi/yolo_wierzyczka/best.pt")

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

counter = 0

try:
    while True:
        now = time.perf_counter()

        if now >= next_t:
            frame = picam2.capture_array()

            # YOLO działa na RGB → NIE konwertujemy
            results = model(
                frame,
                imgsz=imgsz,
                classes=active_classes,
                verbose=False,
                conf=0.7
            )

            # konwersja tylko do wyświetlania
            display = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

            # celownik
            cv2.circle(display, (targetX, targetY), 8, (0, 0, 255), -1)

            boxes = results[0].boxes

            for box in boxes:
                cls = int(box.cls[0])
                conf = float(box.conf[0])

                x1, y1, x2, y2 = map(int, box.xyxy[0])
                cx = (x1 + x2) // 2
                cy = (y1 + y2) // 2

                # bbox (lekkie rysowanie zamiast plot())
                cv2.rectangle(display, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.circle(display, (cx, cy), 4, (0, 255, 0), -1)

                difX = targetX - cx
                difY = targetY - cy

                counter += 1
                if counter % 20 == 0:
                    print(
                        f"{model.names[cls]} | conf={conf:.2f} | dx={difX} dy={difY}"
                    )

            # tło pod tekst
            cv2.rectangle(display, (10, 10), (600, 70), (0, 0, 0), -1)

            cv2.putText(
                display,
                mode_text,
                (20, 50),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                (0, 255, 255),
                2,
                cv2.LINE_AA
            )

            cv2.imshow("YOLO Detection", display)

            next_t += interval
        else:
            time.sleep(min(next_t - now, 0.002))

        key = cv2.waitKey(1) & 0xFF

        if key == ord('q'):
            break
        elif key == ord('a'):
            active_classes = None
            mode_text = "Wszystkie klasy"
        elif key == ord('0'):
            active_classes = [0]
            mode_text = "Niebieski"
        elif key == ord('1'):
            active_classes = [1]
            mode_text = "Różowy"
        elif key == ord('2'):
            active_classes = [2]
            mode_text = "Zielony"
        elif key == ord('3'):
            active_classes = [3]
            mode_text = "Żółty"

finally:
    cv2.destroyAllWindows()
    picam2.stop()
    picam2.close()