import serial
import serial.tools.list_ports
import time
import datetime
import csv

AUTH_KEY = b"g_group"
RESP_KEY = b"ggyes"
BAUDRATE = 9600
TIMEOUT = 2

def find_gg_device():
    ports = serial.tools.list_ports.comports()

    for port in ports:
        try:
            print(f"Пробуем {port.device}")
            with serial.Serial(port.device, BAUDRATE, timeout=TIMEOUT) as ser:
                # time.sleep(2)  # дать время на старт

                ser.reset_input_buffer()
                ser.write(AUTH_KEY)

                time_start = time.time()
                while time.time() - time_start < TIMEOUT:
                    if ser.in_waiting:
                        line = ser.readline().strip()
                        print(f"Ответ: {line}")
                        if RESP_KEY in line:
                            print(f"Подключено к {port.device}")
                            return port.device
        except Exception as e:
            print(f"Ошибка {port.device}: {e}")

    return None

def read_data(port_name, file_name):
    with serial.Serial(port_name, BAUDRATE, timeout=0) as ser:
        print(f"[OK] Чтение с {port_name}")
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8').strip()
                print(f"[RAW] {line}")
                with open(file_name, "a", newline="") as file:
                    csv.writer(file, delimiter=',', quoting=csv.QUOTE_MINIMAL).writerow([i for i in line.split(",")])

                if "counts," in line and ",time," in line:
                    try:
                        parts = line.split(',')
                        counts = int(parts[1])
                        timestamp = int(parts[3])
                        print(f"[PARSED] Counts: {counts}, Time: {timestamp} ms")
                    except Exception as e:
                        print(f"[PARSE ERROR] {e}")

if __name__ == "__main__":
    file_name = "output.csv"
    today_date = datetime.date.today()
    with open(file_name, "w", newline="") as file:
        csv.writer(file, delimiter=',', quoting=csv.QUOTE_MINIMAL).writerow([i for i in today_date.strftime("%Y,%m,%d").split(",")])
    port = find_gg_device()
    if port:
        read_data(port, file_name)
    else:
        print("Микроконтроллер не найден.")


















