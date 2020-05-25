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
V90  - Notifications Enable/Disable
V100 - Controller Status
V101 - NowSeconds
V102 - Mode on Controller
V103 - NotifyStatus : for debugging will delete
V105 - WiFi Signal Strength
Thanks to @Costas:
https://community.blynk.cc/t/time-input-widget-and-eventor/14868/16
*/
#include <blynk.h>
char auth[] = "sgmD0T0O3vBNUZjRcLJFQ2oHAxuclV-W"; 
BlynkTimer timer;
char currentTime[9];
bool clockSync = false;
unsigned int nowseconds = 999;

int mode = 0;
int manuel1 = 0;
int manuel2 = 0;
int manuel3 = 0;
int manuel4 = 0;
int manuel5 = 0;
int auto1 = 0;
int auto2 = 0;
int auto3 = 0;
int auto4 = 0;
int auto5 = 0;
int alertstatus = 1;  //EEPROM location 1
int valve1state = 999;  //999 is error status
int valve2state = 999;
int valve3state = 999;
int valve4state = 999;
int valve5state = 999;


// Zone valves
#define valve1 2
#define valve2 3
#define valve3 4
#define valve4 5
#define valve5 6
#define valve6 7
#define valve7 8

//int counts = 0;

#define GREEN     "#008000"//"#23C48E"
#define BLUE      "#04C0F8"
#define YELLOW    "#ED9D00"
#define RED       "#FF033E" //"#D3435C"
#define DARK_BLUE "#5F7CD8"

WidgetLED led1(V1);
WidgetLED led2(V2);
WidgetLED led3(V3);
WidgetLED led4(V4);
WidgetLED led5(V5);

BLYNK_WRITE(V90)
{
  alertstatus = param.asInt();
  EEPROM.put(50, alertstatus);
}

/////////************* **********/////////
//             Mode Selection           //
/////////************* **********/////////

BLYNK_WRITE(V0) {
  switch (param.asInt())
  {
    case 1: // OFF
      //Serial.println("Item 1 selected");
      mode = 1;
      offmode();
      Blynk.virtualWrite(V100, "Mode: OFF");
      EEPROM.put(1, mode);//not used--may use if Wifi issues and blynk_connect fails
      break;
    case 2: // Manual Mode
      //Serial.println("Item 2 selected");
      mode = 2;
      manualmode();
      Blynk.virtualWrite(V100, "Mode: Manual");
      EEPROM.put(1, mode);
      //led1.setColor(BLYNK_GREEN);
      break;
    case 3: // Automatic Mode
      //Serial.println("Item 3 selected");
      mode = 3;
      automode();
      Blynk.virtualWrite(V100, "Mode: Automatic");
      EEPROM.put(1, mode);
      break;
    default:
      //Serial.println("Unknown item selected");
      mode = 999;
  }
}

/////////************* **********/////////
//             Manual Buttons           //
/////////************* **********/////////

BLYNK_WRITE(V11)
{
  manuel1 = param.asInt();
  if (mode == 2 || mode == 3)
  {
    if (manuel1 == 1)
    {
      digitalWrite(valve1, LOW);
      led1.setColor(GREEN);
      Blynk.virtualWrite(V100, "Zone 1 Turning On");
    }
    else
    {
      digitalWrite(valve1, HIGH);
      led1.setColor(RED);
      Blynk.virtualWrite(V100, "Zone 1 Turning Off");
    }
  }
  if (mode == 1)
  {
      Blynk.virtualWrite(V11, 0);
      Blynk.notify("Select Manual or Auto Mode");
  }
}

BLYNK_WRITE(V12)
{
  manuel2 = param.asInt();
  if (mode == 2 || mode == 3)
  {
    if (manuel2 == 1)
    {
      digitalWrite(valve2, LOW);
      led2.setColor(GREEN);
      Blynk.virtualWrite(V100, "Zone 2 Turning On");
    }
    else
    {
      digitalWrite(valve2, HIGH);
      led2.setColor(RED);
      Blynk.virtualWrite(V100, "Zone 2 Turning Off");
    }
  }
  if (mode == 1)
  {
      Blynk.virtualWrite(V12, 0);
      Blynk.notify("Select Manual or Auto Mode");
  }
}

