//JTAG communication with Pic33 UNO and UART communication at
//9600 baudratewith the computer receiver to read received data.
#include <Arduino.h>
#define TMS 8  // JTAG ports
#define TCK 9
#define TDO 10
#define TDI 11
#define FOSC 16000000 // Clock Speed do atmega328
#define BAUD 9600    // baud rate escolhida
#define MYUBRR (FOSC/16/BAUD - 1) // Used on Uart_init()

///////UART communication with Putty////////
void UART_init(unsigned int ubrr) {// UART 8 data bits + 1 stop bit + 9600 baud rate + no parity
  UBRR0H = (unsigned char)(ubrr>>8); //Set baud rate 
  UBRR0L = (unsigned char)ubrr;      //Set baud rate 
  UCSR0B = (1<<RXEN0)|(1<<TXEN0); //Enable receiving; enable transmiting; set Zn2=0 (for 8 bit data)
  UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);//set Zn1=1 and Zn2=1 (for 8 bit data, in conjunction with Zn2=0)
}

void send_data(uint8_t data) {
  while (!( UCSR0A & (1<<UDRE0))); //while not register empty. again, it waits for empty register; for data currently there to be read
  UDR0 = data; // writes the data into the data register
}

int get_data(void) {
  while (!(UCSR0A & (1<<RXC0)));// RXC0 is "Receive Complete" - is high when there is data to be read (is 0 while there is no data) -> the while loop runs while there is no data
  return UDR0; //returns data here
}
// End of UART communication with Putty //


//JTAG TAP Controller Navigation functions//
void clock(void){ //one clock cycle (2ms)
  digitalWrite(TCK,1);
  delay(1);
  digitalWrite(TCK,0);
  delay(1);
}

void TMSx1(uint8_t x){ //x clock cycles with TMS set HIGH
  digitalWrite(TMS, 1);
  int i;
  for ( i = 1 ; i <= x; i++ ) { 
    clock();
  }
}

void TMSx0(uint8_t x){ //x clock cycles with TMS set LOW
  digitalWrite(TMS, 0);
  int i;
  for ( i = 1 ; i <= x; i++ ) { 
    clock();
  }
}

void soft_reset(void){ //goes to test logic reset state
  TMSx1(5);
}

void go_to_Shift_IR(void){ //from test-logic reset state
  TMSx0(1); //idle
  TMSx1(2); //select IR scan
  TMSx0(2); //Shift IR
}//end of TAP navigation functions//


//JTAG main functions - give instruction, get ID code, set pin values (change LD5 state), get pin values (get button state) 
//the TDI is shifted upon EXITING the shift IR-state and the TDO's first value is read directly upon ENTERING (no shift required)  
void Instruction(uint8_t instruction){ //soft resets, gives instruction, goes to select-DR-Scan state
  soft_reset();
  go_to_Shift_IR();
  Serial.print("\r\nSending instruction (BIN): ");
  Serial.print(instruction,BIN);
  uint8_t store = 0;
  int i;
  for (i=0; i<=4; i++){
    if (i!=4){ //it's not the last shift
      digitalWrite(TDI,(instruction & (1<<i))>>i);
      store |= (digitalRead(TDO)<<i); //read TDO BEFORE shifting
      TMSx0(1);
    }
    else{ //it's the last shift
      digitalWrite(TDI,(instruction & (1<<i))>>i);
      store |= (digitalRead(TDO)<<i);
      TMSx1(3); //we exit shift-IR and go to Select-DR scan
    }
  }
  Serial.print("\r\ninstruction capture (BIN): ");
  Serial.print(store,BIN);
  Serial.print("\r\n");
}

void ID(void){ //starting on Select-DR-Scan, due to the Instruction func, shifts the DR 32 times to get the ID code
  TMSx0(2); //goes to Shift-DR
  uint32_t ID = 0;
  int i;
  for (i=0; i<=31; i++){
    ID |= ((uint32_t) (digitalRead(TDO)) << i);
    if (i!=31){
      TMSx0(1); //shift back into shit-dr state
    }
    else{ //last shift
      TMSx1(1); //exit1-DR
    }
  }
  Serial.print("\r\nID code (BIN): ");
  Serial.print(ID,BIN);
  Serial.print("\r\n");
}

