#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RS PB0
#define RW PB1
#define E PB2
#define DATA_OUT PORTD

#define lcd_Clear           0b00000001          // replace all characters with ASCII 'space'
#define lcd_Home            0b00000010          // return cursor to first position on first line
#define lcd_EntryMode       0b00000110          // shift cursor from left to right on read/write
#define lcd_DisplayOff      0b00001000          // turn display off
#define lcd_DisplayOn       0b00001100          // display on, cursor off, don't blink character
#define lcd_FunctionReset   0b00110000          // reset the LCD
#define lcd_FunctionSet4bit 0b00101000          // 4-bit data, 2-line display, 5 x 7 font
#define lcd_SetCursor       0b10000000          // set the cursor position

volatile float adc_value1 = 0;
volatile float adc_value2 = 0;
volatile float adc_value3 = 0;
volatile uint8_t low = 0, high = 0;

float temp_coeff = 0.22;
float V_iout = 0;
float I_p = 0;


void lcd_write_instruct_8bit(uint8_t instruction){
  PORTB &= ~(1<<RS);
  PORTB &= ~(1<<RW);
  _delay_us(1);
  DATA_OUT = instruction;
  PORTB |= (1<<E);
  _delay_us(150); //tpw
  PORTB &= ~(1<<E);
  _delay_us(1);
  PORTB |= (1<<RS);
  PORTB |= (1<<RW);
}

void lcd_write_instruct_4bit(uint8_t instruction){
  PORTB &= ~(1<<RS);
  PORTB &= ~(1<<RW);
  _delay_us(1);
  DATA_OUT = instruction;
  PORTB |= (1<<E);
  _delay_us(150);
  PORTB &= ~(1<<E);
  _delay_us(150);
  
  DATA_OUT = (instruction << 4);
  PORTB |= (1<<E);
  _delay_us(150);
  PORTB &= ~(1<<E);
  _delay_us(1);
  
  PORTB |= (1<<RS);
  PORTB |= (1<<RW); 
}

void lcd_write_data(uint8_t data){
  PORTB |= (1<<RS);
  PORTB &= ~(1<<RW);
  
  _delay_us(1);
  DATA_OUT = data;
  PORTB |= (1<<E);
  _delay_us(150);
  PORTB &= ~(1<<E);
  
   _delay_us(1);
  DATA_OUT = (data << 4);
  PORTB |= (1<<E);
  _delay_us(150);
  PORTB &= ~(1<<E);  

  _delay_us(1);
  PORTB |= (1<<RS);
  PORTB |= (1<<RW);
}

void lcd_initialize(){
  _delay_ms(40);
  lcd_write_instruct_8bit(lcd_FunctionReset);
  lcd_write_instruct_8bit(0B00100000);
  lcd_write_instruct_4bit(lcd_FunctionSet4bit);
  lcd_write_instruct_4bit(lcd_EntryMode);
  lcd_write_instruct_4bit(lcd_SetCursor);
  lcd_write_instruct_4bit(lcd_DisplayOn);
  lcd_write_instruct_4bit(lcd_Clear);


  lcd_write_instruct_4bit(0x80);
  write_string("Voltage=");

  lcd_write_instruct_4bit(0xC0);
  write_string("Current=");

  lcd_write_instruct_4bit(0x94);
  write_string("Temp=");

}

void write_value(float value){
  char buf[20];
  dtostrf(value, 3, 1, buf);
  write_string(buf);
  
}

void write_string(String sentence){
  for(int i=0; sentence[i]!='\0'; i++){
    lcd_write_data(uint8_t(sentence[i]));
  }
  
}

void set_lcd_cursor(){
  uint16_t first_line = 0x80;
  uint16_t second_line = 0xC0;
  uint16_t third_line = 0x94;
  uint16_t fourth_line = 0xE0; 
}

void init_ADC(){
    ADMUX = 0x40;
    ADCSRA |= (1<<ADPS2);
    ADCSRA |= (1<<ADPS1);
    ADCSRA |= (1<<ADPS0);
      
    ADCSRA |= (1<<ADIE);
    ADCSRA |= (1<<ADEN);
    ADCSRA |= (1<<ADSC);    
}

void fan_on(){
  if(adc_value3 > 40){
    PORTD |= (1<<PD2);
  }

  else{
    PORTD &= ~(1<<PD2);
  }
}

int main(void){
  DDRB |= (1<< RS);
  DDRB |= (1<< RW);
  DDRB |= (1<<E);
  
  DDRD = 0b11111111;
  
  DDRC &= ~(1<<PC0);
  DDRC &= ~(1<<PC1);
  DDRC &= ~(1<<PC2);

  PORTB |= (1<<RS);
  PORTB |= (1<<RW);
  PORTB &= ~(1<<E);
  
  sei();
  
  lcd_initialize();
  _delay_ms(100);
  init_ADC();
  _delay_ms(50);
  
  while(1){
    _delay_ms(10);
  }
  return 0;
}

ISR(ADC_vect){

  if(ADMUX == 0x40){
    low = ADCL;
    high = ADCH;
    
    adc_value1 = float((high << 8) | low);
    adc_value1 = float(5*adc_value1)/1024;
    adc_value1 = adc_value1/0.05;
    lcd_write_instruct_4bit(0x88);
    
    write_value(adc_value1);
    write_string("V");
    ADMUX = 0x41;
    _delay_ms(10);
  }

  else if(ADMUX == 0x41){
    low = ADCL;
    high = ADCH; 
    adc_value3 = float((high << 8) | low);
    adc_value3 = float(5*adc_value3)/1024;
    adc_value3 = adc_value3/0.01;
    //fan_on();
    lcd_write_instruct_4bit(0x98);
    write_value(adc_value3);
    write_string(" Celsius");    
    ADMUX = 0x42;
    _delay_ms(10);
  }

  else if(ADMUX == 0x42){
    low = ADCL;
    high = ADCH;
    
    adc_value2 = float((high << 8) | low);
    adc_value2 = float(5*adc_value2)/1024;
    V_iout = adc_value2;
    I_p = (V_iout - 2.5)/temp_coeff;
    lcd_write_instruct_4bit(0xC8);
    I_p = I_p/0.001;
    
    write_value(I_p);
    write_string("mA");
    ADMUX = 0x40;
    _delay_ms(10);
  }
    ADCSRA |= (1<<ADSC);
}
