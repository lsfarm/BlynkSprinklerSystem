#define BLYNK_TEMPLATE_ID "TMPLNSmAvSmS"
#define BLYNK_DEVICE_NAME "SS I2Crelay8CH TestDevice"
/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
>>>V2<<<
V0   - Mode Switch Off Manual Auto
V1   - Current Time on Controller  -- Time is now auto set. no need for manual offsets
V2   - Signal (wifi or cell)
V3   - Water Pressure
V4   - Temperature
V5   - Freeze temp input shut off point
V6   - Power LED
V7   - battery?? not used yet
V8   - Delay watering Days
V9   - Last reboot
V10  - (Terminal)
V17  - OverView page picture changer
V18  - Numeric input to select which program name to change
V19  - Text Input New Program Name
V20  - Auto Mode Program menu
V21  - Auto Mode Program control > off weekday...
V22  - cyclical day selection > not used yet
V23  - program start time
V24  - delay between zones > not used yet
V25  - seasonal adjust percent slider
V26  - percent set at since V25 can't display it
V27  - reset V25 to 100%
V28  - tied in with v23 if v23 will execpt time back in than this is no longer needed
V30  - Monday
V31  - Tuesday
V32  - Wednesday
V33  - Thurs
V34  - Friday
V35  - Sat
V36  - Sunday
V51  - Manual Valve switch input (Styled Button)
|||  - reserved
V74  - 51-74 reserved for inputs (Styled Button) more???
V75 - Time Input
|||  - reserved
V100 - 151-200 reserved for inputs (Text Input) more???
V101 - actual run time (seansonal adjusted)
|||  - reserved
V124  - 21-45 reserved for inputs (Styled Button) more???
V127 limit????*/
#define numPrograms  5
#define LV //location of device
bool    debugEnable         = 0;
bool    blynkDebugEnable    = 0;
//bool    sendTOblynk     = 1;  //this sends v51-numZones button status to blynk in auto mode < not used in V2 Will send
#ifdef Quinns
char auth[] = "i8tshOaOwzL9AX6mydqURDyKTCE7KqJt";
// old app char auth[] = "V9fruidaVkN2qEB_y7M3Wd_oAC6iprCO";
const int   numZones    = 16;
bool        masterValve = 0;
#endif
#ifdef LB WellHouse
char auth[] = "7GYvygdIZ_1bl_IGZC6WLDPo5hBUkJRc";
const int   numZones    = 9;
bool        masterValve = 1;
#endif
#ifdef LV
char auth[] = "bkOyF5tAaEvGn3nz7heV3443lK26ugNN";
const int   numZones    = 16;
bool        masterValve = 0;
#endif
/**** Particle *******************************************************************************************************************/
int switchdb2b(String command); //particle function
int refreshTable(String command);
String particleVarMode     = "NA";
/**** Blynk* *********************************************************************************************************************/
#include <blynk.h>
WidgetTerminal terminal(V10);
BlynkTimer timer;
enum  MODE { off = 0, manual = 1, automatic = 2, advanced = 4, unknown = 999 }; //https://www.baldengineer.com/state-machine-with-enum-tutorial.html
MODE         mode        = unknown;
void         setMode(MODE m);
MODE          modeBeforePowerFail = unknown;  
int lastminute          = 67;       // used in loop() to do minuteloop()
int lasthour            = 34;       // used in loop() to do hourloop()
int timerNA = 99;
int cycleAUTOtimer1     = timerNA;
int cycleAUTOtimer2     = timerNA;
int setupdelay          = timerNA;  // delayed timer for some blynk stuff that won't load in setup
/**** PowerMonitoring *************************************************************************************************************/
bool hasVUSB(); 
bool hadVUSB = false;
WidgetLED pwLED(V6);
/* I2C 8 ChannelRelay ************************************************************************************************************/
#define BOARD_1 0X20  //all up
#define BOARD_2 0X27  //all down
#define BOARD_3 0X25  //middle pin up - sides down
unsigned char i2c_buffer;
unsigned char i2c_buffer_1;   // i2c relay variable buffer for #1 relay board
unsigned char i2c_buffer_2;   // i2c relay variable buffer for #2 relay board
unsigned char i2c_buffer_3;   // i2c relay variable buffer for #3 relay board
bool masterValveState;
/* Program Variables ************************************************************************************************************/
//char *programNames[] = {"Lawn Zones", "Flower Beds", "Program 3",  "Program 4", "Program 5" }; 
String  programNames[] = {"Lawn Zones", "Flower Beds", "Program 3",  "Program 4", "Program 5" }; 
int     SIG_STR;
int     SIG_QUA;
int     delayDay = 0;
int     mon;
int     tues;
int     wed;
int     thurs;
int     fri;
int     sat;
int     sun;
int     weekDay[7] [numPrograms];       //[0] = Sunday  [6] = Saturday  Time.weekday() 1 = Sunday 7 = Saturday
int     cycleSelected[numPrograms][2];     //0 = off, 1 = everyday ....
long    autoZoneTime[25] [numPrograms];       //[#ofzones+1] [0] holds zone start time  5 programs
unsigned long    adjustedZoneTime[25] [numPrograms];   //[#ofzones+1] [0] holds zone Stop  time  5 programs
int     activeProgram = 0; // which program is selected in app (making changes to this program in app
int     lastRunDay [numPrograms]; //to block out program from running again after it started - all set 36 in BlinkInt()
int     runningCycleTracker[24] [2];
int     notUsed = 90000; // getter than secounds in a day used in runnungCycleTracker
int     timeOffset;
int     previousDay         = 0;
bool            zoneStatus[25]; // >>[1] = Zone 1<< this (and 2 longs below)runs in updateBlynkTable() and is than use to correctly set zone turn off times when mode is changed in setMode() 
unsigned long   zonestartTime[24];
unsigned long   zonestopTime[24];
int  timeUpdateDay      = 0; //used so runOnceADay only runs once a day
int  runDayCounter      = 0; //incermented in runOnceADay()
void finishTimeCal() {// v2 will use this  !!time.zone messes this up!!  terminal.print(Time.format("%D %r - "));
    Time.zone(0);
    //long val = zoneRunTimeAsSec * numZones;
    //long finTime = startTimeInSec + val; //this is sec
    //Blynk.virtualWrite(V7, Time.format(finTime, "%r"));
        //terminal.print("finTime: "); //terminal.println(Time.format(finTime)); //terminal.println(Time.format("%D %r - ")); //terminal.flush();
    Time.zone(timeOffset);
}
/////////************* **********/////////
//             Mode Selection           //
/////////************* **********/////////
BLYNK_WRITE(V0) { //may delete advanced
  switch (mode = (MODE)param.asInt())
  {
    case off:
        //Blynk.virtualWrite(V1, "Mode: OFF");
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
    //EEPROM.put(1, mode);          //not used--may use if Wifi issues and blynk_connect fails
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
      Blynk.virtualWrite(V51+nr, 0);
      Blynk.virtualWrite(V1, "Select Manual Mode");
      Blynk.logEvent("info", "Select Manual or Auto Mode"); //change to AppMessage
      break;
    case manual:
    case automatic: 
        if(!hasVUSB) Blynk.logEvent("info", "Zone Running But No Power At Controller");    //or hadVUSB???
        if(nr+1 <= numZones) {
            if(!value) {
                turnOffRelay(nr+1, 1); 
                //updateBlynkTable(nr, value);
            }
            if(value) { 
                turnOnRelay(nr+1, 1);
                //updateBlynkTable(nr, value);
            }
        }
        else { 
            char msg[20];
            sprintf_P(msg, PSTR("Only %d Zones! [%d]"), numZones, nr+1);
            Blynk.logEvent("info", msg);
        }
      break;
    case advanced:
      Blynk.virtualWrite(V51+nr, 0);//if zone was on, this shuts it off in Blynk but doesn't shut off relay
      Blynk.logEvent("info", "In Auto Mode - Selecting Manual Mode will discountinue todays schedule");
      break;
      
    case unknown:
    default:
      break;
  }
}
BLYNK_WRITE(V51) { blynkWriteManual(0, param.asInt()); }
BLYNK_WRITE(V52) { blynkWriteManual(1, param.asInt()); }
BLYNK_WRITE(V53) { blynkWriteManual(2, param.asInt()); }
BLYNK_WRITE(V54) { blynkWriteManual(3, param.asInt()); }
BLYNK_WRITE(V55) { blynkWriteManual(4, param.asInt()); }
BLYNK_WRITE(V56) { blynkWriteManual(5, param.asInt()); }
BLYNK_WRITE(V57) { blynkWriteManual(6, param.asInt()); }
BLYNK_WRITE(V58) { blynkWriteManual(7, param.asInt()); }
BLYNK_WRITE(V59) { blynkWriteManual(8, param.asInt()); }
BLYNK_WRITE(V60) { blynkWriteManual(9, param.asInt()); }
BLYNK_WRITE(V61) { blynkWriteManual(10, param.asInt()); }
BLYNK_WRITE(V62) { blynkWriteManual(11, param.asInt()); }
BLYNK_WRITE(V63) { blynkWriteManual(12, param.asInt()); }
BLYNK_WRITE(V64) { blynkWriteManual(13, param.asInt()); }
BLYNK_WRITE(V65) { blynkWriteManual(14, param.asInt()); }
BLYNK_WRITE(V66) { blynkWriteManual(15, param.asInt()); }