BLYNK_WRITE(V13)
{
  manuel3 = param.asInt();
  if (mode == 2 || mode == 3)
  {
    if (manuel3 == 1)
    {
      digitalWrite(valve3, LOW);
      led3.setColor(GREEN);
      Blynk.virtualWrite(V100, "Zone 3 Turning On");
    }
    else
    {
      digitalWrite(valve3, HIGH);
      led3.setColor(RED);
      Blynk.virtualWrite(V100, "Zone 3 Turning Off");
    }
  }
  if (mode == 1)
  {
      Blynk.virtualWrite(V13, 0);
      Blynk.notify("Select Manual or Auto Mode");
  }
}

BLYNK_WRITE(V14)
{
  manuel4 = param.asInt();
  if (mode == 2 || mode == 3)
  {
    if (manuel4 == 1)
    {
      digitalWrite(valve4, LOW);
      led4.setColor(GREEN);
      Blynk.virtualWrite(V100, "Zone 4 Turning On");
    }
    else
    {
      digitalWrite(valve4, HIGH);
      led4.setColor(RED);
      Blynk.virtualWrite(V100, "Zone 4 Turning Off");
    }
  }
  if (mode == 1)
  {
      Blynk.virtualWrite(V14, 0);
      Blynk.notify("Select Manual or Auto Mode");
  }
}

BLYNK_WRITE(V15)
{
  manuel5 = param.asInt();
  if (mode == 2 || mode == 3)
  {
    if (manuel5 == 1)
    {
      digitalWrite(valve5, LOW);
      led5.setColor(GREEN);
      Blynk.virtualWrite(V100, "Zone 5 Turning On");
    }
    else
    {
      digitalWrite(valve5, HIGH);
      led5.setColor(RED);
      Blynk.virtualWrite(V100, "Zone 5 Turning Off");
    }
  }
  if (mode == 1)
  {
      Blynk.virtualWrite(V15, 0);
      Blynk.notify("Select Manual or Auto Mode");
  }
}

/////////************* **********/////////
//          Automatic Controls          //
/////////************* **********/////////

BLYNK_WRITE(V21) {   // Scheduler #X Time Input widget  
  TimeInputParam t(param);
  nowseconds = Time.local() % 86400;
  unsigned int startseconds = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);  
  unsigned int stopseconds = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);
  Blynk.virtualWrite(V41, startseconds);
  Blynk.virtualWrite(V51, stopseconds);
  int dayadjustment = -1;  
  if(Time.weekday() == 1){
    dayadjustment = 6; // needed for Sunday Time library is day 1 and Blynk is day 7
  }
  if(t.isWeekdaySelected((Time.weekday() + dayadjustment))){ //Time library starts week on Sunday, Blynk on Monday  
    //Schedule is ACTIVE today 
    if(nowseconds >= startseconds - 31 && nowseconds <= startseconds + 31 ){    // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(BLUE);
      Blynk.virtualWrite(V100, "Zone 1 Starting  AUTOMODE");
      Blynk.setProperty(V1, "color", BLUE);
      Blynk.virtualWrite(V11, HIGH);
      digitalWrite(valve1, LOW);
      if(alertstatus ==1) {Blynk.notify("Zone 1 Starting  AUTOMODE");}
    }                  
    if(nowseconds >= stopseconds - 31 && nowseconds <= stopseconds + 31 ){   // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(YELLOW);
      Blynk.virtualWrite(V100, "Zone 1 Stopping  AUTOMODE");
      Blynk.setProperty(V1, "color", YELLOW);
      Blynk.virtualWrite(V11, LOW);
      digitalWrite(valve1, HIGH);
      if(alertstatus == 1) {Blynk.notify("Zone 1 Stopping  AUTOMODE");}
    }               
  }
}

