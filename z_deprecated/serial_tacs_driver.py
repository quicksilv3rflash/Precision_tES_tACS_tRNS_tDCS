#assume 537 milliseconds / sample!
from time import sleep
import serial
import math


global listof256



def arrayinit():    
    global listof256
    listof256 = []
    for x in range (0, 256):
        listof256.append(2 * math.sin(((math.pi)*x)/128) )
    return

arrayinit()
ser = serial.Serial('COM6', 115200) # Establish the connection on a specific port
sleep(3)

#This function fills the entire 0.131072 seconds of buffer on the device.
#It requires a list of 256 values in mA, sample rate one value per 512us.  
def tx_256_mA_values(listof256_values_in_mA):      
    data_to_transmit = [] 
    for x in range(0, 256):
        data_to_transmit.append(int(round((3.5715)*(2500.0-(1000.0*listof256_values_in_mA[x])))).to_bytes(2,byteorder="big",signed=False))
        #That horrible line of code converts mA values into correct format
        #for the DAC. First it runs the values through the linear equation
        #that converts them to appropriate DAC values, then it ensures the
        #data are formatted in 16-bit unsigned integers; then, we transmit
        #the data in eight 64-byte blocks.    
    index = 0
    while (index < 8):
        if (ser.inWaiting() > 0):
            for x in range(32*index,(32*index + 32)):
                ser.write(data_to_transmit[x])
            ser.read(ser.inWaiting())
            index += 1
    return


def updatescreen():
    return



while True:
    tx_256_mA_values(listof256)
    updatescreen()
    #tx2nd256bytes()


##while true loop
#generate/load values to fill tx buffer (8 64-byte blocks)
#send values in 64 byte blocks until tES device is full (4 blocks)
#screen update stuff
#send values in 64 byte blocks until tES device is full (4 blocks)
