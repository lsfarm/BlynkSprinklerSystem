/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
V0   - Mode Switch (Segment Switch)
V1   - Time Offset from UTC used to set correct time on controller (Numeric Input) <<add in auto time mode
V2   - Time printed back on this channel after adjusted via V1 (Value Display)
V3   - Push Button that prints current time back to V2
V4   - Value Widget that gets time valve in blynk_int() for saving last reboot time
V5   - (Time Input Widget) for auto zone start time  \
V6   - Zone Run time in auto mode (Numeric Input)    /
V8   - Power Supply LED runs in loop() in hasVUSB()
V7   - Auto zone finish time (Value Display)
V9   - (Table Widget) - used for showing value start and stop times
V10  - (Terminal)
V11  - (Time Input Widget) for advanced mode start time         \
V12  - [Master]Advanced mode zone run time (Numeric Input)      /
V13  - Slider seasonal adjust
V15  - Signal Strength
V21  - Manual Valve switch input (Styled Button)
|||  - reserved
V45  - 21-45 reserved for inputs (Styled Button)
V51  - (Numeric Input Widget) V50 sets how long Zone1 runs for in Advanced Mode <<is there a better widget for this?
|||
V75  - 50-75 reserved for inputs
V101 - (Segment Switch) for advanced mode Case 0=Off - 1=EveryDay - 2=Everyother - 3=Every 3rd Day
|||  - reserved
V125 - 101-125 reserved for inputs
V127?? limit
https://github.com/ControlEverythingCom/NCD16Relay/blob/master/firmware/NCD16Relay.cpp
>> https://github.com/ControlEverythingCom/NCD16Relay/blob/master/README.md
relayController.turnOffAllRelays();  !!this doesn't work at all!! if you do this than next time turnOnRelay(relayNumber); comes around it will turn on entire set
relayController.turnOnRelay(relayNumber);
relayController.toggleRelay(i);
      terminal.println(Time.local() % 86400); //this gives time of day in secounds
      terminal.println(Time.day());           //this gives day of month
      
!!Issues!!
-- auto mode contuines even when switched back to manual mode <<bandaid for now is to clear remaning loop from running  >>would like to add clearTimeout()
-- switching modes on V0 doesn't get reflected on BlynkTable times -- it will change icon tho. << fixed>debugging
*/
/**** Particle *******************************************************************************************************************/
int switchdb2b(String command); //particle function
int refreshTable(String command);
/**** Blynk* *********************************************************************************************************************/
#include <blynk.h>
//char auth[] = "zh7lIH_MEeMqn_drxNhOhut37IxDcYrk"; //mine
char auth[] = "7GYvygdIZ_1bl_IGZC6WLDPo5hBUkJRc";
WidgetTerminal terminal(V10);
BlynkTimer timer;
enum  MODE { off = 1, manual = 2, automatic = 3, advanced = 4, unknown = 999 }; //https://www.baldengineer.com/state-machine-with-enum-tutorial.html
MODE         mode        = unknown;
void         setMode(MODE m);
int timerNA = 99;
int cycleAUTOtimer      = timerNA;
int AUTOtimerStartDelay = timerNA;
int cycleADVANtimer     = timerNA;
int setupdelay          = timerNA;  // delayed timer for some blynk stuff that won't load in setup
/**** PowerMonitoring *************************************************************************************************************/
bool hasVUSB(); 
bool hadVUSB = false;
WidgetLED pwLED(V8);
/* I2C 8 ChannelRelay ************************************************************************************************************/
#define BOARD_1 0X20  //all up
#define BOARD_2 0X27  //all down
#define BOARD_3 0X21  //??
unsigned char i2c_buffer;
unsigned char i2c_buffer_1;   // i2c relay variable buffer for #1 relay board
unsigned char i2c_buffer_2;   // i2c relay variable buffer for #2 relay board
unsigned char i2c_buffer_3;   // i2c relay variable buffer for #3 relay board
bool masterValveState;
/* User Adjusted *****************************************************************************************************************/
bool debugEnable = 1;
bool sendTOblynk = 1;  //this sends v21-numZones button status to blynk in auto mode
const int numZones = 15; // need const for advanSched[]
/* Program Variables ************************************************************************************************************/
int     SIG_STR;
int     SIG_QUA;
long    startTimeInSec;
long    zoneRunTime;
long    zoneRunTimeAsSec;
long    startTimeInSecADVAN;
//long    zoneRunTimeADVAN;  moved to array time in
int     timeOffset;
int     previousDay = 0;
int     previousDayADVAN = 0;
int     count = 1;
int     secoundcount;
int             counterADVAN;
int             advanSched[24] [2]; //advanced schedule this holds values from V101 - (V101+numZones) in column [0] and keeps track of the schedule in column [1]  https://www.tutorialspoint.com/arduino/arduino_multi_dimensional_arrays.htm
long   zoneRunTimeADVAN[24]; //to do  plan to tie this to V150-175 for setting run times in advanced mode
bool            zoneStatus[24]; //this runs in updateBlynkTable() and is than use to correctly set zone turn off times when mode is changed in setMode() 
unsigned long   startTime[24];
unsigned long   stopTime[24];
int  timeUpdateDay      = 0; //used so runOnceADay only runs once a day
int  runDayCounter      = 0; //incermented in runOnceADay()

