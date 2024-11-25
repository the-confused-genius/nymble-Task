import serial
import serial.tools.list_ports
import time
import sys
import os 
import colorama
from colorama import Fore

port_keyword = 'STMicroelectronics STLink Virtual COM Port'
buffer_size = 100
Delimiter = "**********\r\n**********"
baudrate = 2400

colorama.init(autoreset=True)

def calculate_speed(start_time, end_time, bits_transferred):
    elapsed_time = end_time - start_time  
    speed = bits_transferred / elapsed_time
    return speed

def format_digit(number):
    return f"{number:04d}"

def auto_select_com_port(keyword=None):
    available_ports = list(serial.tools.list_ports.comports())

    if not available_ports:
        print(f"{Fore.MAGENTA}No COM ports found.")
        return None

    for port in available_ports:
        if keyword and (keyword.lower() in port.description.lower() or keyword.lower() in port.name.lower()):
            print(f"{Fore.MAGENTA}Auto-selected COM port: {port.device} ({port.description})")
            return port.device

    # Fallback to the first available port if no keyword matches
    print(f"{Fore.MAGENTA}No matching keyword. Defaulting to the first available port: {available_ports[0].device} ({available_ports[0].description})")
    return available_ports[0].device

def send_in_chuncks(ser, input_string, packet_size=100):
    byte_data = input_string.encode('utf-8')        #String to byte conversion
    

    for i in range(0, len(byte_data), packet_size):
        packet = byte_data[i:i + packet_size] 
        ser.write(format_digit(len(packet)).encode('utf-8'))
        #time.sleep(0.1)        #to test baudrate calculation
        ser.write(packet)  
        print(f"{Fore.CYAN}Sent packet {i // packet_size + 1}, Size : {format_digit(len(packet))}, Content : {packet.decode('utf-8', errors='ignore')}\n")

def read_until_delimiter(ser):
    buffer = ""
    delimiter_length = len(Delimiter)

    while True:
        byte = ser.read(1).decode('utf-8', errors='ignore')
        if not byte:
            continue

        buffer += byte
        sys.stdout.write(f"{Fore.GREEN}{byte}")
        sys.stdout.flush()
        
        if buffer[-delimiter_length:] == Delimiter:
            break

    return buffer

def main():
    com_port = auto_select_com_port(port_keyword)
    ser = serial.Serial(com_port, baudrate=baudrate, timeout=3)  
    print(f"{Fore.MAGENTA}Baud Rate : {baudrate}")
    text_to_send = str(input("Enter a Paragraph : "))
    
    # PC to MCU
    print("\n\n\nPC TO MCU \n")
    start_time = time.time()
    send_in_chuncks(ser, text_to_send, buffer_size)
    terminate_tx = '0000'
    print(f'{Fore.CYAN}Closing Transmission, Content : {terminate_tx}\n')
    ser.write(terminate_tx.encode('utf-8'))
    end_time = time.time()
    speed = calculate_speed(start_time, end_time, len(text_to_send)*10)
    elapsed_time = end_time - start_time
    print(f"{Fore.YELLOW}Time Elapsed [PC to MCU] : {elapsed_time:.2f} seconds")
    print(f"{Fore.YELLOW}Transmission speed [PC to MCU]: {speed:.2f} bits/second")

    # MCU to PC
    print("\n\n\nMCU TO PC \n")
    start_time = time.time()
    received_data = read_until_delimiter(ser)
    end_time = time.time()
    speed = calculate_speed(start_time, end_time, len(received_data) * 10)
    elapsed_time = end_time - start_time
    print(f"\n\n{Fore.YELLOW}Time Elapsed [MCU to PC] : {elapsed_time:.2f} seconds")
    print(f"{Fore.YELLOW}Transmission speed [MCU to PC]: {speed:.2f} bits/second")

    #print("Received data:", received_data)
    
    

if __name__ == "__main__":
    main()
