import tkinter as tk
from tkinter import ttk
import serial
import threading
import time
import re

# ---------- CONFIG ----------
SERIAL_PORT = "COM12"  # adjust to your port
BAUDRATE = 115200
NUM_MOTORS = 5
MAX_VALUE = 40950

# ---------- GLOBALS ----------
ser = None
running = True

motor_targets = [0] * NUM_MOTORS
encoder_values = [0] * NUM_MOTORS
motor_limits = [[0, MAX_VALUE] for _ in range(NUM_MOTORS)]
calibration_mode = False
diagnostic_mode = False  # Новий режим для діагностики

lock = threading.Lock()

targets_initialized = [False] * NUM_MOTORS
limits_initialized = [False] * NUM_MOTORS
initial_sync_completed = False
initial_slider_set = [False] * NUM_MOTORS

# Змінні для автоповтору кнопок
button_repeat_delay = 300  # мс до початку автоповтору
button_repeat_interval = 50  # мс між повтореннями
button_repeat_jobs = {}  # Словник для зберігання ID повторень

# ---------- OPEN SERIAL ----------
try:
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=0.1)
    time.sleep(1.0)
    print("Serial opened:", SERIAL_PORT)
except Exception as e:
    print("Cannot open serial port:", e)
    ser = None

# ---------- GUI SETUP ----------
root = tk.Tk()
root.title("5 Motors Control (Calibration ready)")
root.geometry("820x720")

sliders = []
value_labels = []
encoder_labels = []
min_labels = []
max_labels = []


def absolute_to_relative(idx, absolute_value):
    """Перетворює абсолютне значення у відносне (відносно min_limit)"""
    return absolute_value - motor_limits[idx][0]


def relative_to_absolute(idx, relative_value):
    """Перетворює відносне значення у абсолютне"""
    return relative_value + motor_limits[idx][0]


def set_slider_position(idx, absolute_position):
    """Встановлює позицію повзунка у відносних значеннях"""
    try:
        relative_pos = absolute_to_relative(idx, absolute_position)
        sliders[idx].set(relative_pos)
    except Exception as e:
        print(f"Error setting slider {idx + 1}: {e}")


def get_slider_absolute_value(idx):
    """Отримує абсолютне значення з повзунка"""
    relative_val = sliders[idx].get()
    return relative_to_absolute(idx, relative_val)


def update_encoder_display(idx, absolute_value):
    """Оновлює відображення енкодера у відносних значеннях"""
    relative_value = absolute_to_relative(idx, absolute_value)
    encoder_labels[idx].config(text=f"Encoder: {int(relative_value)}")


def update_target_display(idx, absolute_value):
    """Оновлює відображення цілі у відносних значеннях"""
    relative_value = absolute_to_relative(idx, absolute_value)
    value_labels[idx].config(text=str(int(relative_value)))


def update_slider_limits(idx):
    """Оновлює межі повзунка у відносних значеннях"""
    min_abs, max_abs = motor_limits[idx]

    # Відносні межі: завжди від 0 до (max - min)
    relative_min = 0
    relative_max = max_abs - min_abs

    try:
        sliders[idx].config(from_=relative_min, to=relative_max)
    except:
        pass

    # Перевіряємо чи поточне значення в межах (тільки в нормальному режимі)
    if not (calibration_mode or diagnostic_mode):
        cur_abs = motor_targets[idx]
        if cur_abs < min_abs:
            with lock:
                motor_targets[idx] = min_abs
            set_slider_position(idx, min_abs)
            update_target_display(idx, min_abs)
        elif cur_abs > max_abs:
            with lock:
                motor_targets[idx] = max_abs
            set_slider_position(idx, max_abs)
            update_target_display(idx, max_abs)


def update_range_labels(idx):
    """Оновлює відображення меж у відносних значеннях"""
    min_abs, max_abs = motor_limits[idx]

    # Відносні значення для відображення
    relative_min = 0
    relative_max = max_abs - min_abs

    min_labels[idx].config(text=str(int(relative_min)))
    max_labels[idx].config(text=str(int(relative_max)))


def set_calibration_ui(is_cal):
    global calibration_mode
    calibration_mode = is_cal
    if is_cal:
        cal_btn.config(text="Stop Calibration")
        for s in sliders: s.state(['disabled'])
        for b in plus_buttons + minus_buttons + setmin_buttons + setmax_buttons:
            b.config(state='normal')
        status_label.config(text="CALIBRATION MODE", fg="red")
    else:
        cal_btn.config(text="Start Calibration")
        for s in sliders: s.state(['!disabled'])
        for b in plus_buttons + minus_buttons + setmin_buttons + setmax_buttons:
            b.config(state='disabled')
        status_label.config(text="Normal mode", fg="green")