BLYNK_WRITE(V1) { //used to adjust for time change  wished I could find a simple way to do this automatically<done V1 used in setZone()
    timeOffset = param[0].asInt();
    Time.zone(timeOffset);
    Blynk.virtualWrite(V2, Time.format("%r %m/%d"));
}
BLYNK_WRITE(V3) {  //Push Button that prints current time back to V2
    int button1 = param.asInt();
    if (button1) {
        Blynk.virtualWrite(V2, Time.format("%r %m/%d"));
        signalStrength(); //get current stregth
        Blynk.virtualWrite(V15, SIG_STR);
    }
}
BLYNK_WRITE(V5) { //Time Input Widget
    startTimeInSec = param[0].asLong();
    if (debugEnable) {
        terminal.print("startTimeInSec: ");
        terminal.println(startTimeInSec);
        terminal.flush();
    }
    finishTimeCal();
}
BLYNK_WRITE(V6) {
    zoneRunTime = param[0].asLong();   //as minute
    zoneRunTime = 60 * zoneRunTime;    //convert minutes to seconds >>take this one out for debuging loops in secounds
    zoneRunTimeAsSec = zoneRunTime;    //used in finsihTimeCal()
    zoneRunTime = zoneRunTime * 1000L;  //convert secounds to  milli sec
    if (debugEnable) { terminal.print("zoneRunTime: "); terminal.println(zoneRunTime); terminal.flush(); }
    finishTimeCal();
}
void finishTimeCal() {// V7  !!time.zone messes this up!!  terminal.print(Time.format("%D %r - "));
    Time.zone(0);
    long val = zoneRunTimeAsSec * numZones;
    long finTime = startTimeInSec + val; //this is sec
    Blynk.virtualWrite(V7, Time.format(finTime, "%r"));
    //terminal.print("finTime: "); //terminal.println(Time.format(finTime)); //terminal.println(Time.format("%D %r - ")); //terminal.flush();
    Time.zone(timeOffset);
}
/////////************* **********/////////
//             Mode Selection           //
/////////************* **********/////////
BLYNK_WRITE(V0) {
  switch (mode = (MODE)param.asInt())
  {
    case off:
        //Blynk.virtualWrite(V100, "Mode: OFF");
        //terminal.println(mode);//terminal.flush();
        break;
    case manual: 
        //Blynk.virtualWrite(V100, "Mode: Manual");
        break;
    case automatic: 
        //Blynk.virtualWrite(V100, "Mode: Automatic");
        break;
    case advanced:
        break;
    default:
        mode = unknown;
    }
  //sendInfoaftercommand();
    setMode(mode);  
  //if (mode != unknown) 
    //EEPROM.put(1, mode);                              //not used--may use if Wifi issues and blynk_connect fails
}

/////////************* **********/////////
//             Manual Buttons           //
/////////************* **********/////////

