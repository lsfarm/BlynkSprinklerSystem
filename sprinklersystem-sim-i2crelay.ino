/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
V0   - Mode Switch
V1   - Time Offset from UTC used to set correct time on controller
V2   - Time printed back on this channel after adjusted via V1
V5   - Time Input Widget for auto zone start time
V6   - Zone Run time in auto mode
V7   - Auto zone finish time
V9   - Table Widget - used for showing value start and stop times
V10  - Terminal
V11  - Time Input Widget for advanced mode start time
V12  - Advanced mode zone run time
V21  - Manual Valve switch input  
V45  - 21-45 reserved for inputs

https://github.com/ControlEverythingCom/NCD16Relay/blob/master/firmware/NCD16Relay.cpp
>> https://github.com/ControlEverythingCom/NCD16Relay/blob/master/README.md
relayController.turnOffAllRelays();  !!this doesn't work at all!! if you do this than next time turnOnRelay(relayNumber); comes around it will turn on entire set
relayController.turnOnRelay(relayNumber);
relayController.toggleRelay(i);
      terminal.println(Time.local() % 86400); //this gives time of day in secounds
      terminal.println(Time.day());           //this gives day of month
      
!!Issues!!
-- auto mode contuines even when switched back to manual mode <<bandaid for now is to clear remaning loop from running  >>would like to add clearTimeout()
*/

#include <blynk.h>
//char auth[] = "zh**"; //mine
char auth[] = "***"; //kelly's
//WidgetTable table;
//BLYNK_ATTACH_WIDGET(table, V9);
//int rowIndex = 0;
WidgetTerminal terminal(V10);
BlynkTimer timer;
enum  MODE { off = 1, manual = 2, automatic = 3, advanced = 4, unknown = 999 }; //https://www.baldengineer.com/state-machine-with-enum-tutorial.html
MODE         mode        = unknown;
void         setMode(MODE m);

#include <NCD16Relay.h>
NCD16Relay R1;
/* User Adjusted ***************************************************************************************************************/
bool debugEnable = 1;
bool sendTOblynk = 1;  //this sends v21-numZones button status to blynk in auto mode
const int numZones = 12; // need const for advanSched[]
/* Program Variables **********************************************************************************************************/
long startTimeInSec;
long zoneRunTime;
long zoneRunTimeAsSec;
long startTimeInSecADVAN;
long zoneRunTimeADVAN;
int timeOffset;
int previousDay = 0;
int previousDayADVAN = 0;
int count = 1;
int secoundcount;
int counterADVAN;
int setupdelay;  // delayed timer for some blynk stuff that won't load in setup
//const int arr = numZones - 1;
int advanSched[numZones] [2]; //[numZones - 1]; //advanced schedule this holds values from V101 - (V101+numZones)  https://www.tutorialspoint.com/arduino/arduino_multi_dimensional_arrays.htm

