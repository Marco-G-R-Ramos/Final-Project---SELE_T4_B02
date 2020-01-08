//Temperature Sensor TMP175 with I2C communication at 100 kHz clock speed 
//and UART communication at 9600 baudrate with the computer receiver.

#include <Arduino.h>
#define FreqCPU 16000000 // Atmega328 clock speed
#define BAUD 9600 //Choosen baud rate for the UART communication with the receiver computer
#define MYUBRR FreqCPU/16/BAUD - 1 

void UART_init(unsigned int ubrr){ // initiates UART communication with 8 data bits + 1 stop bit + 9600 baud rate + no parity
  UBRR0H = (unsigned char)(ubrr>>8); //Setting baud rate 
  UBRR0L = (unsigned char)ubrr;      //Setting baud rate 
  UCSR0B = (1<<RXEN0)|(1<<TXEN0); //Enable receiving; enable transmiting; set Zn2=0 (for 8 bit data)
  UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);//set Zn1=1 and Zn2=1 (for 8 bit data, in conjunction with Zn2=0)
}

void send_data(uint8_t data){ // sends data with UART to the receiver computer
  while (!( UCSR0A & (1<<UDRE0))); //while not register empty. again, it waits for empty register; for data currently there to be read
  UDR0 = data; // writes the data into the data register
}

void I2C_init(void){ // initiates I2C communication with 100 kHz clock speed 
    TWSR = 0x00; // with PrescalerValue(PV) == 1 it makes two bits of TWSR be 0
    TWBR = 72;  // to make the I2C comunicantion frequency 100 kHz
    TWCR = (1<<TWEN);  // basic init for TWCR
}

void I2C_start(uint8_t data1){ // start I2C communication
    TWCR = (1<<(TWINT))|(1<<TWSTA)|(1<<TWEN); // TWSTA makes start state activated
    while (!(TWCR & (1<<TWINT))); // condition to send adress byte
    TWDR = data1;                 // TWDR is the regist to send and receive data with I2C in atmega328
    TWCR = (1<<TWINT)|(1<<TWEN); // turn TWSTA off
    while (!(TWCR & (1<<TWINT))); // condition to go to the next step, which is in this case to read/write a byte
}

void I2C_stop(void){ // stop I2C communication
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO); // TWSTO activates the stop state condition 
}

void I2C_write(uint8_t data2){ // I2C function to write
	TWDR = data2;    // TWDR is the regist to send and receive data with I2C in atmega328
	TWCR = (1<<TWINT)|(1<<TWEN); //to make sure TWCR is at the correct value
	while (!(TWCR & (1<<TWINT))); // condition to go to the next step, which is in this case to read/write a byte
}

void I2C_writeSetup(void){ //writes 8 bits to a specific register
    // configuration register to setup the output as 12 bits
	I2C_start(0x90); // adress to write
	I2C_write(0x01); // register to config 0x01
	I2C_write(0x60); // byte to setup the sensor to read 12 bits in total(8 of integers and the last 4 for decimals)
    I2C_stop();      // making it able to read to a precision of 0,0625 degrees
    // configuration register to enable temperature reads
    I2C_start(0x90);  // adress to write
	I2C_write(0x00); // register to read temperature 0x00
    I2C_stop();      // making the sensor from now on, able to output values of temperature with 12 bits
}

uint8_t I2C_read(void){ //reads what the slave(s) send back
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while (!(TWCR & (1<<TWINT))); // condition to go to the next step, which is in this case to read/write a byte
    return TWDR; // output will be a byte, in this case it will be required to read two times
}

uint16_t I2C_readTemp(void){ //read 8 bits from speciific register
    uint16_t temp1, temp2, temp; // variables neeeded to calculate 1 output of temperature correctly
    I2C_start(0x91); // start by sending the adress to read
	temp1 = I2C_read(); // read the most significant 8 bits first for the integers
    temp2 = I2C_read(); // the decimals come in on the 4 most significant bits of this byte
	I2C_stop(); // stop this operation
    temp = (temp1<<8) + temp2 ; // calculating the output correctly
	return temp; // temp will be the output of 16 bits to be sent with UART to the receiver computer
}

void setup(){ //setups the atmega328 to communicate with the computer with UART and the sensor with I2C
    UART_init(MYUBRR); //run the UART initial functions
    I2C_init();  //run the I2C initial functions
    I2C_writeSetup(); //run the sensor setup functions
}

//initializing the variable for the temperature output
int temperature;

void loop(){ // outputs the temperature with UART communication to the computer receiver
    temperature = I2C_readTemp(); // temperature reading
    Serial.print(temperature,HEX); // print of the temperature in hexadecimal 
    delay(250); // this delay is necessary due to the time the sensor takes to output with 12 bits.
} // end of code