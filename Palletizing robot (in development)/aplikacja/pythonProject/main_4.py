import tkinter as tk
from tkinter import ttk
import serial
import threading
import time
import re

# --- Serial configuration ---
try:
    ser = serial.Serial('COM12', 115200, timeout=1)
    time.sleep(2)
    print("Serial port opened successfully")
    print("Waiting for encoder data from Arduino...")
    print("-" * 50)
except Exception as e:
    print(f"Error opening serial port: {e}")
    exit()

# --- Control variables ---
MAX_VALUE = 40950
motor_targets = [20000, 20000, 20000, 20000, 20000]
encoder_values = [0, 0, 0, 0, 0]
lock = threading.Lock()
running = True

# Змінні для утримання кнопок
button_pressed = [None, None]
button_hold_delay = 0.1
last_button_time = 0

# Змінна для запобігання рекурсії слайдера
slider_updating = [False, False, False, False, False]

# Режим калібрування
calibration_mode = False
motor_limits = [[0, MAX_VALUE] for _ in range(5)]  # [min, max] для кожного мотора


# --- Serial sender function ---
def serial_sender():
    global motor_targets, button_pressed, last_button_time, calibration_mode

    last_sent = [None] * 5

    while running:
        current_time = time.time()

        with lock:
            current_targets = motor_targets.copy()
            current_button = button_pressed.copy()
            current_calibration = calibration_mode

        # Обробка утримання кнопок
        if current_button[0] is not None and current_time - last_button_time >= button_hold_delay:
            motor_index, direction = current_button
            step = 100

            new_value = current_targets[motor_index] + (step * direction)

            # В режимі калібрування не обмежуємо значення
            if not current_calibration:
                new_value = max(motor_limits[motor_index][0], min(motor_limits[motor_index][1], new_value))

            with lock:
                motor_targets[motor_index] = new_value

            # Оновлюємо GUI
            root.after(0, update_motor_display, motor_index, new_value)

            last_button_time = current_time

        # Відправляємо команди
        for i in range(5):
            if current_targets[i] != last_sent[i]:
                command = f"motor{i + 1} {current_targets[i]}"
                ser.write((command + "\n").encode())
                print(f"Sent command: '{command}'")
                last_sent[i] = current_targets[i]

        time.sleep(0.05)


