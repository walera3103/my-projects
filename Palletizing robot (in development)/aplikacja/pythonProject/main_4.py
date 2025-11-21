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
motor_targets = [0, 0, 0, 0, 0]
encoder_values = [0, 0, 0, 0, 0]
lock = threading.Lock()
running = True

# Variables for button holding
button_pressed = [None, None]
button_hold_delay = 0.1
last_button_time = 0

# Slider update prevention
slider_updating = [False, False, False, False, False]

# Calibration mode
calibration_mode = False
motor_limits = [[0, MAX_VALUE] for _ in range(5)]

# Flag for initial synchronization
targets_initialized = [False, False, False, False, False]
initial_sync_completed = False


# --- Serial sender function ---
def serial_sender():
    global motor_targets, button_pressed, last_button_time, calibration_mode, initial_sync_completed

    last_sent = [None] * 5

    while running:
        current_time = time.time()

        with lock:
            current_targets = motor_targets.copy()
            current_button = button_pressed.copy()
            current_calibration = calibration_mode

        # Handle button holding
        if current_button[0] is not None and current_time - last_button_time >= button_hold_delay:
            motor_index, direction = current_button
            step = 100

            new_value = current_targets[motor_index] + (step * direction)

            # In calibration mode, don't limit values
            if not current_calibration:
                new_value = max(motor_limits[motor_index][0], min(motor_limits[motor_index][1], new_value))

            with lock:
                motor_targets[motor_index] = new_value

            # Update GUI
            root.after(0, update_motor_display, motor_index, new_value)

            last_button_time = current_time

        # Send commands only after initial sync is completed
        if initial_sync_completed:
            for i in range(5):
                if current_targets[i] != last_sent[i]:
                    command = f"motor{i + 1} {current_targets[i]}"
                    ser.write((command + "\n").encode())
                    print(f"Sent command: '{command}'")
                    last_sent[i] = current_targets[i]

        time.sleep(0.05)


