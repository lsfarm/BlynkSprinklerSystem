/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
V0   - Display
V1   - signalStrength
V4   - powerLED()
V5   - getTime (button)
V10  - terminal
V11  - manual button
V31  - stop timer enable -- autoStopEnabled[8];
V51  - stop time input in min
V71  - zone stop time (saved in array)
/* User Adjusted *****************************************************************************************************************/
#define Marks //location of device
bool    debugEnable     = 0;
bool    sendTOblynk     = 1;  //this sends v21-numZones button status to blynk in auto mode
#ifdef Marks
char auth[] = "vocAi7O16pGSfVgMqoE4zdKWQ9sm0Z13";
int numZones = 8;
#endif
/**** Particle *******************************************************************************************************************/
int switchdb2b(String command); //particle function
/**** Blynk* *********************************************************************************************************************/
#include <blynk.h>
WidgetTerminal terminal(V10);
BlynkTimer timer;
int timerNA = 99;
int zone1Timer      = timerNA;
//int AUTOtimerStartDelay = timerNA;
//int cycleADVANtimer     = timerNA;
//int setupdelay          = timerNA;  // delayed timer for some blynk stuff that won't load in setup
/**** PowerMonitoring *************************************************************************************************************/
bool hasVUSB(); 
bool hadVUSB = false;
WidgetLED pwLED(V4);
/* I2C 8 ChannelRelay ************************************************************************************************************/
#define BOARD_1 0X20  //all up
#define BOARD_2 0X27  //all down
#define BOARD_3 0X25  //middle pin up - sides down
unsigned char i2c_buffer;
unsigned char i2c_buffer_1;   // i2c relay variable buffer for #1 relay board
unsigned char i2c_buffer_2;   // i2c relay variable buffer for #2 relay board
unsigned char i2c_buffer_3;   // i2c relay variable buffer for #3 relay board
/* Program Variables ************************************************************************************************************/
int     SIG_STR;
int     SIG_QUA;
long    zoneStopTime[8] [3]; // [input in sec] [stop Day] [stop secounds??]
bool    zoneStatus[8];
bool    autoStopEnabled[8];
int     timeOffset;
int     previousDay         = 0;
int  timeUpdateDay      = 0; //used so runOnceADay only runs once a day
int  runDayCounter      = 0; //incermented in runOnceADay()
int lastMin; //used in loop to sendInfo2Blynk()

BLYNK_WRITE(V5) {  //Push Button that prints current time back to V0
    int button1 = param.asInt();
    if (button1) {
        Blynk.virtualWrite(V0, Time.format("%r %m/%d"));
        signalStrength(); //get current stregth
        Blynk.virtualWrite(V1, SIG_STR);
    }
}
/////////************* **********/////////
//             Manual Control           //
/////////************* **********/////////
void blynkWriteManual(int nr, int value) {
  char msg[32];
    if(!hasVUSB) Blynk.notify("Zone Running But No Power At Controller");    //or hadVUSB???
    if(nr+1 <= numZones) {
        if(!value) {
            turnOffRelay(nr+1); 
            //updateBlynkTable(nr, value);
        }
        if(value) { 
            turnOnRelay(nr+1);
            //updateBlynkTable(nr, value);
        }
    }
    else { 
        char msg[20];
        sprintf_P(msg, PSTR("Only %d Zones! [%d]"), numZones, nr+1);
        Blynk.notify(msg);
    }
}
BLYNK_WRITE(V11) { blynkWriteManual(0, param.asInt()); }
/////////************* **********/////////
//                Auto-Stop             //
/////////************* **********/////////
BLYNK_WRITE(V31) {  //Push Button that prints current time back to V0
    int val = param.asInt();
    autoStopEnabled[0] = val;
    if(val) terminal.println("Zone 1 Stop Timer Running"); 
    else terminal.println("Zone 1 Stop Timer Disabled"); 
    terminal.flush();
}
BLYNK_WRITE(V51) {
    float myVar = param[0].asFloat();        //as minute
    myVar = myVar * 3600; //3600 seconds in one hour
    zoneStopTime[0][0] = myVar;
    zoneStopTime[0][1] = zoneStopTime[0][0] + Time.now();
    Blynk.virtualWrite(V71, Time.format(zoneStopTime[0][1], "%I:%M %p   %m/%d"));
    if (sendTOblynk) { 
        terminal.print(Time.format("%r-")); terminal.print("zone1StopSec: "); terminal.println(/*myVar*/zoneStopTime[0][0]); 
        terminal.print("Time Now: "); terminal.println((int) Time.now());
        terminal.print("StopTime: "); terminal.println(zoneStopTime[0][1]); 
        terminal.flush(); 
    }
}