def set_diagnostic_ui(is_diag):
    global diagnostic_mode
    diagnostic_mode = is_diag
    if is_diag:
        diag_btn.config(text="Stop Diagnostic")
        for s in sliders: s.state(['!disabled'])
        for b in plus_buttons + minus_buttons + setmin_buttons + setmax_buttons:
            b.config(state='normal')
        status_label.config(text="DIAGNOSTIC MODE", fg="orange")
    else:
        diag_btn.config(text="Start Diagnostic")
        for s in sliders: s.state(['!disabled'])
        for b in plus_buttons + minus_buttons + setmin_buttons + setmax_buttons:
            b.config(state='disabled')
        status_label.config(text="Normal mode", fg="green")


# ---------- ФУНКЦІЇ ДЛЯ КНОПОК "+" І "-" ----------
def plus_press(idx):
    """Обробка натискання кнопки '+'"""
    if calibration_mode or diagnostic_mode:
        # У режимі калібрування або діагностики можна виходити за межі
        with lock:
            motor_targets[idx] += 100
    else:
        # У нормальному режимі не виходимо за межі
        with lock:
            motor_targets[idx] += 100
            if motor_targets[idx] > motor_limits[idx][1]:
                motor_targets[idx] = motor_limits[idx][1]

    update_target_display(idx, motor_targets[idx])
    set_slider_position(idx, motor_targets[idx])


def minus_press(idx):
    """Обробка натискання кнопки '-'"""
    if calibration_mode or diagnostic_mode:
        # У режимі калібрування або діагностики можна виходити за межі
        with lock:
            motor_targets[idx] -= 100
    else:
        # У нормальному режимі не виходимо за межі
        with lock:
            motor_targets[idx] -= 100
            if motor_targets[idx] < motor_limits[idx][0]:
                motor_targets[idx] = motor_limits[idx][0]

    update_target_display(idx, motor_targets[idx])
    set_slider_position(idx, motor_targets[idx])


def start_repeat(idx, is_plus):
    """Початок автоповтору при утриманні кнопки"""
    if is_plus:
        plus_press(idx)
    else:
        minus_press(idx)

    # Зберігаємо ID завдання для подальшого скасування
    job_id = root.after(button_repeat_interval, lambda: start_repeat(idx, is_plus))
    button_repeat_jobs[(idx, is_plus)] = job_id


def stop_repeat(idx, is_plus):
    """Зупинка автоповтору при відпусканні кнопки"""
    job_key = (idx, is_plus)
    if job_key in button_repeat_jobs:
        root.after_cancel(button_repeat_jobs[job_key])
        del button_repeat_jobs[job_key]


def create_button_events(button, idx, is_plus):
    """Створює події для кнопки з автоповтором"""
    button.bind('<ButtonPress-1>',
                lambda e: root.after(button_repeat_delay,
                                     lambda: start_repeat(idx, is_plus)))
    button.bind('<ButtonRelease-1>',
                lambda e: stop_repeat(idx, is_plus))


# ---------- SERIAL SENDER ----------
def serial_sender():
    global running, motor_targets, calibration_mode, initial_sync_completed
    last_sent = [None] * NUM_MOTORS
    while running:
        with lock:
            cur_targets = motor_targets.copy()
        if initial_sync_completed and ser:
            for i in range(NUM_MOTORS):
                if last_sent[i] != cur_targets[i]:
                    cmd = f"motor{i + 1} {cur_targets[i]}\n"
                    try:
                        ser.write(cmd.encode())
                    except Exception as e:
                        print("Serial write error:", e)
                    last_sent[i] = cur_targets[i]
        time.sleep(0.03)


