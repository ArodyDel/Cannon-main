#include <LiquidCrystal.h>

#define RS  24	// RS pin for J204A LCD display
#define E   22	// E pin for J204A LCD display
#define D4  36	
#define D5  34	// Data pins for J204A
#define D6  32
#define D7  30
#define I0  2   //Interrupt 0 pin

int safe = 1;						// mode of the cannon for now only 1 or 0
int angle = 43;						// cannon angle
int pressure = 67;					// cannon pressure
int velocity = 11;					// calculated muzzle velocity
char* command;						// pointer to the string we will get over serial and interpreted as a command
long debouncing_time = 100;			// I think it's in milisec. If interrupt is triggering more than once increase this value
volatile unsigned long last_micros;	// Used to store the previous time an interrupt was triggered used for debouncing

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);		// Creates an lcd object so that we can use the LiquidCrystal library

void setup() 
{
	pinMode(I0, INPUT_PULLUP);										// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise. 
	attachInterrupt(digitalPinToInterrupt(I0), debounce, RISING);		// use of digitalPinToInterrupt is safer. debounce is the name of the function called when triggered.
	Serial.begin(9600);	
	lcd.begin(20, 4);													// 20, 4 is our lcd size
	displayMenu();													// Initialize the lcd to display the Menu without values
	updateMenu();														// Updates the lcd to display the current values of the cannon state
	Serial.print("Enter command\n");									// \n only does 0xA and not 0xD so we need to add it later. It might be a Windows thing.
	Serial.write(0xD);
}

void loop() 
{
	
}

// It's the debouncing function that's called during interrupt. It simply buffers up the interrupts only allowing once interrupt every debouncing_time milisec
// It calles the actual Interrupt service routine only after the debouncing.
void debounce()
{
	if((long)(micros() - last_micros) >= debouncing_time*1000)
	{
		intFunc();
		last_micros = micros();
	}
}

// The ISR (Interrupt Service Routine). Right now it simply increases the angle for testing purposes.
void intFunc()
{
	angle++;
	if(angle > 80)
		angle = 10;
	updateMenu();			//update the lcd to the new value for the angle. We could split up the updateMenu function into updateAngle, Pressure, State, etc. later if we need to.
}

// Only called when there is something transmitted to the Arduino
void serialEvent()
{
	char buf[16] = "";					// Set up a small buffer for readBytes
  
	while(Serial.available() < 3);		// Poll until we have 3 characters(1 command)

	Serial.readBytes(buf, 3);			// Read the command

	command = buf;						

	Serial.print(command);				// Echo the command for testing purposes
	Serial.write(0xA);
	Serial.write(0xD);
										// Rudamentary switch statement for hangling input from the cannon. The commands can be changed later.
	if(command[0] == 'a')				// If it is a then the command is aXX where XX is the curent angle. Example: a17 is angle of 17 deg.
	{
		angle = (command[1] - 0x30)*10 + (command[2] - 0x30);		// I don't know if this is the best way to convert the 2nd and 3rd char to a number but it works.
	}
	else if(command[0] == 'p')			// pXX is for pressure. Example: p73 is pressure of 73 PSI
	{
		pressure = (command[1] - 0x30)*10 + (command[2] - 0x30);
		velocity = pressure/4.5;		// Ignore for now it just makes a random converion so the velocity updates with the pressure.
	}  
	else if(command[0] == 'r')			//I think we decided to make SAFE and ARMED states internal, so this is unneeded. It was originally "rdy" for ARMED and "war" for SAFE.
	{
		safe = 0;
	}
	else if(command[0] == 'w')
	{
		safe = 1;
	}
	updateMenu();
}

void displayMenu()
{
	lcd.clear();
	lcd.setCursor(0, 0);			// Sets the lcd cursor to 1st line beggining
	lcd.print("Cannon state:");
	lcd.setCursor(0, 1);			// 2nd line beginning and etc.
	lcd.print("Angle:");
	lcd.setCursor(0, 2);
	lcd.print("Pressure:");
	lcd.setCursor(0, 3);
	lcd.print("Speed:");
}

// The X values for setCursor were made so the values will be outputted at the end of each row without overflowwing into the next line. It assumes only 2 decimal values.
// 7 will show up as 07
void updateMenu()
{
	lcd.setCursor(15, 0);
	lcd.print(String( (safe)?" SAFE":"ARMED"));			//Depending on the state variable choose between the 2 strings around the : character
	lcd.setCursor(17, 1);
	lcd.print(String(angle) + char(223));              	//decmal value 223 is the value for the degree sign for the J204A degree character.
	lcd.setCursor(14, 2);
	lcd.print(String(pressure/10) + String(pressure%10) +" PSI");
	lcd.setCursor(14, 3);
	lcd.print(String(velocity/10) + String(velocity%10) + " m/s");	// Just randomply put m/s. I don't know what unit we will use.
}

