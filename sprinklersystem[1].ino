 #include <blynk.h>
char auth[] = "sgmD0T0O3vBNUZjRcLJFQ2oHAxuclV-W"; 
BlynkTimer timer;
char currentTime[9];
bool clockSync = false;
//WidgetTerminal terminal(V30);

// Zone valves
#define valve1 2
#define valve2 3
#define valve3 4
#define valve4 5
#define valve5 6
#define valve6 7
#define valve7 8
int mode = 0;
int manuel1 = 0;
int lastmanuel1 = 3;
int auto1 = 0;
int lastauto1 = 3;
//int counts = 0;

#define GREEN     "#008000"//#23C48E"
#define BLUE      "#04C0F8"
#define YELLOW    "#ED9D00"
#define RED       "#FF033E" // "#D3435C"
#define DARK_BLUE "#5F7CD8"

WidgetLED led1(V1);
WidgetLED led2(V2);
WidgetLED led3(V3);
WidgetLED led4(V4);
WidgetLED led5(V5);

BLYNK_WRITE(V0) {
  switch (param.asInt())
  {
    case 1: // OFF
      //Serial.println("Item 1 selected");
      mode = 1;
      Blynk.virtualWrite(V100, "Mode: OFF");
      shutoffall();
      //led1.off();
      //led1.setColor(BLYNK_RED);
      break;
    case 2: // Manual Mode
      //Serial.println("Item 2 selected");
      mode = 2;
      manualmode();
      Blynk.virtualWrite(V100, "Mode: Manual");
      //led1.setColor(BLYNK_GREEN);
      break;
    case 3: // Automatic Mode
      //Serial.println("Item 3 selected");
      mode = 3;
      Blynk.virtualWrite(V100, "Mode: Automatic");
      break;
    default:
      Serial.println("Unknown item selected");
  }
}

BLYNK_WRITE(V11)
{
  manuel1 = param.asInt();
  if (mode == 2 || mode == 3)
 // digitalWrite(valve1, manuel1);
  //Blynk.virtualWrite(V100, manuel1);
  {
    if (manuel1 == 1)
    {
      digitalWrite(valve1, LOW);
      //led1.on();
      led1.setColor(GREEN);
      Blynk.virtualWrite(V100, "Zone 1 Turning On");
    }
    else
    {
      digitalWrite(valve1, HIGH);
      //led1.on();
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

BLYNK_WRITE(V21) {   // Scheduler #1 Time Input widget  
  TimeInputParam t(param);
  //unsigned int nowseconds = ((hour() * 3600) + (minute() * 60) + second());
  unsigned int nowseconds = Time.local() % 86400;
  //nowseconds = nowseconds - 9000;
  Blynk.virtualWrite(V101, nowseconds);
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
      Blynk.setProperty(V11, "oncolor", "#04C0F8");
      Blynk.virtualWrite(V11, HIGH);
     // Blynk.setProperty(V11, "oncolor", BLUE);
      digitalWrite(valve1, LOW);
      Blynk.notify("Schedule 1 started");
    }                  
    if(nowseconds >= stopseconds - 31 && nowseconds <= stopseconds + 31 ){   // 62s on 60s timer ensures 1 trigger command is sent
      //led1.setColor(YELLOW);
      Blynk.virtualWrite(V100, "Zone 1 Stopping  AUTOMODE");
      Blynk.setProperty(V1, "color", YELLOW);
      digitalWrite(valve1, HIGH);
      Blynk.notify("Schedule 1 finished");
      Blynk.virtualWrite(V11, LOW);
    }               
  }
}

/*BLYNK_WRITE(V21) {  // Called whenever setting Time Input Widget
  TimeInputParam t(param);
  int SThour = t.getStartHour();
  int STmin = t.getStartMinute();
  int STsec = t.getStartSecond();][ ]
  int SPhour = t.getStopHour();
  int SPmin = t.getStopMinute();
  int SPsec = t.getStopSecond();
  int now = millis();
  Blynk.virtualWrite(V25, now);
  terminal.println(SThour);
}*/

BLYNK_CONNECTED() {
  //get data stored in virtual pin V0 from server
  Blynk.syncVirtual(V0);
}

void setup() 
{
    Blynk.begin(auth);
    Time.zone(-5);   //-2.5
    timer.setInterval(60000L, activetoday);  // check every 60s if ON / OFF trigger time has been reached
    timer.setInterval(1000L, clockDisplay);  // check every second if time has been obtained from the server
    timer.setInterval(10000L, sendinfo);
    //Blynk.virtualWrite(V0, mode);
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

}

void loop()
{
  Blynk.run();
  timer.run();
  
}

void sendinfo()
{
    Blynk.virtualWrite(V100, Time.format("%r - %a %D"));
    int valve1state = digitalRead(valve1);
    valve1state = map(valve1state, 0, 1, 1, 0);//to deal with backwards relay
    Blynk.virtualWrite(V31, valve1state);
}

void shutoffall()
{
    led1.off();
    led2.off();
    led3.off();
    led4.off();
    led5.off();
    
    Blynk.virtualWrite(V11, LOW);
    
    digitalWrite(valve1, HIGH);
}
void manualmode()
{
    led1.on();
    led2.on();
    led3.on();
    led4.on();
    led5.on();
    led1.setColor(RED);
    led2.setColor(BLUE);
    led3.setColor(YELLOW);
    led4.setColor(DARK_BLUE);
}


void activetoday(){         // check if schedule #1 should run today
  if(Time.year() != 1970){
    Blynk.syncVirtual(V21);  // sync scheduler #1  
  }
}

void clockDisplay(){  // only needs to be done once after time sync
  if((Time.year() != 1970) && (clockSync == false)){ 
    sprintf(currentTime, "%02d:%02d:%02d", Time.hour(), Time.minute(), Time.second());
    Serial.println(currentTime);
    clockSync = true;
  } 
} 