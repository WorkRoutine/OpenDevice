MDIO
-------------------------------------

Managenment Data Input/Output

### MDIO READ Transaction

![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/signal/MDIO_READ.jpg)

### MDIO WRITE Transaction

![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/signal/MDIO_WRITE.jpg)

### Usage

  * base.c
    
    Simple mdio device driver, containts alloc() and register()

  * Soc_sunxi_mdio.c
 
    Simple mdio device driver, running on Allwinner platform.

  * Mdio_Hardware_Usage.c

    Basic hardware operation for MDIO.

  * MDIO_MISC.c

    MDIO bus device driver with MISC format, exprot file operations.

  * MDIO_userspace_API.c

    MDIO application demo code, base on MDIO driver.
