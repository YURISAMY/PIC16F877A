#include <xc.h>

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000

void main(void) {
    
OPTION_REGbits.nRBPU = 0; // liga os pull-ups internos do PORTB
 
TRISD = 0b00000000; 
PORTD = 0b00000000;
TRISB = 0b11110000;  
PORTB = 0b11111110;          // só RB0 em 0

while(1){

__delay_us(50);

PORTD = 0;   
    
if (!PORTBbits.RB4){
   PORTD =  0b11000000;
}

if (!PORTBbits.RB5){
   PORTD =  0b00000011;
}

if (!PORTBbits.RB6){
  PORTD =  0b00001100;
}

if (!PORTBbits.RB7){
  PORTD =  0b00110000;
}
}


}