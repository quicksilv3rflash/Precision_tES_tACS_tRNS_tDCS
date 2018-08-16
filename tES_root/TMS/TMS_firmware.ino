// required to address liquid crystal display
#include <LiquidCrystal.h>

//global variable that stores capacitor voltage. Risk of device explosion if this value exceeds 1350. Updated in loop().
volatile uint16_t v_cap = 0;

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
uint8_t counter = 0;
long delta_millis = 0;
long prev_millis = 0;
// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int setoutput(uint8_t mode){ //0 = no outputs, 1 = charge, 2 = pulse, 3 = bleed, 4 = blarge
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);  
  switch(mode){
    case 0:{//do nothing, no outputs
      break;
    }
    case 1:{//charge
      digitalWrite(1, HIGH);
      break;
    }
    case 2:{//pulse
      digitalWrite(0, HIGH);
      delay(1);
      digitalWrite(0, LOW);
      break;
    }
    case 3:{//bleed
      digitalWrite(2, HIGH);
      break;
    }
    case 4:{//blarge
      digitalWrite(1, HIGH);
      digitalWrite(2, HIGH);
    }
  }
}

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 // For VMA203 V1.1 use this threshold
 /*
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  

 // For VMA203 V1.0 comment the other threshold and use the one below:
*/
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnLEFT; 
 if (adc_key_in < 790)  return btnSELECT;   



 return btnNONE;  // when all others fail, return this...
}

void setup(){
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0,0);
  lcd.print("TMS Control Mod."); // print a simple message
}
 
void loop(){
  uint8_t auto_var = 0;
  uint16_t cap_v_sense = 0;  
  cap_v_sense = analogRead(1);
  v_cap = (11*cap_v_sense) + 340; //a linear approximation. close enough for our operating range 600v - 1200v for the main capacitor
  
 lcd.setCursor(6,1);            // move cursor to second line "1" and 9 spaces over
 lcd.print(v_cap);
 lcd.print("V");
 lcd.print(delta_millis);      // display adc reading from main capacitor voltage


 lcd.setCursor(0,1);            // move to the begining of the second line
 lcd_key = read_LCD_buttons();  // read the buttons

 switch (lcd_key)               // depending on which button was pushed, we perform an action
 {
   case btnRIGHT:
     {
      setoutput(3);
     lcd.print("BLEED ");
     break;
     }
   case btnLEFT:
     {
      setoutput(1);
     lcd.print("CHARGE");
     break;
     }
   case btnUP:
     {
      setoutput(2);
     lcd.print("PULSE ");
     break;
     }
   case btnDOWN:
     {
     lcd.print("AUTO  ");
     auto_var = 1;
     break;
     }
   case btnSELECT:
     {
     lcd.print("SELECT");
     break;
     }
     case btnNONE:
     {
      setoutput(0);
     lcd.print("NULL  ");
     break;
     }
 }
  if(auto_var == 1){
    if(v_cap < 800){
      setoutput(4);
    }else{
      counter ++;
      if(counter > 1){
        delta_millis = millis() - prev_millis;
      }
      prev_millis = millis();
      
      setoutput(0);
      delay(10);
      setoutput(2);
      setoutput(0);
      delay(10);
    }
    
  }
}

