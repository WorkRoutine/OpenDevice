/*
 * Aup SMI/I2C/SPI Tools
 * 
 * Build by Sword, Buddy
 * 
 * <buddy.zhang@aliyun.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Pin Define */
#define MDC    3 /* D3 PIN for MDC */
#define MDIO   2 /* D2 PIN for MDIO */
#define OUT    1
#define IN     0

/*
 * MDC: Generate a rising edge pulse
 */
void MDC_Pulse()
{
    volatile uint8_t i;

    digitalWrite(MDC, 0);
    /* delay(1): Change from 1Kbit/s to 10Kbit/s */
    delayMicroseconds(100);
    i++;
    digitalWrite(MDC, 1);
    delayMicroseconds(100);
    i++;
}

/*
 * SMI Bus 22Class Read Register
 */
uint16_t SMI_22C_Read_Register(uint8_t PHY, uint8_t Reg)
{
    uint8_t byte;
    volatile uint16_t word, data;

    data = 0;

    /* MDIO Bus Pin Output */
    pinMode(MDIO, OUTPUT);
    digitalWrite(MDIO, 1);
    digitalWrite(MDC, 1);

    /* Establish 32 cycle */
    for (byte = 0; byte < 32; byte++) {
        MDC_Pulse();
    }

    /* Stat Code */
    digitalWrite(MDIO, 0);
    MDC_Pulse();
    digitalWrite(MDIO, 1);
    MDC_Pulse();

    /* Read OP Code */
    digitalWrite(MDIO, 1);
    MDC_Pulse();
    digitalWrite(MDIO, 0);
    MDC_Pulse();

    /* PHY Address - 5 bits */
    for (byte = 0x10; byte != 0; ) {
        if (byte & PHY) {
            digitalWrite(MDIO, 1);
            MDC_Pulse();  
        } else {
            digitalWrite(MDIO, 0);
            MDC_Pulse();  
        }
        byte = byte >> 1;
    }

    /* Register Address - 5 bits */
    for (byte = 0x10; byte != 0; ) {
        if (byte & Reg) {
            digitalWrite(MDIO, 1);
            MDC_Pulse();  
        } else {
            digitalWrite(MDIO, 0);
            MDC_Pulse();  
        }
        byte = byte >> 1;
    }

    /* Trun around bits. Now, MDIO is input */
    pinMode(MDIO, INPUT);
    pinMode(MDC, OUTPUT);
    MDC_Pulse();
    MDC_Pulse();

    /* Data - 16 Bits */
    for (word = 0x8000; word !=0; ) {
        if (digitalRead(MDIO)) {
            data |= word;  
        }
        MDC_Pulse();
        word = word >> 1;
    }

    /* This is needed for some reason... */
    MDC_Pulse();
    /* Stay in 0 state */
    pinMode(MDIO, INPUT);

    return data;
}

/*
 * SMI Bus 22Class Read entry
 */
void SMI_22C_Read()
{
    uint8_t PHY, Reg, Val;

    Serial.print("SMI Bus 22Class Read Operation\n\r");
    Serial.print("SMI PHY Port: ");
    Serial.setTimeout(100000);
    PHY = Serial.parseInt();
    Serial.print(PHY, HEX);
    Serial.print("\n\r");
    if (PHY > 32) {
        Serial.print("Invalid PHY Address, more then 32\n\r");  
    } else {
        Serial.print("SMI Register: ");
        Serial.setTimeout(100000);  
        Reg = Serial.parseInt();
        Serial.print(Reg, HEX);
        Serial.print("\n\r");
        if (Reg > 32) {
            Serial.print("Invalid Register Address, more than 32\n\r");  
        } else {
            while (1)
            Val = SMI_22C_Read_Register(PHY, Reg);
            Serial.print("\n\r");
            Serial.print("PHY: ");
            Serial.print(PHY, HEX);
            Serial.print(" Register: ");
            Serial.print(Reg, HEX);
            Serial.print(" Value: ");
            Serial.print(Val, HEX);
            Serial.print("\n\r");
            Serial.print("\n\r"); 
        }
    }
}

/*
 * SMI Bus 22Class Write Register
 */