/*
void finishTimeCal() {// V7  !!time.zone messes this up!!  terminal.print(Time.format("%D %r - "));
    Time.zone(0);
    long val = zoneRunTimeAsSec * numZones;
    long finTime = startTimeInSec + val; //this is sec
    Blynk.virtualWrite(V7, Time.format(finTime, "%r"));
    //terminal.print("finTime: "); //terminal.println(Time.format(finTime)); //terminal.println(Time.format("%D %r - ")); //terminal.flush();
    Time.zone(timeOffset);
}*/

void setup() { //wished could delay loop() if zone on time is in the past on restart 1st zone turns on right away, but doesn't get recorded in table until its turned off
     
    Particle.function("Debug2Blynk", switchdb2b);
    Particle.function("RefreshTable", refreshTable);
    Particle.variable("Debug2Blynk", debugEnable);
    
    Wire.begin(); //for I2C relays
    Blynk.begin(auth);
    setZone();
    Blynk.syncVirtual(V11, V31, V51);
    pwLED.off(); //preset this to off in case power is off when it boots
    Blynk.notify("Battery Failure Controller Has Restarted!");
}

void loop() {  
    Blynk.run();
    timer.run();
    if (Time.hour() == 3 && Time.day() != timeUpdateDay) runOnceADay();  // update time and daylight savings
    bool curVUSB = hasVUSB(); // for checking power supply status at USB
    if (curVUSB != hadVUSB) { hadVUSB = curVUSB;  if(curVUSB) {pwLED.on(); powerRegain();}   else{pwLED.off(); powerFail();}   } //this neeeds to stay above startcycles()
    if(Time.minute() != lastMin) { lastMin = Time.minute(); sendInfo2Blynk(); checkStopTimers(); } //runs every minute on the minute change

} //end loop

void checkStopTimers() {
    if(zoneStatus[0] && autoStopEnabled[0] && Time.now() > zoneStopTime[0][1]) {
        turnOffRelay(1);
        Blynk.virtualWrite(V11, LOW); // keep things synced
    }
}

void powerRegain() { //this also runs 1 time on reboot -- need some version of turnOff relays that where running when powerFail()
    Blynk.notify("Power Restored");
    for (int i = 0; i < numZones; i++) {
        turnOffRelay(i+1); delay (50);
        //if(zoneStatus[i]){//if selected zone is on
          //  Blynk.virtualWrite(V21+i, LOW);
            //updateBlynkTable(i, 0);
        //}
    }
    if (debugEnable) {
        //if(R1.initialized){ terminal.println("Power Restored - Relay is ready"); terminal.flush(); }
        //else{ terminal.println("Relay not ready"); terminal.flush(); }
    }
}
void powerFail() { //what should happen when VUSB goes dead
    if (debugEnable) {terminal.println("powerFail()"); terminal.flush();}
    Blynk.notify("Power Outage!");
    for (int i = 0; i < numZones; i++) {
        turnOffRelay(i+1); delay (50);
    }
}
void sendInfo2Blynk() {
    Blynk.virtualWrite(V0, Time.format("%I:%M %p   %m/%d")); //("%r %m/%d")
    signalStrength(); //get current stregth
    Blynk.virtualWrite(V1, SIG_STR);
}
bool hasVUSB() { //checks if power supplied at USB this runs in loop() - bool curVUSB = hasVUSB(); 
    uint32_t *pReg = (uint32_t *)0x40000438; // USBREGSTATUS

    return (*pReg & 1) != 0;
}

