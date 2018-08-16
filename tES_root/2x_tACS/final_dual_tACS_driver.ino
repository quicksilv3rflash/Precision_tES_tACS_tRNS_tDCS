#include <SPI.h> // necessary library for SPI data transmission to DAC7311
#include <LiquidCrystal.h>

//define the sine function lookup table
const int16_t sine_table[256] = {0, 9, 18, 26, 35, 44, 53, 61, 70, 78, 87, 95, 104, 112, 121, 129, 137, 145, 153, 161, 169, 176, 184, 192, 199, 206, 213, 220, 227, 234, 240, 247, 253, 259, 265, 271, 277, 282, 288, 293, 298, 302, 307, 311, 316, 320, 324, 327, 331, 334, 337, 340, 343, 345, 347, 349, 351, 353, 354, 355, 356, 357, 358, 358, 358, 358, 358, 357, 356, 355, 354, 353, 351, 349, 347, 345, 343, 340, 337, 334, 331, 327, 324, 320, 316, 311, 307, 302, 298, 293, 288, 282, 277, 271, 265, 259, 253, 247, 240, 234, 227, 220, 213, 206, 199, 192, 184, 176, 169, 161, 153, 145, 137, 129, 121, 112, 104, 95, 87, 78, 70, 61, 53, 44, 35, 26, 18, 9, 0, -9, -18, -26, -35, -44, -53, -61, -70, -78, -87, -95, -104, -112, -121, -129, -137, -145, -153, -161, -169, -176, -184, -192, -199, -206, -213, -220, -227, -234, -240, -247, -253, -259, -265, -271, -277, -282, -288, -293, -298, -302, -307, -311, -316, -320, -324, -327, -331, -334, -337, -340, -343, -345, -347, -349, -351, -353, -354, -355, -356, -357, -358, -358, -358, -358, -358, -357, -356, -355, -354, -353, -351, -349, -347, -345, -343, -340, -337, -334, -331, -327, -324, -320, -316, -311, -307, -302, -298, -293, -288, -282, -277, -271, -265, -259, -253, -247, -240, -234, -227, -220, -213, -206, -199, -192, -184, -176, -169, -161, -153, -145, -137, -129, -121, -112, -104, -95, -87, -78, -70, -61, -53, -44, -35, -26, -18, -9};

volatile uint8_t timer_flag = 0;//timer flag is set every 16us, counter_a and counter_b are incremented every 16us
volatile uint16_t counter_a = 0;
volatile uint16_t counter_b = 0;

uint16_t buttontimer = 0;
uint8_t debounce = 0;

uint8_t whichscreen = 0; //0=channel A screen, 1=channel B screen, 2=MODE screen
int8_t whichfield = 0; //0=frequency, 1=mA_ac, 2=mA_dc
int8_t whichmode = 0; //0=normal, 1=bursts, 2=phase, 3=cross-frequency

int16_t phase = 180;//phase shift to be applied B_DEG = A_DEGREES + phase

uint8_t peakortrough = 1; //1=peak,0=trough (activation for cross frequency mode)
int16_t cf_ms_on = 50; //milliseconds on for cross frequency mode
uint8_t cf_on = 0; //designates whether cross-frequency channel B is on or off
uint8_t index_zero_cross = 0; // indicates if the channel A index (sine table pointer) has crossed zero

int16_t burst_ms_on = 50; //ms on for burst mode
int16_t burst_ms_off = 117; //ms off for burst mode

unsigned long prev_millis = 0; //previous millis() value
uint8_t burst_on = 1; //designates whether burst mode is on or off

//output frequencies
uint8_t hertz_a = 6;
uint8_t hertz_b = 6;

//sine wave peak-baseline amplitudes in 0.1mA increments
int8_t decimilliamps_a = 10;
int8_t decimilliamps_b = 10;

//offset current in tenths of milliamps
int8_t decimilliamps_offset_a = 0;
int8_t decimilliamps_offset_b = 0;


// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

