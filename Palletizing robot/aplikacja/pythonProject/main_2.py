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
MAX_VALUE = 40960
motor_targets = [20000, 20000]
encoder_values = [0, 0]
lock = threading.Lock()
running = True

# Змінні для утримання кнопок
button_pressed = [None, None]
button_hold_delay = 0.1
last_button_time = 0

# Змінна для запобігання рекурсії слайдера
slider_updating = [False, False]


# --- Serial sender function ---
def serial_sender():
    global motor_targets, button_pressed, last_button_time

    last_sent = [None, None]

    while running:
        current_time = time.time()

        with lock:
            current_targets = motor_targets.copy()
            current_button = button_pressed.copy()

        # Обробка утримання кнопок
        if current_button[0] is not None and current_time - last_button_time >= button_hold_delay:
            motor_index, direction = current_button
            step = 100

            new_value = current_targets[motor_index] + (step * direction)
            new_value = max(0, min(MAX_VALUE, new_value))

            with lock:
                motor_targets[motor_index] = new_value

            # Оновлюємо GUI
            root.after(0, update_motor_display, motor_index, new_value)

            last_button_time = current_time

        # Відправляємо команди
        for i in range(2):
            if current_targets[i] != last_sent[i]:
                command = f"motor{i + 1} {current_targets[i]}"
                ser.write((command + "\n").encode())
                print(f"Sent command: '{command}'")
                last_sent[i] = current_targets[i]

        time.sleep(0.05)


# --- Serial receiver function ---
def serial_receiver():
    global encoder_values

    while running:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    print(line)

                    if line.startswith("Motor") and "Target:" in line:
                        match = re.search(r'Motor\s+(\d+)\s+\|\s+Target:\s+(\d+)\s+\|\s+Actual:\s+(\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            actual_val = int(match.group(3))
                            if 0 <= motor_num < 2:
                                with lock:
                                    encoder_values[motor_num] = actual_val
                                root.after(0, update_encoder_display, motor_num, actual_val)

        except Exception as e:
            print(f"Error reading serial: {e}")

        time.sleep(0.01)


# --- Функції для керування моторами ---
def update_motor_target(motor_index, new_value, update_slider=True):
    """Оновлює цільове значення мотора"""
    global motor_targets
    new_value = max(0, min(MAX_VALUE, new_value))
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
    """Обробник зміни слайдера"""
    # Запобігаємо рекурсії
    if not slider_updating[motor_index]:
        slider_updating[motor_index] = True
        new_value = int(float(value))
        update_motor_target(motor_index, new_value, update_slider=False)
        slider_updating[motor_index] = False


def start_increment(motor_index):
    """Початок утримання кнопки '+'"""
    global button_pressed, last_button_time
    button_pressed = [motor_index, 1]
    last_button_time = time.time()
    increment_motor(motor_index)


def start_decrement(motor_index):
    """Початок утримання кнопки '-'"""
    global button_pressed, last_button_time
    button_pressed = [motor_index, -1]
    last_button_time = time.time()
    decrement_motor(motor_index)


def stop_button():
    """Зупинка утримання кнопки"""
    global button_pressed
    button_pressed = [None, None]


def increment_motor(motor_index):
    """Збільшення значення на крок"""
    current = motor_targets[motor_index]
    if current < MAX_VALUE:
        step = 100
        new_value = min(MAX_VALUE, current + step)
        update_motor_target(motor_index, new_value)


def decrement_motor(motor_index):
    """Зменшення значення на крок"""
    current = motor_targets[motor_index]
    if current > 0:
        step = 100
        new_value = max(0, current - step)
        update_motor_target(motor_index, new_value)


def update_encoder_display(motor_index, value):
    """Оновлює відображення значень енкодерів"""
    encoder_labels[motor_index].config(text=f"Encoder: {value}")


def update_motor_display(motor_index, value):
    """Оновлює відображення цільових значень"""
    update_motor_target(motor_index, value)


# --- GUI ---
root = tk.Tk()
root.title("Motor Control with Hold Buttons")
root.geometry("600x450")

button_font = ("Arial", 10)
sliders = []
value_labels = []
encoder_labels = []


def create_motor_controls(motor_number, row):
    frame = tk.Frame(root, relief=tk.GROOVE, bd=2, padx=10, pady=10)
    frame.grid(row=row, column=0, columnspan=4, padx=10, pady=8, sticky="ew")

    # Заголовок мотора
    title_frame = tk.Frame(frame)
    title_frame.grid(row=0, column=0, columnspan=5, pady=(0, 10))
    tk.Label(title_frame, text=f"MOTOR {motor_number}",
             font=("Arial", 12, "bold")).pack()

    # Кнопка "-"
    minus_btn = tk.Button(frame, text="-", font=button_font, width=4, height=1,
                          bg="lightcoral")
    minus_btn.grid(row=1, column=0, padx=5)

    # Повзунок
    slider = ttk.Scale(frame, from_=0, to=MAX_VALUE, orient=tk.HORIZONTAL,
                       length=250)
    slider.set(motor_targets[motor_number - 1])
    slider.grid(row=1, column=1, padx=10, sticky="ew")
    sliders.append(slider)

    # Кнопка "+"
    plus_btn = tk.Button(frame, text="+", font=button_font, width=4, height=1,
                         bg="lightgreen")
    plus_btn.grid(row=1, column=2, padx=5)

    # Лейбл з цільовим значенням
    value_label = tk.Label(frame, text=str(motor_targets[motor_number - 1]),
                           font=("Arial", 12, "bold"), width=6, fg="blue")
    value_label.grid(row=1, column=3, padx=10)
    value_labels.append(value_label)

    # Лейбл з значенням енкодера
    encoder_label = tk.Label(frame, text="Encoder: 0",
                             font=("Arial", 10), fg="green")
    encoder_label.grid(row=2, column=0, columnspan=4, pady=(10, 0))
    encoder_labels.append(encoder_label)

    # Лейбл з мітками мінімуму та максимуму
    range_frame = tk.Frame(frame)
    range_frame.grid(row=3, column=1, sticky="ew", pady=(5, 0))
    tk.Label(range_frame, text="0", font=("Arial", 8)).pack(side=tk.LEFT)
    tk.Label(range_frame, text=f"{MAX_VALUE}", font=("Arial", 8)).pack(side=tk.RIGHT)

    # Прив'язка подій
    motor_index = motor_number - 1

    # Прив'язка слайдера
    slider.configure(command=lambda v: on_slider_change(motor_index, v))

    # Кнопки
    plus_btn.bind("<ButtonPress>", lambda e: start_increment(motor_index))
    plus_btn.bind("<ButtonRelease>", lambda e: stop_button())

    minus_btn.bind("<ButtonPress>", lambda e: start_decrement(motor_index))
    minus_btn.bind("<ButtonRelease>", lambda e: stop_button())


# Створюємо контроли для 2 моторів
for i in range(1, 3):
    create_motor_controls(i, i - 1)

# Інструкція
instruction = tk.Label(root, text="Hold +/- buttons to continuously change target values",
                       font=("Arial", 10), fg="blue", pady=10)
instruction.grid(row=3, column=0, columnspan=4)

# Статус
status_label = tk.Label(root, text="Buttons work while holding. Slider for precise control.",
                        font=("Arial", 9), fg="green")
status_label.grid(row=4, column=0, columnspan=4)

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
    ser.write("\n".encode())
    time.sleep(0.2)
    ser.close()
    print("Serial port closed")
    root.destroy()


root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()