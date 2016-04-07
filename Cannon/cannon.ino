
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


int angle;
int safe;
int aDirection;
// Actuator
const int MIN_ANGLE_READING = 55;
const int MAX_ANGLE_READING = 945;
const double TOTAL_LENGTH = 3.93 * 25.4;  // in millimeter
const double LENGTH_PER_READING = 0.108;
const double ANGLE_INTERVAL = 68.0;


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
   
  // Angle initialization
  digitalWrite(aDIR, 1);
  analogWrite(aPWM, 255);
  while(analogRead(aPOS) <= MAX_ANGLE_READING);
  analogWrite(aPWM, 0);
  digitalWrite(aDIR, aDirection);
  angle = 10;
  
  // Pressure initialization
  while(!analogRead(deadmanSwi));
  pressureOperation(25);
  
  // Send ready signal
}


// Execution order: loop() -> serialEvent() -> loop() -> ...
void loop(){
  // Check pressure/ proximity sensors/ deadman switch ...
  safe = true;
  
  int psi = readingToPsi(analogRead(pressureSen));
  safe &= (psi <= 50);
  Serial.print(psi);
  if(!safe){
    //Blow off 
  }
  
  safe &= (analogRead(proxSen) < 30);
  safe &= (digitalRead(deadmanSwi) == 1);
  
  //Check angle & Send angle
}


void serialEvent(){
  char buf[8] = "";
  if(Serial.available() >= 3){
    Serial.readBytes(buf, 3); 
    
    if(safe){
      switch(buf[0]){
        // Angle
        case 'a':
          if(buf[1,2] == "up" && angle < 80){
            angle++;
            angleOperation(angle);
          } else if(buf[1,2] == "dn" && angle > 10){
            angle--;
            angleOperation(angle);
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
  double length = ((angle - 10) / ANGLE_INTERVAL) * TOTAL_LENGTH;
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