uint8_t lcd_key = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// read the buttons
int read_LCD_buttons()
{
 int adc_key_in = analogRead(0);
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnLEFT; 
 if (adc_key_in < 790)  return btnSELECT;   
 return btnNONE;  // when all others fail, return this...
}

void DACwrite(uint16_t value, uint8_t cs){  //Why is this a separate function? So you can easily play with
                                //the DAC without following my specific read/write/buffer protocol!                                                                
  digitalWrite(cs, LOW);        //pull *CS LOW ("Hey, DAC, listen! We're talking to you!")  
  SPI.transfer16(value);        //write the value to the DAC
  digitalWrite(cs, HIGH);       //pull *CS HIGH ("DAC, we're done talking to you. Output the value we sent you.")  
}

ISR(TIMER2_OVF_vect){           //When timer2 overflows,
  timer_flag = 1;
  counter_a++;
  counter_b++;
}

void UpdateScreen(void){
  uint8_t dummyoffset = 0;
  lcd.setCursor(0,0);
  if(whichscreen == 0){
    lcd.print("CHANNEL A ");
    if(hertz_a < 100){
      lcd.print(" ");
    }
    if(hertz_a < 10){
      lcd.print(" ");
    }
    if(whichfield == 0){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    lcd.print(hertz_a);
    lcd.print("Hz");
    lcd.setCursor(0,1);
    lcd.print("mA:");
    if(whichfield == 1){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    lcd.print(decimilliamps_a/10);
    lcd.print(".");
    lcd.print(decimilliamps_a - 10*(decimilliamps_a/10));
    lcd.print("AC");
    if(whichfield == 2){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    if(decimilliamps_offset_a < 0){
      lcd.print("-");
      dummyoffset = -decimilliamps_offset_a;
    }else{
      dummyoffset = decimilliamps_offset_a;
    }
    lcd.print(dummyoffset/10);
    lcd.print(".");
    lcd.print(dummyoffset - 10*(dummyoffset/10));
    lcd.print("DC ");    
  }else if(whichscreen == 1){
    lcd.print("CHANNEL B ");
    if(whichmode != 2){
      if(hertz_b < 100){
        lcd.print(" ");
      }
      if(hertz_b < 10){
        lcd.print(" ");
      }
    }
    if(whichfield == 0){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    if(whichmode != 2){
      lcd.print(hertz_b);
    }else{
      lcd.print("_A_");
    }
    lcd.print("Hz");
    lcd.setCursor(0,1);
    lcd.print("mA:");
    if(whichfield == 1){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    lcd.print(decimilliamps_b/10);
    lcd.print(".");
    lcd.print(decimilliamps_b - 10*(decimilliamps_b/10));
    lcd.print("AC");
    if(whichfield == 2){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    if(decimilliamps_offset_b < 0){
      lcd.print("-");
      dummyoffset = -decimilliamps_offset_b;
    }else{
      dummyoffset = decimilliamps_offset_b;
    }
    lcd.print(dummyoffset/10);
    lcd.print(".");
    lcd.print(dummyoffset - 10*(dummyoffset/10));
    lcd.print("DC ");    
  }else{
    lcd.print("MODE:");
    if(whichfield == 0){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    if(whichmode == 0){
      lcd.print("NORMAL    ");
    }else if(whichmode == 1){
      lcd.print("BURSTS    ");
    }else if(whichmode == 2){
      lcd.print("PHASE SET ");
    }else if(whichmode == 3){
      lcd.print("CROSS-FREQ");
    }
    lcd.setCursor(0,1);
    if(whichmode == 0){
      lcd.print("OUTPUTS AS SHOWN");
    }else if(whichmode == 1){
      if(burst_ms_on < 100){
        lcd.print(" ");
      }
      if(burst_ms_on < 10){
        lcd.print(" ");
      }
      lcd.print(burst_ms_on);
      if(whichfield == 1){
        lcd.print("<");
      }else{
        lcd.print("m");
      }
      lcd.print("sON");
      if(burst_ms_off < 1000){
        lcd.print(" ");
      }
      if(burst_ms_off < 100){
        lcd.print(" ");
      }
      if(burst_ms_off < 10){
        lcd.print(" ");
      }
      lcd.print(burst_ms_off);
      if(whichfield == 2){
        lcd.print("<");
      }else{
        lcd.print("m");
      }
      lcd.print("sOFF");
    }else if(whichmode == 2){
      lcd.print("B_DEG=A_DEG");
      if(whichfield == 1){
        lcd.print(">");
      }else{
        lcd.print(" ");
      }      
      if(phase >= 0){
        lcd.print("+");
      }
      lcd.print(phase);
      if(phase < 100 && phase > -100){
        lcd.print(" ");
      }
      if(phase < 10 && phase > -10){
        lcd.print(" ");
      }
    }else if(whichmode == 3){
      lcd.print("IF[A");
      if(whichfield == 1){
        lcd.print(">");
      }else{
        lcd.print("]");
      }
      if(peakortrough == 1){
        lcd.print("PEAK,");
      }else if(peakortrough == 0){
        lcd.print("TROU,");
      }
      lcd.print("B");
      if(cf_ms_on < 100){
      lcd.print("^");
      }
      if(cf_ms_on < 10){
        lcd.print(" ");
      }
      lcd.print(cf_ms_on);
      if(whichfield == 2){
        lcd.print("<");
      }else{
        lcd.print("m");
      }
      lcd.print("s");
    }
  }
  delay(1);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(2, OUTPUT); //pin 2 is *CS for channel A
  pinMode(3, OUTPUT); //pin 3 is *CS for channel B
  cli();                        //first we disable interrupts,

  SPI.begin();                  //start SPI
  
  SPI.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE1)); 
                                //max SPI transmit speed is 50MHz, most significant bit first, SPI mode 1 (CPOL = 0, CPHA = 1)
//-------------------------------------------------------------------------------
                                //setting up timer2 to generate an overflow interrupt every 32us:
                                
  TCCR2A &= 0xFC;               //TCCR2A configuration:
                                //set timer2 to normal (not CTC) mode.
                                //timer2 simply always runs, overflows at >255 (it's a uint8_t), and sets an interrupt flag when it overflows.
     
  TCCR2B |= 0x01;               //TCCR2B configuration:
  TCCR2B &= 0xF3;               //set clock prescaler (divider) to 8, so effective frequency is:
                                //16000000_CPU_SPEED/(1_PRESCALER * 256_CLOCKS_TO_OVERFLOW) = 62.5 kHz, overflow every 16us                                    
  
  TIMSK2 |= (1 << TOIE2);       //TIMSK2 (Timer Interrupt Mask 2) configuration:
                                //enable overflow interrupt. Timer Overflow Inerrupt Enable 2(as in, the one for timer2)
                                
  TCNT2 = 0x00;                 //TCNT2 (Timer/Counter register 2) configuration:
                                //set the timer2 counter to zero, to be sure it ACTUALLY STARTS at zero when our loop starts.
                                //Who knows what value it powered on with?
//-------------------------------------------------------------------------------
                                //all our sensitive one-time setup stuff is done, so we can now safely  
  sei();                        //enable interrupts
  lcd.begin(16, 2);              // start the library
  UpdateScreen();
}

void loop() {
  if(whichmode == 0){//normal mode
    if(timer_flag == 1){
    timer_flag = 0;
    if(counter_a > 62500/hertz_a){
      counter_a = 0;
    }
    if(counter_b > 62500/hertz_b){
      counter_b = 0;
    }
    uint8_t index_a = (uint8_t)(((uint32_t)64*(uint32_t)counter_a*(uint32_t)hertz_a)/15625);
    uint8_t index_b = (uint8_t)(((uint32_t)64*(uint32_t)counter_b*(uint32_t)hertz_b)/15625);
    uint16_t dacwrite_a = (0x22C8 + (decimilliamps_a*sine_table[index_a]) - (358*decimilliamps_offset_a));
    uint16_t dacwrite_b = (0x22C8 + (decimilliamps_b*sine_table[index_b]) - (358*decimilliamps_offset_b));
    if(dacwrite_a < 0x06D0){
      dacwrite_a = 0x06D0;
    }
    if(dacwrite_b < 0x06D0){
      dacwrite_b = 0x06D0;
    }
    if(dacwrite_a > 0x3EC0){
      dacwrite_a = 0x3EC0;
    }
    if(dacwrite_b > 0x3EC0){
      dacwrite_b = 0x3EC0;
    }
    DACwrite(dacwrite_a, 2);
    DACwrite(dacwrite_b, 3);        
  }
  }else if(whichmode == 1){//burst mode
    unsigned long millis_now = millis();
    if((burst_on == 1 && millis_now - prev_millis >= burst_ms_on)||(burst_on == 0 && millis_now - prev_millis >= burst_ms_off)){
      burst_on ^= 0x01;
      prev_millis = millis_now;
    }
    if(burst_on == 1){
      if(timer_flag == 1){
    timer_flag = 0;
    if(counter_a > 62500/hertz_a){
      counter_a = 0;
    }
    if(counter_b > 62500/hertz_b){
      counter_b = 0;
    }
    uint8_t index_a = (uint8_t)(((uint32_t)64*(uint32_t)counter_a*(uint32_t)hertz_a)/15625);
    uint8_t index_b = (uint8_t)(((uint32_t)64*(uint32_t)counter_b*(uint32_t)hertz_b)/15625);
    uint16_t dacwrite_a = (0x22C8 + (decimilliamps_a*sine_table[index_a]) - (358*decimilliamps_offset_a));
    uint16_t dacwrite_b = (0x22C8 + (decimilliamps_b*sine_table[index_b]) - (358*decimilliamps_offset_b));
    if(dacwrite_a < 0x06D0){
      dacwrite_a = 0x06D0;
    }
    if(dacwrite_b < 0x06D0){
      dacwrite_b = 0x06D0;
    }
    if(dacwrite_a > 0x3EC0){
      dacwrite_a = 0x3EC0;
    }
    if(dacwrite_b > 0x3EC0){
      dacwrite_b = 0x3EC0;
    }
    DACwrite(dacwrite_a, 2);
    DACwrite(dacwrite_b, 3);        
  }
    }else{
      DACwrite(0x22C8,2);
      DACwrite(0x22C8,3);
    }
    
  }else if(whichmode == 2){//phase mode
    if(timer_flag == 1){
    timer_flag = 0;
    if(counter_a > 62500/hertz_a){
      counter_a = 0;
    }
    uint8_t index_a = (uint8_t)(((uint32_t)64*(uint32_t)counter_a*(uint32_t)hertz_a)/15625);
    uint8_t index_b = (uint8_t)((((uint32_t)64*(uint32_t)counter_a*(uint32_t)hertz_a)/15625) + ((256*(int32_t)phase)/360));
    uint16_t dacwrite_a = (0x22C8 + (decimilliamps_a*sine_table[index_a]) - (358*decimilliamps_offset_a));
    uint16_t dacwrite_b = (0x22C8 + (decimilliamps_b*sine_table[index_b]) - (358*decimilliamps_offset_b));
    if(dacwrite_a < 0x06D0){
      dacwrite_a = 0x06D0;
    }
    if(dacwrite_b < 0x06D0){
      dacwrite_b = 0x06D0;
    }
    if(dacwrite_a > 0x3EC0){
      dacwrite_a = 0x3EC0;
    }
    if(dacwrite_b > 0x3EC0){
      dacwrite_b = 0x3EC0;
    }
    DACwrite(dacwrite_a, 2);
    DACwrite(dacwrite_b, 3);        
  }
  }else if(whichmode == 3){//cross-frequency burst mode
    unsigned long millis_now = millis();
    if(index_zero_cross == 1){
      index_zero_cross = 0;
      prev_millis = millis_now;
    }
    cf_on = 0;
    if(peakortrough == 0){
      if(millis_now - prev_millis > (250/hertz_a) - (cf_ms_on/2)){
        if(millis_now - prev_millis < (250/hertz_a) + (cf_ms_on/2)){
          cf_on = 1;
        }
      }
    }
    if(peakortrough == 1){
      if(millis_now - prev_millis > (750/hertz_a) - (cf_ms_on/2)){
        if(millis_now - prev_millis < (750/hertz_a) + (cf_ms_on/2)){
          cf_on = 1;
          
        }
        
      }
      
    }


    
    if(timer_flag == 1){
    timer_flag = 0;
    if(counter_a > 62500/hertz_a){
      counter_a = 0;
      index_zero_cross = 1;
    }
    if(counter_b > 62500/hertz_b){
      counter_b = 0;
    }
    uint8_t index_a = (uint8_t)(((uint32_t)64*(uint32_t)counter_a*(uint32_t)hertz_a)/15625);
    uint8_t index_b = (uint8_t)(((uint32_t)64*(uint32_t)counter_b*(uint32_t)hertz_b)/15625);
    uint16_t dacwrite_a = (0x22C8 + (decimilliamps_a*sine_table[index_a]) - (358*decimilliamps_offset_a));
    uint16_t dacwrite_b = (0x22C8 + (decimilliamps_b*sine_table[index_b]) - (358*decimilliamps_offset_b));
    if(dacwrite_a < 0x06D0){
      dacwrite_a = 0x06D0;
    }
    if(dacwrite_b < 0x06D0){
      dacwrite_b = 0x06D0;
    }
    if(dacwrite_a > 0x3EC0){
      dacwrite_a = 0x3EC0;
    }
    if(dacwrite_b > 0x3EC0){
      dacwrite_b = 0x3EC0;
    }
    DACwrite(dacwrite_a, 2);
    if(cf_on == 1){
      DACwrite(dacwrite_b, 3);
    }else{//if cf_on is not set,
      DACwrite(0x22C8, 3);//set DAC to output 0mA on channel B
    }      
  }
  


    
  }
  

  lcd_key = read_LCD_buttons();
   if(lcd_key != btnNONE){
      UpdateScreen();
      switch (lcd_key){
         case btnRIGHT:
           {
            if(debounce == 0){
              debounce = 1;
              whichfield++;
              }
            if(whichfield > 2){
              whichfield = 0;
            }
           break;
           }
         case btnLEFT:
           {
            if(debounce == 0){
              debounce = 1;
              whichfield--;
            }
            if(whichfield < 0){
              whichfield = 2;
            }
           break;
           }
         case btnUP:
           {
           buttontimer++;
           if(buttontimer > 75 && buttontimer % 4 == 0){
            if(whichscreen == 0){
              if(whichfield == 0){
                hertz_a++;
              }else if(whichfield == 1 && decimilliamps_offset_a + decimilliamps_a < 24 && -decimilliamps_offset_a + decimilliamps_a < 24){
                decimilliamps_a++;
              }else if(whichfield == 2 && decimilliamps_offset_a + decimilliamps_a < 24){
                decimilliamps_offset_a++;
              }
           }else if(whichscreen == 1){
              if(whichfield == 0){
                hertz_b++;
              }else if(whichfield == 1 && decimilliamps_offset_b + decimilliamps_b < 24 && -decimilliamps_offset_b + decimilliamps_b < 24){
                decimilliamps_b++;
              }else if(whichfield == 2 && decimilliamps_offset_b + decimilliamps_b < 24){
                decimilliamps_offset_b++;
              }
           }else{
              //mode stuff_rapidscroll
              if(whichfield == 1 && whichmode == 1){
                burst_ms_on++;
              }
              if(whichfield == 2 && whichmode == 1){
                burst_ms_off++;
              }
              if(burst_ms_on > 9999){
                burst_ms_on = 9999;
              }
              if(burst_ms_off > 9999){
                burst_ms_off = 9999;
              }
              if(burst_ms_on < 0){
                burst_ms_on = 0;
              }
              if(burst_ms_off < 0){
                burst_ms_off = 0;
              }
              if(whichfield == 1 && whichmode == 2){
                phase++;
              }
              if(whichfield == 2 && whichmode == 3){
              cf_ms_on++;
              if(cf_ms_on > 999){
                cf_ms_on = 999;
              }
            }
           }
           }else{
            if(debounce == 0){
              debounce = 1;
              if(whichscreen == 0){
                if(whichfield == 0){
                hertz_a++;
              }else if(whichfield == 1 && decimilliamps_offset_a + decimilliamps_a < 24 && -decimilliamps_offset_a + decimilliamps_a < 24){
                decimilliamps_a++;
              }else if(whichfield == 2 && decimilliamps_offset_a + decimilliamps_a < 24){
                decimilliamps_offset_a++;
              }
           }else if(whichscreen == 1){
                if(whichfield == 0){
                hertz_b++;
              }else if(whichfield == 1 && decimilliamps_offset_b + decimilliamps_b < 24 && -decimilliamps_offset_b + decimilliamps_b < 24){
                decimilliamps_b++;
              }else if(whichfield == 2 && decimilliamps_offset_b + decimilliamps_b < 24){
                decimilliamps_offset_b++;
              }
           }else{
            if(whichfield == 1 && whichmode == 2){
                phase++;
                if(phase > 180){
                  phase = 180;
                }
              }
            if(whichfield == 2 && whichmode == 3){
              cf_ms_on++;
              if(cf_ms_on > 999){
                cf_ms_on = 999;
              }
            }
            if(whichfield == 1 && whichmode == 3){
              peakortrough ^= 0x01;
            }
              if(whichfield == 0){
                whichmode++;
              if(whichmode > 3){
                whichmode = 0;
              }
              }
              if(whichfield == 1 && whichmode == 1){
                burst_ms_on++;
              }
              if(whichfield == 2 && whichmode == 1){
                burst_ms_off++;
              }
              if(burst_ms_on > 9999){
                burst_ms_on = 9999;
              }
              if(burst_ms_off > 9999){
                burst_ms_off = 9999;
              }
              if(burst_ms_on < 0){
                burst_ms_on = 0;
              }
              if(burst_ms_off < 0){
                burst_ms_off = 0;
              }
              //more mode stuff
           }
            }
           }
           if(phase > 180){
                  phase = 180;
                }
           if(hertz_a > 200){
            hertz_a = 200;
           }
           if(hertz_b > 200){
            hertz_b = 200;
           }
           if(hertz_a == 0){
            hertz_a = 1;
           }
           if(hertz_b == 0){
            hertz_b = 1;
           }
           if(decimilliamps_a < 0){
            decimilliamps_a = 0;
           }
           if(decimilliamps_b < 0){
            decimilliamps_b = 0;
           }
           if(decimilliamps_a > 20){
            decimilliamps_a = 20;
           }
           if(decimilliamps_b > 20){
            decimilliamps_b = 20;
           }
           if(decimilliamps_offset_a > 20){
            decimilliamps_offset_a = 20;
           }
           if(decimilliamps_offset_a < -20){
            decimilliamps_offset_a = -20;
           }
           if(decimilliamps_offset_b > 20){
            decimilliamps_offset_b = 20;
           }
           if(decimilliamps_offset_b < -20){
            decimilliamps_offset_b = -20;
           }
           break;
           }
         case btnDOWN:
           {
           buttontimer++;
           if(buttontimer > 75 && buttontimer % 4 == 0){
            if(whichscreen == 0){
              if(whichfield == 0){
                hertz_a--;
              }else if(whichfield == 1){
                decimilliamps_a--;
              }else if(whichfield == 2 && -decimilliamps_offset_a + decimilliamps_a < 24){
                decimilliamps_offset_a--;
              }
           }else if(whichscreen == 1){
              if(whichfield == 0){
                hertz_b--;
              }else if(whichfield == 1){
                decimilliamps_b--;
              }else if(whichfield == 2 && -decimilliamps_offset_b + decimilliamps_b < 24){
                decimilliamps_offset_b--;
              }
           }else{
              //mode stuff _rapidscroll
              if(whichfield == 1 && whichmode == 1){
                burst_ms_on--;
              }
              if(whichfield == 2 && whichmode == 1){
                burst_ms_off--;
              }
              if(burst_ms_on > 9999){
                burst_ms_on = 9999;
              }
              if(burst_ms_off > 9999){
                burst_ms_off = 9999;
              }
              if(burst_ms_on < 0){
                burst_ms_on = 0;
              }
              if(burst_ms_off < 0){
                burst_ms_off = 0;
              }
              if(whichfield == 1 && whichmode == 2){
                phase--;
                }
              if(whichfield == 2 && whichmode == 3){
              cf_ms_on--;
              if(cf_ms_on < 0){
                cf_ms_on = 0;
              }
            }
           }
           }else{
            if(debounce == 0){
              debounce = 1;
              if(whichscreen == 0){
                if(whichfield == 0){
                hertz_a--;
              }else if(whichfield == 1){
                decimilliamps_a--;
              }else if(whichfield == 2 && -decimilliamps_offset_a + decimilliamps_a < 24){
                decimilliamps_offset_a--;
              }
           }else if(whichscreen == 1){
                if(whichfield == 0){
                hertz_b--;
              }else if(whichfield == 1){
                decimilliamps_b--;
              }else if(whichfield == 2 && -decimilliamps_offset_b + decimilliamps_b < 24){
                decimilliamps_offset_b--;
              }
           }else{
            if(whichfield == 1 && whichmode == 1){
                burst_ms_on--;
              }
              if(whichfield == 2 && whichmode == 1){
                burst_ms_off--;
              }
              if(burst_ms_on > 9999){
                burst_ms_on = 9999;
              }
              if(burst_ms_off > 9999){
                burst_ms_off = 9999;
              }
              if(burst_ms_on < 0){
                burst_ms_on = 0;
              }
              if(burst_ms_off < 0){
                burst_ms_off = 0;
              }
            if(whichfield == 1 && whichmode == 2){
                phase--;
                if(phase < -180){
                  phase = -180;
                }
              }
            if(whichfield == 2 && whichmode == 3){
              cf_ms_on--;
              if(cf_ms_on < 0){
                cf_ms_on = 0;
              }
            }
            if(whichfield == 1 && whichmode == 3){
              peakortrough ^= 0x01;
            }
            if(whichfield == 0){
                whichmode--;
              if(whichmode < 0){
                whichmode = 3;
              }
           }
           }
            }
           }
           if(phase < -180){
                  phase = -180;
                }
           if(hertz_a > 200){
            hertz_a = 200;
           }
           if(hertz_b > 200){
            hertz_b = 200;
           }
           if(hertz_a == 0){
            hertz_a = 1;
           }
           if(hertz_b == 0){
            hertz_b = 1;
           }
           if(decimilliamps_a < 0){
            decimilliamps_a = 0;
           }
           if(decimilliamps_b < 0){
            decimilliamps_b = 0;
           }
           if(decimilliamps_a > 20){
            decimilliamps_a = 20;
           }
           if(decimilliamps_b > 20){
            decimilliamps_b = 20;
           }
           if(decimilliamps_offset_a > 20){
            decimilliamps_offset_a = 20;
           }
           if(decimilliamps_offset_a < -20){
            decimilliamps_offset_a = -20;
           }
           if(decimilliamps_offset_b > 20){
            decimilliamps_offset_b = 20;
           }
           if(decimilliamps_offset_b < -20){
            decimilliamps_offset_b = -20;
           }
           break;
           }
         case btnSELECT:
           {
           if(debounce == 0){
            debounce = 1;
            whichscreen++;
            if(whichscreen >= 3){
              whichscreen = 0;
            }
           }
           break;
           }
        }
     }else{
      debounce = 0;
      buttontimer = 0;
     }
  
}