# ---------- SERIAL RECEIVER ----------
def serial_receiver():
    global running, calibration_mode, motor_limits, motor_targets, targets_initialized, initial_sync_completed, limits_initialized, initial_slider_set

    def process_motor_data(idx, min_val, max_val, target_val, actual_val):
        if 0 <= idx < NUM_MOTORS:
            with lock:
                motor_limits[idx][0] = min_val  # Може бути від'ємним!
                motor_limits[idx][1] = max_val
                if not targets_initialized[idx]:
                    motor_targets[idx] = target_val
                    encoder_values[idx] = actual_val
                    targets_initialized[idx] = True

                    # ВСТАНОВЛЮЄМО ПОВЗУНОК ТІЛЬКИ ПРИ СТАРТІ
                    if not initial_slider_set[idx]:
                        root.after(0, set_slider_position, idx, target_val)
                        initial_slider_set[idx] = True

                limits_initialized[idx] = True

            # Оновлюємо відображення
            root.after(0, update_slider_limits, idx)
            root.after(0, update_range_labels, idx)
            root.after(0, update_target_display, idx, motor_targets[idx])
            root.after(0, update_encoder_display, idx, actual_val)

            print(f"Motor {idx + 1} data: min={min_val}, max={max_val}, target={target_val}, actual={actual_val}")

    while running and ser:
        try:
            line = ser.readline().decode(errors='ignore').strip()
            if not line:
                time.sleep(0.01)
                continue
            print("RX:", line)

            if line.startswith("MOTOR_DATA:"):
                parts = line.split(':')
                if len(parts) >= 6:
                    idx = int(parts[1]) - 1
                    min_val = int(parts[2])
                    max_val = int(parts[3])
                    target_val = int(parts[4])
                    actual_val = int(parts[5])
                    process_motor_data(idx, min_val, max_val, target_val, actual_val)

            elif line.startswith("LOG_MIN:"):
                parts = line.split(':')
                if len(parts) >= 3:
                    idx = int(parts[1]) - 1
                    min_val = int(parts[2])
                    motor_limits[idx][0] = min_val  # Може бути від'ємним!
                    limits_initialized[idx] = True
                    root.after(0, update_slider_limits, idx)
                    root.after(0, update_range_labels, idx)

            elif line.startswith("LOG_MAX:"):
                parts = line.split(':')
                if len(parts) >= 3:
                    idx = int(parts[1]) - 1
                    max_val = int(parts[2])
                    motor_limits[idx][1] = max_val
                    limits_initialized[idx] = True
                    root.after(0, update_slider_limits, idx)
                    root.after(0, update_range_labels, idx)

            elif line.startswith("LOG_POS:"):
                parts = line.split(':')
                if len(parts) >= 3:
                    idx = int(parts[1]) - 1
                    pos_val = int(parts[2])
                    if not targets_initialized[idx]:
                        with lock:
                            motor_targets[idx] = pos_val
                            encoder_values[idx] = pos_val
                            targets_initialized[idx] = True
                        # Встановлюємо позицію повзунка ТІЛЬКИ ПРИ СТАРТІ
                        if not initial_slider_set[idx]:
                            root.after(0, set_slider_position, idx, pos_val)
                            initial_slider_set[idx] = True
                        root.after(0, update_target_display, idx, pos_val)

            elif line.startswith("MOTOR_STATUS:"):
                parts = line.split(':')
                if len(parts) >= 3:
                    idx = int(parts[1]) - 1
                    target_val = int(parts[2])
                    actual_val = int(parts[3])
                    with lock:
                        encoder_values[idx] = actual_val
                    root.after(0, update_encoder_display, idx, actual_val)

            elif line.startswith("MIN_SET:"):
                parts = line.split(':')
                if len(parts) >= 3:
                    idx = int(parts[1]) - 1
                    min_val = int(parts[2])
                    motor_limits[idx][0] = min_val
                    root.after(0, update_slider_limits, idx)
                    root.after(0, update_range_labels, idx)

            elif line.startswith("MAX_SET:"):
                parts = line.split(':')
                if len(parts) >= 3:
                    idx = int(parts[1]) - 1
                    max_val = int(parts[2])
                    motor_limits[idx][1] = max_val
                    root.after(0, update_slider_limits, idx)
                    root.after(0, update_range_labels, idx)

            elif line == "CALIBRATION_STARTED":
                calibration_mode = True
                root.after(0, set_calibration_ui, True)
            elif line == "CALIBRATION_STOPPED":
                calibration_mode = False
                root.after(0, set_calibration_ui, False)
            elif line == "SYSTEM_READY":
                root.after(0, lambda: status_label.config(text="System Ready - Synchronizing...", fg="orange"))

            # Перевіряємо завершення синхронізації
            if (not initial_sync_completed) and all(targets_initialized) and all(limits_initialized):
                initial_sync_completed = True
                print("Initial sync completed.")
                root.after(0, lambda: status_label.config(text="SYNC COMPLETED - Normal operation", fg="blue"))

        except Exception as e:
            print("Serial read error:", e)
        time.sleep(0.01)


# ---------- UI CREATION ----------
frame = ttk.Frame(root)
frame.pack(padx=10, pady=10, fill='x')

for i in range(NUM_MOTORS):
    row = ttk.Frame(frame)
    row.pack(fill='x', pady=6)

    ttk.Label(row, text=f"Motor {i + 1}", width=8).pack(side='left')

    # Повзунок працює у відносних значеннях (завжди від 0 до max-min)
    s = ttk.Scale(row, from_=0, to=MAX_VALUE, orient='horizontal', length=350,
                  command=lambda val, idx=i: slider_changed(idx, val))
    s.pack(side='left', padx=6)
    sliders.append(s)

    vlab = ttk.Label(row, text="0", width=7, foreground='blue')
    vlab.pack(side='left', padx=6)
    value_labels.append(vlab)

    elab = ttk.Label(row, text="Encoder: 0", width=15, foreground='green')
    elab.pack(side='left', padx=6)
    encoder_labels.append(elab)

    rframe = ttk.Frame(root)
    rframe.pack(fill='x')
    min_l = ttk.Label(rframe, text="0", width=8)  # Завжди показуємо 0
    min_l.pack(side='left', padx=10)
    max_l = ttk.Label(rframe, text=str(MAX_VALUE), width=8)  # Максимум буде оновлено
    max_l.pack(side='right', padx=50)
    min_labels.append(min_l)
    max_labels.append(max_l)