BLYNK_WRITE(V22) {   // Scheduler #X Time Input widget  
  TimeInputParam t(param);
  nowseconds = Time.local() % 86400;
  unsigned int startseconds = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);  
  unsigned int stopseconds = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);
  Blynk.virtualWrite(V42, startseconds);
  Blynk.virtualWrite(V52, stopseconds);
  int dayadjustment = -1;  
  if(Time.weekday() == 1){
    dayadjustment = 6; // needed for Sunday Time library is day 1 and Blynk is day 7
  }
  if(t.isWeekdaySelected((Time.weekday() + dayadjustment))){ //Time library starts week on Sunday, Blynk on Monday  
    //Schedule is ACTIVE today 
    if(nowseconds >= startseconds - 31 && nowseconds <= startseconds + 31 ){    // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(BLUE);
      Blynk.virtualWrite(V100, "Zone 2 Starting  AUTOMODE");
      Blynk.setProperty(V2, "color", BLUE);
      Blynk.virtualWrite(V12, HIGH);
      digitalWrite(valve2, LOW);
      if(alertstatus ==1) {Blynk.notify("Zone 2 Starting  AUTOMODE");}
    }                  
    if(nowseconds >= stopseconds - 31 && nowseconds <= stopseconds + 31 ){   // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(YELLOW);
      Blynk.virtualWrite(V100, "Zone 2 Stopping  AUTOMODE");
      Blynk.setProperty(V2, "color", YELLOW);
      Blynk.virtualWrite(V12, LOW);
      digitalWrite(valve2, HIGH);
      if(alertstatus == 1) {Blynk.notify("Zone 2 Stopping  AUTOMODE");}
    }               
  }
}

BLYNK_WRITE(V23) {   // Scheduler #X Time Input widget  
  TimeInputParam t(param);
  nowseconds = Time.local() % 86400;
  unsigned int startseconds = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);  
  unsigned int stopseconds = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);
  Blynk.virtualWrite(V43, startseconds);
  Blynk.virtualWrite(V53, stopseconds);
  int dayadjustment = -1;  
  if(Time.weekday() == 1){
    dayadjustment = 6; // needed for Sunday Time library is day 1 and Blynk is day 7
  }
  if(t.isWeekdaySelected((Time.weekday() + dayadjustment))){ //Time library starts week on Sunday, Blynk on Monday  
    //Schedule is ACTIVE today 
    if(nowseconds >= startseconds - 31 && nowseconds <= startseconds + 31 ){    // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(BLUE);
      Blynk.virtualWrite(V100, "Zone 3 Starting  AUTOMODE");
      Blynk.setProperty(V3, "color", BLUE);
      Blynk.virtualWrite(V13, HIGH);
      digitalWrite(valve3, LOW);
      if(alertstatus ==1) {Blynk.notify("Zone 3 Starting  AUTOMODE");}
    }                  
    if(nowseconds >= stopseconds - 31 && nowseconds <= stopseconds + 31 ){   // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(YELLOW);
      Blynk.virtualWrite(V100, "Zone 3 Stopping  AUTOMODE");
      Blynk.setProperty(V3, "color", YELLOW);
      Blynk.virtualWrite(V13, LOW);
      digitalWrite(valve3, HIGH);
      if(alertstatus == 1) {Blynk.notify("Zone 3 Stopping  AUTOMODE");}
    }               
  }
}

BLYNK_WRITE(V24) {   // Scheduler #X Time Input widget  
  TimeInputParam t(param);
  nowseconds = Time.local() % 86400;
  unsigned int startseconds = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);  
  unsigned int stopseconds = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);
  Blynk.virtualWrite(V44, startseconds);
  Blynk.virtualWrite(V54, stopseconds);
  int dayadjustment = -1;  
  if(Time.weekday() == 1){
    dayadjustment = 6; // needed for Sunday Time library is day 1 and Blynk is day 7
  }
  if(t.isWeekdaySelected((Time.weekday() + dayadjustment))){ //Time library starts week on Sunday, Blynk on Monday  
    //Schedule is ACTIVE today 
    if(nowseconds >= startseconds - 31 && nowseconds <= startseconds + 31 ){    // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(BLUE);
      Blynk.virtualWrite(V100, "Zone 4 Starting  AUTOMODE");
      Blynk.setProperty(V4, "color", BLUE);
      Blynk.virtualWrite(V14, HIGH);
      digitalWrite(valve4, LOW);
      if(alertstatus ==1) {Blynk.notify("Zone 4 Starting  AUTOMODE");}
    }                  
    if(nowseconds >= stopseconds - 31 && nowseconds <= stopseconds + 31 ){   // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(YELLOW);
      Blynk.virtualWrite(V100, "Zone 4 Stopping  AUTOMODE");
      Blynk.setProperty(V4, "color", YELLOW);
      Blynk.virtualWrite(V14, LOW);
      digitalWrite(valve4, HIGH);
      if(alertstatus == 1) {Blynk.notify("Zone 4 Stopping  AUTOMODE");}
    }               
  }
}