/////////************* **********/////////
//             Advanced Mode            //
/////////************* **********/////////
BLYNK_WRITE(V8) { //slider on home tab to delay watering if it rains
    delayDay = param.asInt();
    delayDay = delayDay + 1; // this gets minus 1 at 1 sec into new day will this make this work?
    // combined into V8 dat stream Blynk.virtualWrite(V12, delayDay);
}
BLYNK_WRITE(V19) {
    programNames[activeProgram] = param.asStr();
    terminal.println(programNames[activeProgram]); terminal.flush();
    Blynk.setProperty(V20, "labels", programNames[0], programNames[1], programNames[2], programNames[3], programNames[4]);
    Blynk.virtualWrite(V19, "Saved! " + programNames[activeProgram]);
    timer.setTimeout(5000L, clearName);
} //program menu name change tool
void clearName () {
    Blynk.virtualWrite(V19, "");
}
BLYNK_WRITE(V20) { //program selection menu
    activeProgram = param.asInt();
    Time.zone(0);
    Blynk.virtualWrite(V28, Time.format(autoZoneTime[0] [activeProgram], "%r")); //needs formated
    Time.zone(timeOffset);
    for(byte i = 0; i < 7; i++) {  //weekday buttons
        Blynk.virtualWrite(V30+i, weekDay[i] [activeProgram]); delay(50);
        terminal.print(i); terminal.flush(); //to be deleted
    }
    Blynk.virtualWrite(V22, cycleSelected[activeProgram] [0]); // load cycle selected
    for(byte i = 0; i < numZones; i++) { //zone run times
        long runTimeAutoAsMin = autoZoneTime[i+1] [activeProgram] / 60;
        //runTimeAutoAsMin = runTimeAutoAsMin / 60;
        Blynk.virtualWrite(V75+i, runTimeAutoAsMin); //delay(250);
        long runTimeAsMin = adjustedZoneTime[i+1] [activeProgram] / 60;
        //long runTimeAsMin = runTimeAsMin / 60;
        Blynk.virtualWrite(V101+i, runTimeAsMin); //delay(250);
        //Blynk.virtualWrite(V75+i, autoZoneTime[i+1] [activeProgram]);
        //Blynk.virtualWrite(V101+i, adjustedZoneTime[i+1] [activeProgram]);
    }
}
BLYNK_WRITE(V21) { //program control > off weekday cyclical   add odd even?
  switch (param.asInt())
  {
    case 0: //Off
        if(blynkDebugEnable) {terminal.println("Item off"); terminal.flush();}
        cycleSelected[activeProgram] [0] = 0;
        Blynk.virtualWrite(V22, 0);
        for(byte i = 0; i < 7; i++) {
            weekDay[i] [activeProgram] = 0;
            Blynk.virtualWrite(V30+i, 0); delay(50);
        }
      break;
    case 1: //Weekday
        if(blynkDebugEnable) {terminal.println("Item weekday"); terminal.flush();}
        cycleSelected[activeProgram] [0] = 0;
        Blynk.virtualWrite(V22, 0);
        for(byte i = 0; i < 7; i++) {
            weekDay[i] [activeProgram] = 1;
            Blynk.virtualWrite(V30+i, 1); delay(50);
        }
      break;
    case 2: //Cyclical
        if(blynkDebugEnable) {terminal.println("Item cyclical"); terminal.flush();}
        cycleSelected[activeProgram] [0] = 1;
        Blynk.virtualWrite(V22, 1);
        for(byte i = 0; i < 7; i++) {
            weekDay[i] [activeProgram] = 0;
            Blynk.virtualWrite(V30+i, 0); delay(50);
            //terminal.print(i); terminal.flush(); //to be deleted
        }
      break;
    default:
      terminal.println("Unknown item selected"); terminal.flush();
  }
}
BLYNK_WRITE(V22) { //save selection to array
    cycleSelected[activeProgram] [0] = param.asInt();
    if(blynkDebugEnable) {terminal.print("BLYNK_WriteV22(): "); terminal.println(cycleSelected[activeProgram] [0]); terminal.flush();}
}
BLYNK_WRITE(V23) {  //program start time
    autoZoneTime[0] [activeProgram] = param[0].asLong();
    Time.zone(0);
    Blynk.virtualWrite(V28, Time.format(autoZoneTime[0] [activeProgram], "%r")); //needs formated
    Time.zone(timeOffset);
    if(blynkDebugEnable) {terminal.print("ProgramStartTime"); terminal.println(autoZoneTime[0] [activeProgram]); terminal.flush();}
    //finishTimeCal();???
}
BLYNK_WRITE(V25) { //seasonal adjustment slider
    int adjustValue = param.asInt();
    Blynk.virtualWrite(V26, adjustValue); 
    for(byte i = 0; i < numZones; i++) {
        //terminal.print("1: "); terminal.println(autoZoneTime[i+1] [activeProgram]); terminal.flush();
        adjustedZoneTime[i+1] [activeProgram] = autoZoneTime[i+1] [activeProgram] * adjustValue;
        //terminal.print("2: "); terminal.println(autoZoneTime[i+1] [activeProgram]); terminal.flush();
        adjustedZoneTime[i+1] [activeProgram] = adjustedZoneTime[i+1] [activeProgram] / 100;
        //terminal.print("3: "); terminal.println(autoZoneTime[i+1] [activeProgram]); terminal.flush();
        long runTimeAsMin = adjustedZoneTime[i+1] [activeProgram] / 60;
        //runTimeAsMin = runTimeAsMin / 60;
        Blynk.virtualWrite(V101+i, runTimeAsMin); //delay(250); // !!> +i  suppose to be 201
    }
}
BLYNK_WRITE(V27) {
    int ref = param.asInt();
    if(ref) { Blynk.virtualWrite(V25, 100); Blynk.virtualWrite(V26, 100); Blynk.syncVirtual(V25); }
}