# Calibration controls
cal_frame = ttk.LabelFrame(root, text="Calibration controls")
cal_frame.pack(fill='x', padx=10, pady=10)

plus_buttons = []
minus_buttons = []
setmin_buttons = []
setmax_buttons = []


def slider_changed(idx, relative_val):
    """Викликається при ручному переміщенні повзунка"""
    global motor_targets
    if calibration_mode or diagnostic_mode:
        return

    # Перетворюємо відносне значення в абсолютне
    absolute_val = relative_to_absolute(idx, int(float(relative_val)))
    with lock:
        motor_targets[idx] = absolute_val
    update_target_display(idx, absolute_val)


def do_set_min(idx):
    if ser:
        cmd = f"calibration set_min {idx + 1}\n"
        ser.write(cmd.encode())
        print("Sent:", cmd.strip())


def do_set_max(idx):
    if ser:
        cmd = f"calibration set_max {idx + 1}\n"
        ser.write(cmd.encode())
        print("Sent:", cmd.strip())


for i in range(NUM_MOTORS):
    r = ttk.Frame(cal_frame)
    r.pack(fill='x', pady=4)

    b_minus = ttk.Button(r, text="-", width=3)
    b_minus.pack(side='left', padx=6)
    minus_buttons.append(b_minus)
    create_button_events(b_minus, i, False)

    b_plus = ttk.Button(r, text="+", width=3)
    b_plus.pack(side='left', padx=6)
    plus_buttons.append(b_plus)
    create_button_events(b_plus, i, True)

    b_min = ttk.Button(r, text="Set MIN", command=lambda idx=i: do_set_min(idx))
    b_min.pack(side='left', padx=8)
    setmin_buttons.append(b_min)

    b_max = ttk.Button(r, text="Set MAX", command=lambda idx=i: do_set_max(idx))
    b_max.pack(side='left', padx=8)
    setmax_buttons.append(b_max)

for b in plus_buttons + minus_buttons + setmin_buttons + setmax_buttons:
    b.config(state='disabled')

ctrl_frame = ttk.Frame(root)
ctrl_frame.pack(fill='x', padx=10, pady=8)


def toggle_calibration():
    global calibration_mode, diagnostic_mode
    if ser is None:
        print("No serial")
        return
    if not calibration_mode:
        # Вимикаємо діагностичний режим при включенні калібрування
        if diagnostic_mode:
            set_diagnostic_ui(False)
        ser.write(b"calibration start\n")
        set_calibration_ui(True)
    else:
        ser.write(b"calibration stop\n")
        set_calibration_ui(False)


def toggle_diagnostic():
    global diagnostic_mode, calibration_mode
    if ser is None:
        print("No serial")
        return
    if not diagnostic_mode:
        # Вимикаємо режим калібрування при включенні діагностики
        if calibration_mode:
            set_calibration_ui(False)
        set_diagnostic_ui(True)
    else:
        set_diagnostic_ui(False)


cal_btn = ttk.Button(ctrl_frame, text="Start Calibration", command=toggle_calibration)
cal_btn.pack(side='left', padx=6)

diag_btn = ttk.Button(ctrl_frame, text="Start Diagnostic", command=toggle_diagnostic)
diag_btn.pack(side='left', padx=6)

stop_btn = ttk.Button(ctrl_frame, text="Emergency STOP", command=lambda: ser.write(b"STOP\n") if ser else None)
stop_btn.pack(side='left', padx=6)

status_label = ttk.Label(ctrl_frame, text="CONNECTING...", foreground='orange')
status_label.pack(side='right', padx=8)

# ---------- THREADS ----------
sender_thread = threading.Thread(target=serial_sender, daemon=True)
sender_thread.start()

receiver_thread = threading.Thread(target=serial_receiver, daemon=True)
receiver_thread.start()


# Запитуємо дані при старті
def request_initial_data():
    if not ser: return
    time.sleep(0.5)
    for _ in range(5):
        try:
            ser.write(b"GET_POS\n")
        except:
            pass
        time.sleep(0.1)


if ser:
    threading.Thread(target=request_initial_data, daemon=True).start()


# ---------- WINDOW CLOSE ----------
def on_closing():
    global running
    running = False
    time.sleep(0.05)
    if ser:
        try:
            ser.write(b"calibration stop\n")
            time.sleep(0.05)
            ser.close()
        except:
            pass
    root.destroy()


root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()