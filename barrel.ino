
#define ff2 9
#define ff1 10
#define pwm 11
#define dir 7
#define pos A0

int position = 0;
int angle;
const int MIN_READING = 55;
const int MAX_READING = 945;
const double TOTAL_LENGTH = 3.93 * 25.4;	// in millimeter
const double LENGTH_PER_READING = 0.108;	// in millimeter
const double ANGLE_INTERVAL = 68.0;

void setup() {
  pinMode(ff2, INPUT);      
  pinMode(ff1, INPUT);
  pinMode(pwm, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(pos, INPUT);
  Serial.begin(9600);
  srand(998298182);		//some number
  
  digitalWrite(dir, 1);
  analogWrite(pwm, 255);
  while(analogRead(pos) <= MAX_READING);
  
  analogWrite(pwm, 0);
  digitalWrite(dir, position);
}

void loop(){
  if(digitalRead(ff1)){ Serial.println("FF1 Error detected"); }
  if(digitalRead(ff2)){ Serial.println("FF2 Error detected"); }

  angle = rand() % 70 + 10;
  Serial.println(angle);
  delay(2000);
  barrelOperation(getRequiredReading(10));
  delay(15000);
}

int barrelOperation(int requiredReading){
  int reading = analogRead(pos);
  while(abs(reading - requiredReading) > 1){
    position = (reading - requiredReading) < 0 ? 1 : 0;
    digitalWrite(dir, position);
    analogWrite(pwm, 100);
    reading = analogRead(pos);
  }
  analogWrite(pwm, 0);
}

int getRequiredReading(int angle){
  if(angle > 80){ return MIN_READING; }
  if(angle < 10){ return MAX_READING; }
  double length = ((angle - 10) / ANGLE_INTERVAL) * TOTAL_LENGTH;
  return (int) MAX_READING - (length / LENGTH_PER_READING);
}

