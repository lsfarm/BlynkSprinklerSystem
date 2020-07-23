/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
V0   - Mode Switch
VX   - Status LED :: Not working correctly, won't switch colors with app closed
V1X  - Manual Switch
V2X  - Auto Switch :: Time Input Channel
V3X  - Actual Valve State
V4X  - Channel Start Seconds
V5X  - Channel Stop Seconds
V90  - Blynk App Notifications Enable/Disable
V100 - Controller Status
V101 - Garage Door Last Activatated
V102 - Mode on Controller 1=OFF 2=Manual 3=Auto
V103 - NotifyStatus : for debugging will delete
V105 - WiFi Signal Strength
V106 - Garage Door on D7 Dustins
V110 - dB Wifi Signal >>>>DELETE<<<<<
Thanks to @Costas:
https://community.blynk.cc/t/time-input-widget-and-eventor/14868/16
*/
#include <blynk.h>
bool holdoffsend = FALSE;
int delayedsend;
int delayedsend2;
int gdoorstate;

//const char   *auth       = "***"; //mine
const char   *auth       = "***"; //Dustins
const char   *GREEN      = "#008000"; //"#23C48E"
const char   *BLUE       = "#04C0F8";
const char   *YELLOW     = "#ED9D00";
const char   *RED        = "#FF033E"; //"#D3435C"
const char   *DARK_BLUE  = "#5F7CD8";

enum  MODE { off = 1, manual = 2, automatic = 3, unknown = 999 };

MODE         mode        = unknown;
unsigned int secNow      = 86400;
int          alertStatus = off;  //EEPROM location 1

struct valve_t {
  int        pin;
  bool       manual; // false .. automatic
  bool       state;
  WidgetLED  led;
};
const int nValves = 8;                                      // number of valves
valve_t valve[nValves] = 
{ { D0, false, LOW, WidgetLED(V1) }
, { D1, false, LOW, WidgetLED(V2) }
, { D2, false, LOW, WidgetLED(V3) }
, { D3, false, LOW, WidgetLED(V4) }
, { D4, false, LOW, WidgetLED(V5) }
, { D5, false, LOW, WidgetLED(V6) }
, { D6, false, LOW, WidgetLED(V7) }
, { D7, false, LOW, WidgetLED(V8) }
};

// Software Timers have too limited stack for the calls
//Timer timActivity(60000L, activeToday);                   // check every 60s if ON / OFF trigger time has been reached
//Timer timClock(1000L,  clockDisplay);                     // check every second if time has been obtained from the server
//Timer timInfo(1000L, sendInfo);
BlynkTimer timer;

void         setMode(MODE m);

BLYNK_WRITE(V90)
{
  alertStatus = param.asInt();
  EEPROM.put(50, alertStatus);
}

void door_off()
{
    digitalWrite(valve[7].pin, HIGH);
}

BLYNK_WRITE(V106)
{
    gdoorstate = param.asInt();
    if (gdoorstate == 1)
    {
       digitalWrite(valve[7].pin, !gdoorstate);
       int delayedOff = timer.setTimeout(800, door_off); 
       Blynk.virtualWrite(V100, "!!Garage Door Moving!!");
       Blynk.virtualWrite(V101, Time.format("%r - %a %D"));
       sendInfoaftercommand();
    }
    
}

/////////************* **********/////////
//             Mode Selection           //
/////////************* **********/////////

BLYNK_WRITE(V0) {
  switch (mode = (MODE)param.asInt())
  {
    case off:
      Blynk.virtualWrite(V100, "Mode: OFF");
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
  sendInfoaftercommand();
  setMode(mode);  
  //if (mode != unknown) 
    //EEPROM.put(1, mode);                              //not used--may use if Wifi issues and blynk_connect fails
}

/////////************* **********/////////
//             Manual Buttons           //
/////////************* **********/////////

void blynkWriteManual(int nr, int value) {
  char msg[32];
  valve[nr].manual = value;
  
  switch (mode) {
    case off:
      //Blynk.virtualWrite(V11, 0);
      Blynk.virtualWrite(V11+nr, 0);
      Blynk.notify("Select Manual or Auto Mode");
      break;
      
    case manual:
    case automatic:
      digitalWrite(valve[nr].pin, !valve[nr].manual);
      valve[nr].led.setColor(valve[nr].manual ? GREEN : RED);
      snprintf(msg, sizeof(msg), "Zone %d Turning %s", nr+1, valve[nr].manual ? "On" : "Off");
      Blynk.virtualWrite(V100, msg);
      sendInfoaftercommand();
      //int delayedOff = timer.setTimeout(8, sendInfo);
      break;
      
    case unknown:
    default:
      break;
  }
}

BLYNK_WRITE(V11) { blynkWriteManual(0, param.asInt()); }
BLYNK_WRITE(V12) { blynkWriteManual(1, param.asInt()); }
BLYNK_WRITE(V13) { blynkWriteManual(2, param.asInt()); }
BLYNK_WRITE(V14) { blynkWriteManual(3, param.asInt()); }
BLYNK_WRITE(V15) { blynkWriteManual(4, param.asInt()); }


/////////************* **********/////////
//          Automatic Controls          //
/////////************* **********/////////

void blynkWriteAuto(int nr, const BlynkParam& param) {
  TimeInputParam t(param);
  char msg[32] = "";
  //int weekDay = Time.weekday();
  int weekDay = Time.weekday() + ((Time.weekday() == 1) ? +6 : -1);       // weekday mapping 1=Sun,2=Mon,...,7=Sat -> 1=Mon,2=Tue,...,7=Sun
  Blynk.virtualWrite(V99, weekDay);
  unsigned int secStart = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);  
  unsigned int secStop = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);

  secNow = Time.local() % 86400;
  Blynk.virtualWrite(V41+nr, secStart);
  Blynk.virtualWrite(V51+nr, secStop);

  if(t.isWeekdaySelected(weekDay)) {  
    //Schedule is ACTIVE today 
    if(secStart - 31 <= secNow && secNow <= secStart + 31) {            // 62s on 60s timer ensures 1 trigger command is sent
      snprintf(msg, sizeof(msg), "Zone %d Starting  AUTOMODE", nr+1);
      Blynk.virtualWrite(V100, msg);
      Blynk.setProperty(V1+nr, "color", BLUE);
      Blynk.virtualWrite(V11+nr, HIGH);
      digitalWrite(valve[nr].pin, LOW);
      sendInfoaftercommand();
    }                  
    if(secStop -31 <= secNow && secNow <= secStop + 31) {               
      snprintf(msg, sizeof(msg), "Zone %d Stopping  AUTOMODE", nr+1);
      Blynk.virtualWrite(V100, msg);
      Blynk.setProperty(V1+nr, "color", YELLOW);
      Blynk.virtualWrite(V11+nr, LOW);
      digitalWrite(valve[nr].pin, HIGH);
      sendInfoaftercommand();
    }               
    
    if(alertStatus) 
      Blynk.notify(msg);
  }
}

