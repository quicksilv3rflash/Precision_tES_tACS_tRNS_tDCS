/*
 Test program generates sawtooth wave, 4mA pk-pk
*/
 
#include "SPI.h" // necessary library for SPI to DAC7311
uint8_t cs=10; // using digital pin 10 for DAC7311 chip select
uint16_t DACregister=0;    
uint16_t value=0;
uint16_t bufferpointer= 0;
uint16_t databuffer[768]; //initialize buffer array

 
void DACwrite(uint16_t value)
{
  digitalWrite(cs, LOW); //pull *CS LOW
  SPI.transfer16(value);
  digitalWrite(cs, HIGH); //pull *CS HIGH
}
 
void setup()
{
  for(uint16_t i=0;i<768;i++){
    databuffer[i]=i;
  }
  pinMode(cs, OUTPUT); // we use this for SS pin
  SPI.begin(); // wake up the SPI bus.
  SPI.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE1));
}
 
void loop()
{

if(value<=0x6CF){
  value=0x06D0;
}
if(value>=0x3EC1){
  value=0x06D0;
}
     value += 0x10;

    DACwrite(value);
    
    delay(1);
}
