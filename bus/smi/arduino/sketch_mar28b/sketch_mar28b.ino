/*
 * SMI bus: MDIO Read/Write
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DIR 4 // 74HCT245 Dir
#define MDC 3 // D3 PIN for MDC
#define MDIO 2 // D2 PIN for MDIO
#define OUT 1
#define IN  0
#define CHIP_ID_88E6390     (0x0a10)

#define GLOBAL2_6390_ADDR    (0x1C)

#define GLOBAL2_6390_CMD     (0x18)

#define GLOBAL2_6390_DATA    (0x19)

int smi_init(void)
{
    pinMode(DIR, OUTPUT);
    digitalWrite(DIR, OUT);	//245 A to Y
    pinMode(MDC, OUTPUT);
    pinMode(MDIO, OUTPUT);
    return 0;
}


/*! Generates a rising edge pulse on MDC */
void pulse_mdc(void)
{
    volatile uint8_t i;
    //pinMode(MDC, OUTPUT);
    //i++;
    digitalWrite(MDC, 0);
    //delay(1);  change from 1Kbit/s to 10Kbit/s
    delayMicroseconds(100);
    i++;
    digitalWrite(MDC, 1);
    //delay(1);
    delayMicroseconds(100);
    i++;
}

/*! Performs a smi write */
void write_smi(uint8_t phy, uint8_t reg, uint16_t data)
{
    uint8_t byte;
    uint16_t word;

    /* MDIO pin is output */
    digitalWrite(DIR, OUT);
    pinMode(MDIO, OUTPUT);

    digitalWrite(MDIO, 1);
    digitalWrite(MDC, 1);
    for (byte = 0;byte < 32; byte++)
        pulse_mdc();

    /* Stat code */
    digitalWrite(MDIO, 0);
    pulse_mdc();
    digitalWrite(MDIO, 1);
    pulse_mdc();

    /* Write OP Code */
    digitalWrite(MDIO, 0);
    pulse_mdc();
    digitalWrite(MDIO, 1);
    pulse_mdc();

    /* PHY address - 5 bits */
    for (byte=0x10;byte!=0;byte=byte>>1){
        if (byte & phy)
            digitalWrite(MDIO, 1);
        else
            digitalWrite(MDIO, 0);
        pulse_mdc();
    }
    /* REG address - 5 bits */
    for (byte=0x10;byte!=0;byte=byte>>1){
        if (byte & reg)
            digitalWrite(MDIO, 1);
        else
            digitalWrite(MDIO, 0);

        pulse_mdc();
    }
    /* Turn around bits */
    digitalWrite(MDIO, 1);
    pulse_mdc();
    digitalWrite(MDIO, 0);
    pulse_mdc();

    /* Data - 16 bits */
    for(word=0x8000;word!=0;word=word>>1){
        if (word & data)
            digitalWrite(MDIO, 1);
        else
            digitalWrite(MDIO, 0);

        pulse_mdc();
    }

    /* This is needed for some reason... */
    pulse_mdc();
    /* Stay in 0 state */
//  MDC = 0;
    digitalWrite(DIR, IN);
    pinMode(MDIO, INPUT);

}

uint16_t read_smi(uint8_t phy, uint8_t reg)
{
    uint8_t byte;
    volatile uint16_t word, data;
    data = 0;

    /* MDIO pin is output */
    digitalWrite(DIR, OUT);
    pinMode(MDIO, OUTPUT);

    digitalWrite(MDIO, 1);
    digitalWrite(MDC, 1);
    for (byte = 0;byte < 32; byte++){
        pulse_mdc();
    }

    /* Stat code */
    digitalWrite(MDIO, 0);
    pulse_mdc();
    digitalWrite(MDIO, 1);
    pulse_mdc();

    /* Read OP Code */
    digitalWrite(MDIO, 1);
    pulse_mdc();
    digitalWrite(MDIO, 0);
    pulse_mdc();

    /* PHY address - 5 bits */
    for (byte=0x10;byte!=0;){
        if (byte & phy){
            digitalWrite(MDIO, 1);
            pulse_mdc();
        }else{
            digitalWrite(MDIO, 0);
            pulse_mdc();
        }
        byte=byte>>1;
    }
    /* REG address - 5 bits */
    for (byte=0x10;byte!=0;){
        if (byte & reg){
            digitalWrite(MDIO, 1);
            pulse_mdc();
        }else{
            digitalWrite(MDIO, 0);
            pulse_mdc();
        }
        byte=byte>>1;
    }
    /* Turn around bits */
    /* MDIO now is input */
    digitalWrite(DIR, IN);
    pinMode(MDIO, INPUT);
    pinMode(MDC, OUTPUT);
    pulse_mdc();
    pulse_mdc();
    /* Data - 16 bits */
    for(word=0x8000;word!=0;){

        if (digitalRead(MDIO)){
            data |= word;
        }
        pulse_mdc();
        word=word>>1;
    }

    /* This is needed for some reason... */
    pulse_mdc();
    /* Stay in 0 state */
//  MDC = 0;
    digitalWrite(DIR, IN);
    pinMode(MDIO, INPUT);

    return data;
}