BLYNK_WRITE(V25) {   // Scheduler #X Time Input widget  
  TimeInputParam t(param);
  nowseconds = Time.local() % 86400;
  unsigned int startseconds = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);  
  unsigned int stopseconds = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);
  Blynk.virtualWrite(V45, startseconds);
  Blynk.virtualWrite(V55, stopseconds);
  int dayadjustment = -1;  
  if(Time.weekday() == 1){
    dayadjustment = 6; // needed for Sunday Time library is day 1 and Blynk is day 7
  }
  if(t.isWeekdaySelected((Time.weekday() + dayadjustment))){ //Time library starts week on Sunday, Blynk on Monday  
    //Schedule is ACTIVE today 
    if(nowseconds >= startseconds - 31 && nowseconds <= startseconds + 31 ){    // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(BLUE);
      Blynk.virtualWrite(V100, "Zone 5 Starting  AUTOMODE");
      Blynk.setProperty(V5, "color", BLUE);
      Blynk.virtualWrite(V15, HIGH);
      digitalWrite(valve5, LOW);
      if(alertstatus ==1) {Blynk.notify("Zone 5 Starting  AUTOMODE");}
    }                  
    if(nowseconds >= stopseconds - 31 && nowseconds <= stopseconds + 31 ){   // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(YELLOW);
      Blynk.virtualWrite(V100, "Zone 5 Stopping  AUTOMODE");
      Blynk.setProperty(V5, "color", YELLOW);
      Blynk.virtualWrite(V15, LOW);
      digitalWrite(valve5, HIGH);
      if(alertstatus == 1) {Blynk.notify("Zone 5 Stopping  AUTOMODE");}
    }               
  }
}

BLYNK_CONNECTED() {//get data stored in virtual pin V0 from server
  Blynk.syncVirtual(V0);
}

void setup() 
{
    Blynk.begin(auth);
    Time.zone(-5);   //-2.5
    timer.setInterval(60000L, activetoday);  // check every 60s if ON / OFF trigger time has been reached
    timer.setInterval(1000L, clockDisplay);  // check every second if time has been obtained from the server
    timer.setInterval(10000L, sendinfo);
    //Blynk.virtualWrite(V0, 2);
    pinMode(valve1, OUTPUT);
    pinMode(valve2, OUTPUT);
    pinMode(valve3, OUTPUT);
    pinMode(valve4, OUTPUT);
    pinMode(valve5, OUTPUT);
    pinMode(valve6, OUTPUT);
    pinMode(valve7, OUTPUT);
    //pinMode(valve8, OUTPUT);
    digitalWrite(valve1, HIGH);
    digitalWrite(valve2, HIGH);
    digitalWrite(valve3, HIGH);
    digitalWrite(valve4, HIGH);
    digitalWrite(valve5, HIGH);
    digitalWrite(valve6, HIGH);
    digitalWrite(valve7, HIGH);
    //digitalWrite(valve8, HIGH);
    firstrun();

}

void loop()
{
  Blynk.run();
  timer.run();
  
}

void firstrun()
{
    EEPROM.get(50, alertstatus);
    Blynk.virtualWrite(V90, alertstatus);
}