void blynkWriteManual(int nr, int value) {
  char msg[32];
  //valve[nr].manual = value;
  
  switch (mode) {
    case off:
      //Blynk.virtualWrite(V11, 0);
      Blynk.virtualWrite(V21+nr, 0);
      Blynk.notify("Select Manual or Auto Mode");
      break;
      
    case manual:
      if(!value) {
          turnOffRelay(nr+1); 
          updateBlynkTable(nr, value);
          //Blynk.virtualWrite(V9, "update", nr, (Time.format("%I:%M%p[%d]")), (Time.format("%I:%M%p[%d]"))); 
          //Blynk.virtualWrite(V9, F("deselect"), nr);
        }
      if(value) { 
          turnOnRelay(nr+1);
          updateBlynkTable(nr, value);
          //Blynk.virtualWrite(V9, "update", nr, "on", nr+1);  
          //Blynk.virtualWrite(V9, "select", nr);
        }
      break;
    case automatic: 
    case advanced:
      Blynk.virtualWrite(V21+nr, 0);//if zone was on, this shuts it off in Blynk but doesn't shut off relay
      Blynk.notify("In Auto Mode - Selecting Manual Mode will discountinue todays schedule");
      break;
      
    case unknown:
    default:
      break;
  }
}

void updateBlynkTable(int zoneIndex, bool zoneOn) { //function to update the table with zone start and stop times
    zoneStatus[zoneIndex] = zoneOn;
    if (zoneOn) {startTime[zoneIndex] = Time.now();} //save the time zone ?? started
    if (!zoneOn) {stopTime[zoneIndex] = Time.now();} //save the time zone ?? stopped
    String value = (Time.format(stopTime[zoneIndex], "%I:%M%p[%d]"));
    String name =  (Time.format(startTime[zoneIndex], "%I:%M%p[%d]"));
    name = name + " - " + value;
    Blynk.virtualWrite(V9, "update", zoneIndex, name, zoneIndex + 1);    // Updates name space in Blynk table
    if (zoneOn) { Blynk.virtualWrite(V9, "select", zoneIndex);   }       //  Start       Stop
    if (!zoneOn){ Blynk.virtualWrite(V9, "deselect", zoneIndex); }       //Time[Day] - Time[Day]
}
void refreshBlynkTable() {
    for (int i = 0; i < numZones; i++) {
        String value = (Time.format(stopTime[i], "%I:%M%p[%d]"));
        String name =  (Time.format(startTime[i], "%I:%M%p[%d]"));
        name = name + " - " + value;
        Blynk.virtualWrite(V9, "update", i, name, i + 1);    // Updates name space in Blynk table
        if (zoneStatus[i]) { Blynk.virtualWrite(V9, "select", i);   }       //  Start       Stop
        if (!zoneStatus[i]){ Blynk.virtualWrite(V9, "deselect", i); }       //Time[Day] - Time[Day]
    }
}

BLYNK_WRITE(V21) { blynkWriteManual(0, param.asInt()); }
BLYNK_WRITE(V22) { blynkWriteManual(1, param.asInt()); }
BLYNK_WRITE(V23) { blynkWriteManual(2, param.asInt()); }
BLYNK_WRITE(V24) { blynkWriteManual(3, param.asInt()); }
BLYNK_WRITE(V25) { blynkWriteManual(4, param.asInt()); }
BLYNK_WRITE(V26) { blynkWriteManual(5, param.asInt()); }
BLYNK_WRITE(V27) { blynkWriteManual(6, param.asInt()); }
BLYNK_WRITE(V28) { blynkWriteManual(7, param.asInt()); }
BLYNK_WRITE(V29) { blynkWriteManual(8, param.asInt()); }
BLYNK_WRITE(V30) { blynkWriteManual(9, param.asInt()); }
BLYNK_WRITE(V31) { blynkWriteManual(10, param.asInt()); }
BLYNK_WRITE(V32) { blynkWriteManual(11, param.asInt()); }

