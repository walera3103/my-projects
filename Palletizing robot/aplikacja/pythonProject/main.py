import tkinter as tk
import serial
import threading
import time

# --- Serial configuration ---
try:
    ser = serial.Serial('COM12', 115200, timeout=1)
    time.sleep(2)
    print("Serial port opened successfully")
except Exception as e:
    print(f"Error opening serial port: {e}")
    exit()

# --- Control variables ---
active_command = None
lock = threading.Lock()
running = True


# --- Serial sender function ---
def serial_sender():
    global active_command
    while running:
        with lock:
            current_cmd = active_command
        if current_cmd is not None:
            ser.write((current_cmd + "\n").encode())
            print(f"Sent: '{current_cmd}'")
        time.sleep(0.1)


# --- Button handlers ---
def on_press(command):
    global active_command
    with lock:
        active_command = command
    print(f"Button pressed: {command}")


def on_release():
    global active_command
    with lock:
        active_command = ""  # Надсилаємо порожню команду для зупинки
    print("Button released - stopping motor")


# --- GUI ---
root = tk.Tk()
root.title("Control 5 Motors - Auto Stop")
root.geometry("400x350")

button_font = ("Arial", 12)


def create_motor_controls(motor_number, row):
    frame = tk.Frame(root)
    frame.grid(row=row, column=0, columnspan=3, padx=10, pady=8, sticky="ew")

    tk.Label(frame, text=f"Motor {motor_number}", font=button_font, width=10).pack(side=tk.LEFT)

    plus_btn = tk.Button(frame, text="+", font=button_font, width=6, height=2,
                         bg="lightgreen")
    plus_btn.pack(side=tk.LEFT, padx=5)

    minus_btn = tk.Button(frame, text="-", font=button_font, width=6, height=2,
                          bg="lightcoral")
    minus_btn.pack(side=tk.LEFT, padx=5)

    # Bind events - мотор зупиняється автоматично при відпусканні
    plus_btn.bind("<ButtonPress>", lambda e: on_press(f"motor {motor_number} +"))
    plus_btn.bind("<ButtonRelease>", lambda e: on_release())

    minus_btn.bind("<ButtonRelease>", lambda e: on_release())
    minus_btn.bind("<ButtonPress>", lambda e: on_press(f"motor {motor_number} -"))


# Create controls for 5 motors
for i in range(1, 6):
    create_motor_controls(i, i - 1)

# Status label
status_label = tk.Label(root, text="Motors stop automatically when buttons are released",
                        font=("Arial", 10), fg="blue")
status_label.grid(row=5, column=0, columnspan=3, pady=15)

# Start serial thread
thread = threading.Thread(target=serial_sender)
thread.daemon = True
thread.start()


def on_closing():
    global running
    running = False
    # Надсилаємо порожню команду для зупинки всіх моторів
    ser.write("\n".encode())
    time.sleep(0.2)
    ser.close()
    root.destroy()


root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()