# --- Serial receiver function ---
def serial_receiver():
    global encoder_values, motor_limits, calibration_mode, motor_targets, targets_initialized, initial_sync_completed

    while running:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    print(line)

                    # Update limits from Arduino messages
                    if "min limit set to:" in line:
                        match = re.search(r'Motor\s+(\d+)\s+min limit set to:\s+(-?\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            min_val = int(match.group(2))
                            if 0 <= motor_num < 5:
                                with lock:
                                    motor_limits[motor_num][0] = min_val
                                root.after(0, update_slider_limits, motor_num)
                                root.after(0, update_range_labels, motor_num)

                    elif "max limit set to:" in line:
                        match = re.search(r'Motor\s+(\d+)\s+max limit set to:\s+(-?\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            max_val = int(match.group(2))
                            if 0 <= motor_num < 5:
                                with lock:
                                    motor_limits[motor_num][1] = max_val
                                root.after(0, update_slider_limits, motor_num)
                                root.after(0, update_range_labels, motor_num)

                    # Track calibration mode changes
                    elif "CALIBRATION MODE STARTED" in line:
                        with lock:
                            calibration_mode = True
                        root.after(0, update_calibration_ui, True)

                    elif "CALIBRATION MODE STOPPED" in line:
                        with lock:
                            calibration_mode = False
                        root.after(0, update_calibration_ui, False)

                    # Load limits at system start
                    elif "Loaded min limit for motor" in line:
                        match = re.search(r'Loaded min limit for motor\s+(\d+):\s+(-?\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            min_val = int(match.group(2))
                            if 0 <= motor_num < 5:
                                with lock:
                                    motor_limits[motor_num][0] = min_val
                                # Fix negative limits to 0 for safety
                                if min_val < 0:
                                    motor_limits[motor_num][0] = 0
                                    print(f"Fixed negative min limit for motor {motor_num + 1}")
                                root.after(0, update_slider_limits, motor_num)
                                root.after(0, update_range_labels, motor_num)

                    elif "Loaded max limit for motor" in line:
                        match = re.search(r'Loaded max limit for motor\s+(\d+):\s+(-?\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            max_val = int(match.group(2))
                            if 0 <= motor_num < 5:
                                with lock:
                                    motor_limits[motor_num][1] = max_val
                                    # Ensure max is reasonable
                                    if max_val < 100:
                                        motor_limits[motor_num][1] = MAX_VALUE
                                        print(f"Fixed invalid max limit for motor {motor_num + 1}")
                                root.after(0, update_slider_limits, motor_num)
                                root.after(0, update_range_labels, motor_num)

                    # Update encoder values and set targets from Arduino
                    elif line.startswith("Motor") and "| Target:" in line:
                        match = re.search(r'Motor\s+(\d+)\s+\|\s+Target:\s+(-?\d+)\s+\|\s+Actual:\s+(-?\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            target_val = int(match.group(2))
                            actual_val = int(match.group(3))
                            if 0 <= motor_num < 5:
                                with lock:
                                    encoder_values[motor_num] = actual_val

                                    # Initialize target from Arduino only once
                                    if not targets_initialized[motor_num]:
                                        # Use actual position as target to avoid sudden movements
                                        motor_targets[motor_num] = actual_val
                                        targets_initialized[motor_num] = True
                                        print(
                                            f"Motor {motor_num + 1} initialized with target: {actual_val} (from actual position)")

                                root.after(0, update_encoder_display, motor_num, actual_val)
                                root.after(0, update_motor_display, motor_num, motor_targets[motor_num])

                    # Track position loading at start
                    elif "Loaded position for motor" in line:
                        match = re.search(r'Loaded position for motor\s+(\d+):\s+(-?\d+)', line)
                        if match:
                            motor_num = int(match.group(1)) - 1
                            position_val = int(match.group(2))
                            if 0 <= motor_num < 5:
                                with lock:
                                    # Use actual position instead of loaded position to avoid conflicts
                                    if not targets_initialized[motor_num]:
                                        # We'll use the actual position from the next Motor X | Target: line
                                        print(
                                            f"Motor {motor_num + 1} position loaded: {position_val}, waiting for actual position...")

                    # Check if all motors are initialized
                    if not initial_sync_completed and all(targets_initialized):
                        initial_sync_completed = True
                        print("Initial synchronization completed - commands will now be sent to Arduino")
                        root.after(0, lambda: status_label.config(text="SYNC COMPLETED - Normal operation", fg="blue"))

        except Exception as e:
            print(f"Error reading serial: {e}")

        time.sleep(0.01)


# --- Motor control functions ---
def update_motor_target(motor_index, new_value, update_slider=True):
    global motor_targets
    # In calibration mode, don't limit values
    if not calibration_mode:
        new_value = max(motor_limits[motor_index][0], min(motor_limits[motor_index][1], new_value))

    with lock:
        motor_targets[motor_index] = new_value

    # Update label
    value_labels[motor_index].config(text=str(new_value))

    # Update slider only if not called from slider
    if update_slider and not slider_updating[motor_index]:
        slider_updating[motor_index] = True
        sliders[motor_index].set(new_value)
        slider_updating[motor_index] = False


def on_slider_change(motor_index, value):
    """Slider change handler considering calibration mode"""
    if not slider_updating[motor_index]:
        slider_updating[motor_index] = True
        new_value = int(float(value))

        # In calibration mode allow any values
        if calibration_mode:
            update_motor_target(motor_index, new_value, update_slider=False)
        else:
            # In normal mode limit values
            new_value = max(motor_limits[motor_index][0], min(motor_limits[motor_index][1], new_value))
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

    # In calibration mode don't limit
    if not calibration_mode:
        new_value = min(motor_limits[motor_index][1], new_value)

    update_motor_target(motor_index, new_value)


def decrement_motor(motor_index):
    current = motor_targets[motor_index]
    step = 100
    new_value = current - step

    # In calibration mode don't limit
    if not calibration_mode:
        new_value = max(motor_limits[motor_index][0], new_value)

    update_motor_target(motor_index, new_value)


def update_encoder_display(motor_index, value):
    encoder_labels[motor_index].config(text=f"Encoder: {value}")


def update_motor_display(motor_index, value):
    update_motor_target(motor_index, value)


def update_slider_limits(motor_index):
    """Update slider limits when calibration changes"""
    min_val = motor_limits[motor_index][0]
    max_val = motor_limits[motor_index][1]

    # Ensure reasonable limits
    if min_val < 0:
        min_val = 0
    if max_val < 100:
        max_val = MAX_VALUE

    sliders[motor_index].config(from_=min_val, to=max_val)

    # Update current value if it's out of bounds
    current_val = motor_targets[motor_index]
    if current_val < min_val:
        update_motor_target(motor_index, min_val)
    elif current_val > max_val:
        update_motor_target(motor_index, max_val)


def update_range_labels(motor_index):
    """Update min and max labels under slider"""
    min_val = motor_limits[motor_index][0]
    max_val = motor_limits[motor_index][1]

    # Fix negative values in display
    if min_val < 0:
        min_val = 0
    if max_val < 100:
        max_val = MAX_VALUE

    range_labels[motor_index][0].config(text=str(min_val))
    range_labels[motor_index][1].config(text=str(max_val))


def update_calibration_ui(is_calibration_mode):
    """Update interface when calibration mode changes"""
    global calibration_mode
    calibration_mode = is_calibration_mode

    if is_calibration_mode:
        calibration_btn.config(text="Calibration ACTIVE", bg="red", state="disabled")
        stop_calibration_btn.config(state="normal")
        status_label.config(text="CALIBRATION MODE - Limits disabled", fg="red")

        # Make calibration buttons more visible
        for btn in min_buttons + max_buttons:
            btn.config(state="normal", bg="orange")
    else:
        calibration_btn.config(text="Start Calibration", bg="lightblue", state="normal")
        stop_calibration_btn.config(state="disabled")
        status_label.config(text="Normal mode - Limits enabled", fg="green")

        # Return normal appearance to calibration buttons
        for btn in min_buttons:
            btn.config(bg="lightyellow")
        for btn in max_buttons:
            btn.config(bg="lightyellow")

        # Update all slider limits and fix any negative values
        for i in range(5):
            if motor_limits[i][0] < 0:
                motor_limits[i][0] = 0
            update_slider_limits(i)
            update_range_labels(i)


# --- Calibration functions ---
def start_calibration():
    ser.write(b"calibration start\n")
    print("Calibration mode started")


def stop_calibration():
    ser.write(b"calibration stop\n")
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
root.geometry("800x750")

button_font = ("Arial", 10)
sliders = []
value_labels = []
encoder_labels = []
min_buttons = []
max_buttons = []
range_labels = []  # For storing min and max labels


def create_motor_controls(motor_number, row):
    frame = tk.Frame(root, relief=tk.GROOVE, bd=2, padx=10, pady=8)
    frame.grid(row=row, column=0, columnspan=6, padx=10, pady=5, sticky="ew")

    # Motor title
    title_frame = tk.Frame(frame)
    title_frame.grid(row=0, column=0, columnspan=6, pady=(0, 8))
    tk.Label(title_frame, text=f"MOTOR {motor_number}", font=("Arial", 11, "bold")).pack()

    # "-" button
    minus_btn = tk.Button(frame, text="-", font=button_font, width=3, height=1, bg="lightcoral")
    minus_btn.grid(row=1, column=0, padx=2)

    # Slider - initially set to 0, will be updated with actual target value
    slider = ttk.Scale(frame, from_=0, to=MAX_VALUE, orient=tk.HORIZONTAL, length=180)
    slider.set(0)  # Temporary 0, will be updated
    slider.grid(row=1, column=1, padx=5, sticky="ew")
    sliders.append(slider)

    # "+" button
    plus_btn = tk.Button(frame, text="+", font=button_font, width=3, height=1, bg="lightgreen")
    plus_btn.grid(row=1, column=2, padx=2)

    # Target value label
    value_label = tk.Label(frame, text="0",  # Initially 0, will be updated
                           font=("Arial", 11, "bold"), width=6, fg="blue")
    value_label.grid(row=1, column=3, padx=5)
    value_labels.append(value_label)

    # Calibration buttons
    min_btn = tk.Button(frame, text="Set MIN", font=("Arial", 8), width=6, height=1,
                        bg="lightyellow", command=lambda: set_min_limit(motor_number - 1))
    min_btn.grid(row=1, column=4, padx=2)
    min_buttons.append(min_btn)

    max_btn = tk.Button(frame, text="Set MAX", font=("Arial", 8), width=6, height=1,
                        bg="lightyellow", command=lambda: set_max_limit(motor_number - 1))
    max_btn.grid(row=1, column=5, padx=2)
    max_buttons.append(max_btn)

    # Encoder value label
    encoder_label = tk.Label(frame, text="Encoder: 0", font=("Arial", 9), fg="green")
    encoder_label.grid(row=2, column=0, columnspan=6, pady=(6, 0))
    encoder_labels.append(encoder_label)

    # Min and max labels
    range_frame = tk.Frame(frame)
    range_frame.grid(row=3, column=1, sticky="ew", pady=(3, 0))
    min_label = tk.Label(range_frame, text="0", font=("Arial", 7))
    min_label.pack(side=tk.LEFT)
    max_label = tk.Label(range_frame, text=f"{MAX_VALUE}", font=("Arial", 7))
    max_label.pack(side=tk.RIGHT)
    range_labels.append([min_label, max_label])  # Store both labels

    # Event bindings
    motor_index = motor_number - 1
    slider.configure(command=lambda v: on_slider_change(motor_index, v))
    plus_btn.bind("<ButtonPress>", lambda e: start_increment(motor_index))
    plus_btn.bind("<ButtonRelease>", lambda e: stop_button())
    minus_btn.bind("<ButtonPress>", lambda e: start_decrement(motor_index))
    minus_btn.bind("<ButtonRelease>", lambda e: stop_button())


# Create controls for 5 motors
for i in range(1, 6):
    create_motor_controls(i, i - 1)

# Calibration button
calibration_btn = tk.Button(root, text="Start Calibration", font=("Arial", 12, "bold"),
                            bg="lightblue", command=start_calibration, width=20, height=2)
calibration_btn.grid(row=5, column=0, columnspan=3, pady=10)

# Stop calibration button
stop_calibration_btn = tk.Button(root, text="Stop Calibration", font=("Arial", 12, "bold"),
                                 bg="lightcoral", command=stop_calibration, width=20, height=2,
                                 state="disabled")
stop_calibration_btn.grid(row=5, column=3, columnspan=3, pady=10)

# Instruction
instruction = tk.Label(root, text="Hold +/- buttons to change targets. Calibration mode disables limits.",
                       font=("Arial", 10), fg="blue", pady=8)
instruction.grid(row=6, column=0, columnspan=6)

# Status
status_label = tk.Label(root, text="SYNCHRONIZING... Waiting for Arduino data",
                        font=("Arial", 9), fg="orange")
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
    ser.write(b"calibration stop\n")  # Exit calibration mode
    time.sleep(0.2)
    ser.close()
    print("Serial port closed")
    root.destroy()


root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()