void SMI_22C_Write_Register(uint8_t PHY, uint8_t Reg, uint16_t data)
{
    uint8_t byte;
    uint16_t word;

    /* MDIO pin set up ouput */
    pinMode(MDIO, OUTPUT);
    digitalWrite(MDIO, 1);
    digitalWrite(MDC, 1);

    /* Establish 32 edge */
    for (byte = 0; byte < 32; byte++)
        MDC_Pulse();

    /* Stat Code */
    digitalWrite(MDIO, 0);
    MDC_Pulse();
    digitalWrite(MDIO, 1);
    MDC_Pulse();

    /* Write OP Code */
    digitalWrite(MDIO, 0);
    MDC_Pulse();
    digitalWrite(MDIO, 1);
    MDC_Pulse();

    /* PHY address - 5 bits */
    for (byte = 0x10; byte != 0; byte = byte >> 1) {
        if (byte & PHY)
            digitalWrite(MDIO, 1);
        else
            digitalWrite(MDIO, 0);
        MDC_Pulse();  
    }
    /* Register address - 5 bits */
    for (byte = 0x10; byte != 0; byte = byte >> 1) {
        if (byte & Reg)
            digitalWrite(MDIO, 1);
        else
            digitalWrite(MDIO, 0);
        MDC_Pulse();
    }
    
    /* Trun around bits */
    digitalWrite(MDIO, 1);
    MDC_Pulse();
    digitalWrite(MDIO, 0);
    MDC_Pulse();

    /* Data - 16 bits */
    for (word = 0x8000; word != 0; word = word >> 1) {
        if (word & data)
            digitalWrite(MDIO, 1);
        else
            digitalWrite(MDIO, 0);
        MDC_Pulse();  
    }

    /* This is needed for some reason... */
    MDC_Pulse();
    /* Stay in 0 state */
    pinMode(MDIO, INPUT);
}

/*
 * SMI Bus 22Class Write entry
 */
void SMI_22C_Write()
{
    uint8_t PHY, Reg, Val;

    Serial.print("SMI Bus 22Class Write Operation\n\r");
    Serial.print("SMI PHY Port: ");
    Serial.setTimeout(100000);
    PHY = Serial.parseInt();
    Serial.print(PHY, HEX);
    Serial.print("\n\r");
    if (PHY > 32) {
        Serial.print("Invalid PHY Address, more then 32\n\r");  
    } else {
        Serial.print("SMI Register: ");
        Serial.setTimeout(100000);  
        Reg = Serial.parseInt();
        Serial.print(Reg, HEX);
        Serial.print("\n\r");
        if (Reg > 32) {
            Serial.print("Invalid Register Address, more than 32\n\r");  
        } else {
            Serial.print("Write Data: ");
            Serial.setTimeout(100000);
            Val = Serial.parseInt();
            Serial.print(Val, HEX);
            Serial.print("\n\r");
            SMI_22C_Write_Register(PHY, Reg, Val);
            Serial.print("\n\r"); 
        }
    }
}

/*
 * SMI Bus SERDES Read Register
 */
uint16_t SMI_SERDES_Read_Register(uint8_t Dev, uint8_t Reg)
{
    uint16_t command = 0x8000;
    int i = 100;
    unsigned short flag;

    /* SMIFunc: 0 - Internal Access. 1 - External Access. */

    /* Support General IEEE 802.3 Clause 22 SMI frames */
    command |= 0x1000;
    /* Read Data register */
    command |= 0x800;
    /* Device Address */
    command |= (Dev & 0x1F) << 5;
    /* Register Address */
    command |= (Reg & 0x1F);

    while (i--) {
        SMI_22C_Write_Register(0x1C, 0x19, 0x0000);
        flag = SMI_22C_Read_Register(0x1C, 0x18);
        if ((flag & 0x8000) == 0)
            break;
        delayMicroseconds(1);
    }

    /* Write command to Global 2 Register: SMI PHY Command Register */
    SMI_22C_Write_Register(0x1C, 0x18, command);

    while (i--) {
        flag = SMI_22C_Read_Register(0x1C, 0x18);
        if ((flag & 0x8000) == 0)
            break;
        delayMicroseconds(1);
    }
    /* Write Data from Global 2 Register: SMI PHY Data Register */
    return SMI_22C_Read_Register(0x1C, 0x19);
}

/*
 * SMI Bus SERDES Read entry
 */