/////////************* **********/////////
//             Advanced Mode            //
/////////************* **********/////////
BLYNK_WRITE(V11) { //Time Input Widget  Zone Start Time
  startTimeInSecADVAN = param[0].asLong();
  if(debugEnable)  { terminal.print("startTimeInSecADVAN: "); terminal.println(startTimeInSecADVAN); terminal.flush(); }
  //finishTimeCal();
}
BLYNK_WRITE(V12) { //master time in
    long timeInputAsMin = param[0].asLong();        //as minute
    long timeInput = timeInputAsMin * 60;    //convert minutes to seconds
    timeInput = timeInput * 1000;  //converts seconds to millisec
    //long timeInputed = (long)timeInput;
    for(byte i = 0; i < numZones; i++) {
        zoneRunTimeADVAN[i] = timeInput;
        Blynk.virtualWrite(V51+i, timeInputAsMin); delay(250);
    }
    if (debugEnable) { terminal.print("zoneRunTimeADVAN_Master: "); terminal.println(timeInput); terminal.flush(); }
    //finishTimeCal();
}
BLYNK_WRITE(V13) {
    int adjustValue = param.asInt();
    for(byte i = 0; i < numZones; i++) {
        zoneRunTimeADVAN[i] = zoneRunTimeADVAN[i] * adjustValue;
        zoneRunTimeADVAN[i] = zoneRunTimeADVAN[i] / 100;
        long runTimeAsMin = zoneRunTimeADVAN[i] / 1000;
        runTimeAsMin = runTimeAsMin / 60;
        Blynk.virtualWrite(V51+i, runTimeAsMin); delay(250);
    }
}
BLYNK_WRITE(V101) {
    switch (advanSched[0][0] = param.asInt() - 1) {}
    advanSched[0][1] = advanSched[0][0];
    //terminal.print("Run every ?? day >");
    //terminal.println(advanSched[0][0]);
    //terminal.print("Run Today?? 1 = yes");
    //terminal.println(advanSched[0][1]);
    //terminal.flush();
}
BLYNK_WRITE(V102) { switch (advanSched[1][0] = param.asInt() - 1) {} advanSched[1][1] = advanSched[1][0];}
BLYNK_WRITE(V103) { switch (advanSched[2][0] = param.asInt() - 1) {} advanSched[2][1] = advanSched[2][0];}
BLYNK_WRITE(V104) { switch (advanSched[3][0] = param.asInt() - 1) {} advanSched[3][1] = advanSched[3][0];}
BLYNK_WRITE(V105) { switch (advanSched[4][0] = param.asInt() - 1) {} advanSched[4][1] = advanSched[4][0];}
BLYNK_WRITE(V106) { switch (advanSched[5][0] = param.asInt() - 1) {} advanSched[5][1] = advanSched[5][0];}
BLYNK_WRITE(V107) { switch (advanSched[6][0] = param.asInt() - 1) {} advanSched[6][1] = advanSched[6][0];}
BLYNK_WRITE(V108) { switch (advanSched[7][0] = param.asInt() - 1) {} advanSched[7][1] = advanSched[7][0];}
BLYNK_WRITE(V109) { switch (advanSched[8][0] = param.asInt() - 1) {} advanSched[8][1] = advanSched[8][0];}
BLYNK_WRITE(V110) { switch (advanSched[9][0] = param.asInt() - 1) {} advanSched[9][1] = advanSched[9][0];}
BLYNK_WRITE(V111) { switch (advanSched[10][0] = param.asInt() - 1) {} advanSched[10][1] = advanSched[10][0];}
BLYNK_WRITE(V112) { switch (advanSched[11][0] = param.asInt() - 1) {} advanSched[11][1] = advanSched[11][0];}

BLYNK_WRITE(V113) { switch (advanSched[12][0] = param.asInt() - 1) {} advanSched[12][1] = advanSched[12][0];} //will this work V113 will write to unknown data location if set on 12 zones

BLYNK_WRITE(V51) {
    zoneRunTimeADVAN[0] = param[0].asLong();        //as minute
    zoneRunTimeADVAN[0] = zoneRunTimeADVAN[0] * 60;    //convert minutes to seconds
    zoneRunTimeADVAN[0] = zoneRunTimeADVAN[0] * 1000;  //converts seconds to millisec
    if (debugEnable) { terminal.print("zone1RunTimeADVAN: "); terminal.println(zoneRunTimeADVAN[0]); terminal.flush(); }
    //finishTimeCal();
}
BLYNK_WRITE(V52) {
    zoneRunTimeADVAN[1] = param[0].asLong();        //as minute
    zoneRunTimeADVAN[1] = zoneRunTimeADVAN[1] * 60;    //convert minutes to seconds
    zoneRunTimeADVAN[1] = zoneRunTimeADVAN[1] * 1000;  //converts seconds to millisec
    if (debugEnable) { terminal.print("zone2RunTimeADVAN: "); terminal.println(zoneRunTimeADVAN[1]); terminal.flush(); }
    //finishTimeCal();
}