BLYNK_WRITE(V30) { //0 = sunday 7 = saturday swith to sunday to lien up with time.weekday
    weekDay[0] [activeProgram] = param.asInt();
    //terminal.print("Monday Switch to: "); terminal.println(weekday[0] [activeProgram]); terminal.flush();
}
BLYNK_WRITE(V31) { weekDay[1] [activeProgram] = param.asInt(); } //terminal.print("Tuesday Switch to: "); terminal.println(weekday[1] [activeProgram]); terminal.flush(); }
BLYNK_WRITE(V32) { weekDay[2] [activeProgram] = param.asInt(); }
BLYNK_WRITE(V33) { weekDay[3] [activeProgram] = param.asInt(); }
BLYNK_WRITE(V34) { weekDay[4] [activeProgram] = param.asInt(); }
BLYNK_WRITE(V35) { weekDay[5] [activeProgram] = param.asInt(); }
BLYNK_WRITE(V36) { weekDay[6] [activeProgram] = param.asInt(); }

void updateActual(int zone) { // print new time off to app than store updated runtime in adjustedZoneTime array >> this value will be used to set timers
    Blynk.virtualWrite(V100+zone, autoZoneTime[zone] [activeProgram]);
    adjustedZoneTime[zone] [activeProgram] = autoZoneTime[zone] [activeProgram] * 60;    //convert minutes to seconds
    //adjustedZoneTime[zone] [activeProgram] = adjustedZoneTime[zone] [activeProgram] * 1000;  //converts seconds to millisec
}