void sendinfo()
{
    Blynk.virtualWrite(V100, Time.format("%r - %a %D"));
    Blynk.virtualWrite(V101, nowseconds);
    Blynk.virtualWrite(V102, mode);
    Blynk.virtualWrite(V103, alertstatus); //may delete
    WiFiSignal sig = WiFi.RSSI();
    float strength = sig.getStrength();
    Blynk.virtualWrite(V105, strength);
    
    
    valve1state = digitalRead(valve1);
    valve1state = map(valve1state, 0, 1, 1, 0);//to deal with backwards relay
    Blynk.virtualWrite(V31, valve1state);
    
    valve2state = digitalRead(valve2);
    valve2state = map(valve2state, 0, 1, 1, 0);//to deal with backwards relay
    Blynk.virtualWrite(V32, valve2state);
    
    valve3state = digitalRead(valve3);
    valve3state = map(valve3state, 0, 1, 1, 0);//to deal with backwards relay
    Blynk.virtualWrite(V33, valve3state);
    
    valve4state = digitalRead(valve4);
    valve4state = map(valve4state, 0, 1, 1, 0);//to deal with backwards relay
    Blynk.virtualWrite(V34, valve4state);
    
    valve5state = digitalRead(valve5);
    valve5state = map(valve5state, 0, 1, 1, 0);//to deal with backwards relay
    Blynk.virtualWrite(V35, valve5state);
}

void offmode()
{
    led1.off();
    led2.off();
    led3.off();
    led4.off();
    led5.off();
    
    Blynk.virtualWrite(V11, LOW);
    Blynk.virtualWrite(V12, LOW);
    Blynk.virtualWrite(V13, LOW);
    Blynk.virtualWrite(V14, LOW);
    Blynk.virtualWrite(V15, LOW);
    
    digitalWrite(valve1, HIGH);
    digitalWrite(valve2, HIGH);
    digitalWrite(valve3, HIGH);
    digitalWrite(valve4, HIGH);
    digitalWrite(valve5, HIGH);
}
void manualmode()
{
    led1.on();
    led2.on();
    led3.on();
    led4.on();
    led5.on();
    led1.setColor(RED);
    led2.setColor(RED);
    led3.setColor(RED);
    led4.setColor(RED);
    led5.setColor(RED);
    
    Blynk.virtualWrite(V11, LOW);
    Blynk.virtualWrite(V12, LOW);
    Blynk.virtualWrite(V13, LOW);
    Blynk.virtualWrite(V14, LOW);
    Blynk.virtualWrite(V15, LOW);
    
    digitalWrite(valve1, HIGH);
    digitalWrite(valve2, HIGH);
    digitalWrite(valve3, HIGH);
    digitalWrite(valve4, HIGH);
    digitalWrite(valve5, HIGH);
}

void automode()
{
    led1.on();
    led2.on();
    led3.on();
    led4.on();
    led5.on();
    led1.setColor(YELLOW);
    led2.setColor(YELLOW);
    led3.setColor(YELLOW);
    led4.setColor(YELLOW);
    led5.setColor(YELLOW);
    
    Blynk.virtualWrite(V11, LOW);
    Blynk.virtualWrite(V12, LOW);
    Blynk.virtualWrite(V13, LOW);
    Blynk.virtualWrite(V14, LOW);
    Blynk.virtualWrite(V15, LOW);
    
    digitalWrite(valve1, HIGH);
    digitalWrite(valve2, HIGH);
    digitalWrite(valve3, HIGH);
    digitalWrite(valve4, HIGH);
    digitalWrite(valve5, HIGH);
}


void activetoday(){         // check if schedule #1 should run today
  if(Time.year() != 1970){
    Blynk.syncVirtual(V21);  // sync scheduler #1
    Blynk.syncVirtual(V22);
    Blynk.syncVirtual(V23);
    Blynk.syncVirtual(V24);
    Blynk.syncVirtual(V25);
  }
}

void clockDisplay(){  // only needs to be done once after time sync
  if((Time.year() != 1970) && (clockSync == false)){ 
    sprintf(currentTime, "%02d:%02d:%02d", Time.hour(), Time.minute(), Time.second());
    Serial.println(currentTime);
    clockSync = true;
  } 
} 
