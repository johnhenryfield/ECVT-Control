/* m19_controller_timing_test.ino
 * 
 * test to evaluate the clock time of the controller. trying to figure out how
 * much we can load the arduino and how much should be offloaded to another
 * board.
 * 
 * author: Tyler McCown(tylermccown@enineering.ucla.edu)
 * created: 31 January 2020
 */

#include <Servo.h>

// timing test
#define nCycles 1e3

// PWM constants
#define PW_STOP 1515
#define PW_MIN 1000
#define PW_MAX 2000
#define U_K_LAUNCH_SLOW -200
#define U_K_LAUNCH_FAST -350
#define U_K_ABS_MIN PW_MIN - PW_STOP
#define U_K_ABS_MAX PW_MAX - PW_STOP

// actuator
Servo Actuator;
const byte actuator_pin = 9;
#define POT_MIN 163
#define POT_MAX 254
#define POT_ENGAGE 245
unsigned int pot_lim_out = POT_MAX;
unsigned int pot_lim_in = POT_MIN;
int u_k_min = U_K_ABS_MIN; // [dpwm]
int u_k_max = U_K_ABS_MAX; // [dpwm]
const byte pot_pin = A1;
unsigned int current_pos(0);

// filters
const double eg_filt_const_1 = .112;
const double eg_filt_const_2 = .777;
const double gb_filt_const_1 = .059;
const double gb_filt_const_2 = .882;

// reference signals
// ***** ENGINE ***** //
const unsigned int EG_IDLE = 1750;
const unsigned int EG_ENGAGE = 2100;
const unsigned int EG_LAUNCH = 2600;
const unsigned int EG_TORQUE = 2700;
const unsigned int EG_POWER = 3400;
// ***** GB ***** //
const unsigned int GB_LAUNCH = 80; // ~ 5 mph
const unsigned int GB_POWER = 621.6; // ~ 39 mph

// controller
int r_k = EG_TORQUE;
const unsigned int control_period = 40e3; // [us]
const byte Kp = 1;
byte t = 0;
// unsigned long last_control_time(0); // [us]

// engine sensor
#define HF_HIGH 800
#define HF_LOW 200
#define HALL_WATCHDOG 1e6 // [us]
const byte hall_pin = A3;
bool eg_state(LOW);
unsigned long eg_last_trigger(0); // [us]
double eg_rpm(0); // [rpm*10]
double eg_rpm_raw(0); // [rpm/10]
double eg_rpm_raw_prev(0); // [rpm/10]

// gearbox sensor
const byte gb_pin = 3;
bool gb_state(LOW);
unsigned long gb_last_trigger(0); // [us]
double gb_rpm(0); // [rpm*10]
double gb_rpm_raw(0); // [rpm/10]
double gb_rpm_raw_prev(0); // [rpm/10]

void setup() {

  // arduino is a good boy
  bool good_boy = true;

  // open serial connection
  Serial.begin(9600);

  // setup actuator
  Actuator.attach(actuator_pin);
  Actuator.writeMicroseconds(PW_STOP);
  pinMode(pot_pin, INPUT);
  current_pos = analogRead(pot_pin);

  // setup engine sensor
  pinMode(hall_pin, INPUT);

  // setup gearbox sensor
  pinMode(gb_pin, INPUT);

  delay(1000);

  unsigned long t_start = millis();
  for (unsigned long i = 0; i < nCycles; i++) {
    control_function();
  }
  unsigned long t_elapsed = millis() - t_start;
  double time_per_cycle = t_elapsed/nCycles;
  Serial.print("Number of cycles: ");
  Serial.println(nCycles);
  Serial.print("Elapsed time [ms]: ");
  Serial.println(t_elapsed);
  Serial.print("Time per cycle [ms]: ");
  Serial.println(time_per_cycle);

}

void loop() {  
}

void control_function() {
  // calculate rpms
  gb_rpm = gb_filt_const_1*(gb_rpm_raw + gb_rpm_raw_prev) + gb_filt_const_2*gb_rpm;
  eg_rpm = eg_filt_const_1*(eg_rpm_raw + eg_rpm_raw_prev) + eg_filt_const_2*eg_rpm;

  // adjust reference
 if (gb_rpm > GB_POWER) {
   r_k = EG_POWER;
 } else if (gb_rpm > GB_LAUNCH) {
   r_k = map(gb_rpm, GB_LAUNCH, GB_POWER, EG_LAUNCH, EG_POWER);
 } else {
   r_k = EG_LAUNCH;
 }

    // change pot outer limit
  if (eg_rpm <= EG_ENGAGE) {
    pot_lim_out = map(eg_rpm, EG_IDLE, EG_ENGAGE, POT_MAX, POT_ENGAGE);
  } else {
    pot_lim_out = POT_ENGAGE;
  }

  // constrain control output
  // ***** PWM LIMITS ***** //
  u_k_min = U_K_ABS_MIN;
  u_k_max = U_K_ABS_MAX;
  // ***** LAUNCH ***** //
  if (gb_rpm < GB_LAUNCH && eg_rpm >= EG_TORQUE) {
    u_k_min = U_K_LAUNCH_FAST;
  }
  // ***** POT LIMITS ***** //
  current_pos = analogRead(pot_pin);
  if (current_pos >= pot_lim_out) {
    u_k_max = 0;
  } else if (current_pos <= pot_lim_in) {
    u_k_min = 0;
  }

  // compute error
  int e_k = (int) r_k - eg_rpm; // [rpm*10]

  // compute control signal
  int u_k = Kp*e_k; // [dpwm*10]
  u_k = u_k; // [dpwm]
  u_k = constrain(u_k, u_k_min, u_k_max);

  // write to actuator
  Actuator.writeMicroseconds(u_k + PW_STOP);

  // update previous filter values
  gb_rpm_raw_prev = gb_rpm_raw;
  eg_rpm_raw_prev = eg_rpm_raw;

  Serial.print(eg_rpm);
//  Serial.print(" ");
  Serial.print(gb_rpm);
//  Serial.print(" ");
  Serial.print("\n");
}
