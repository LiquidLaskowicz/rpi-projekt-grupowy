from ultralytics import YOLO
from picamera2 import Picamera2
import cv2
import time
import math


class YoloTracker:
    def __init__(
        self,
        model_path="/home/pi/yolo_wierzyczka/best.pt",
        frame_width=1280,
        frame_height=720,
        target_fps=20,
        imgsz=640,
        conf=0.7,
        detected_classes=None,
        class_name_map=None,
    ):
        self.frame_width = frame_width
        self.frame_height = frame_height
        self.target_fps = target_fps
        self.imgsz = imgsz
        self.conf = conf
        self.detected_classes = detected_classes

        # Punkt celowania
        self.targetX = self.frame_width // 2
        self.targetY = self.frame_height // 2

        self.model = YOLO(model_path)

        self.class_name_map = class_name_map or {
            "blue": 0,
            "pink": 1,
            "green": 2,
            "yellow": 3,
        }

        # Kamera
        self.picam2 = Picamera2()
        config = self.picam2.create_preview_configuration(
            main={"size": (self.frame_width, self.frame_height), "format": "RGB888"}
        )
        self.picam2.configure(config)
        self.picam2.start()

        # Hold state
        self.last_error_x = None
        self.last_error_y = None
        self.last_detection_time = 0.0

    def _capture_frame(self):
        # YOLO akceptuje RGB → brak konwersji = szybciej
        return self.picam2.capture_array()

    def _resolve_class_name(self, class_name):
        if class_name not in self.class_name_map:
            available = ", ".join(self.class_name_map.keys())
            raise ValueError(
                f"Nieznana nazwa klasy: '{class_name}'. Dostępne: {available}"
            )
        return self.class_name_map[class_name]

    # --- konfiguracja targetu ---

    def set_target_point(self, x, y):
        self.targetX = int(x)
        self.targetY = int(y)

    def set_all_classes(self):
        self.detected_classes = None

    def set_single_class(self, class_id):
        self.detected_classes = [int(class_id)]

    def set_classes(self, class_ids):
        if class_ids is None:
            self.detected_classes = None
        else:
            self.detected_classes = [int(x) for x in class_ids]

    def set_class_by_name(self, class_name):
        self.detected_classes = [self._resolve_class_name(class_name)]

    def set_classes_by_names(self, class_names):
        self.detected_classes = [
            self._resolve_class_name(name) for name in class_names
        ]

    # --- główna logika ---

    def get_nearest_target(self):
        frame = self._capture_frame()

        results = self.model(
            frame,
            imgsz=self.imgsz,
            classes=self.detected_classes,
            verbose=False,
            conf=self.conf,
        )

        boxes = results[0].boxes
        if len(boxes) == 0:
            return None

        nearest_target = None
        best_score = float("inf")

        for box in boxes:
            cls = int(box.cls[0])
            confidence = float(box.conf[0])

            x1, y1, x2, y2 = box.xyxy[0]
            cx = int((x1 + x2) / 2)
            cy = int((y1 + y2) / 2)

            error_x_px = self.targetX - cx
            error_y_px = self.targetY - cy
            distance_px = math.hypot(error_x_px, error_y_px)

            # 🔥 lepszy scoring: dystans + confidence
            score = distance_px / max(confidence, 1e-6)

            if score < best_score:
                best_score = score
                nearest_target = {
                    "class_id": cls,
                    "class_name": self.model.names[cls],
                    "confidence": confidence,
                    "cx": cx,
                    "cy": cy,
                    "error_x_px": error_x_px,
                    "error_y_px": error_y_px,
                    "distance_px": distance_px,
                }

        return nearest_target

    def get_pid_state_normalized(self, hold_time=0.2):
        target = self.get_nearest_target()
        now = time.perf_counter()

        if target is not None:
            error_x = target["error_x_px"] / (self.frame_width / 2)
            error_y = target["error_y_px"] / (self.frame_height / 2)

            # 🔥 clamp
            error_x = max(-1.0, min(1.0, error_x))
            error_y = max(-1.0, min(1.0, error_y))

            self.last_error_x = error_x
            self.last_error_y = error_y
            self.last_detection_time = now

            return {
                "detected": True,
                "held": False,
                "error_x": error_x,
                "error_y": error_y,
                "class_id": target["class_id"],
                "class_name": target["class_name"],
                "confidence": target["confidence"],
            }

        if (
            self.last_error_x is not None
            and (now - self.last_detection_time) <= hold_time
        ):
            return {
                "detected": False,
                "held": True,
                "error_x": self.last_error_x,
                "error_y": self.last_error_y,
                "class_id": None,
                "class_name": None,
                "confidence": None,
            }

        return {
            "detected": False,
            "held": False,
            "error_x": None,
            "error_y": None,
            "class_id": None,
            "class_name": None,
            "confidence": None,
        }

    def reset_hold(self):
        self.last_error_x = None
        self.last_error_y = None
        self.last_detection_time = 0.0

    def close(self):
        try:
            self.picam2.stop()
            self.picam2.close()
        finally:
            cv2.destroyAllWindows()