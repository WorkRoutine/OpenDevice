#include <Wire.h>

/*
 * aup_i2c_read()
 * 
 * I2C read operation on Arduino.
 * 
 * @addr:    Slave address
 * @offset:  Offset on Soc
 * @val:     Store read data
 * @nr:      number byte for reading
 * 
 * return 0 successful
 *        1 failure
 */
int aup_i2c_read(unsigned char addr, unsigned long offset, char *val, int nr)
{
  int i = 0;

  /* Write Register offset */
  Wire.beginTransmission(addr);
  Wire.write(offset);
  Wire.endTransmission();

  /* Read data from I2C Bus */
  Wire.requestFrom(addr, nr);
  while (Wire.available()) {
    val[i++] = Wire.read();
  }
  return 0;  
}

/*
 * aup_i2c_write()
 * 
 * I2C write operation on Arduino
 * 
 * @addr:   Slave address
 * @offset: Offset on Soc
 * @val:    ready to write data
 * 
 * return 0 successful
 *        1 failure
 */
int aup_i2c_write(unsigned char addr, unsigned long offset, char *val, int nr)
{
    int i;
    
    Wire.beginTransmission(addr);
    /* Write offset on Soc */
    Wire.write(offset);
    for (i = 0; i < nr; i++)
      Wire.write(val[i]);
    Wire.endTransmission();
    return 0;  
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
}

int vai = 0;

void loop() {
  char value[32];

  delay(1000);
  Wire.beginTransmission(80);
  Wire.write(0x10);
  Wire.endTransmission();

  Serial.println(vai);
  vai++;
}
