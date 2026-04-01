from yolo_tracker import YoloTracker
import time
import os

fifo_path = "/tmp/yolo_fifo"
if not os.path.exists(fifo_path):
    os.mkfifo(fifo_path)

tracker = YoloTracker(
    model_path="/home/pi/yolo_wierzyczka/best.pt",
    frame_width=1280,
    frame_height=720,
    target_fps=20,
    imgsz=640,
    conf=0.7,
    detected_classes=None,
)

selected_target = "blue"
last_selected_target = None

last_x = 0.0
last_y = 0.0

counter = 0

try:
    print("Czekam na C...")
    with open(fifo_path, "w") as fifo:
        print("Start!")

        while True:
            if selected_target != last_selected_target:
                if selected_target is None:
                    tracker.set_all_classes()
                else:
                    tracker.set_class_by_name(selected_target)

                tracker.reset_hold()
                last_selected_target = selected_target

            state = tracker.get_pid_state_normalized(hold_time=0.15)

            if state["detected"] or state["held"]:
                x = state["error_x"]
                y = state["error_y"]
                last_x, last_y = x, y
                status = 1
            else:
                x = last_x
                y = last_y
                status = 0

            counter += 1
            if counter % 10 == 0:
                print(f"[TRACK] status={status} x={x:.3f} y={y:.3f}")

            try:
                fifo.write(f"{status},{x},{y}\n")
                fifo.flush()
            except BrokenPipeError:
                print("C padło — kończę")
                break

            time.sleep(0.03)

except KeyboardInterrupt:
    pass

finally:
    tracker.close()