BLYNK_WRITE(V1) { //used to adjust for time change  wished I could find a simple way to do this automatically
    timeOffset = param[0].asInt();
    Time.zone(timeOffset);
    Blynk.virtualWrite(V2, Time.format("%r:%D"));
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
    if (debugEnable) {
        terminal.print("zoneRunTime: ");
        terminal.println(zoneRunTime);
        terminal.flush();
    }
    finishTimeCal();
}
void finishTimeCal() {// V7  !!time.zone messes this up!!  terminal.print(Time.format("%D %r - "));
    Time.zone(0);
    long val = zoneRunTimeAsSec * numZones;
    long finTime = startTimeInSec + val; //this is sec
    Blynk.virtualWrite(V7, Time.format(finTime, "%r"));
    //terminal.print("finTime: ");
    //terminal.println(Time.format(finTime));
    //terminal.println(Time.format("%D %r - "));
    //terminal.flush();
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
      //terminal.println(mode);
      //terminal.flush();
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
          R1.turnOffRelay(nr+1); 
          Blynk.virtualWrite(V9, "update", nr, (Time.format("%D %r - ")), nr); 
          Blynk.virtualWrite(V9, F("deselect"), 6);
        }
      if(value) { R1.turnOnRelay(nr+1);  Blynk.virtualWrite(V9, "update", nr, "on", nr+1);  Blynk.virtualWrite(V9, "deselect", 5);}
      break;
    case automatic: 
      Blynk.virtualWrite(V21+nr, 0);
      Blynk.notify("In Auto Mode - Selecting Manual Mode will dicountinue todays schedule");
      break;
      
    case unknown:
    default:
      break;
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
  if(debugEnable){
      terminal.print("startTimeInSecADVAN: ");
      terminal.println(startTimeInSecADVAN);
      terminal.flush();
  }
  //finishTimeCal();
}
BLYNK_WRITE(V12) {
    zoneRunTimeADVAN = param[0].asLong();        //as minute
    zoneRunTimeADVAN = zoneRunTimeADVAN * 60;    //convert minutes to seconds
    zoneRunTimeADVAN = zoneRunTimeADVAN * 1000;  //converts seconds to millisec
    if (debugEnable) {
        terminal.print("zoneRunTimeADVAN: ");
        terminal.println(zoneRunTimeADVAN);
        terminal.flush();
    }
    //finishTimeCal();
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

void setup() {
    //Time.zone(-6);
    Blynk.begin(auth);
    Blynk.syncVirtual(V0, V1, V5, V6, V11, V12); // V101, V102, V103, V104, V105, V106, V107, V108, V109, V110, V111, V112);
    Blynk.virtualWrite(V9, "clr");
    setupdelay = timer.setTimeout(5000L, Blynk_init);
    //table.addRow(rowIndex, "Test row", millis() / 1000);
    //table.pickRow(rowIndex);
    //timer.setInterval(3000L, startcycleADVAN);
    Blynk.notify("!Power Outage!  Controller has restarted");
    R1.setAddress(1, 1, 1);
    if(R1.initialized){
        terminal.println("Relay is ready");
        terminal.flush();
    }else{
        terminal.println("Relay not ready");
        terminal.flush();
    }
    //R1.turnOffAllRelays();
}

void Blynk_init() { //running this in setup causes device to lockup
    for(byte i = 0; i<numZones; i++) {
        Blynk.syncVirtual(V101+i);
        //delay(500);
    }
    for(byte i = 0; i < numZones; i++) {
        char nodeName[9];
        sprintf_P(nodeName, PSTR("Zone %d"), (i+1));
        Blynk.virtualWrite(V9, F("add"), i, F("Unknown"), nodeName);
        Blynk.virtualWrite(V9, F("deselect"), i);
    }
}

void loop() {
    Blynk.run();
    timer.run();
    if (previousDay != Time.day() &&  Time.local() % 86400 >= startTimeInSec && mode == automatic) { //auto mode cycle
        previousDay = Time.day();
        startcycleAUTO();
   }
       if (previousDayADVAN != Time.day() &&  Time.local() % 86400 >= startTimeInSecADVAN && mode == advanced) { //advanced mode cycle
        previousDayADVAN = Time.day();
        terminal.println("inloop345678");
        terminal.flush();
        counterADVAN = 0;
        startcycleADVAN();
    }
}

void continuecycleADVAN() {
    R1.turnOffRelay(counterADVAN + 1);
    counterADVAN++;
    if(counterADVAN < numZones) {startcycleADVAN();}
    //else {previousDayADVAN = 0; counterADVAN = 0; terminal.println("previousday reset"); terminal.flush();}  //for debugging
}

void startcycleADVAN() {
    terminal.print("co#: ");
    terminal.println(counterADVAN);
    if (advanSched[counterADVAN][1] == 1) {R1.turnOnRelay(counterADVAN + 1); terminal.print("running advanced CYCLE Zone: "); terminal.println(counterADVAN + 1);terminal.flush();}
    int delayedStop = timer.setTimeout(zoneRunTimeADVAN, continuecycleADVAN);
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
        if (advanSched[counterADVAN][1] == 1) { advanSched[counterADVAN][1] = 3; }
        else { advanSched[counterADVAN][1] = advanSched[counterADVAN][1] - 1; }
        break;
      default:
        terminal.println("Error in startcycle SwitchCase");
      break;
    }    
    for(int i = 0; i<12; i++){
        terminal.print(i + 1);
        terminal.print(": ");
        terminal.print(advanSched[i][0]);
        terminal.print(":");
        terminal.println(advanSched[i][1]);
        terminal.flush();
        delay(100);
    } 
}

void startcycleAUTO() {
    int delay1 = timer.setTimeout(1000L, [] () { R1.turnOnRelay(count); if(sendTOblynk) {Blynk.virtualWrite(V20+count, HIGH); terminal.print("Zone ON Count: "); terminal.println(count); terminal.flush();}});
    int delayedOff = timer.setTimeout(zoneRunTime, [] () {
        if (count != numZones) {//if we haven't reached the last zone
            R1.turnOffRelay(count);
            if(sendTOblynk) {Blynk.virtualWrite(V20+count, LOW);}
            terminal.print(Time.format("%D %r - "));
            terminal.println(" - Manualtimerset");
            count++;
            startcycleAUTO();
        }
        else { //we've reached last zone, turn it off and reset count
            R1.turnOffRelay(numZones);
            if(sendTOblynk) {Blynk.virtualWrite(V20+numZones, LOW);}
            count = 1;
            terminal.print(Time.format("%D %r - "));
            terminal.println("last zone reached");
            terminal.flush();
            //previousDay = 1;  //used to debug
        }
    });
    terminal.print(Time.format("%D %r - "));
    terminal.println("bottom of startcycleAUTO()");
    terminal.flush();
}

void setMode(MODE m) {
  switch (m) {
    case off:
      for (int i = 0; i < numZones; i++) {
          R1.turnOffRelay(i+1);
          Blynk.virtualWrite(V21+i, LOW);
      }
      terminal.println("inMODEoff");
      terminal.flush();
      break;
      
    case manual:
    case automatic:
    case advanced:
      for (int i = 0; i < numZones; i++) {
          R1.turnOffRelay(i+1);
          Blynk.virtualWrite(V21+i, LOW);
      }
      terminal.println("inMODEmanandauto");
      terminal.flush();
    default: 
      break;
  }
}