# --- Serial receiver function ---
def serial_receiver():
    global encoder_values, motor_limits

    while running:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    print(line)

                    # Оновлюємо межі з повідомлень Arduino
                    if "min limit set to:" in line:
                        match = re.search(r'Motor\s+(\d+)\s+min limit set to:\s+(\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            min_val = int(match.group(2))
                            if 0 <= motor_num < 5:
                                with lock:
                                    motor_limits[motor_num][0] = min_val
                                root.after(0, update_slider_limits, motor_num)

                    elif "max limit set to:" in line:
                        match = re.search(r'Motor\s+(\d+)\s+max limit set to:\s+(\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            max_val = int(match.group(2))
                            if 0 <= motor_num < 5:
                                with lock:
                                    motor_limits[motor_num][1] = max_val
                                root.after(0, update_slider_limits, motor_num)

        except Exception as e:
            print(f"Error reading serial: {e}")

        time.sleep(0.01)


# --- Функції для керування моторами ---
def update_motor_target(motor_index, new_value, update_slider=True):
    global motor_targets
    # В режимі калібрування не обмежуємо значення
    if not calibration_mode:
        new_value = max(motor_limits[motor_index][0], min(motor_limits[motor_index][1], new_value))

    with lock:
        motor_targets[motor_index] = new_value

    # Оновлюємо лейбл
    value_labels[motor_index].config(text=str(new_value))

    # Оновлюємо слайдер тільки якщо не викликано з слайдера
    if update_slider and not slider_updating[motor_index]:
        slider_updating[motor_index] = True
        sliders[motor_index].set(new_value)
        slider_updating[motor_index] = False


def on_slider_change(motor_index, value):
    if not calibration_mode and not slider_updating[motor_index]:
        slider_updating[motor_index] = True
        new_value = int(float(value))
        update_motor_target(motor_index, new_value, update_slider=False)
        slider_updating[motor_index] = False


def start_increment(motor_index):
    global button_pressed, last_button_time
    button_pressed = [motor_index, 1]
    last_button_time = time.time()
    increment_motor(motor_index)


def start_decrement(motor_index):
    global button_pressed, last_button_time
    button_pressed = [motor_index, -1]
    last_button_time = time.time()
    decrement_motor(motor_index)


def stop_button():
    global button_pressed
    button_pressed = [None, None]


def increment_motor(motor_index):
    current = motor_targets[motor_index]
    step = 100
    new_value = current + step

    # В режимі калібрування не обмежуємо
    if not calibration_mode:
        new_value = min(motor_limits[motor_index][1], new_value)

    update_motor_target(motor_index, new_value)


def decrement_motor(motor_index):
    current = motor_targets[motor_index]
    step = 100
    new_value = current - step

    # В режимі калібрування не обмежуємо
    if not calibration_mode:
        new_value = max(motor_limits[motor_index][0], new_value)

    update_motor_target(motor_index, new_value)


def update_encoder_display(motor_index, value):
    encoder_labels[motor_index].config(text=f"Encoder: {value}")


def update_motor_display(motor_index, value):
    update_motor_target(motor_index, value)


def update_slider_limits(motor_index):
    """Оновлює межі слайдера при зміні калібрування"""
    sliders[motor_index].config(from_=motor_limits[motor_index][0], to=motor_limits[motor_index][1])


# --- Функції калібрування ---
def start_calibration():
    global calibration_mode
    calibration_mode = True
    ser.write(b"calibration start\n")
    calibration_btn.config(text="Stop Calibration", bg="red")
    status_label.config(text="CALIBRATION MODE - Limits disabled", fg="red")
    print("Calibration mode started")


def stop_calibration():
    global calibration_mode
    calibration_mode = False
    ser.write(b"calibration stop\n")
    calibration_btn.config(text="Start Calibration", bg="lightblue")
    status_label.config(text="Normal mode - Limits enabled", fg="green")
    print("Calibration mode stopped")


def set_min_limit(motor_index):
    ser.write(f"calibration set_min {motor_index + 1}\n".encode())
    print(f"Setting min limit for motor {motor_index + 1}")


def set_max_limit(motor_index):
    ser.write(f"calibration set_max {motor_index + 1}\n".encode())
    print(f"Setting max limit for motor {motor_index + 1}")


# --- GUI ---
root = tk.Tk()
root.title("5 Motors Control with Calibration")
root.geometry("750x700")

button_font = ("Arial", 10)
sliders = []
value_labels = []
encoder_labels = []
min_buttons = []
max_buttons = []


def create_motor_controls(motor_number, row):
    frame = tk.Frame(root, relief=tk.GROOVE, bd=2, padx=10, pady=8)
    frame.grid(row=row, column=0, columnspan=6, padx=10, pady=5, sticky="ew")

    # Заголовок мотора
    title_frame = tk.Frame(frame)
    title_frame.grid(row=0, column=0, columnspan=6, pady=(0, 8))
    tk.Label(title_frame, text=f"MOTOR {motor_number}", font=("Arial", 11, "bold")).pack()

    # Кнопка "-"
    minus_btn = tk.Button(frame, text="-", font=button_font, width=3, height=1, bg="lightcoral")
    minus_btn.grid(row=1, column=0, padx=2)

    # Повзунок
    slider = ttk.Scale(frame, from_=0, to=MAX_VALUE, orient=tk.HORIZONTAL, length=180)
    slider.set(motor_targets[motor_number - 1])
    slider.grid(row=1, column=1, padx=5, sticky="ew")
    sliders.append(slider)

    # Кнопка "+"
    plus_btn = tk.Button(frame, text="+", font=button_font, width=3, height=1, bg="lightgreen")
    plus_btn.grid(row=1, column=2, padx=2)

    # Лейбл з цільовим значенням
    value_label = tk.Label(frame, text=str(motor_targets[motor_number - 1]),
                           font=("Arial", 11, "bold"), width=6, fg="blue")
    value_label.grid(row=1, column=3, padx=5)
    value_labels.append(value_label)

    # Кнопки калібрування
    min_btn = tk.Button(frame, text="Set MIN", font=("Arial", 8), width=6, height=1,
                        bg="orange", command=lambda: set_min_limit(motor_number - 1))
    min_btn.grid(row=1, column=4, padx=2)
    min_buttons.append(min_btn)

    max_btn = tk.Button(frame, text="Set MAX", font=("Arial", 8), width=6, height=1,
                        bg="yellow", command=lambda: set_max_limit(motor_number - 1))
    max_btn.grid(row=1, column=5, padx=2)
    max_buttons.append(max_btn)

    # Лейбл з значенням енкодера
    encoder_label = tk.Label(frame, text="Encoder: 0", font=("Arial", 9), fg="green")
    encoder_label.grid(row=2, column=0, columnspan=6, pady=(6, 0))
    encoder_labels.append(encoder_label)

    # Прив'язка подій
    motor_index = motor_number - 1
    slider.configure(command=lambda v: on_slider_change(motor_index, v))
    plus_btn.bind("<ButtonPress>", lambda e: start_increment(motor_index))
    plus_btn.bind("<ButtonRelease>", lambda e: stop_button())
    minus_btn.bind("<ButtonPress>", lambda e: start_decrement(motor_index))
    minus_btn.bind("<ButtonRelease>", lambda e: stop_button())


# Створюємо контроли для 5 моторів
for i in range(1, 6):
    create_motor_controls(i, i - 1)

# Кнопка калібрування
calibration_btn = tk.Button(root, text="Start Calibration", font=("Arial", 12, "bold"),
                            bg="lightblue", command=start_calibration, width=20, height=2)
calibration_btn.grid(row=5, column=0, columnspan=3, pady=10)

# Інструкція
instruction = tk.Label(root, text="Hold +/- buttons to change targets. Calibration mode disables limits.",
                       font=("Arial", 10), fg="blue", pady=8)
instruction.grid(row=6, column=0, columnspan=6)

# Статус
status_label = tk.Label(root, text="Normal mode - Limits enabled", font=("Arial", 9), fg="green")
status_label.grid(row=7, column=0, columnspan=6)

# Start serial threads
sender_thread = threading.Thread(target=serial_sender)
sender_thread.daemon = True
sender_thread.start()

receiver_thread = threading.Thread(target=serial_receiver)
receiver_thread.daemon = True
receiver_thread.start()


def on_closing():
    global running
    running = False
    time.sleep(0.1)
    ser.write(b"calibration stop\n")  # Виходимо з режиму калібрування
    ser.write(b"\n")
    time.sleep(0.2)
    ser.close()
    print("Serial port closed")
    root.destroy()


root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()