BLYNK_WRITE(V21) { blynkWriteAuto(0, param); }          // Scheduler #X Time Input widget  
BLYNK_WRITE(V22) { blynkWriteAuto(1, param); }   
BLYNK_WRITE(V23) { blynkWriteAuto(2, param); }     
BLYNK_WRITE(V24) { blynkWriteAuto(3, param); }     
BLYNK_WRITE(V25) { blynkWriteAuto(4, param); }     

BLYNK_CONNECTED() {                                                     //get data stored in virtual pin V0 from server
  Blynk.syncVirtual(V0);
}

void setup() {
    Blynk.begin(auth);
    Time.zone(-5);   
  
    // limited stack size prevents the use of Software Timers
    //timActivity.start();
    //timClock.start();
    //timInfo.start();

    timer.setInterval(60000L, activeToday);  // check every 60s if ON / OFF trigger time has been reached
    timer.setInterval(1000L, clockDisplay);  // check every second if time has been obtained from the server
    timer.setInterval(20000L, sendInfo);
    //delayedsend = timer.setTimeout(5500, sendInfo);
    //delayedsend2 = timer.setTimeout(6000, resethold);
  
    //Blynk.virtualWrite(V0, 2);
    for (int i = 0; i < nValves; i++) {
      pinMode(valve[i].pin, OUTPUT);
      digitalWrite(valve[i].pin, HIGH);
    }

    firstrun();
}

void loop() {
  Blynk.run();
  timer.run();
}

void firstrun() {
  EEPROM.get(50, alertStatus);
  Blynk.virtualWrite(V90, alertStatus);
}

void resethold () {
    holdoffsend = 0;
} 

void sendInfoaftercommand () {
    delayedsend = timer.setTimeout(4500, sendInfo2);
    delayedsend2 = timer.setTimeout(10000, resethold);
    //timer.restartTimer(delayedsend);
    //timer.restartTimer(delayedsend2);
    holdoffsend = 1;
    //int delayedOff = timer.setTimeout(5000, sendInfo);
}

void sendInfo2() {
    //Blynk.virtualWrite(V100, "heyu");
    Blynk.virtualWrite(V100, Time.format("%r - %a %D"));
    //Blynk.virtualWrite(V105, WiFi.RSSI().getStrength());
    Blynk.virtualWrite(V110, WiFi.RSSI());
    
    for (int i = 0; i < nValves; i++) {
        valve[i].state = !digitalRead(valve[i].pin);
        Blynk.virtualWrite(V31+i, valve[i].state);
  }
}
void sendInfo() {
  if (holdoffsend == 0)
  {
    Blynk.virtualWrite(V100, Time.format("%r - %a %D"));
    //Blynk.virtualWrite(V101, secNow);
    //Blynk.virtualWrite(V102, mode);
    //Blynk.virtualWrite(V103, alertStatus); // may delete
    Blynk.virtualWrite(V105, WiFi.RSSI().getStrength());
    //Blynk.virtualWrite(V110, WiFi.RSSI());
  }
    
  for (int i = 0; i < nValves; i++) {
    valve[i].state = !digitalRead(valve[i].pin);
    Blynk.virtualWrite(V31+i, valve[i].state);
  }
}

void setMode(MODE m) {
  switch (m) {
    case off:
      for (int i = 0; i < nValves; i++) {
        valve[i].led.off();
        Blynk.virtualWrite(V11+i, LOW);
        digitalWrite(valve[i].pin, HIGH);
      }
      break;
      
    case manual:
    case automatic:
      for (int i = 0; i < nValves; i++) {
        valve[i].led.on();
        valve[i].led.setColor((m == manual)? RED : YELLOW);
        Blynk.virtualWrite(V11+i, LOW);
        digitalWrite(valve[i].pin, HIGH);
      }
      
    default: 
      break;
  }
}

void activeToday() {  // check if schedule # should run today
  if(mode != 3 || !Time.isValid()) 
    return;

  for (int i = 0; i < nValves; i++) 
    Blynk.syncVirtual(V21+i);  // sync scheduler #
}

void clockDisplay() {  // only needs to be done once after time sync
  static bool hasRun = false;
  if (hasRun || !Time.isValid) return;
  hasRun = true;
  Serial.println(Time.format("%T"));
} 