BLYNK_WRITE(V75) { 
    autoZoneTime[1] [activeProgram] = param[0].asLong();        //as minute
    updateActual(1);
    autoZoneTime[1] [activeProgram] = autoZoneTime[1] [activeProgram] * 60;    //convert minutes to seconds
    //autoZoneTime[1] [activeProgram] = autoZoneTime[1] [activeProgram] * 1000;  //converts seconds to millisec
    if(blynkDebugEnable) {terminal.print("zone1RunTime: "); terminal.println(autoZoneTime[1] [activeProgram]); terminal.flush(); }
}
BLYNK_WRITE(V76) { autoZoneTime[2] [activeProgram] = param[0].asLong(); updateActual(2); autoZoneTime[2] [activeProgram] = autoZoneTime[2] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone2RunTime: "); terminal.println(autoZoneTime[2] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V77) { autoZoneTime[3] [activeProgram] = param[0].asLong(); updateActual(3); autoZoneTime[3] [activeProgram] = autoZoneTime[3] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone3RunTime: "); terminal.println(autoZoneTime[3] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V78) { autoZoneTime[4] [activeProgram] = param[0].asLong(); updateActual(4); autoZoneTime[4] [activeProgram] = autoZoneTime[4] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone4RunTime: "); terminal.println(autoZoneTime[4] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V79) { autoZoneTime[5] [activeProgram] = param[0].asLong(); updateActual(5); autoZoneTime[5] [activeProgram] = autoZoneTime[5] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone5RunTime: "); terminal.println(autoZoneTime[5] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V80) { autoZoneTime[6] [activeProgram] = param[0].asLong(); updateActual(6); autoZoneTime[6] [activeProgram] = autoZoneTime[6] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone6RunTime: "); terminal.println(autoZoneTime[6] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V81) { autoZoneTime[7] [activeProgram] = param[0].asLong(); updateActual(7); autoZoneTime[7] [activeProgram] = autoZoneTime[7] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone7RunTime: "); terminal.println(autoZoneTime[7] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V82) { autoZoneTime[8] [activeProgram] = param[0].asLong(); updateActual(8); autoZoneTime[8] [activeProgram] = autoZoneTime[8] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone8RunTime: "); terminal.println(autoZoneTime[8] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V83) { autoZoneTime[9] [activeProgram] = param[0].asLong(); updateActual(9); autoZoneTime[9] [activeProgram] = autoZoneTime[9] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone9RunTime: "); terminal.println(autoZoneTime[9] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V84) { autoZoneTime[10] [activeProgram] = param[0].asLong(); updateActual(10); autoZoneTime[10] [activeProgram] = autoZoneTime[10] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone10RunTime: "); terminal.println(autoZoneTime[10] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V85) { autoZoneTime[11] [activeProgram] = param[0].asLong(); updateActual(11); autoZoneTime[11] [activeProgram] = autoZoneTime[11] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone11RunTime: "); terminal.println(autoZoneTime[11] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V86) { autoZoneTime[12] [activeProgram] = param[0].asLong(); updateActual(12); autoZoneTime[12] [activeProgram] = autoZoneTime[12] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone12RunTime: "); terminal.println(autoZoneTime[12] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V87) { autoZoneTime[13] [activeProgram] = param[0].asLong(); updateActual(13); autoZoneTime[13] [activeProgram] = autoZoneTime[13] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone13RunTime: "); terminal.println(autoZoneTime[13] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V88) { autoZoneTime[14] [activeProgram] = param[0].asLong(); updateActual(14); autoZoneTime[14] [activeProgram] = autoZoneTime[14] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone14RunTime: "); terminal.println(autoZoneTime[14] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V89) { autoZoneTime[15] [activeProgram] = param[0].asLong(); updateActual(15); autoZoneTime[15] [activeProgram] = autoZoneTime[15] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone15RunTime: "); terminal.println(autoZoneTime[15] [activeProgram]); terminal.flush(); } }
BLYNK_WRITE(V90) { autoZoneTime[16] [activeProgram] = param[0].asLong(); updateActual(16); autoZoneTime[16] [activeProgram] = autoZoneTime[16] [activeProgram] * 60;  if (debugEnable) { terminal.print("zone16RunTime: "); terminal.println(autoZoneTime[16] [activeProgram]); terminal.flush(); } }

