
// Angle control Related
#define aFF2 14
#define aFF1 15
#define aPWM 12
#define aDIR 16
#define aPOS A0

// Pressure control Related
#define pRelay 17

// Sensor
#define deadmanSwi 18
#define angleSen A1
#define pressureSen A2
#define proxSen 11

#define AUP  20	// Angle up pin. Left joystick
#define ADW  21	// Angle down pin. Left joystick

int angle;
int safe;
int aDirection;

// Actuator
const int MIN_ANGLE = 10;
const int MAX_ANGLE = 80;
const int MIN_ANGLE_READING = 60;
const int MAX_ANGLE_READING = 940;
const double TOTAL_LENGTH = 3.93 * 25.4;  // in millimeter
const double LENGTH_PER_READING = TOTAL_LENGTH / (MAX_ANGLE_READING - MIN_ANGLE_READING);
const double ANGLE_INTERVAL = MAX_ANGLE - MIN_ANGLE;

long debouncing_time = 100;
volatile unsigned long last_micros;


void setup(){
  pinMode(aFF2, INPUT);      
  pinMode(aFF1, INPUT);
  pinMode(aPWM, OUTPUT);
  pinMode(aDIR, OUTPUT);
  pinMode(aPOS, INPUT);
  pinMode(pRelay, OUTPUT);
  pinMode(deadmanSwi, INPUT);
  pinMode(angleSen, INPUT);
  pinMode(pressureSen, INPUT);
  pinMode(proxSen, INPUT);

  Serial.begin(9600);
  aDirection = 0;
   
  pinMode(AUP, INPUT_PULLUP);			
	pinMode(ADW, INPUT_PULLUP);
   
  // Angle initialization
  digitalWrite(aDIR, 1);
  analogWrite(aPWM, 255);
  while(analogRead(aPOS) <= MAX_ANGLE_READING);
  analogWrite(aPWM, 0);
  digitalWrite(aDIR, aDirection);
  angle = MIN_ANGLE;
  
  // Pressure initialization
  //while(!analogRead(deadmanSwi));
  //pressureOperation(25);
  
   attachInterrupt(digitalPinToInterrupt(AUP), aUpDebounce, RISING);
   attachInterrupt(digitalPinToInterrupt(ADW), aDwDebounce, RISING);
  
  // Send ready signal
}


void loop(){
  // Check pressure/ proximity sensors/ deadman switch
  safe = true;
  digitalWrite(pRelay, aDirection);

//  int psi = readingToPsi(analogRead(pressureSen));
//  safe &= (psi <= 50);
//  Serial.print(psi);
//  if(!safe){
//    //Blow off 
//  }
//  
//  safe &= (analogRead(proxSen) < 30);
//  safe &= (digitalRead(deadmanSwi) == 1);
  
  //Check angle & Send angle
  
}


void serialEvent(){
  char buf[8] = "";
  if(Serial.available() >= 3){
    Serial.readBytes(buf, 3); 
    Serial.println(buf);
    
    if(safe){
      switch(buf[0]){
        // Angle
        case 'a':
          if(buf[1] == 'u' && angle < MAX_ANGLE){
            angleOperation(++angle);
          } else if(buf[1] == 'd' && angle > MIN_ANGLE){
            angleOperation(--angle);
          } else {
            // Error 
          }
          break;
          
        // Pressure
        case 'p':
          int psi = readingToPsi(analogRead(pressureSen));
          if(buf[1] == 'u' && psi < 50){
            // Pressure + 1
          } else if(buf[1] == 'd' && psi > 0){
            // Pressure - 1 
          } else {
            // Error
          }
          break;
          
        //Arm/ Unarm/ Fire
      }
    }
  }  
}

void aUpDebounce(){
	if((long)(micros() - last_micros) >= debouncing_time*1000 && angle < MAX_ANGLE){
    angleOperation(++angle);
		last_micros = micros();
	}
}

void aDwDebounce(){
	if((long)(micros() - last_micros) >= debouncing_time*1000 && angle > MIN_ANGLE){
    angleOperation(--angle);
		last_micros = micros();
	}
}


void angleOperation(int angle){
  int requiredReading = angleToReading(angle);
  int reading = analogRead(aPOS);
  while(abs(reading - requiredReading) > 1){
    aDirection = (reading - requiredReading) < 0 ? 1 : 0;
    digitalWrite(aDIR, aDirection);
    analogWrite(aPWM, 100);
    reading = analogRead(aPOS);
  }
  analogWrite(aPWM, 0);
}

int angleToReading(int angle){
  if(angle <= MIN_ANGLE){ return MAX_ANGLE_READING; }
  if(angle >= MAX_ANGLE){ return MIN_ANGLE_READING; }
  double length = ((angle - MIN_ANGLE) / ANGLE_INTERVAL) * TOTAL_LENGTH;
  return (int) MAX_ANGLE_READING - (length / LENGTH_PER_READING);
}


void pressureOperation(int psi){
  // code
}

int readingToPsi(int reading){
  // code  
  return 0;
}

int psiToReading(int psi){
  // code  
  return 0;
}