void SMI_SERDES_Read()
{
    uint8_t PHY, Reg, Val;

    Serial.print("SMI Bus SERDES Read Operation\n\r");
    Serial.print("SMI SERDES Device: ");
    Serial.setTimeout(100000);
    PHY = Serial.parseInt();
    Serial.print(PHY, HEX);
    Serial.print("\n\r");
    if (PHY > 32) {
        Serial.print("Invalid Device Address, more then 32\n\r");  
    } else {
        Serial.print("SMI SERDES Register: ");
        Serial.setTimeout(100000);  
        Reg = Serial.parseInt();
        Serial.print(Reg, HEX);
        Serial.print("\n\r");
        if (Reg > 32) {
            Serial.print("Invalid Register Address, more than 32\n\r");  
        } else {
            Val = SMI_SERDES_Read_Register(PHY, Reg);
            Serial.print("\n\r");
            Serial.print("Device: ");
            Serial.print(PHY, HEX);
            Serial.print(" Register: ");
            Serial.print(Reg, HEX);
            Serial.print(" Value: ");
            Serial.print(Val, HEX);
            Serial.print("\n\r");
            Serial.print("\n\r"); 
        }
    }
}

/*
 * SMI Bus SERDES Write Register
 */
int SMI_SERDES_Write_Register(uint8_t Dev, uint8_t Reg, uint16_t Val)
{
    uint16_t command = 0x8000;
    int i = 100;
    unsigned short flag;

    /* Support General IEEE 802.3 Clause 22 SMI frames */
    command |= 0x1000;
    /* Write Data register */
    command |= 0x400;
    /* Device Address */
    command |= (Dev & 0x1F) << 5;
    /* Register Address */
    command |= (Reg & 0x1F);

    while (i--) {
        flag = SMI_22C_Read_Register(0x1C, 0x18);
        if ((flag & 0x8000) == 0)
            break;
        delayMicroseconds(1);
    }

    /* Write Data from Global 2 Register: SMI PHY Data Register */
    SMI_22C_Write_Register(0x1C, 0x19, Val);

    while (i--) {
        flag = SMI_22C_Read_Register(0x1C, 0x18);
        if ((flag & 0x8000) == 0)
            break;
        delayMicroseconds(1);
    }
    /* Write Command to Global 2 Register: SMI PHY Command Register */
    SMI_22C_Write_Register(0x1C, 0x18, command);
    return 0;
}

/*
 * SMI Bus SERDES Write entry
 */
void SMI_SERDES_Write()
{
    uint8_t PHY, Reg, Val;

    Serial.print("SMI Bus SERDES Write Operation\n\r");
    Serial.print("SMI SERDES Device: ");
    Serial.setTimeout(100000);
    PHY = Serial.parseInt();
    Serial.print(PHY, HEX);
    Serial.print("\n\r");
    if (PHY > 32) {
        Serial.print("Invalid Device Address, more then 32\n\r");  
    } else {
        Serial.print("SMI SERDES Register: ");
        Serial.setTimeout(100000);  
        Reg = Serial.parseInt();
        Serial.print(Reg, HEX);
        Serial.print("\n\r");
        if (Reg > 32) {
            Serial.print("Invalid Register Address, more than 32\n\r");  
        } else {
            Serial.print("SMI SERDES Data: ");
            Serial.setTimeout(100000);
            Val = Serial.parseInt();
            Serial.print(Val, HEX);
            Serial.print("\n\r");
            Serial.print("\n\r"); 
            SMI_SERDES_Write_Register(PHY, Reg, Val);
        }
    }
}

/*
 * Marvell 88E6321
 * 
 * Enable two Copper-RJ45: Port#3 and Port#4
 */
void Marvell_88E6321_2Copper2RJ45()
{
    /* Reset Swtich */
    SMI_22C_Write_Register(0x1B, 0x4, 0xC001);

    /* Select Page 0 on MAC-PHY Register Map */
    SMI_SERDES_Write_Register(0x3, 0x16, 0);
    SMI_SERDES_Write_Register(0x4, 0x16, 0);

    /* Copper Power up */
    SMI_SERDES_Write_Register(0x3, 0x10, 0x3070);
    SMI_SERDES_Write_Register(0x4, 0x10, 0x3070);

    /* Copper Reset and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x3, 0x0, 0x1140);
    SMI_SERDES_Write_Register(0x4, 0x0, 0x1140);

    /* Port Forwarding */
    SMI_22C_Write_Register(0x13, 0x4, 0x7F);
    SMI_22C_Write_Register(0x14, 0x4, 0x7F);
}

/*
 * Marvell 88E6321
 *  
 * Enable 0's Copper-RJ45(J10) to 1000Base-X0
 */
