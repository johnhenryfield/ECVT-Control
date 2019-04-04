// ecvt_lag_compensator.ino
// this sketch runs a lag compensator on the ecvt
//
// author: Tyler McCown (tylermccown@engineering.ucla.edu)
// created: 4/2/19

#include <Servo.h>

// buttons
#define DEBOUNCE_DELAY 50
const int button1_pin = 13;
int button1_state = HIGH;
int button1_last_state = HIGH;
int button1_last_debounce(0);
const int button2_pin = 12;
int button2_state = HIGH;
int button2_last_state = HIGH;
int button2_last_debounce(0);
const int button3_pin = 11;
int button3_state = HIGH;
int button3_last_state = HIGH;
int button3_last_debounce(0);
const int button4_pin = 10;
int button4_state = HIGH;
int button4_last_state = HIGH;
int button4_last_debounce(0);

// PWM constants
#define PW_STOP 1510
#define PW_MIN 1000
#define PW_MAX 2000
const int u_k_min = PW_MIN - PW_STOP;
const int u_k_max = PW_MAX - PW_STOP;

// reference signals
#define MAX_TORQUE 2700
#define MAX_POWER 3400

// actuator
Servo Actuator;
const int actuator_pin = 9;
#define POT_MARGIN 10
#define POT_MIN 146 + POT_MARGIN
#define POT_MAX 762 - POT_MARGIN
const int pot_pin = A5;
int current_pos(0);
int u_k(0);
int u_k1(0);
int e_k(0);
int e_k1(0);
int r_k = MAX_TORQUE;
const int controlPeriod = 20; // [ms]
const double Ts = controlPeriod/1000; // controlPeriod [s]
const double K = .367; // controller gain
const double z_1 = .346; // lag compensator zero
const double p_1 = .116; // lag compensator pole
const double A = K*(z_1+2/Ts)/(p_1+2/Ts);
const double B = K*(z_1-2/Ts)/(p_1+2/Ts);
const double C = (p_1-2/Ts)/(p_1+2/Ts);
int lastControlTime(0);

// hall effect sensor
#define NUM_MAGNETS 1
const int sensor_pin = 3;
int trigger_time(0);
int last_trigger(0);
int rpm(0);

void setup() {
  // setup buttons
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(button3_pin, INPUT);
  pinMode(button4_pin, INPUT);

  // setup actuator
  Actuator.attach(actuator_pin);
  Actuator.writeMicroseconds(PW_STOP);
  pinMode(pot_pin, INPUT);
  current_pos = analogRead(pot_pin);

  // create hall effect interrupt
  pinMode(sensor_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensor_pin), hall_effect_interrupt, FALLING);
  interrupts();
  trigger_time = millis();
  last_trigger = trigger_time;

  // create timer interrupt
  OCR0A = 0xFF;
  TIMSK0 |= _BV(OCIE0A);
}

void hall_effect_interrupt() {
  last_trigger = trigger_time;
  trigger_time = millis();
}

SIGNAL(TIMER0_COMPA_vect) {
  int current_millis = millis();
  if (current_millis - lastControlTime >= controlPeriod) {
    control_function();
    lastControlTime = current_millis;

//    Serial.print(u_k);
//    Serial.print(" ");
//    Serial.print(rpm);
//    Serial.print(" ");
//    Serial.print(current_pos);
//    Serial.print(" ");
//    Serial.print(millis());
//    Serial.print("\n");
  }
}

void control_function() {

  // compute current error
  e_k = r_k - rpm;

  // compute control signal and write to actuator
  u_k = A*e_k + B*e_k1 - C*e_k1;
  int u_k_final = max(min(u_k, u_k_max), u_k_min);
  Actuator.writeMicroseconds(u_k_final + PW_STOP);

  // update past values
  u_k1 = u_k;
  e_k1 = e_k;
}

void loop() {
  // check actuator limits
  current_pos = analogRead(pot_pin);
  if ((current_pos >= POT_MAX && u_k > 0) || (current_pos <= POT_MIN && u_k < 0)) {
    u_k = 0;
  }

  // calculate rpm
  detachInterrupt(digitalPinToInterrupt(sensor_pin));
  int new_rpm = 60000.0 / NUM_MAGNETS / (trigger_time - last_trigger);
  if (new_rpm != 0) {
    rpm = new_rpm;
  }
  attachInterrupt(digitalPinToInterrupt(sensor_pin), hall_effect_interrupt, FALLING);

  // check button presses
  button1_state = debounce_button(digitalRead(button1_pin), button1_state, button1_last_state, button1_last_debounce); 
  button2_state = debounce_button(digitalRead(button2_pin), button2_state, button2_last_state, button2_last_debounce);
  button3_state = debounce_button(digitalRead(button3_pin), button3_state, button3_last_state, button3_last_debounce);
  button4_state = debounce_button(digitalRead(button4_pin), button4_state, button4_last_state, button4_last_debounce);
  
  // update reference value
  if (button1_state == LOW) {
    r_k = MAX_TORQUE;
  }
  if (button2_state == LOW) {
    r_k = MAX_POWER;
  }
  if (button3_state == LOW) {
    r_k = 5000;
  }
}

bool debounce_button(const int reading, int &button_state, int &last_button_state , int &last_debounce) {
  if (reading != last_button_state) {
    last_debounce = millis();
  }

  if ((millis() - last_debounce) > DEBOUNCE_DELAY) {

    if (reading != button_state) {
      button_state = reading;
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  last_button_state = reading;
}