void setup() { //wished could delay loop() if zone on time is in the past on restart 1st zone turns on right away, but doesn't get recorded in table until its turned off
    //Time.zone(-6); in blynk sync virtual V1
    Blynk.begin(auth);
    Wire.begin(); //for I2C relays
    Blynk.syncVirtual(V0, V1, V5, V6, V11 /*V12*/); // V101, V102, V103, V104, V105, V106, V107, V108, V109, V110, V111, V112);
    pwLED.off(); //preset this to off in case power is off when it boots
    Blynk.virtualWrite(V9, "clr"); //clear the table
    setupdelay = timer.setTimeout(5000L, Blynk_init);
    Blynk.notify("Battery Failure Controller Has Restarted!");
    delay(8000); //allow setup to finish before starting loop() because if in advan mode at power cycle all advan cycles will be skipped V101-numZones hasn't synced yet
    //moved to powerRegain()
    /*R1.setAddress(1, 1, 1);
    if(R1.initialized){ terminal.println("Relay is ready"); terminal.flush(); }
    else{ terminal.println("Relay not ready"); terminal.flush(); }*/
    Particle.function("Debug2Blynk", switchdb2b);
    Particle.function("RefreshTable", refreshTable);
    Particle.variable("Debug2Blynk", debugEnable);
    Particle.variable("Mode", mode); //this isn't working
}

void Blynk_init() { //running this in setup causes device to lockup
    Blynk.virtualWrite(V4, Time.format("%r %m/%d")); //last reboot time
    for(byte i = 0; i<numZones; i++) {
        Blynk.syncVirtual(V101+i);
        //delay(500);
    }
    for(byte i = 0; i < numZones; i++) {
        char nodeName[9];
        sprintf_P(nodeName, PSTR("Zone %d"), (i+1));
        Blynk.virtualWrite(V9, F("add"), i, F("Unknown"), nodeName);
        Blynk.virtualWrite(V9, F("deselect"), i);
        startTime[i] = Time.now(); //pass in current time to array for blynk table !!this actually happens in blynk.syncvirtual V0>>not sure how to do this
        stopTime[i] = Time.now();  //pass in current time to array for blynk table
    }
    setupdelay = timerNA; 
}

void loop() {  
    Blynk.run();
    timer.run();
    if (Time.hour() == 3 && Time.day() != timeUpdateDay) runOnceADay();  // update time and daylight savings
    bool curVUSB = hasVUSB(); // for checking power supply status at USB
    if (curVUSB != hadVUSB) { hadVUSB = curVUSB;  if(curVUSB) {pwLED.on(); powerRegain();}   else{pwLED.off(); powerFail();}   } //this neeeds to stay above startcycles()
    if (previousDay != Time.day() &&  Time.local() % 86400 >= startTimeInSec && mode == automatic) { //auto mode cycle
        if (curVUSB) {
            if(debugEnable) {terminal.println("startcycleAUTO() called from loop"); terminal.flush();}
            previousDay = Time.day();
            count = 1; // in case mode gets changed
            startcycleAUTO(); //timer already running check in this function
        }
        else{
            previousDay = Time.day();
            Blynk.notify("Power Outage Auto Cycle Skipped Today");
            if(debugEnable) { terminal.println("startcycleAUTO() skipped Power Outage"); terminal.flush();}
        }
    }
    if (previousDayADVAN != Time.day() &&  Time.local() % 86400 >= startTimeInSecADVAN && mode == advanced && !timer.isEnabled(cycleADVANtimer)) { //advanced mode cycle
        if(curVUSB) { //we have 120VAC power - run cycle
            if(debugEnable) { terminal.println("startcycleADVAN() called from loop"); terminal.flush();}
            previousDayADVAN = Time.day();
            counterADVAN = 0; // in case mode gets changed
            startcycleADVAN();
        }
        else{
            previousDayADVAN = Time.day();
            Blynk.notify("Power Outage Advanced Cycle Skipped Today");
            if(debugEnable) { terminal.println("startcycleADVAN() skipped Power Outage"); terminal.flush();}
        }
    }
} //end loop