BLYNK_WRITE(V15) { //test button
    int ref = param.asInt();
    if (ref) {
        terminal.print("Time: "); terminal.println(Time.local() % 86400);
        terminal.println("Program: [0] - [4]");
        for (byte i = 0; i < 7; i++) {
            terminal.print(i); terminal.print(": ");
            terminal.print(weekDay[i][0]);
            terminal.print(weekDay[i][1]);
            terminal.print(weekDay[i][2]);
            terminal.print(weekDay[i][3]);
            terminal.println(weekDay[i][4]);
        }
        for (int i = 1; i < numZones+1; i++) {
            terminal.print("zoneStatus"); terminal.print(i); terminal.print(": ");
            if(zoneStatus[i]){ terminal.println("ON"); }
            else { terminal.println("OFF"); }
        }
        terminal.print("ActiveProgram: "); terminal.println(activeProgram);
        terminal.flush();
    }
}

void setup() { //wished could delay loop() if zone on time is in the past on restart 1st zone turns on right away, but doesn't get recorded in table until its turned off
     
    Particle.function("DebugEnable", switchdb2b);
    //Particle.function("RefreshTable", refreshTable); // not used
    Particle.variable("RunDayCounter", runDayCounter);
    Particle.variable("DebugEnabled", debugEnable);
    Particle.variable("Mode", particleVarMode);
    
    Wire.begin(); //for I2C relays
    Blynk.begin(auth);
    setZone();
    for(byte i = 0; i<numZones; i++) {
        runningCycleTracker[i] [0] = notUsed;
    }
    Blynk.syncVirtual(V0 /*V1-autoCompleted V5, V6, V11 V12*/); // Dont sync master time V12.. sync Zone times V51-numZones
    pwLED.off(); //preset this to off in case power is off when it boots

    setupdelay = timer.setTimeout(10000L, Blynk_init);
}

