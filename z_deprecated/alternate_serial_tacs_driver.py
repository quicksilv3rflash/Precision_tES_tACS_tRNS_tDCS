#assume 512 microseconds / sample!
#main loop iterates every 65.536 ms (approximately 15.25 FPS)
import random, pygame, sys, math, time, serial
from time import sleep
from pygame.locals import *


#generate waveform produces sine, square, triangle, ramp_up
#offset = dc bias
def generate_wave_points(waveform,frequency,amplitude,offset):
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
            wave_list[x] = (1.0 - (random.random()*2))*amplitude + offset

    for x in range(0, len(wave_list)):
        if(wave_list[x] > 2.002): #check positive limit
            wave_list[x] = 2.001
        if(wave_list[x] < -2.002):#check negative limit
            wave_list[x] = -2.001
    
    return wave_list







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

def addindicator(indicated_mode):
    returnstring = " "
    if (which_mode == indicated_mode):
        returnstring += "<<"
    return returnstring

def setactivecolor(indicated_mode):
    returntuple = (160,100,100)
    if (which_mode == indicated_mode):
        returntuple = (255,0,0)
    return returntuple

def updateGUI():
    global displaysurface
    global wave_points
    global which_mode
    global frequency
    global amplitude
    global dc_bias
    global halted

    updatewave = 0
    
    
    displaysurface.fill((0,0,0))
    
    pygame.draw.line(displaysurface,(255,0,0),(0,230),(305,230),2)
    pygame.draw.line(displaysurface,(255,0,0),(305,230),(305,480),2)

    pygame.draw.line(displaysurface,(100,100,160),(325,310),(640,310),2)
    pygame.draw.line(displaysurface,(100,100,160),(325,395),(640,395),2)

    pygame.draw.line(displaysurface,(0,0,255),(325,230),(640,230),2)
    pygame.draw.line(displaysurface,(0,0,255),(325,230),(325,480),2)
    
    font = pygame.font.Font(None, 50)
    if(halted != 0):
        introtext = font.render("[ RUN STIMULATOR ]",0,(0,255,0))
    if(halted == 0):
        introtext = font.render("[HALT STIMULATOR]",0,(255,0,0))
    runstop = displaysurface.blit(introtext, (5,190))
    
    font = pygame.font.Font(None, 36)
    introtext = font.render("    -  - SET MODE -  -", 0,(200,50,50))
    displaysurface.blit(introtext, (5,240))    
    introtext = font.render("[  DIRECT CURRENT ]" + addindicator("dc"), 0, setactivecolor("dc"))
    dcbutton = displaysurface.blit(introtext, (5,270))    
    introtext = font.render("[        SINE WAVE       ]" + addindicator("sine"), 0,setactivecolor("sine"))
    sinebutton = displaysurface.blit(introtext, (5, 300))    
    introtext = font.render("[    SQUARE WAVE    ]" + addindicator("square"), 0,setactivecolor("square"))
    squarebutton = displaysurface.blit(introtext, (5, 330))    
    introtext = font.render("[  TRIANGLE WAVE   ]" + addindicator("triangle"), 0,setactivecolor("triangle"))
    trianglebutton = displaysurface.blit(introtext, (5, 360))    
    introtext = font.render("[  SAWTOOTH (UP)   ]" + addindicator("ramp_up"), 0,setactivecolor("ramp_up"))
    sawtoothupbutton = displaysurface.blit(introtext, (5, 390))
    introtext = font.render("[SAWTOOTH(DOWN)]" + addindicator("ramp_down"), 0,setactivecolor("ramp_down"))
    sawtoothdownbutton = displaysurface.blit(introtext, (5, 420))    
    introtext = font.render("[   RANDOM NOISE   ]" + addindicator("random"), 0,setactivecolor("random"))
    randombutton = displaysurface.blit(introtext, (5, 450))

    introtext = font.render("[10]",0,(0,0,255))
    hzmns10 = displaysurface.blit(introtext, (340,235))
    introtext = font.render("[1]",0,(0,0,255))
    hzmns1 = displaysurface.blit(introtext, (395,235))
    introtext = font.render("[.1]",0,(0,0,255))
    hzmnspt1 = displaysurface.blit(introtext, (435,235))
    introtext = font.render("-|+",0,(0,0,255))
    displaysurface.blit(introtext, (475,235))
    introtext = font.render("[.1]",0,(0,0,255))
    hzplspt1 = displaysurface.blit(introtext, (510,235))
    introtext = font.render("[1]",0,(0,0,255))
    hzpls1 = displaysurface.blit(introtext, (555,235))
    introtext = font.render("[10]",0,(0,0,255))
    hzpls10 = displaysurface.blit(introtext, (590,235))

    introtext = font.render("[1]",0,(0,0,255))
    ampmns1 = displaysurface.blit(introtext, (340,320))
    introtext = font.render("[.1]",0,(0,0,255))
    ampmnspt1 = displaysurface.blit(introtext, (380,320))
    introtext = font.render("[.01]",0,(0,0,255))
    ampmnspt01 = displaysurface.blit(introtext, (420,320))
    introtext = font.render("-|+",0,(0,0,255))
    displaysurface.blit(introtext, (475,320))
    introtext = font.render("[.01]",0,(0,0,255))
    ampplspt01 = displaysurface.blit(introtext, (510,320))
    introtext = font.render("[.1]",0,(0,0,255))
    ampplspt1 = displaysurface.blit(introtext, (565,320))
    introtext = font.render("[1]",0,(0,0,255))
    amppls1 = displaysurface.blit(introtext, (605,320))

    introtext = font.render("[1]",0,(0,0,255))
    dcbmns1 = displaysurface.blit(introtext, (340,405))
    introtext = font.render("[.1]",0,(0,0,255))
    dcbmnspt1 = displaysurface.blit(introtext, (380,405))
    introtext = font.render("[.01]",0,(0,0,255))
    dcbmnspt01 = displaysurface.blit(introtext, (420,405))
    introtext = font.render("-|+",0,(0,0,255))
    displaysurface.blit(introtext, (475,405))
    introtext = font.render("[.01]",0,(0,0,255))
    dcbplspt01 = displaysurface.blit(introtext, (510,405))
    introtext = font.render("[.1]",0,(0,0,255))
    dcbplspt1 = displaysurface.blit(introtext, (565,405))
    introtext = font.render("[1]",0,(0,0,255))
    dcbpls1 = displaysurface.blit(introtext, (605,405))
    
    if(which_mode != "dc" and which_mode != "random"):
        introtext = font.render("FREQUENCY(Hz):", 0, (100,100,160))
        displaysurface.blit(introtext, (340,260))
        introtext = font.render("      " + str(round(frequency,1)),0,(180,100,180))
        displaysurface.blit(introtext, (340,285))

    if(which_mode != "dc"):
        introtext = font.render("AMPLITUDE(mA peak):",0, (100,100,160))
        displaysurface.blit(introtext, (340,345))
        introtext = font.render("      " + str(round(amplitude,2)),0,(180,100,180))
        displaysurface.blit(introtext, (340,370))
        
    
    introtext = font.render("DC BIAS(mA):",0, (100,100,160))
    displaysurface.blit(introtext, (340,430))
    introtext = font.render("      " + str(round(dc_bias,2)),0, (180,100,180))
    displaysurface.blit(introtext, (340,455))

    pygame.draw.line(displaysurface,(100,70,70),(10,20),(560,20),3)
    pygame.draw.line(displaysurface,(100,70,70),(10,180),(560,180),3)
    pygame.draw.line(displaysurface,(100,70,70),(10,100),(560,100),3)
    introtext = font.render("+2mA", 0, (120,120,120))
    displaysurface.blit(introtext, (560,20))
    introtext = font.render("+0mA", 0, (120,120,120))
    displaysurface.blit(introtext, (560,100))
    introtext = font.render("-2mA", 0, (120,120,120))
    displaysurface.blit(introtext, (560,180))

    
    listlength = len(wave_points)
    if(which_mode != "random"):
        for x in range (0,100):
            pygame.draw.line(displaysurface,(255,255,255),(10+(5*x),100 - (40*wave_points[round(x*(listlength/40)) % listlength])),(15 +(5*x),100 - (40*wave_points[round((x+1)*(listlength/40)) % listlength])),2)
    else:
        for x in range (0,100):
            pygame.draw.line(displaysurface,(255,255,255),(10+(5*x),100 - (40*wave_points[round(x*(listlength/40)) % listlength])),(15 +(5*x),100 - (40*wave_points[round(x*(listlength/40)) % listlength])),2)

    
    
    pygame.display.update()
    #quit cleanly. The user input event handler lives here too.
    for event in pygame.event.get():
        if(event.type == QUIT):
            pygame.quit()
            sys.exit()
        if(event.type == pygame.MOUSEBUTTONDOWN and event.button == 1):
            cursor = pygame.mouse.get_pos()
            if(dcbutton.collidepoint(cursor)):                
                which_mode = "dc"
                updatewave = 1
            if(sinebutton.collidepoint(cursor)):
                wave_points = generate_wave_points("sine",frequency,2,0)
                which_mode = "sine"
                updatewave = 1
            if(squarebutton.collidepoint(cursor)):
                wave_points = generate_wave_points("square",frequency,2,0)
                which_mode = "square"
                updatewave = 1
            if(trianglebutton.collidepoint(cursor)):
                wave_points = generate_wave_points("triangle",frequency,2,0)
                which_mode = "triangle"
                updatewave = 1
            if(sawtoothupbutton.collidepoint(cursor)):
                wave_points = generate_wave_points("ramp_up",frequency,2,0)
                which_mode = "ramp_up"
                updatewave = 1
            if(sawtoothdownbutton.collidepoint(cursor)):
                wave_points = generate_wave_points("ramp_down",frequency,2,0)
                which_mode = "ramp_down"
                updatewave = 1
            if(randombutton.collidepoint(cursor)):
                which_mode = "random"
                updatewave = 1
            if(hzpls10.collidepoint(cursor)):
                frequency += 10
                updatewave = 1
            if(hzpls1.collidepoint(cursor)):
                frequency += 1
                updatewave = 1
            if(hzplspt1.collidepoint(cursor)):
                frequency += 0.1
                updatewave = 1
            if(hzmnspt1.collidepoint(cursor)):
                frequency -= 0.1
                updatewave = 1
            if(hzmns1.collidepoint(cursor)):
                frequency -= 1
                updatewave = 1
            if(hzmns10.collidepoint(cursor)):
                frequency -= 10
                updatewave = 1
            if(amppls1.collidepoint(cursor)):
                amplitude += 1
                updatewave = 1
            if(ampplspt1.collidepoint(cursor)):
                amplitude +=0.1
                updatewave = 1
            if(ampplspt01.collidepoint(cursor)):
                amplitude += 0.01
                updatewave = 1
            if(ampmnspt01.collidepoint(cursor)):
                amplitude -= 0.01
                updatewave = 1
            if(ampmnspt1.collidepoint(cursor)):
                amplitude -= 0.1
                updatewave = 1
            if(ampmns1.collidepoint(cursor)):
                amplitude -= 1
                updatewave = 1
            if(dcbpls1.collidepoint(cursor)):
                dc_bias += 1
                updatewave = 1
            if(dcbplspt1.collidepoint(cursor)):
                dc_bias +=0.1
                updatewave = 1
            if(dcbplspt01.collidepoint(cursor)):
                dc_bias += 0.01
                updatewave = 1
            if(dcbmnspt01.collidepoint(cursor)):
                dc_bias -= 0.01
                updatewave = 1
            if(dcbmnspt1.collidepoint(cursor)):
                dc_bias -= 0.1
                updatewave = 1
            if(dcbmns1.collidepoint(cursor)):
                dc_bias -= 1
                updatewave = 1
                
            if(runstop.collidepoint(cursor)):
                if(halted != 0):
                    halted = 0
                else:
                    halted = 1
                    
    if(updatewave == 1):
        if(which_mode == "dc"):
            wave_points = generate_wave_points("square",50,0,dc_bias)
        else:
            if(which_mode == "random"):
                wave_points = generate_wave_points("random",0.1,amplitude,dc_bias)
            else:
                wave_points = generate_wave_points(which_mode,frequency,amplitude,dc_bias)
    return