void powerRegain() { //this also runs 1 time on reboot -- need some version of turnOff relays that where running when powerFail()
    if(cycleAUTOtimer != 99) {Blynk.notify("Power Restored Auto Cycle canceled"); if(debugEnable) {terminal.println("power restored autocycle canceled"); terminal.flush();} }
    else if (cycleADVANtimer != 99) {Blynk.notify("Power Restored Advanced Cycle canceled"); if(debugEnable) {terminal.println("power restored advancycle canceled"); terminal.flush();} }
    else {Blynk.notify("Power Restored"); if(debugEnable) {terminal.println("power restored nothing canceled"); terminal.flush();}}
    timer.deleteTimer(cycleAUTOtimer);
    cycleAUTOtimer = timerNA;
    timer.deleteTimer(cycleADVANtimer);
    cycleADVANtimer = timerNA;
    //is this still needed? R1.setAddress(1, 1, 1); //restart comunication with relay board
    #if (PLATFORM_ID == PLATFORM_ARGON) //the I2C relay board on Argon shuts off at powerFail so it doesn't work to shut running zones off from powerFail() for argon
        for (int i = 0; i < numZones; i++) {
            if(zoneStatus[i]){//if selected zone is on
                turnOffRelay(i+1);
                Blynk.virtualWrite(V21+i, LOW);
                updateBlynkTable(i, 0);
            }
        }
    #endif
    if (debugEnable) {
        //if(R1.initialized){ terminal.println("Power Restored - Relay is ready"); terminal.flush(); }
        //else{ terminal.println("Relay not ready"); terminal.flush(); }
    }
}
void powerFail() { //what should happen when VUSB goes dead
    if (debugEnable) {terminal.println("powerFail()"); terminal.flush();}
    Blynk.notify("Power Outage!");
    #if (PLATFORM_ID == PLATFORM_BORON) //the I2C relay board on boron stays on through power fail
        for (int i = 0; i < numZones; i++) {
            if(zoneStatus[i]){//if selected zone is on
                turnOffRelay(i+1);
                Blynk.virtualWrite(V21+i, LOW);
                updateBlynkTable(i, 0);
            }
        }
    #endif
}

void startcycleADVAN() {
    if(debugEnable) { terminal.print(Time.format("%D %r - ")); terminal.print("co#: "); terminal.println(counterADVAN); }
    if (advanSched[counterADVAN][1] == 1) { //if selected zone needs to run today
        turnOnRelay(counterADVAN + 1);     //turn  zone on
        updateBlynkTable(counterADVAN, 1);    //update Blynk table
        stopcycleADVAN();                     //wait zoneRunTime than shut it off
        if(debugEnable) { terminal.print("ADVANCYCLE Zone #"); terminal.print(counterADVAN + 1); terminal.println(" Running"); terminal.flush(); }
    }
    else { //if selected zone doesn't need to run today, skip stop cycle and jump to array update.
        if(debugEnable) {  terminal.print("ADVANCYCLE Zone #"); terminal.print(counterADVAN + 1); terminal.println(" Skipped"); terminal.flush(); }
        updateAdvanSchedArray();  
    }
}
  
void stopcycleADVAN() { 
    cycleADVANtimer = timer.setTimeout(zoneRunTimeADVAN[counterADVAN], [] () {
        cycleADVANtimer = timerNA; //reset timer point to not used
        turnOffRelay(counterADVAN + 1); 
        updateBlynkTable(counterADVAN, 0);
        updateAdvanSchedArray();
    } );
}

void updateAdvanSchedArray() {
    switch (advanSched[counterADVAN][0]) {
      case 0://off
        advanSched[counterADVAN][1] = 0;
        break;
      case 1://everyday
        advanSched[counterADVAN][1] = 1;
        break;
      case 2://every other day
        if (advanSched[counterADVAN][1] == 1) {
          advanSched[counterADVAN][1] = 2;
        }
        else {
          advanSched[counterADVAN][1] = 1;
        }
        break;
      case 3://every 3rd day
        if (advanSched[counterADVAN][1] == 1) {
          advanSched[counterADVAN][1] = 3;
        }
        else {
          advanSched[counterADVAN][1] = advanSched[counterADVAN][1] - 1;
        }
        break;
      default:
        terminal.println("Error in updateAdvanSchedArray() SwitchCase"); terminal.flush();
        Blynk.notify("Error 1 Contact Lance");
        break;
    }
    counterADVAN++;
    if (counterADVAN < numZones && mode == advanced) { //if we haven't reached the last zone and the mode is still set to advanced
      startcycleADVAN(); //to do - add delay in here before next round
    }
    else {counterADVAN = 0; }//reset flag
}

