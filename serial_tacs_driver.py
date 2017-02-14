#assume 537 milliseconds / sample!
from time import sleep
import serial
import math
import sys

global listof256
global txbuffer
txbuffer = []

def arrayinit():
    
    global listof256
    listof256 = []
    for x in range (0, 256):
        listof256.append(2 * math.sin(((math.pi)*x)/128) )
    return

arrayinit()
ser = serial.Serial('COM6', 115200) # Establish the connection on a specific port
sleep(3)

def tx1st256bytes():
    print("tx1st256bytes")
    done = 0
    
    while (done == 0):
        if (ser.inWaiting() > 0):
            for x in range (0, 32):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
            
    while (done == 1):
        if (ser.inWaiting() > 0):
            for x in range (32, 64):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
            
    while (done == 2):
        if (ser.inWaiting() > 0):
            for x in range (64, 96):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
            
    while (done == 3):
        if (ser.inWaiting() > 0):
            for x in range (96, 128):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
    return


def tx2nd256bytes():
    print("tx2nd256bytes")
    done = 0
    
    while (done == 0):
        if (ser.inWaiting() > 0):
            for x in range (128, 160):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
            
    while (done == 1):
        if (ser.inWaiting() > 0):
            for x in range (160, 192):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
            
    while (done == 2):
        if (ser.inWaiting() > 0):
            for x in range (192, 224):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
            
    while (done == 3):
        if (ser.inWaiting() > 0):
            for x in range (224, 256):
                ser.write(txbuffer[x])
            ser.read(ser.inWaiting())
            done += 1
    return    


    


def updatescreen():
    return



def milliamps2dacwrite(milliamps):
    print("milliamps2dacwrite")
    integer_write_value = int(round( (3.5715)*(2.5 - milliamps) ))
    dacwrite = integer_write_value.to_bytes(2, byteorder="big", signed=False)
    return dacwrite

def loadtxbuffer():
    print("loadtxbuffer")
    global txbuffer
    global listof256
    for x in range(0,256):
        txbuffer.append(milliamps2dacwrite(listof256[x]))
    return

x=0
while x<256:
    x+=1
    ser.write(b"\x06\xF0")



while True:
    loadtxbuffer()
    tx1st256bytes()
    updatescreen()
    tx2nd256bytes()


##while true loop
#generate/load values to fill tx buffer (8 64-byte blocks)
#send values in 64 byte blocks until tES device is full (4 blocks)
#screen update stuff
#send values in 64 byte blocks until tES device is full (4 blocks)
