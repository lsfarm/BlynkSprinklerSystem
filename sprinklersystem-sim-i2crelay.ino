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
relayController.turnOffAllRelays();
relayController.turnOnRelay(relayNumber);
relayController.toggleRelay(i);
      terminal.println(Time.local() % 86400); //this gives time of day in secounds
      terminal.println(Time.day());           //this gives day of month
*/

#include <blynk.h>
char auth[] = "zh7lIH_MEeMqn_drxNhOhut37IxDcYrk";
WidgetTerminal terminal(V10);
BlynkTimer timer;
enum  MODE { off = 1, manual = 2, automatic = 3, unknown = 999 }; //https://www.baldengineer.com/state-machine-with-enum-tutorial.html
MODE         mode        = unknown;
void         setMode(MODE m);

#include <NCD16Relay.h>
NCD16Relay R1;

long startTimeInSec;
long zoneRunTime;
int previousDay = 0;
int count = 1;
int secoundcount;
long zorun = 1000L;
//>> User Adjusted <<//
int numZones = 12;

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


BLYNK_WRITE(V1) {
  startTimeInSec = param[0].asLong();
  terminal.print("startTimeInSec: ");
  terminal.println(startTimeInSec);
  terminal.flush();
}
BLYNK_WRITE(V2) {
    zoneRunTime = param[0].asLong();
    zoneRunTime = zoneRunTime * 60;
    terminal.print("zoneRunTime: ");
    terminal.println(zoneRunTime);
    terminal.flush();
}

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

void turnon() {
    R1.turnOnRelay(count);
}

void startcycle()
{
    R1.turnOffRelay(count - 1);
    secoundcount = count;
    int delay1 = timer.setTimeout(1000L, [] () { R1.turnOnRelay(secoundcount); });
    terminal.print("Count: ");
    terminal.println(count);
    if (count != numZones) {
        int delayedOff = timer.setTimeout(zoneRunTime, startcycle);
        count++;
    }
    else {
        int delayedOff = timer.setTimeout(zoneRunTime, [] () {
            R1.turnOffAllRelays();
            //previousDay = 1;  //used to debug
        });
        count = 1;
    }
    
    //int hey = R1.readRelayStatus(1);
    //terminal.print(Time.format("%D %r - "));
    //terminal.println(hey);
    terminal.flush();
}

void setMode(MODE m) {
  switch (m) {
    case off:
    terminal.println("inmodeOFF");
    terminal.flush();
      /*if(D0D7relay){
          for (int i = 0; i < nValves; i++) {
              valve[i].led.off();
              Blynk.virtualWrite(V21+i, LOW);
              digitalWrite(valve[i].pin, HIGH);
            }
        }
        if(I2Crelay){ //bit write 1 == off 
            for (int i = 0; i < nValves; i++) {
              valve[i].led.off();
              Blynk.virtualWrite(V21+i, LOW);
            }
        }*/
      break;
      
    case manual:
    case automatic:
      terminal.println("inMODEmanandauto");
      terminal.flush();
      /*if(D0D7relay){
          for (int i = 0; i < nValves; i++) {
              valve[i].led.on();
              valve[i].led.setColor((m == manual)? RED : YELLOW);
              Blynk.virtualWrite(V21+i, LOW);
              digitalWrite(valve[i].pin, HIGH);
            }
       }
       if(I2Crelay){ //bit write 1 == off      
          for (int i = 0; i < nValves; i++) {
              valve[i].led.on();
              valve[i].led.setColor((m == manual)? RED : YELLOW);
              Blynk.virtualWrite(V21+i, LOW);
           }
        }*/
       
    default: 
      break;
  }
}