void startcycleAUTO() {
    if (!timer.isEnabled(cycleAUTOtimer)) {// if the timers already running don't start another cycle (double up)
        AUTOtimerStartDelay = timer.setTimeout(1000L, [] () { 
            turnOnRelay(count); 
            if(sendTOblynk) Blynk.virtualWrite(V20+count, HIGH); 
            updateBlynkTable(count - 1, 1); 
            if (debugEnable) { terminal.print(Time.format("%D %r - ")); terminal.print("AUTOCYCLE Zone #"); terminal.print(count); terminal.println(" Running"); terminal.flush();}
            AUTOtimerStartDelay = timerNA;
        });
        cycleAUTOtimer = timer.setTimeout(zoneRunTime, [] () {
            cycleAUTOtimer = timerNA; //reset pointer for next round
            if (count != numZones) {//if we haven't reached the last zone
                turnOffRelay(count);
                updateBlynkTable(count-1, 0);
                if(sendTOblynk) {Blynk.virtualWrite(V20+count, LOW);}
                if (debugEnable) {terminal.print(Time.format("%D %r - ")); terminal.print("AUTOCYCLE Zone #"); 
                    terminal.print(count); terminal.println(" Stopped"); terminal.flush(); }
                count++;
                if (mode == automatic) {startcycleAUTO();}
            }
            else { //we've reached last zone, turn it off and reset count
                turnOffRelay(numZones);
                updateBlynkTable(numZones - 1, 0);
                if(sendTOblynk) {Blynk.virtualWrite(V20+numZones, LOW);}
                count = 1;
                if (debugEnable) {terminal.print(Time.format("%D %r - ")); terminal.println("last AUTOCYCLE zone reached"); terminal.flush(); }
                //previousDay = 1;  //used to debug
            }
        });
        //if (debugEnable) { terminal.print(Time.format("%D %r - ")); terminal.println("bottom of startcycleAUTO()"); terminal.flush();}
    }
    else {
        Blynk.notify("Error 2: Auto Timer Already Running");
    }
}

void setMode(MODE m) {
  switch (m) {
    case off:
        for (int i = 0; i < numZones; i++) {
            if(zoneStatus[i]){//if selected zone is on
                turnOffRelay(i+1);
                Blynk.virtualWrite(V21+i, LOW);
                updateBlynkTable(i, 0);
            }
        }
        terminal.println("setMode() = Off");
        terminal.flush();
        break;
      
    case manual:
    case automatic:
    case advanced:
        timer.deleteTimer(cycleAUTOtimer); //trash all future loops if mode is changed //count = 1???? think leave this out for now in case wanna restart latter < put in loop()
        for (int i = 0; i < numZones; i++) {
            if(zoneStatus[i]){
                turnOffRelay(i+1);
                Blynk.virtualWrite(V21+i, LOW);
                updateBlynkTable(i, 0);
            }
       }
        terminal.println("setMode() = Man/Auto/Advan");
        terminal.flush();
    default: 
      break;
  }
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
        refreshBlynkTable();
        return 1;
    }
    else return -1;
}

void turnOnRelay(int relay) {
    relayONcommand(relay);
    relayONcommand(numZones+1); masterValveState = 1;
}
void relayONcommand(int relay) {   
    if(debugEnable) Particle.publish("turnOnRelay()", String(relay));
    if(relay >= 1 && relay <= 8) channel_mode(BOARD_1, relay, 1);
    else if(relay >= 9 && relay <= 16) channel_mode(BOARD_2, (relay-8), 1);
    else if(relay >= 17 && relay <= 24) channel_mode(BOARD_3, (relay-16), 1);
    else if(debugEnable) Particle.publish("error/turnOnRelay()", String(relay));
}
void turnOffRelay(int relay) {
    relayOFFcommand(numZones+1); 
    if(masterValveState) delay(500);
    masterValveState = 0;
    relayOFFcommand(relay);
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
		int offset = (previousSunday >= 8)? -5 : -6;
		Time.zone(offset);
		Blynk.virtualWrite(V1, offset);
	} else{
		int offset = (previousSunday <= 0)? -5 : -6;
		Time.zone(offset);
		Blynk.virtualWrite(V1, offset);
	}
}
