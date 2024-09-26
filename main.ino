/*

Adapted from the top work here:

https://github.com/wrybread/ArduinoWeatherStation

Works with a Davis anemometer.

See 

Simple program to display values on serial monitor

Testet on Arduino Pro Mini (5V)

Wireing 
        - - - yellow to VCC (3.3 V)
        - - - green to pin A0
        - - - red to GND
        - - - black to pin 2

*/


#include "TimerOne.h" // Timer Interrupt set to 2 second for read sensors 

#define WindSensorPin 2 // The pin location of the anemometer sensor 
#define WindVanePin A0 // The pin the wind vane sensor is connected to 
#define VaneOffset 0; // define the anemometer offset from magnetic north 


int vaneValue; // raw analog value from wind vane 
int windDirection; // translated 0 - 360 direction 
int windCalDirection; // converted value with offset applied 
String windCompassDirection; // wind direction as compass points
String dirTable[]= {"N","NNO","NO","ONO","O","OSO","SO","SSO","S","SSW","SW","WSW","W","WNW","NW","NNW"}; // wind direction compass names
int lastWindDirectionValue; // last direction value 

volatile bool IsSampleRequired; // this is set true every 2.5s. Get wind speed 
volatile unsigned int TimerCount; // used to determine 2.5sec timer count 
volatile unsigned long Rotations; // cup rotation counter used in interrupt routine 
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in isr 

float WindSpeed; // speed miles per hour 


void setup() { 

  lastWindDirectionValue = 0; 
  
  IsSampleRequired = false; 
  
  TimerCount = 0; 
  Rotations = 0; // Set Rotations to 0 ready for calculations 
  
  Serial.begin(9600); 
  
  pinMode(WindSensorPin, INPUT_PULLUP); // with pullup no external pullup needed

  //@@ Ooops, typo in this line from the original script
  //attachInterrupt(digitalPinToInterrupt(WindSensorPin), rotation, FALLING); 
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); 
  
  Serial.println("Arduino Weather Station"); 
  
  // Setup the timer interupt 
  Timer1.initialize(500000);// Timer interrupt every 2.5 seconds 
  Timer1.attachInterrupt(isr_timer); 

} 

void loop() { 

  getWindDirection(); 
  
  // Only update the display if change greater than 5 degrees. 
  if(abs(windCalDirection - lastWindDirectionValue) > 5) { 
    lastWindDirectionValue = windCalDirection; 
  } 

  if(IsSampleRequired) { 
    // convert to mp/h using the formula V=P(2.25/T) 
    // V = P(2.25/2.5) = P * 0.9 
    WindSpeed = Rotations * 0.9; 
    Rotations = 0; // Reset count for next sample 
    
    IsSampleRequired = false; 

    // output the data in a way that's easy to read by a Raspberry Pi or whatever
    // will output for example: wind_speed=5.47,wind_direction=343,wind_compass_direction=N,temperature=0,barometer=0,
    Serial.print("wind_speed_km/h             = "); Serial.println(getKmh(WindSpeed));
    Serial.print("wind_vane_value             = "); Serial.println(vaneValue);
    Serial.print("wind_direction              = "); Serial.println(windDirection);
    Serial.print("wind_direction_with_offset  = "); Serial.println(windCalDirection);
    Serial.print("wind_compass_direction      = "); Serial.println(windCompassDirection);  
    Serial.println();
  
  } 
} 


// isr handler for timer interrupt 
void isr_timer() { 

  TimerCount++; 
  
  if(TimerCount == 6) { 
    IsSampleRequired = true; 
    TimerCount = 0; 
  } 
} 


// This is the function that the interrupt calls to increment the rotation count 
void isr_rotation() { 

  if((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact. 
    Rotations++; 
    ContactBounceTime = millis(); 
  } 

} 


// Get Wind Direction 
void getWindDirection() { 

  vaneValue = analogRead(WindVanePin); 
  windDirection = map(vaneValue, 0, 1023, 0, 360); 
  windCalDirection = windDirection + VaneOffset; 

  windCompassDirection = dirTable[int(windCalDirection/22.5)];

} 

// Convert MPH to Knots 
float getKnots(float speed) { 
  return speed * 0.868976; 
} 

float getKmh(float speed) { 
  return speed * 1.609; 
} 