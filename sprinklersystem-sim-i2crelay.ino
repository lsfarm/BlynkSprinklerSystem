/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
V0   - Mode Switch
V21  - Manual Valve switch input  
V45  - 21-45 reserved for inputs
V10  = Terminal
https://github.com/ControlEverythingCom/NCD16Relay/blob/master/firmware/NCD16Relay.cpp
>> https://github.com/ControlEverythingCom/NCD16Relay/blob/master/README.md
relayController.turnOffAllRelays();  !!this doesn't work at all!! if you do this than next time turnOnRelay(relayNumber); comes around it will turn on entire set
relayController.turnOnRelay(relayNumber);
relayController.toggleRelay(i);
      terminal.println(Time.local() % 86400); //this gives time of day in secounds
      terminal.println(Time.day());           //this gives day of month
      
!!Issues!!
-- auto mode contuines even when switched back to manual mode
*/

#include <blynk.h>
//char auth[] = "**"; //mine
char auth[] = "**"; //kelly's
WidgetTerminal terminal(V10);
BlynkTimer timer;
enum  MODE { off = 1, manual = 2, automatic = 3, unknown = 999 }; //https://www.baldengineer.com/state-machine-with-enum-tutorial.html
MODE         mode        = unknown;
void         setMode(MODE m);

#include <NCD16Relay.h>
NCD16Relay R1;

long startTimeInSec;
long zoneRunTime;
long zoneRunTimeAsSec;
int previousDay = 0;
int count = 1;
int secoundcount;
//>> User Adjusted <<//
bool sendTOblynk = 1;  //this sends v21-numZones button status to blynk in auto mode
int numZones = 12;

BLYNK_WRITE(V1) { //Time Input Widget
  startTimeInSec = param[0].asLong();
  terminal.print("startTimeInSec: ");
  terminal.println(startTimeInSec);
  terminal.flush();
  finishTimeCal();
}
BLYNK_WRITE(V2) {
    zoneRunTime = param[0].asLong(); //as minute  !!not yet tho
    zoneRunTimeAsSec = zoneRunTime;
    zoneRunTime = zoneRunTime * 1000;  //as milli sec
    terminal.print("zoneRunTime: ");
    terminal.println(zoneRunTime);
    terminal.flush();
    finishTimeCal();
}

void finishTimeCal() {// time.zone messes this up!!  terminal.print(Time.format("%D %r - "));
    Time.zone(0);
    long finTime = startTimeInSec + zoneRunTimeAsSec; //this is sec
    Blynk.virtualWrite(V3, Time.format(finTime, "%r"));
    terminal.print("finTime: ");
    terminal.println(Time.format(finTime));
    terminal.println(Time.format("%D %r - "));
    terminal.flush();
    Time.zone(-6);
}
/////////************* **********/////////
//             Mode Selection           //
/////////************* **********/////////
BLYNK_WRITE(V0) {
  switch (mode = (MODE)param.asInt())
  {
    case off:
      Blynk.virtualWrite(V100, "Mode: OFF");
      //terminal.println(mode);
      //terminal.flush();
      break;
    case manual: 
      Blynk.virtualWrite(V100, "Mode: Manual");
      break;
    case automatic: 
      Blynk.virtualWrite(V100, "Mode: Automatic");
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
      if(value == 0) {R1.turnOffRelay(nr+1);}
      if(value)      {R1.turnOnRelay(nr+1);}
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

void setup() {
    Time.zone(-6);
    Blynk.begin(auth);
    Blynk.syncVirtual(V0, V1, V2);
    //timer.setInterval(1000L, startcycle);
    Blynk.notify("!Power Outage!  Controller has restarted");
    terminal.println(zoneRunTime);
    R1.setAddress(1, 1, 1);
    if(R1.initialized){
        terminal.println("Relay is ready");
        terminal.flush();
    }else{
        terminal.println("Relay not ready");
        terminal.flush();
    }
    R1.turnOffAllRelays();
}

void loop() {
    Blynk.run();
    timer.run();
    //if (mode == automatic) {terminal.println("autoMODE"); delay(1000);}
    if (previousDay != Time.day() &&  Time.local() % 86400 >= startTimeInSec && mode ==automatic) {
        previousDay = Time.day();
        startcycle();
  }
}

void startcycle()
{
    R1.turnOffRelay(count - 1);
    if(sendTOblynk) {Blynk.virtualWrite(V20+count-1, LOW);}
    secoundcount = count;  //count gets ++ before R1.turnOnRelay;
    int delay1 = timer.setTimeout(1000L, [] () { R1.turnOnRelay(secoundcount); if(sendTOblynk) {Blynk.virtualWrite(V20+secoundcount, HIGH);}});
    terminal.print("Count: ");
    terminal.println(count);
    if (count != numZones) {
        int delayedOff = timer.setTimeout(zoneRunTime, startcycle);
        count++;
    }
    else {
        int delayedOff = timer.setTimeout(zoneRunTime, [] () {
            R1.turnOffRelay(numZones);
            if(sendTOblynk) {Blynk.virtualWrite(V20+numZones, LOW);}
            //previousDay = 1;  //used to debug
        });
        count = 1;
    }
    
    //int hey = R1.readRelayStatus(2);
    //terminal.print(Time.format("%D %r - "));
    //terminal.println(hey);
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