void Marvell_88E6321_0Copper_1000Base_X0()
{
    /* Reset Swtich */
    SMI_22C_Write_Register(0x1B, 0x4, 0xC001);

    /* Copper-RJ45(J10) */

    /* Select Page 0 on MAC-PHY Register Map */
    SMI_SERDES_Write_Register(0x3, 0x16, 0);
    /* Copper Power up */
    SMI_SERDES_Write_Register(0x3, 0x10, 0x3070);
    /* Copper Reset and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x3, 0x0, 0x1140);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x13, 0x4, 0x7F);

    /* 1000Base-X 0 */
    
    /* Select Page 1 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x12, 0x16, 1);   
    /* Fiber Reset, Power up, and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x12, 0x0, 0x8000); 
    /* Fiber Port Mode */ 
    SMI_SERDES_Write_Register(0x12, 0x10, 0x400D);
    /* Select Page 0 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x12, 0x16, 0);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x10, 0x4, 0x7F);
}

/*
 * Marvell 88E6321
 *  
 * Enable 1's Copper-RJ45(J10) to 1000Base-X1
 */
void Marvell_88E6321_1Copper_1000Base_X1()
{
    /* Reset Swtich */
    SMI_22C_Write_Register(0x1B, 0x4, 0xC001);

    /* Copper-RJ45(J12) */

    /* Select Page 0 on MAC-PHY Register Map */
    SMI_SERDES_Write_Register(0x4, 0x16, 0);
    /* Copper Power up */
    SMI_SERDES_Write_Register(0x4, 0x10, 0x3070);
    /* Copper Reset and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x4, 0x0, 0x1140);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x14, 0x4, 0x7F);

    /* 1000Base-X 1 */
    
    /* Select Page 1 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x13, 0x16, 1);   
    /* Fiber Reset, Power up, and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x13, 0x0, 0x8000); 
    /* Fiber Port Mode */ 
    SMI_SERDES_Write_Register(0x13, 0x10, 0x400D);
    /* Select Page 0 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x13, 0x16, 0);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x11, 0x4, 0x7F);
}

/*
 * Marvell 88E6321 initialization on VPU2600
 */
int SMI_88E6321_VPU2600_init()
{
    /* Reset Swtich */
    SMI_22C_Write_Register(0x1B, 0x4, 0xC001);

    /* Copper-RJ45 */

    /* Select Page 0 on MAC-PHY Register Map */
    SMI_SERDES_Write_Register(0x3, 0x16, 0);
    /* Copper Power up */
    SMI_SERDES_Write_Register(0x3, 0x10, 0x3070);
    /* Copper Reset and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x3, 0x0, 0x1140);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x13, 0x4, 0x7F);

    /* RGMII to CPU */

    /* Setup RGMII TX and RX delay */
    SMI_22C_Write_Register(0x15, 0x1, 0xC003);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x15, 0x4, 0x7F);

    /* 1000Base-X 0 */
    
    /* Select Page 1 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x12, 0x16, 1);   
    /* Fiber Reset, Power up, and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x12, 0x0, 0x8000); 
    /* Fiber Port Mode */ 
    SMI_SERDES_Write_Register(0x12, 0x10, 0x400D);
    /* Select Page 0 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x12, 0x16, 0);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x10, 0x4, 0x7F);

    /* 1000Base-X 1 */
    
    /* Select Page 1 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x13, 0x16, 1);   
    /* Fiber Reset, Power up, and Auto-Negotiation */
    SMI_SERDES_Write_Register(0x13, 0x0, 0x8000); 
    /* Fiber Port Mode */ 
    SMI_SERDES_Write_Register(0x13, 0x10, 0x400D);
    /* Select Page 0 on Fiber/SERDES Register Map */
    SMI_SERDES_Write_Register(0x13, 0x16, 0);
    /* Port Forwarding */
    SMI_22C_Write_Register(0x11, 0x4, 0x7F);

    return 0;
}

/*
 * Marvell 88E6321 initialization on TestBoard
 */
int SMI_88E6321_TestBoard_init()
{
    uint8_t sel;

    for (;;) {
        Serial.print("**************************************\n\r");
        Serial.print("Marvall 88E6321 TestBoard\n\r");
        Serial.print("0.  0's and 1's Copper-RJ45 Connect\n\r");
        Serial.print("1.  0's Copper-RJ45(J10) to 0's 1000Base-X Connect\n\r");
        Serial.print("2.  1's Copper-RJ45(J12) to 1's 1000Base-X Connect\n\r");
        Serial.print("9.  Exit to prev Menu\n\r");
        Serial.print("Input >> ");

        Serial.setTimeout(100000);
        sel = Serial.parseInt();
        Serial.print(sel, HEX);
        Serial.print("\n\r");
        switch (sel) {
        case 0:
            Marvell_88E6321_2Copper2RJ45();
            break;  
        case 1:
            Marvell_88E6321_0Copper_1000Base_X0();
            break;
        case 2:
            Marvell_88E6321_1Copper_1000Base_X1();
            break;
        case 9:
            goto out;
        default:
            Serial.print("Invalid operands\n\r");
            break;
        }
    }
out:
    return 0;
}

/*
 * Marvell Switch/PHY initialization
 */
int SMI_Marvell()
{
    uint8_t sel;

    for (;;) {
        Serial.print("***************************************\n\r");
        Serial.print("** Marvell Switch/PHY initialization **\n\r");
        Serial.print("***************************************\n\r");
        Serial.print("0.  88E6321 on VPU2600\n\r");
        Serial.print("1.  88E6390 on ESW2600\n\r");
        Serial.print("2.  88E6321 on TestBoard\n\r");
        Serial.print("9.  Exit to prev menu\n\r");  
        Serial.print("Input >> ");

        Serial.setTimeout(100000);
        sel = Serial.parseInt();
        Serial.print(sel, HEX);
        Serial.print("\n\r");

        switch (sel) {
        case 0:
            SMI_88E6321_VPU2600_init();
            break;
        case 2:
            SMI_88E6321_TestBoard_init();
            break;
        case 9:
            goto out;  
        default:
            Serial.print("Invalid operands\n\r");
            break;
        }
        
    }
out:
    return 0;
}

/*
 * SMI Bus initialization
 */
int SMI_Bus_init()
{
    pinMode(MDC, OUTPUT);
    pinMode(MDIO, OUTPUT);
    return 0;  
}

/*
 * SMI Bus Entry
 */
void SMI_Bus()
{
    uint8_t sel;
    
    for (;;) {
        Serial.print("**********************************\n\r");
        Serial.print("  SMI Bus tools\n\r");
        Serial.print("**********************************\n\r");
        Serial.print("0.  SMI-22Class Read Register\n\r");
        Serial.print("1.  SMI-22Class Write Register\n\r");
        Serial.print("2.  SMI-45Class Read Register\n\r");
        Serial.print("3.  SMI-45Class Write Register\n\r");
        Serial.print("4.  SERDES Read Register\n\r");
        Serial.print("5.  SERDES Write Register\n\r");
        Serial.print("6.  Dump SMI-22Class Registers\n\r");
        Serial.print("7.  Dump SMI-45Class Registers\n\r");
        Serial.print("8.  Dump SERDES Register\n\r");
        Serial.print("9.  Marvell Switch/PHY special initialization\n\r");
        Serial.print("10. Exit to Main Menu\n\r");
        Serial.print("Input >> ");

        Serial.setTimeout(100000);
        sel = Serial.parseInt();
        Serial.print(sel);
        Serial.print("\n\r");
        switch (sel) {
        case 0:
            SMI_22C_Read();
            break;
        case 1:
            SMI_22C_Write();
            break;
        case 4:
            SMI_SERDES_Read();
            break;
        case 5:
            SMI_SERDES_Write();
            break;
        case 9:
            SMI_Marvell();
        case 10:
            goto out;
        default:
            Serial.print("Invalid operands\n\r");
            break;
        }
    }
out:
    Serial.print("Bace to Main-Menu\n\r");
}

/*
 * Setup Arduino
 */
void setup()
{
    Serial.begin(9600);
    SMI_Bus_init();
    
    Serial.print("*************************************\n\r");
    Serial.print("  Initialize Tools done\n\r");
    Serial.print("*************************************\n\r");
}

/*
 * Loop 
 */
void loop()
{
    uint8_t sel;

    Serial.print("*************************************\n\r");
    Serial.print("  Welcome to Aup Arduino tools\n\r");
    Serial.print("*************************************\n\r");
    Serial.print("0. SMI Bus tools\n\r");
    Serial.print("1. I2C Bus tools\n\r");
    Serial.print("2. SPI Bus tools\n\r");
    Serial.print("3. GPIO tools\n\r");
    Serial.print("Input >> ");
    Serial.setTimeout(100000);

    sel = Serial.parseInt();
    Serial.print(sel);
    Serial.print("\n\r");
    switch (sel) {
    case 0:
      SMI_Bus();
      break;
    default:
      Serial.print("Invalid operands\n\r");
      break;
    }
}
