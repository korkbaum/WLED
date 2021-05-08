#pragma once

#include "wled.h"

#ifdef ESP32		//won't run on ESP8266

#define PWR_LED_PIN 17     //Hardware pin to attach mosfet gate for power led control 
#define threshold 60       //KK: MagicReel: 12,  CubeBall: 9 
#define touchPin T3

//this is to run analogwrite style pwm dimming on esp32
#define LEDC_CHANNEL_0     0
#define LEDC_TIMER_13_BIT  13
#define LEDC_BASE_FREQ     5000

void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);   // calculate duty, 8191 from 2 ^ 13 - 1
  ledcWrite(channel, duty);
}


class usermod_powerled : public Usermod {
	
  private:
	unsigned long lastTime = 0;         //Interval
    unsigned long lastTouch = 0;        //Timestamp of last Touch
    unsigned long lastRelease = 0;      //Timestamp of last Touch release
    boolean released = true;            //current Touch state (touched/released)
    uint16_t touchReading = 0;          //sensor reading
    uint16_t touchDuration = 0;         //duration of last touch
    uint16_t touchreading_old = 0;      //to compare last value and current value
	
  public:

    // some variables for Power Led control
    bool PWRon = 0;
    bool PWRonLast = 1;
    uint8_t PWRbri = 32;
    uint8_t PWRbriLast = 0;

    void setup() {
      lastTouch = millis();
      lastRelease = millis();
      lastTime = millis();
		
  		pinMode(PWR_LED_PIN, OUTPUT);    //mosfet gate pin for power led pwm control
  
  		//for pwm dimming pwr led, no "analogwrite" function on esp32 like on ESP8266
  		ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  		ledcAttachPin(PWR_LED_PIN, LEDC_CHANNEL_0);			
    }

    void connected() {
    }

    void loop() {
		
        if (millis() - lastTime >= 50) {                           //Check every 50ms if a touch occurs
          lastTime = millis();
          touchReading = touchRead(touchPin);                      //Read touch sensor 
          //Serial.println(touchReading);

          if(touchReading < threshold && released) {              //touch started
            released = false;
            lastTouch = millis();
          }
          else if (touchReading >= threshold && !released) {      //Touch released
            released = true;
            lastRelease = millis();
            touchDuration = lastRelease - lastTouch;               //Calculate duration
          }
  
          if(touchDuration >= 500 && released) {                   //800 Toggle power if button press is longer than 800ms
            touchDuration = 0;                                     //Reset touch duration to avoid multiple actions on same touch
            toggleOnOff();                                    
            Serial.println("toggle");
          } 
          else if (touchDuration >= 100 && released) {              //100 Switch to next brightness if touch is between 100 and 800ms
            touchDuration = 0;                                     //Reset touch duration to avoid multiple actions on same touch
            PWRon = !PWRon;
            PWRtoggleOnOff();
            Serial.println("PWRtoggle");
          }
        }
		
    		if (PWRbri != PWRbriLast) {                             //change pwr led brightness if it was changed in GUI
          if (!PWRon) PWRon = 1;
    			ledcAnalogWrite(LEDC_CHANNEL_0, PWRbri);
    			PWRbriLast = PWRbri;
          updateInterfaces(NOTIFIER_CALL_MODE_DIRECT_CHANGE);
    		}
    
    		if (PWRon != PWRonLast) {                               //change pwr on/off state if it was changed in GUI
          PWRtoggleOnOff();
    		}		

    } // loop end


  	void PWRtoggleOnOff() { 
      if (PWRon) {
        PWRbri = PWRbriLast;
        ledcAnalogWrite(LEDC_CHANNEL_0, PWRbri);
      }
      else {
        ledcAnalogWrite(LEDC_CHANNEL_0, 0);
      }
      PWRonLast = PWRon;
      updateInterfaces(NOTIFIER_CALL_MODE_DIRECT_CHANGE);
  		//Serial.print("PWRtoggleOnOff on - bri - last: ");Serial.print(PWRon);Serial.print(" ");Serial.print(PWRbri);Serial.print(" ");Serial.println(PWRbriLast);
  	}

    void addToJsonState(JsonObject& root){
  		root["PWRon"] = PWRon;
  		root["PWRbri"] = PWRbri;
    }

    void readFromJsonState(JsonObject& root){
  		PWRbri = root["PWRbri"] | PWRbri;
      userVar0 = PWRbri;
  		PWRon = root["PWRon"] | PWRon;
      userVar1 = PWRon;
    }

    void addToConfig(JsonObject& root){
  		JsonObject top = root.createNestedObject("usermod_powerled");
      top["PWRbri"] = PWRbri;
      userVar0 = PWRbri;
  		top["PWRon"] = PWRon;	
      userVar1 = PWRon;
    }

    void readFromConfig(JsonObject& root){
  		JsonObject top = root["usermod_powerled"];
      PWRbri = top["PWRbri"] | userVar0;
  		PWRon  = top["PWRon"] | userVar1;
    }

    void addToJsonInfo(JsonObject& root){                 // I missed an easy access to the device IP at times
      char ip[32];
      IPAddress localIP = Network.localIP();
      sprintf(ip, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
      
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");

      JsonArray IPArr = user.createNestedArray("IP");     //name
      IPArr.add(ip);                                      //value
    }
    
    uint16_t getId(){
      return USERMOD_ID_POWERLED;
    }
};
#endif
