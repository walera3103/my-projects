import tkinter as tk
import serial
import threading
import time

# --- Konfiguracja portu szeregowego ---
# Zmień 'COM3' na swój port (np. '/dev/ttyUSB0' dla Linux)
ser = serial.Serial('COM12', 9600, timeout=1)

# --- Zmienne sterujące ---
active_command = None
lock = threading.Lock()
running = True

# --- Funkcja wysyłająca dane do portu COM ---
def serial_sender():
    global active_command
    while running:
        with lock:
            msg = active_command if active_command else "0"
        ser.write((msg + "\n").encode())
        time.sleep(0.05)

# --- Funkcje obsługi przycisków ---
def on_press(command):
    global active_command
    with lock:
        active_command = command

def on_release():
    global active_command
    with lock:
        active_command = None

# --- GUI: Tworzenie aplikacji ---
root = tk.Tk()
root.title("Sterowanie silnikami")

# Styl
button_font = ("Arial", 16)
    
# Funkcja pomocnicza do tworzenia przycisków
def create_motor_controls(motor_number, row):
    tk.Label(root, text=f"Silnik {motor_number}", font=button_font).grid(row=row, column=0, padx=10, pady=10)
    plus_btn = tk.Button(root, text="+", font=button_font, width=5)
    minus_btn = tk.Button(root, text="-", font=button_font, width=5)

    plus_btn.grid(row=row, column=1)
    minus_btn.grid(row=row, column=2)

    # Przypisanie zdarzeń
    plus_btn.bind("<ButtonPress>", lambda e: on_press(f"motor {motor_number} +"))
    plus_btn.bind("<ButtonRelease>", lambda e: on_release())

    minus_btn.bind("<ButtonPress>", lambda e: on_press(f"motor {motor_number} -"))
    minus_btn.bind("<ButtonRelease>", lambda e: on_release())

# Tworzenie przycisków dla 3 silników
for i in range(1, 4):
    create_motor_controls(i, i - 1)

# Uruchomienie wątku do wysyłania danych
thread = threading.Thread(target=serial_sender)
thread.daemon = True
thread.start()

# Obsługa zamknięcia aplikacji
def on_closing():
    global running
    running = False
    time.sleep(0.2)
    root.destroy()            # mozliwy blad

root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()