/*
 * Read Date on internal SMI bus.
 */
int aup_SERDES_read(unsigned short port, unsigned short reg,
             uint16_t *value)
{
	uint16_t val;
	int8_t i;

	for(i=0;i<100;i++)
	{

		val = read_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_CMD);

		if ((val & 0x8000) == 0) break;

	}

	val = 0x9800 | (port << 5) | reg; //read with device address and register address

	write_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_CMD, val);

	delayMicroseconds(1);

	for(i=0;i<100;i++)	//wait the indirect operation end

	{

		val = read_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_CMD);

		if ((val & 0x8000) == 0) break;

	}

	val = read_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_DATA);

	*value = val;
}

/*
 * Write data to SERDES register
 */
int aup_SERDES_write(unsigned short port, unsigned short reg, uint16_t value)
{
	uint16_t val;

	int8_t i,rc;

	

	for(i=0;i<100;i++)

	{

		val = read_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_CMD);

		if ((val & 0x8000) == 0) break;

	}

	write_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_DATA, value);

	delayMicroseconds(1);

	for(i=0;i<100;i++)

	{

		val = read_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_CMD);

		if ((val & 0x8000) == 0) break;

	}

	val = 0x9400 | (port << 5) | reg; //write with device address and register address

	write_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_CMD, val);

	delayMicroseconds(1);

	for(i=0;i<100;i++)	//wait the indirect operation end

	{

		val = read_smi(GLOBAL2_6390_ADDR, GLOBAL2_6390_CMD);

		if ((val & 0x8000) == 0) break;

	}
}

/* Software-Reset 88E6185 Switch */
int SW_Reset_88E6185(void)
{
    write_smi(0x1B, 0x04, 0x8000);
    return 0;
}

void Switch_6390init()
{
  int value;
  int idreg = 0;
  

  if ((idreg & 0xfff0) != 0xA10) {
      idreg = read_smi(10, 3);
      if ((idreg & 0xFFF0) != 0xA10) {
         Serial.print("\n\rIncorrect 88E6390\n\r");
       }
  }
    /* Read Port10 Statues */
   value = read_smi(10, 0);
   Serial.print("\n\rCurrentValue:");
   Serial.print(value, HEX);
   if ((value & 0xF) != 0x9) {
       value &= 0xFFF0;
       value |= 0x9;
       /* Setup C_mode as SGMII or 1000Base-X */
       Serial.print("\n\rWrite C_mode 1000Base-X to 88E6390: \r\n");
       write_smi(10, 0, value);
   }
}

void setupm()
{
    int value;
    int idreg = 0;

    Serial.begin(9600);
    Serial.print("mdio init\n");
    smi_init();

    while (1) {
        delay(10000);
        if ((idreg & 0xfff0) != 0xA10) {
          idreg = read_smi(10, 3);
          if ((idreg & 0xFFF0) != 0xA10) {
              Serial.print("\n\rIncorrect 88E6390\n\r");
              continue;
          }
        }
        /* Read Port10 Statues */
        value = read_smi(10, 0);
        Serial.print("\n\rPort10 - CurrentValue:");
        Serial.print(value, HEX);
        if ((value & 0xF) != 0x9) {
            value &= 0xFFF0;
            value |= 0x9;
            /* Setup C_mode as SGMII or 1000Base-X */
            Serial.print("\n\rWrite C_mode 1000Base-X to Port10 88E6390: \r\n");
            write_smi(10, 0, value);
        }
        
        /* Setup Port9 to DX3336 mode as 10Gb-RXAUI */
        value = read_smi(9, 0);
        Serial.print("\n\rPort9 - CurrentValue:");
        Serial.print(value, HEX);
        if ((value & 0xF) != 0xD) {
            value &= 0xFFF0;
            value |= 0xD;
            /* Setup C_mode as 10Gb-RXAUI */
            Serial.print("\n\rWrite C_mode 10Gb-RXAUI to Port9 88E6390: \r\n");
            write_smi(9, 0, value);
        }   
    }
}