void PINS_LED5(uint8_t set_pins[19]){ //starting on Select-DR-Scan, shifts the DR 148 times, sending input pin values and storing current ones
  TMSx0(2); //goes to Shift-DR
  uint8_t store[19] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //19 bytes (152 bits in total -> we want  to store 148)
  int i, j;
  for (j=0; j<= 18; j++){
    if (j!=18){ //it's not the last shifted bit
      for (i=0; i<=7; i++){
        digitalWrite(TDI, ((uint8_t) ((set_pins[j] & (1<<(7-i)) )>>(7-i)) ) ); 
        store[j] |= ((uint8_t) (digitalRead(TDO)<<(7-i)) );
        TMSx0(1); //shift
      }
    }
    else{ //we are on the last byte
      for (i=0; i<=3; i++){ 
        if (i!=3){ // not the last bit
          digitalWrite(TDI, ((uint8_t) ((set_pins[j] & (1<<(7-i)) )>>(7-i)) ) ); 
          store[j] |= ((uint8_t) (digitalRead(TDO)<<(7-i)));
          TMSx0(1); //shift 
        }
        else{ // last bit
          digitalWrite(TDI, ((uint8_t) ((set_pins[j] & (1<<(7-i)) )>>(7-i)) ) ); 
          store[j] |= ((uint8_t) (digitalRead(TDO)<<(7-i)));
          TMSx1(2); //shift last time and UPDATE DR 
        }
      }
    }
  }
  //n=19 e n=20 are the byte 2: [16 17 18 19 20 21 22 23] (bits 3 and 4, where 0 is LSB)
  if (set_pins[2] & (1<<4)){
    Serial.println("LD5 turned ON");
  }
  else{
    Serial.println("LD5 turned OFF");
  }
}

void PINS_button(uint8_t set_pins[19]){ //starting on Select-DR-Scan, shifts the DR 148 times, sending input pin values and storing current ones
  TMSx0(2); //goes to Shift-DR
  uint8_t store[19] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //19 bytes (152 bits in total -> we want  to store 148)
  int i, j;
  for (j=0; j<= 18; j++){
    if (j!=18){ //it's not the last shifted bit
      for (i=0; i<=7; i++){
        digitalWrite(TDI, ((uint8_t) ((set_pins[j] & (1<<(7-i)) )>>(7-i)) ) ); 
        store[j] |= ((uint8_t) (digitalRead(TDO)<<(7-i)) );
        TMSx0(1); //shift
      }
    }
    else{ //we are on the last byte
      for (i=0; i<=3; i++){ 
        if (i!=3){ // not the last bit
          digitalWrite(TDI, ((uint8_t) ((set_pins[j] & (1<<(7-i)) )>>(7-i)) ) ); 
          store[j] |= ((uint8_t) (digitalRead(TDO)<<(7-i)));
          TMSx0(1); //shift 
        }
        else{ // last bit
          digitalWrite(TDI, ((uint8_t) ((set_pins[j] & (1<<(7-i)) )>>(7-i)) ) ); 
          store[j] |= ((uint8_t) (digitalRead(TDO)<<(7-i)));
          TMSx1(2); //shift last time and UPDATE DR 
        }
      }
    }
  } //n=3 is the byte 0: [0 1 2 3 4 5 6 7] (bit 4, where 0 is LSB)
  if (store[0] & (1<<4)){
    Serial.println("Button is ON");
  }
  else{
    Serial.println("Button is OFF");
  }
}

///////////////////////Setup///////////////////
void setup(){
  UART_init(MYUBRR);//USART initiallizing functions
  pinMode(TMS, OUTPUT); //JTAG PINs
  pinMode(TCK, OUTPUT);
  pinMode(TDI, OUTPUT); 
  pinMode(TDO, INPUT); 
  digitalWrite(TMS,1); //TMS=1   Initial value
  digitalWrite(TCK,0); //TCK=0   Initial value
  digitalWrite(TDI,0); //TMS=0   Initial value
}//////////////End of setup//////////////////


/////////////////Main Loop//////////////////
void loop() {
  uint8_t dataPC = get_data(); //input variable from UART communications
  delay(100);
  if (dataPC == 0x64){ // ASCII "d" in hex 
  /////get ID CODE/////   
    Instruction(0b00001); //gives id code instruction: (00001)
    ID();  // gets it and prints it
  }
  if (dataPC == 0x31) // ASCII "1" in hex
  { /////turn on LD5/////
    Instruction(0b00110); //gives extest instruction (00110)
    uint8_t set_pins[19] = {0,0,0b00011000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //sets pin 19 and 20
    PINS_LED5(set_pins); //changes and prints current state
  }

  if (dataPC == 0x30) // ASCII "0" in hex
  ////turn off LD5/////
  {//led5: pin 43 chipkit -> pin 58 of PIC32 (RF0) which is n=18 input, n=19 output, n=20 control
    Instruction(0b00110); //gives extest instruction (00110)
    uint8_t set_pins[19] = {0,0,0b00000000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //lowers pin 19 and 20
    PINS_LED5(set_pins); //changes and prints current state
  }
  if (dataPC == 0x62) // ser b
  //get button state/// 
  {   //button: pin 29 chipkit -> pin 63 of PIC32 (RE3) which is n=3 input, n=4 output, n=5 control
    Instruction(0b00110); //gives extest instruction (00110)
    uint8_t set_pins[19] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // irrelevant inputs
    PINS_button(set_pins); //reads and prints the state
  } 
}//End of main loop//
//End of code//