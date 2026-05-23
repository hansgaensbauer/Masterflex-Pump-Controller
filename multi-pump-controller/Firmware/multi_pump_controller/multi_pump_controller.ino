#define CMDSIZE 30

#define PUMP_OFF_TIME_MAX 600000UL
#define PUMP_DEFAULT_SPEED 75

//Mask for handling all pumps
#define EVERY_PUMP 0b01111111 
#define PUMP(i) (1 << (i-1))

#define START_STOP_DDR DDRA
#define START_STOP_PORT PORTA
#define START_STOP_OFFSET 0

#define DIR_DDR DDRC
#define DIR_PORT PORTC
#define DIR_OFFSET 1

#define PRIME_DDR DDRJ
#define PRIME_PORT PORTJ
#define PRIME_OFFSET 0

#define ALARM_DDR DDRF
#define ALARM_PORT PORTF
#define ALARM_PIN PINF
#define ALARM_OFFSET 0

#define START_STOP_LED_1 (1 << 7)

//arduino PWM pins for analogWrite()
const uint8_t vctrl[8] = {46, 45, 44, 6, 7, 8, 9};

//Buffer for holding input commands
char cbuff[CMDSIZE];
uint8_t clen;

int speed[7] = {PUMP_DEFAULT_SPEED, PUMP_DEFAULT_SPEED, 
  PUMP_DEFAULT_SPEED, PUMP_DEFAULT_SPEED, PUMP_DEFAULT_SPEED, 
  PUMP_DEFAULT_SPEED, PUMP_DEFAULT_SPEED};

bool pump_off_flag = false;
unsigned long pump_off_time;

void setup() {
  //Clamp motor
  DDRC = 0x0F;

  //VCTRL pins as outputs, pump off
  for(uint8_t i = 0; i < 8; i++){
    pinMode(vctrl[i], OUTPUT);
    analogWrite(vctrl[i], tospeed(PUMP_DEFAULT_SPEED)); //zero speed to start
  }

  //Drive all pump inputs low, toggle them through the DDR
  START_STOP_PORT &= ~(EVERY_PUMP << START_STOP_OFFSET);
  START_STOP_DDR |= (EVERY_PUMP << START_STOP_OFFSET); //All outputs (on)

  DIR_DDR &= ~(EVERY_PUMP << DIR_OFFSET);
  DIR_PORT &= ~(EVERY_PUMP << DIR_OFFSET);

  PRIME_DDR &= ~(EVERY_PUMP << PRIME_OFFSET);
  PRIME_PORT &= ~(EVERY_PUMP << PRIME_OFFSET);

  ALARM_DDR &= ~(EVERY_PUMP << ALARM_OFFSET);
  ALARM_PORT |= (EVERY_PUMP << ALARM_OFFSET); //INPUT PULLUP

  
  Serial.begin(115200);
  
  delay(10);

}

void loop() {
  if(Serial.available() > 0){
    parsecmd();
  }

//Safety turn-on timer for MRR
#ifdef MRR
  if(pump_off_flag && (millis() - pump_off_time > PUMP_OFF_TIME_MAX)){
    Serial.println("Force Restarting Pump");
    Serial.println("Starting Motor");
    START_STOP_DDR |= (PUMP(1) << START_STOP_OFFSET); //Start pump
    pump_off_flag = false;
  }

#endif

}

//Parse an incoming command
void parsecmd() {
  delay(50);
  int i = 0;
  clen = 0;
  while(Serial.available() && i < CMDSIZE){
    cbuff[i] = Serial.read();
    if(cbuff[i] == ' '){
      clen = i + 1;
      break;
    }
    i++;
  }
  clen = i;
  
  //parse the command
  if(command_is("setdir")){
    int pump = Serial.parseInt(SKIP_NONE, 50); //TODO: What's going on here??
    int direction = Serial.parseInt(SKIP_NONE, 50);
    if(direction == 0) {
      DIR_DDR |= (PUMP(pump) << DIR_OFFSET);
      Serial.println("Setting Direction to CCW");
    }else {
      DIR_DDR &= ~(PUMP(pump) << DIR_OFFSET);
      Serial.println("Setting Direction to CW");
    }
  }

  else if(command_is("setspeed")){
    int pump = Serial.parseInt(SKIP_NONE, 50); //TODO: What's going on here??
    speed[pump-1] = Serial.parseInt(SKIP_NONE, 50);
    analogWrite(vctrl[pump - 1], tospeed(speed[pump-1]));
    Serial.print("Setting Speed to ");
    Serial.println(speed[pump - 1]);
  }

  else if(command_is("start")){
    int pump = Serial.parseInt(SKIP_NONE, 50); //TODO: What's going on here??
    Serial.println("Starting Motor");
    START_STOP_DDR |= (PUMP(pump) << START_STOP_OFFSET); //Start pump
    pump_off_flag = false;
  }

  else if(command_is("stop")){
    int pump = Serial.parseInt(SKIP_NONE, 50); //TODO: What's going on here??
    Serial.println("Stopping Motor");
    START_STOP_DDR &= ~(PUMP(pump) << START_STOP_OFFSET); //Start pump

    //Start a timer to prevent 
    pump_off_flag = true;
    pump_off_time = millis();
  }

  else if(command_is("getspeed")){
    int pump = Serial.parseInt(SKIP_NONE, 50); //TODO: What's going on here??
    Serial.println(speed[pump - 1]);
  }

  else if(command_is("getalarm")){
    int pump = Serial.parseInt(SKIP_NONE, 50); //TODO: What's going on here??
    bool alm = ALARM_PIN & (PUMP(pump) << ALARM_OFFSET);
    if(alm){
      Serial.println("Safe");
    }else{
      Serial.println("Alarm!");
    }
  }
  cbuff[0] = 0;
}

void printcmd(){
  Serial.println(clen);
  for(int i = 0; i < clen; i++){
    if(cbuff[i] != 0) Serial.print(cbuff[i]);
  }
  Serial.println();
}

bool command_is(String cmd){
  if (clen == 0) return false;
  for(int i = 0; i < clen; i++){
    if(cmd.charAt(i) != cbuff[i]) return false;
  }
  return true;
}

void empty(){
    while(Serial.available()) Serial.read();
}

int tospeed(int percent){
  return (int) map(percent,0,100,0,255);
}

