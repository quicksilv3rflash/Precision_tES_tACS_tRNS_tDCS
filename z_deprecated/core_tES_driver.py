

whichport = input("Enter the serial port location the Arduino is connected to,\nfor example, COM6. This location is displayed in the Arduino IDE\nunder Tools>PORT:  ")


#SYSTEM DEVELOPED ON SOFTWARE VERSIONS:
# * Arduino 1.8.1
# * Python 3.2.5
# * PySerial 2.6

# * SYSTEM DEVELOPED ON ARDUINO HARDWARE: 
# * "MINI USB Nano V3.0 ATmega328P CH340G 5V 16M"

#assume 512 microseconds / sample!
#main loop iterates every 65.536 ms (approximately 15.25 FPS)
import random, sys, math, time, serial
from time import sleep



#generate waveform produces sine, square, triangle, ramp_up, ramp_down, and random.
#offset = dc bias
def generate_wave_points(waveform,frequency,amplitude,offset):
    if(offset > 2.002):
        offset = 2.001
    if(offset < -2.002):
        offset = -2.001
    if(amplitude > 2.002):
        amplitude = 2.001
    samples_per_cycle = int(round(1/(float(frequency) * .000512)))
    wave_list = [0]* samples_per_cycle
    if(waveform == "sine"):
        for x in range (0, samples_per_cycle):
            wave_list[x] = amplitude*(math.sin(2*(math.pi)*(x/samples_per_cycle))) + offset
        wave_list[samples_per_cycle - 1] = (wave_list[0] + wave_list[samples_per_cycle - 2])/2.0
    if(waveform == "square"):
        for x in range (0, samples_per_cycle):
            if(x < round(samples_per_cycle/2)):
                wave_list[x] = amplitude + offset
            if(x >= round(samples_per_cycle/2)):
                wave_list[x] = (-1*amplitude) + offset
    if(waveform == "triangle"):
        for x in range (0, samples_per_cycle):
            if(x < round(samples_per_cycle/4)):
                wave_list[x] = amplitude * (x/(samples_per_cycle/4)) + offset
            if((x>=round(samples_per_cycle/4)) and (x<round((3*samples_per_cycle) / 4))):
                wave_list[x] = (amplitude + offset)- amplitude*((x - (samples_per_cycle /4))/(samples_per_cycle / 4))
            if(x >= round((3*samples_per_cycle)/4)):
                wave_list[x] = (-amplitude + offset) + amplitude *((x - ((3*samples_per_cycle) /4))/(samples_per_cycle / 4))
    if(waveform == "ramp_up"):
        for x in range (0, samples_per_cycle):
            wave_list[x] = (-amplitude) + amplitude*(x/(samples_per_cycle/2.0)) + offset
    if(waveform == "ramp_down"):
        for x in range (0, samples_per_cycle):
            wave_list[x] = (-1)*((-amplitude) + amplitude*(x/(samples_per_cycle/2.0))) + offset
    if(waveform == "random"):
        for x in range (0, samples_per_cycle):
            if((x % 2)==0):
                wave_list[x] = (1.0 - (random.random()*2))*amplitude + offset
            else:
                wave_list[x] = wave_list[x-1]

    for x in range(0, len(wave_list)):
        if(wave_list[x] > 2.002): #check positive limit
            wave_list[x] = 2.001
        if(wave_list[x] < -2.002):#check negative limit
            wave_list[x] = -2.001
    
    return wave_list
#the returned wave_list is a discretized version of the desired waveform sampled at 512us per sample






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

#This function fills half of the 0.131072 seconds of buffer on the device.
#It requires a list of 128 values in mA;
#   output sample rate: one value per 512us.  
def tx_128_mA_values(listof128_values_in_mA):      
    data_to_transmit = [0] * 128 
    for x in range(0, 128):
        data_to_transmit[x] = mA_2_DAC_write(listof128_values_in_mA[x])
        
    data_sent = 0
    while (data_sent < 1):
        if (ser.inWaiting() > 0):
            for x in range(0, 128):
                ser.write(data_to_transmit[x])
            ser.read(ser.inWaiting())
            data_sent += 1
    return



def initializebuffer(): #loads the device's buffer with "output zero mA" commands
    for x in (0,256):
        ser.write(mA_2_DAC_write(0.0))
    return
        
    

ser = serial.Serial(whichport, 115200) # Establish the connection on a specific port
sleep(2)    #wait 2 seconds for the connection to settle
ticks = 0


acdc = input("\n TYPE AC   OR    DC\n")

if (acdc == "DC"):
    currentvalue = input("\n enter current value in mA\n")
    ser.write(mA_2_DAC_write(float(currentvalue)))
    print("\nDC current value set.\n")

if (acdc == "AC"):
    waveform = input("\n enter waveform: sine, square, triangle, ramp_up, ramp_down, random:  ")
    frequency = input("\n enter frequency, Hz:  ")
    amplitude = input("\n enter amplitude, mA:  ")
    offset = input("\n enter DC offset, mA:  ")
    wave_points = generate_wave_points(waveform, float(frequency), float(amplitude), float(offset))
    print("\nREMEMBER: THE PROGRAM WILL HANG HERE IF YOU SELECTED A PERIODIC WAVEFORM\n")
    initializebuffer()
    while True:
        listlength = len(wave_points)
        txlist = [0]*128
        for x in range (0, 128):
                txlist[x] = wave_points[(ticks + x)% listlength]
        tx_128_mA_values(txlist) #if this function is called often enough it'll guarantee 65.5ms for each iteration of the loop
        ticks += 128
        if((ticks % listlength) == 0):
            ticks = 0
        
