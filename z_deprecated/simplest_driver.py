

whichport = input("Enter the serial port location the Arduino is connected to,\nfor example, COM6. This location is displayed in the Arduino IDE\nunder Tools>PORT:  ")


#SYSTEM DEVELOPED ON SOFTWARE VERSIONS:
# * Arduino 1.8.1
# * Python 3.2.5
# * PySerial 2.6

# * SYSTEM DEVELOPED ON ARDUINO HARDWARE: 
# * "MINI USB Nano V3.0 ATmega328P CH340G 5V 16M"

#assume 512 microseconds / sample!
#main loop iterates every 65.536 ms (approximately 15.25 FPS)
import time, serial
from time import sleep





#this function converts mA values to appropriate DAC write values
def mA_2_DAC_write(value_in_mA):
    if(value_in_mA > 2.002): #check positive limit
        value_in_mA = 2.001
    if(value_in_mA < -2.002):#check negative limit
        value_in_mA = -2.001
    dacwrite = (int(round(((16383*1.0866)/5)*(2.5-value_in_mA)))).to_bytes(2,byteorder="big",signed=False)
    #That horrible line of code converts mA values into correct format
    #for the DAC. First it runs the values through the linear equation
    #that converts them to appropriate DAC values, then it ensures the
    #data are formatted in 16-bit unsigned integers.
    return dacwrite

  
    

ser = serial.Serial(whichport, 115200) # Establish the connection on a specific port
sleep(2)    #wait 2 seconds for the connection to settle
ticks = 0

dcvalue = input("\nEnter DC output current value for tES device, in mA:  ")
ser.read(ser.inWaiting())
ser.write(mA_2_DAC_write(float(dcvalue)))
print("\nCurrent set.")
