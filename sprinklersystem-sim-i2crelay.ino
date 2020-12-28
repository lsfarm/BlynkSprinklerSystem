/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
V0   - Mode Switch
*/

#include <blynk.h>
char auth[] = "zh7lIH_MEeMqn_drxNhOhut37IxDcYrk";
WidgetTerminal terminal(V10);
BlynkTimer timer;

#include <OneWire.h>
#define  ON 1
#define OFF 0
#define address 0x20    //!!0x20!!        // i2c slave address from 0x20 to 0x27

unsigned char i;
unsigned char variable_LOW;
unsigned char variable_HIGH;
unsigned int  mode_byte;        // 16bit unsigned variable
unsigned int FAST = 1000;

void setup() {
    Time.zone(-5);
    Blynk.begin(auth);
    timer.setInterval(5000L, startcycle);
    Wire.begin();
    Wire.beginTransmission(0x20);
    Wire.write(0x00);             // A register
    Wire.write(0x00);             // set all of port A to outputs
    Wire.endTransmission();

    Wire.beginTransmission(0x20);
    Wire.write((byte)0x01);       // B register
    Wire.write((byte)0x00);       // set all of port B to outputs
    Wire.endTransmission(); 
    
    turnonrelays();
}


void loop() {
    Blynk.run();
    timer.run();
    //x = 0b01111111;  // this will shut everything off
         //bitWrite(x, nr, !valve[nr].manual);  // write 1 to the least significant bit of x
    //i2c_relay(ADDR, x);
    ch_mode(address,1,ON);  delay(FAST);//ch_mode(address,1,OFF);  delay(FAST);   // Relay  #1 ON and after delay OFF
    //ch_mode(address,2,ON);
    terminal.println(Time.format("%D %r - "));
}

void i2c_relay(unsigned char addr, unsigned char value)
{
  Wire.beginTransmission(addr);             
  Wire.write(value);               
  Wire.endTransmission();  
  
}

void startcycle() 
{
    
}

void ch_mode(unsigned char addr, unsigned int channel, unsigned char mode){   

     switch (channel) { case 16 : channel =  7; break;
                        case 15 : channel =  6; break;
                        case 14 : channel =  5; break;
                        case 13 : channel =  4; break;
                        case 12 : channel =  3; break;
                        case 11 : channel =  2; break;
                        case 10 : channel =  1; break;
                        case  9 : channel =  0; break;
                        case  8 : channel = 15; break;
                        case  7 : channel = 14; break;
                        case  6 : channel = 13; break;
                        case  5 : channel = 12; break;
                        case  4 : channel = 11; break;
                        case  3 : channel = 10; break;
                        case  2 : channel =  9; break;
                        case  1 : channel =  8; break; }

     mode_byte &= ~(1<<(channel));
     terminal.print(mode_byte);
     terminal.println(" mode_byte");
     terminal.flush();
     mode_byte |= mode<<channel;

     //variable_LOW = lowByte(mode_byte);
     //variable_HIGH = highByte(mode_byte);

     Wire.beginTransmission(addr);
     Wire.write(0x12);            // address bank A
     Wire.write(variable_LOW);
     Wire.endTransmission();
     terminal.println(addr);
     
     Wire.beginTransmission(addr);
     Wire.write(0x13);            // address bank B
     Wire.write(variable_HIGH);
     Wire.endTransmission();
     terminal.println(addr);
  
}


void turnonrelays(){
    Wire.beginTransmission(address);
    Wire.write(18);
    Wire.write(255);
    Wire.write(255);
    byte status = Wire.endTransmission();
}