def initializeGUI():
    pygame.init()
    global displaysurface
    displaysurface = pygame.display.set_mode((640, 480))
    pygame.display.set_caption("tES device control program")
    return

def initializebuffer(): #loads the device's buffer with "output zero mA" commands
    for x in (0,256):
        ser.write(mA_2_DAC_write(0.0))
    return
        
    

ser = serial.Serial('COM6', 115200) # Establish the connection on a specific port
sleep(1)    #wait 3 seconds for the connection to settle
ticks = 0
global halted
halted = 1
global which_mode
which_mode = "dc"
global frequency
frequency = 0.1
global amplitude
amplitude = 1.0
global dc_bias
dc_bias = 0.0

global wave_points
wave_points = generate_wave_points("square",50,0,0)
listlength = len(wave_points)

initializeGUI()
initializebuffer()
while True:
    listlength = len(wave_points)
    txlist = [0]*128
    for x in range (0, 128):
        if(halted == 0):
            txlist[x] = wave_points[(ticks + x)% listlength]
        else:
            txlist[x] = 0
    tx_128_mA_values(txlist) #if this function is called often enough it'll guarantee 65.5ms for each iteration of the loop
    updateGUI()

    if(frequency > 976.6):
        frequency = 976.5625
    if(frequency < 0.09):
        frequency = 0.1

    if(dc_bias > 2.002):
        dc_bias = 2.001
    if(dc_bias <-2.002):
        dc_bias = -2.001

    if(amplitude > 2.002):
        amplitude = 2.001
    if(amplitude < 0):
        amplitude = 0

    
    ticks += 128
    if((ticks % listlength) == 0):
        ticks = 0
    
