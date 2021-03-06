/* PCF8593_test.ino
 * 
 * Functionality testing of the PCF8593 event counter IC. Functions here
 * have been outsourced to a library and should not be implemented as shown
 * 
 * author: Tyler McCown (tylermccown@engineering.ucla.edu)
 * created: 9 February 2020
 */

#include <Wire.h>

byte int_pin = 2;

// i2c address
byte pcf8593_address = 0x00 | 1<<6 | 1<<4 | 1<<0; // B1010001, 0x51, 81

byte csr = 0x00;
byte alarm_ctrl = 0x00;
byte alarm_val = 0x00;

void setup() {

  // overwrite current configuration
  //  reset_pcf8593();

  // control and status register (00h)
  csr |= 1<<5; // event counter mode
  csr |= 1<<2; // enable alarm control register (08h)

  // alarm control register (08h)
  alarm_ctrl |= 0<<4; // event alarm
  alarm_ctrl |= 0<<7; // alarm no-interrupt enable
  //  alarm_val = decToBcd(10); // alarm at 99 counts

  //  digitalWrite(7, HIGH);
  Wire.begin(); // join i2c bus

  // input pin for interrupt
  //  pinMode(2, INPUT);
  
  Serial.begin(9600);

  // register setup
  set_csr();
  Serial.println(csr, BIN);

  reset_counters();
  
//  set_alarm();

  // attach counter overflow interrupt
//  attachInterrupt(digitalPinToInterrupt(int_pin), overflow_isr, FALLING);

}

void loop() {
  Serial.println(get_count());
  delay(20);
}

void set_csr() {
  // sets the control and status register (0x00h) of the PCF8593
  Wire.beginTransmission(pcf8593_address); // transmit to device
  Wire.write(0x00); // set register pointer to csr
  Wire.write(csr); // location 00h
  Wire.endTransmission();
}

void reset_counters() {
  // sets the counter registers (0x01h - 0x03h) of the PCF8593 to zero
  Wire.beginTransmission(pcf8593_address); // transmit to device
  Wire.write(0x01); // set register pointer to first counter register
  Wire.write(0x00); // location 01h
  Wire.write(0x00); // location 02h
  Wire.write(0x00); // location 03h
  Wire.endTransmission();
}

void set_alarm() {
  // sets the alarm control register (0x08h) of the PCF8593
  Wire.beginTransmission(pcf8593_address);
  Wire.write(0x08); // set register pointer to alarm control register
  Wire.write(alarm_ctrl); // 0x08h
  Wire.write(decToBcd(0)); // 0x09h
  Wire.write(decToBcd(0)); // 0x0Ah
  Wire.write(decToBcd(0)); // 0x0Bh
  Wire.endTransmission();
}

unsigned long get_count() {
  // requests and returns the count from the PCF8593

  unsigned long count(0);
  
  Wire.beginTransmission(pcf8593_address);
  Wire.write(0x01); // set register pointer to first counter register
  Wire.endTransmission();
  
  Wire.requestFrom(pcf8593_address, 3); // request data from three counter registers
  if (Wire.available()) {

    // read registers
    byte c01h = bcdToDec(Wire.read());
    byte c02h = bcdToDec(Wire.read());
    byte c03h = bcdToDec(Wire.read());

    // calculate count
    count += c01h;
    count += 1e2*c02h;
    count += 1e4*c03h;
  }

  return(count);
}

byte bcdToDec(byte value) {
  // convert binary coded decimal to regular decimal
  return ((value/16)*10 + value%16);
}

byte decToBcd(byte value) {
  // convert regular decimal to binary coded decimal
  return ((value/10)*16 + value%10);
}
