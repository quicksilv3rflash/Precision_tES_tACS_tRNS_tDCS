from time import sleep
import serial
ser = serial.Serial('COM6', 115200) # Establish the connection on a specific port
x=0
sleep(3)
while x<512:
    x+=1
    ser.write(b"\x06\xF0")
while True:
    if ser.inWaiting() > 0:
        ser.read(ser.inWaiting())
        x=0
        while x<8: #8 * 8 bytes = 64 bytes!
            x+=1
            ser.write(b"\x06\xF0\x3F\xFF\x06\xF0\x3F\xFF")
