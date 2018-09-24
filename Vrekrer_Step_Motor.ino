/*
  Arduino SCPI step motor controler

  Diego Gonzalez Chávez (diego.gonzalez.chavez@gmail.com)
*/

#include "Arduino.h";
#include "Vrekrer_scpi_parser.h"

SCPI_Parser my_instrument;

//Output Pins (connections to step motor hardware controler)
#define pin_motor_enable 11 //motor state
#define pin_pulse 13         //pulse to step
#define pin_dir 12          //movement direction

//Motor State
const boolean c_Motor_Locked = LOW;
const boolean c_Motor_Free = HIGH;
boolean motor_status;

//Motor Movement
const boolean c_Dir_CW = LOW;    //Clock Wise
const boolean c_Dir_CCW = HIGH;  //Counter Clock Wise
long queued_steps;
long step_half_period;


void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(pin_motor_enable, OUTPUT);
  pinMode(pin_pulse, OUTPUT);
  pinMode(pin_dir, OUTPUT);

  my_instrument.RegisterCommand(F("*IDN?"), &Identify);
  my_instrument.SetCommandTreeBase(F("MCONtrol")); //Motor CONtrol
  my_instrument.RegisterCommand(F(":STATe"), &SetMotorState);
  my_instrument.RegisterCommand(F(":STATe?"), &GetMotorState);
  my_instrument.RegisterCommand(F(":MOVE"), &Move);
  my_instrument.RegisterCommand(F(":MOVE:STEPs"), &Move);
  my_instrument.RegisterCommand(F(":MOVE:STATe?"), &GetMotorMovementState);
  my_instrument.RegisterCommand(F(":SHPEriod"), &SetMotorSpeed); //Step half period
  my_instrument.RegisterCommand(F(":SHPEriod?"), &GetMotorSpeed);
  my_instrument.SetCommandTreeBase("");
  my_instrument.RegisterCommand(F("MOVE"), &Move);
  my_instrument.RegisterCommand(F("MOVE:STATe?"), &GetMotorMovementState);


  motor_status = c_Motor_Free;
  queued_steps = 0;
  step_half_period = 50; //50 micro seconds;
}

void loop() {
  my_instrument.ProcessInput(Serial, "\n");
  do_steps();
}

void do_steps() {
  if (motor_status == c_Motor_Locked) {
    unsigned long ini_time = millis();
    unsigned long current_time = millis();
    //Run continuously for 100 ms if there are queued_steps
    while ( (current_time - ini_time <= 100) && (queued_steps != 0) ) {
      current_time = millis();
      if (queued_steps > 0) {
        queued_steps--;
        digitalWrite(pin_dir, c_Dir_CW);
      } else {
        queued_steps++;
        digitalWrite(pin_dir, c_Dir_CCW);
      }
      digitalWrite(pin_pulse, HIGH);
      delay_us(step_half_period);
      digitalWrite(pin_pulse, LOW);
      delay_us(step_half_period);
    }
  } else {
    queued_steps = 0;
  }
}

void delay_us(long delay_time) {
  if (delay_time > 16000) {
    delay(delay_time / 1000);
  } else {
    delayMicroseconds(delay_time);
  }
}


/* SCPI FUNCTIONS */

void Identify(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  String IDN = F("PUC_Rio,StepPL,SN00,V.0.1\n");
  interface.print(IDN);
}

void SetMotorState(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  char* motor_strs[] = {"1", "ON", "TRUE",
                        "0", "OFF", "FALSE"
                       };
  bool motor_values[] = {c_Motor_Locked, c_Motor_Locked, c_Motor_Locked,
                         c_Motor_Free, c_Motor_Free, c_Motor_Free
                        };
  for (uint8_t i = 0; i < 6; i++) {
    int c = strcmp(parameters.First(), motor_strs[i]);
    if (c == 0) {
      motor_status = motor_values[i];
      digitalWrite(pin_motor_enable, motor_status);
    }
  }
}

void GetMotorState(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (motor_status == c_Motor_Locked) interface.print("ON\n");
  if (motor_status == c_Motor_Free) interface.print("OFF\n");
}

void Move(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (motor_status == c_Motor_Locked) {
    queued_steps = String(parameters[0]).toInt();
  }
}

void GetMotorMovementState(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (queued_steps != 0) {
    interface.print("Moving\n");
  } else {
    interface.print("Idle\n");
  }
}

void SetMotorSpeed(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  step_half_period = String(parameters[0]).toInt();
}

void GetMotorSpeed(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  interface.print(step_half_period);
  interface.print("\n");
}



