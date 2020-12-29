/////////************* **********/////////
//          Blynk Assignments           //
/////////************* **********/////////
/*
V0   - Mode Switch
V10  = Terminal
https://github.com/ControlEverythingCom/NCD16Relay/blob/master/firmware/NCD16Relay.cpp
>> https://github.com/ControlEverythingCom/NCD16Relay/blob/master/README.md
relayController.turnOffAllRelays();
relayController.turnOnRelay(relayNumber);
relayController.toggleRelay(i);
*/

#include <blynk.h>
char auth[] = "zh7lIH_MEeMqn_drxNhOhut37IxDcYrk";
WidgetTerminal terminal(V10);
BlynkTimer timer;
enum  MODE { off = 1, manual = 2, automatic = 3, unknown = 999 };
MODE         mode        = unknown;
void         setMode(MODE m);

#include <NCD16Relay.h>
NCD16Relay R1;

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
  //sendInfoaftercommand();
  setMode(mode);  
  //if (mode != unknown) 
    //EEPROM.put(1, mode);                              //not used--may use if Wifi issues and blynk_connect fails
}

BLYNK_WRITE(V1) {
  long startTimeInSecs = param[0].asLong();
  terminal.println(startTimeInSecs);
  //Serial.println();
}

void setup() {
    Time.zone(-4);
    Blynk.begin(auth);
    timer.setInterval(1000L, startcycle);
    
    R1.setAddress(0, 0, 0);
    if(R1.initialized){
        terminal.println("Controller is ready");
    }else{
        terminal.println("Controller not ready");
    }
    R1.turnOffAllRelays();
}

void loop() {
    Blynk.run();
    timer.run();
}

void startcycle()
{
    R1.turnOnAllRelays(1);
    R1.turnOnRelay(15);
    int hey = R1.readRelayStatus(1);
    /*for(int i = 1; i < 17; i++){
        delay(50);
        relayController.toggleRelay(i);
    }*/
    terminal.print(Time.format("%D %r - "));
    terminal.println(hey);
    terminal.flush();
}

void setMode(MODE m) {
  switch (m) {
    case off:
    terminal.println("inmodeOFF");
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