int signalStrength() {
    #if Wiring_WiFi
        SIG_STR = WiFi.RSSI().getStrength();
        SIG_QUA = WiFi.RSSI().getQuality();
    #endif
    #if Wiring_Cellular
        SIG_STR = Cellular.RSSI().getStrength();
        SIG_QUA = Cellular.RSSI().getQuality();
    #endif
    return SIG_STR; //omitting this results in SOS
}

int switchdb2b(String command) {
    if(command == "1") {
        debugEnable = 1;
        return 1;
    }
    else if (command == "0") {
        debugEnable = 0;
        return 0;
    }
    else return -1;
} 
int refreshTable(String command) {
    if(command == "1") {
        //refreshBlynkTable();
        return 1;
    }
    else return -1;
}

void turnOnRelay(int relay) {
    relayONcommand(relay);
    zoneStatus[relay-1] = 1;
    if(sendTOblynk) { terminal.print(Time.format("%r %m/%d: Valve ")); terminal.print(relay); terminal.println(" ON"); terminal.flush(); }
}
void relayONcommand(int relay) {   
    if(debugEnable) Particle.publish("turnOnRelay()", String(relay));
    if(relay >= 1 && relay <= 8) channel_mode(BOARD_1, relay, 1);
    else if(relay >= 9 && relay <= 16) channel_mode(BOARD_2, (relay-8), 1);
    else if(relay >= 17 && relay <= 24) channel_mode(BOARD_3, (relay-16), 1);
    else if(debugEnable) Particle.publish("error/turnOnRelay()", String(relay));
}
void turnOffRelay(int relay) {
    relayOFFcommand(relay);
    zoneStatus[relay-1] = 0;
    if(sendTOblynk) { terminal.print(Time.format("%r %m/%d: Valve ")); terminal.print(relay); terminal.println(" OFF"); terminal.flush(); }
}    
void relayOFFcommand(int relay) {   
    if(relay >= 1 && relay <= 8) channel_mode(BOARD_1, relay, 0);
    else if(relay >= 9 && relay <= 16) channel_mode(BOARD_2, (relay-8), 0);
    else if(relay >= 17 && relay <= 24) channel_mode(BOARD_3, (relay-16), 0);
    else if(debugEnable) Particle.publish("error/turnOffRelay()", String(relay));    
}
void channel_mode(unsigned char addr, unsigned char channel, unsigned char value) {
 switch (addr) {   case BOARD_1: i2c_buffer = i2c_buffer_1; break;
                   case BOARD_2: i2c_buffer = i2c_buffer_2; break;
                   case BOARD_3: i2c_buffer = i2c_buffer_3; break;  }
                   //case BOARD_4: i2c_buffer = i2c_buffer_4; break;  }
                  

 channel = 8-channel;

 i2c_buffer &= ~(1<<(channel));
 i2c_buffer |= value<<channel;

 
 switch (addr) {   case BOARD_1: i2c_buffer_1 = i2c_buffer; break;
                   case BOARD_2: i2c_buffer_2 = i2c_buffer; break;
                   case BOARD_3: i2c_buffer_3 = i2c_buffer; break;  }
                   //case BOARD_4: i2c_buffer_4 = i2c_buffer; break;  }
 

 Wire.beginTransmission(addr);             
 Wire.write(~i2c_buffer);               
 Wire.endTransmission();  
}

void runOnceADay() {
    timeUpdateDay = Time.day();
    runDayCounter++;
    Particle.syncTime();
    setZone();
}
void setZone() {
	int month = Time.month();
	int day = Time.day();
	int weekday = Time.weekday();
	int previousSunday = day - weekday + 1;

	if (month < 3 || month > 11) {
		Time.zone(-6);
	}else if (month > 3 && month < 11) {
		Time.zone(-5);
	}else if (month == 3) {
		//int offset = (previousSunday >= 8)? -5 : -6;
		timeOffset = (previousSunday >= 8)? -5 : -6;
		Time.zone(timeOffset);
		Blynk.virtualWrite(V1, timeOffset);
	} else{
		//int offset = (previousSunday <= 0)? -5 : -6;
		timeOffset = (previousSunday >= 0)? -5 : -6;
		Time.zone(timeOffset);
		Blynk.virtualWrite(V1, timeOffset);
	}
	//terminal.println(previousSunday); terminal.flush();
}