void setup()
{
  uint8_t phy, reg, val, i, sel, rw;
  String inStr = "";
  uint16_t reg_val = 0;
  
  // start serial port at 9600 bps:
  Serial.begin(9600);
  Serial.print("mdio init\n");
  smi_init();
  //Switch_6390init();
  for(;;){
    Serial.print("===========================================\r\n");
    Serial.print("Arduino SMI bus tools\r\n");
    Serial.print("1. Read register\r\n2. Write register\r\n3. Dump register\r\n");
    Serial.print("4. Read SERDES\r\n5. Write SERDES\r\n6. Dump SERDES\r\n");
    Serial.print("7. SW-Reset 88E6185\r\n8. SW_Reset 88E6390\r\n");
    Serial.setTimeout(100000);
    sel = Serial.parseInt();
    switch(sel){
      case 1:
          Serial.print("Read-Port: ");
          Serial.setTimeout(100000);
          phy = Serial.parseInt();
          Serial.print(phy);
          if (phy > 31) 
              break;
          Serial.print(" Register: ");
          Serial.setTimeout(100000);
          reg = Serial.parseInt();
          Serial.print(reg);
          if (reg > 31) 
              break;
          reg_val = read_smi(phy, reg);
          Serial.print("\n\r\n\rRead-Port: ");
          Serial.print(phy);
          Serial.print(" Register: ");
          Serial.print(reg);
          Serial.print(" Value [0x");
          Serial.print(reg_val, HEX);
          Serial.print("]\r\n\r\n");
      break;
      case 2:
          Serial.print("Write-Port: ");
          Serial.setTimeout(100000);
          phy = Serial.parseInt();
          Serial.print(phy);
          if (phy > 31) 
              break;
          Serial.print(" Register: ");
          Serial.setTimeout(100000);
          reg = Serial.parseInt();
          Serial.print(reg);
          if (reg > 31) 
              break;
          Serial.print(" Value: ");
          reg_val = Serial.parseInt();
          Serial.print(reg_val, HEX);
          write_smi(phy, reg, reg_val);
          while (1);
          Serial.print("\n\r\n\rWrite Port: ");
          Serial.print(phy);
          Serial.print(" Register: ");
          Serial.print(reg);
          Serial.print(" Value [0x");
          Serial.print(reg_val, HEX);
          Serial.print("]\r\n\r\n");
      break;
      case 3:
        Serial.print("Dump Start port: ");
        phy = Serial.parseInt();
        Serial.print(phy);
        Serial.print(" End port: ");
        Serial.setTimeout(100000);
        i = Serial.parseInt();
        Serial.print(i);
        Serial.print("\r\n");
        for (; phy<i; phy++){
          Serial.print("/*0x");
          Serial.print(phy, HEX);
          Serial.print("*/");
          Serial.print("{");
          for (reg=0; reg<32; reg++){
            Serial.print("0x");
            reg_val = read_smi(phy, reg);
            Serial.print((reg_val & 0xffff), HEX);
            if (reg<31) Serial.print(",");
            delay(10);
          }
          Serial.print("}\r\n");
        }
      break;
      case 4: /* Read Data from Internal-SMI bus */
          Serial.print("Device Address: ");
          Serial.setTimeout(100000);
          phy = Serial.parseInt();
          Serial.print(phy);
          if (phy > 31) 
              break;
          Serial.print(" Register: ");
          Serial.setTimeout(100000);
          reg = Serial.parseInt();
          Serial.print(reg);
          if (reg > 31) 
              break;
          aup_SERDES_read(phy, reg, &reg_val);
          Serial.print("\n\r\n\rSERDES-Dev: ");
          Serial.print(phy);
          Serial.print(" SERDES-Reg: ");
          Serial.print(reg);
          Serial.print(" Value [0x");
          Serial.print(reg_val, HEX);
          Serial.print("]\r\n\r\n");
      break;
      case 5: /* Write Data to Internal-SMI bus */
          Serial.print("Device Address: ");
          Serial.setTimeout(100000);
          phy = Serial.parseInt();
          Serial.print(phy);
          if (phy > 31) 
              break;
          Serial.print(" Register: :");
          Serial.setTimeout(100000);
          reg = Serial.parseInt();
          Serial.print(reg);
          if (reg > 31)
              break;
          Serial.print(" Value: ");
          reg_val = Serial.parseInt();
          Serial.print(reg_val, HEX);
          aup_SERDES_write(phy, reg, reg_val);
          Serial.print("\n\r\n\rSERDES-Dev: ");
          Serial.print(phy);
          Serial.print(" SERDES-Reg: ");
          Serial.print(reg);
          Serial.print(" Value [0x");
          Serial.print(reg_val, HEX);
          Serial.print("]\r\n\r\n");
      break;
      case 6: /* Dump all Internal-SMI register */
        Serial.print("Dump Start-Dev: ");
        phy = Serial.parseInt();
        Serial.print(phy);
        Serial.print(" End-Dev: ");
        Serial.setTimeout(100000);
        i = Serial.parseInt();
        Serial.print(i);
        Serial.print("\r\n");
        for (; phy < i; phy++) {
          Serial.print("/*0x");
          Serial.print(phy, HEX);
          Serial.print("*/");
          Serial.print("{");
          for (reg=0; reg<32; reg++) {
            Serial.print("0x");
            aup_SERDES_read(phy, reg, &reg_val);
            Serial.print((reg_val & 0xffff), HEX);
            if (reg<31) 
                Serial.print(",");
            delay(10);
          }
          Serial.print("}\r\n");
        }
      break;
      case 7:
          Serial.print("\r\nSW-Reset 88E6185");
          SW_Reset_88E6185();
          Serial.print("\r\n\r\n");
      break;
      case 8:
          Serial.print("\r\nSW-Reset 88E6390");
          Switch_6390init();
          Serial.print("\r\n\r\n");
      break;
      default:
        Serial.print("input wrong\r\n");
      break;
    }
  }
}

void loop()
{
}