void Blynk_init() { //running this in setup causes device to lockup
    setupdelay = timerNA; // Blynk_init() called from setup by this timer
    Blynk.logEvent("power_failure", "Battery backup failure - !!Reenter Auto Setup Info!!");
    Blynk.virtualWrite(V9, Time.format("%r %m/%d")); //last reboot time
    Blynk.virtualWrite(V20, 0);   //preset menu to item 0
    Blynk.virtualWrite(V25, 100); //presetslider to 100 so it doesn't get off
    Blynk.virtualWrite(V26, 100);
    for(byte i = 0; i < numPrograms; i++) { //preset lastRunDay
        lastRunDay[i] = 36;
    }
}

void loop() {  
    Blynk.run();
    timer.run();
    bool curVUSB = hasVUSB(); // for checking power supply status at USB
    if (curVUSB != hadVUSB) { hadVUSB = curVUSB;  if(curVUSB) {pwLED.on(); powerRegain();}   else{pwLED.off(); powerFail();}   } //this neeeds to stay above startcycles()
    if(Time.minute() != lastminute) minuteloop(); 
    if(Time.hour() != lasthour) hourloop();
    if (Time.hour() == 3 && Time.day() != timeUpdateDay) runOnceADay();  // update time and daylight savings
} //end loop

/* Program Functions ************************************************************************************************************/
void minuteloop() {
    lastminute = Time.minute();
    Blynk.virtualWrite(V1, Time.format("%r %m/%d"));
    for(byte i=0; i<numZones; i++) {
        if(Time.local() % 86400 >= runningCycleTracker[i] [0] && mode == automatic) {
            if(blynkDebugEnable) {terminal.print(Time.format("%r-%d ")); terminal.print("minuteloop() stop zone: "); terminal.println(i+1); terminal.flush();}
            stopcycleAUTO(i+1, runningCycleTracker[i] [1]);
            runningCycleTracker[i+1] [1] = runningCycleTracker[i] [1]; //move programIdex up to next zone
            runningCycleTracker[i] [0] = notUsed; runningCycleTracker[i] [1] = notUsed;
            startcycleAUTO(i+2, runningCycleTracker[i+1] [1]);
        }
    } //stop running zones before trying to start more stuff
    if (delayDay == 0)  {
        weekdayloop();  //add cyclical adjust down by day
        cyclicalloop();
    }
}
void hourloop() {
    //if(debugEnable) Particle.publish("hourloop()", String(Time.format("%r %m/%d")));
    lasthour = Time.hour();
    signalStrength(); //get current reading
    Blynk.virtualWrite(V2, SIG_STR);
    if (Time.hour() == 3) runOnceADay();
    if (Time.hour() == 0 && delayDay != 0) {
        int oldDelayDay = delayDay;
        delayDay = delayDay - 1; 
        Blynk.virtualWrite(V8, delayDay);
        if(debugEnable) Particle.publish("hourloop()/delayDay", String::format("%d,%d", oldDelayDay, delayDay));
    }
    if (Time.hour() == 0 && delayDay == 0) {
        for(byte i=0; i<numPrograms; i++) {
            if(cycleSelected [i] [1] != 0) cycleSelected [i] [1] = cycleSelected [i] [1] - 1;
        }
    }
    //terminal.print("Today is: "); terminal.println(Time.weekday()); terminal.flush();
}
void weekdayloop() {
    long add5min[numPrograms]; //only allow 5 min start window after start time
    for(byte i=0; i<numPrograms; i++) {
        add5min[i] = autoZoneTime[0] [i] + 300;
        if(lastRunDay[i] != Time.day() && weekDay[Time.weekday()-1] [i] && Time.local() % 86400 >= autoZoneTime[0] [i] && Time.local() % 86400 <= add5min[i] && mode == automatic) {
            lastRunDay[i] = Time.day(); //for blocking out programs to run more than once a day
            Blynk.logEvent("zone", String("WeekDay Program Starting: ") + programNames[i]);
            startcycleAUTO(1, i);// if has Vusb??
        }
    }
}
void cyclicalloop() { //V22
    long add5min[numPrograms]; //only allow 5 min start window after start time
    for(byte i=0; i<numPrograms; i++) {
        add5min[i] = autoZoneTime[0] [i] + 300;
        if(lastRunDay[i] != Time.day() && cycleSelected [i] [1] == 0 && Time.local() % 86400 >= autoZoneTime[0] [i] && Time.local() % 86400 <= add5min[i] && mode == automatic) {
            lastRunDay[i] = Time.day(); //for blocking out programs to run more than once a day
            cycleSelected [i] [1] = cycleSelected [i] [0];
            Blynk.logEvent("zone", String("Cyclical Program Starting: ") + programNames[i]);
            startcycleAUTO(1, i);// if has Vusb??
        }
    }
}
void startcycleAUTO(int zone, int programIndex) {
    if(adjustedZoneTime[zone] [programIndex] != 0) {
        turnOnRelay(zone, 1);
        Blynk.virtualWrite(V50+zone, 1);
        runningCycleTracker[zone-1] [0] = Time.local() % 86400; //store time right now as secounds into day
        runningCycleTracker[zone-1] [0] = runningCycleTracker[zone-1] [0] + adjustedZoneTime[zone] [programIndex]; //add amount of time to run for
        runningCycleTracker[zone-1] [1] = programIndex;
        if(blynkDebugEnable) {terminal.print(Time.format("%r-%d ")); terminal.print("startCycleAUTO() zone: "); terminal.println(zone);}
        if(blynkDebugEnable) {terminal.print(runningCycleTracker[zone-1] [0]); terminal.println(runningCycleTracker[zone-1] [1]); terminal.flush();}
    }
    else if(zone < numZones) { //if zone has 0 sec in adjustedZoneTime skip to next Zone
        //delay(500);
        zone = zone + 1; // advance to next zone
        if(blynkDebugEnable) {terminal.print(Time.format("%r-%d ")); terminal.print("startCycleAUTO() esle if zone: "); terminal.println(zone); terminal.flush();}
        startcycleAUTO(zone, programIndex); 
    }
}
void stopcycleAUTO(int zone, int programIndex) {
    turnOffRelay(zone, 1);
    Blynk.virtualWrite(V50+zone, LOW);
    if(blynkDebugEnable) {terminal.print(Time.format("%r-%d ")); terminal.print("stopCycleAUTO() zone: "); terminal.println(zone); terminal.flush();}
}


void powerRegain() { //this also runs 1 time on reboot -- need some version of turnOff relays that where running when powerFail()
    Blynk.logEvent("info", "Main power restored Mode: " + particleVarMode); //add in mode switched to
    setMode(modeBeforePowerFail);
    Blynk.virtualWrite(V0, modeBeforePowerFail);
}
void powerFail() { //what should happen when VUSB goes dead
    Blynk.logEvent("power_failure", "Main power failure - Running on battery");
    if(blynkDebugEnable) {terminal.println("powerFail()"); terminal.flush();}
    modeBeforePowerFail = mode;
    setMode(off); //try this instead of code below
    Blynk.virtualWrite(V0, 0);
}
/* Relay Functions **************************************************************************************************************/
void turnOnRelay(int relay, bool zoneEvent) {
    relayONcommand(relay);
    if(masterValve) { relayONcommand(numZones+1); masterValveState = 1; }
    zoneStatus[relay] = 1;
    char msg[20];
    sprintf_P(msg, PSTR("Zone %d ON"), relay);
    Blynk.logEvent("zone", msg);
}
void relayONcommand(int relay) {   
    if(debugEnable) Particle.publish("turnOnRelay()", String(relay));
    if(relay >= 1 && relay <= 8) channel_mode(BOARD_1, relay, 1);
    else if(relay >= 9 && relay <= 16) channel_mode(BOARD_2, (relay-8), 1);
    else if(relay >= 17 && relay <= 24) channel_mode(BOARD_3, (relay-16), 1);
    else if(debugEnable) Particle.publish("error/turnOnRelay()", String(relay));
    Blynk.virtualWrite(V17, relay);
}
void turnOffRelay(int relay, bool zoneEvent) {
    if(debugEnable) Particle.publish("turnOffRelay()", String(relay));
    if(masterValve) relayOFFcommand(numZones+1); 
    if(masterValveState) delay(500); //if master valve was on, wait a bit before shutting off zone valve
    masterValveState = 0;
    relayOFFcommand(relay);
    zoneStatus[relay] = 0;
    char msg[20];
    sprintf_P(msg, PSTR("Zone %d OFF"), relay);
    Blynk.logEvent("zone", msg);
}    
void relayOFFcommand(int relay) {   
    if(relay >= 1 && relay <= 8) channel_mode(BOARD_1, relay, 0);
    else if(relay >= 9 && relay <= 16) channel_mode(BOARD_2, (relay-8), 0);
    else if(relay >= 17 && relay <= 24) channel_mode(BOARD_3, (relay-16), 0);
    else if(debugEnable) Particle.publish("error/turnOffRelay()", String(relay));    
    Blynk.virtualWrite(V17, 0);
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
/* Particle Functions ***********************************************************************************************************/
int switchdb2b(String command) {
    if(command == "2") {
        blynkDebugEnable = 1;
        return 2;
    }
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

void setMode(MODE m) { 
  switch (m) {
    case off:
        for (int i = 1; i < numZones+1; i++) {
            if(zoneStatus[i]){//if selected zone is on
                turnOffRelay(i, 1);
                Blynk.virtualWrite(V50+i, LOW);
                //updateBlynkTable(i, 0);
            }
        }
        if(blynkDebugEnable) {terminal.println("setMode() = Off"); terminal.flush();}
        particleVarMode = "OFF";
        break;
      
    case manual:
        if(blynkDebugEnable) {terminal.println("setMode() = manual"); terminal.flush();}
        particleVarMode = "Manual";
        break;
    case automatic:
        if(blynkDebugEnable) {terminal.println("setMode() = automatic"); terminal.flush();}
        particleVarMode = "Auto";
        break;
    case advanced:
        if(blynkDebugEnable) {terminal.println("setMode() = advanced"); terminal.flush();}
        particleVarMode = "unUsed";
    default: 
      break;
  }
}
bool hasVUSB() { //checks if power supplied at USB this runs in loop() - bool curVUSB = hasVUSB(); 
    uint32_t *pReg = (uint32_t *)0x40000438; // USBREGSTATUS

    return (*pReg & 1) != 0;
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
	    timeOffset = -6;
		Time.zone(timeOffset);
	}else if (month > 3 && month < 11) {
		timeOffset = -5;
		Time.zone(timeOffset);
	}else if (month == 3) {
		//int offset = (previousSunday >= 8)? -5 : -6;
		timeOffset = (previousSunday >= 8)? -5 : -6;
		Time.zone(timeOffset);
		//Blynk.virtualWrite(V?, timeOffset);
	} else{
		//int offset = (previousSunday <= 0)? -5 : -6;
		timeOffset = (previousSunday >= 0)? -5 : -6;
		Time.zone(timeOffset);
		//put into string with other info if need to use this Blynk.virtualWrite(V?, timeOffset);
	}
	terminal.println(previousSunday); terminal.flush();
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
