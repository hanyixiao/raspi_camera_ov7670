/*
  Reads GPIO state and dumps to console.
  Allows GPIO hacking to set and get GPIO state.
  Author: James Adams
*/
#include "raspi_io.h"

static int memfd = -1;

static const char *gpio_alt_names_2708[54*6] =
{
    "SDA0"      , "SA5"        , "PCLK"      , "AVEOUT_VCLK"   , "AVEIN_VCLK" , "-"         ,
    "SCL0"      , "SA4"        , "DE"        , "AVEOUT_DSYNC"  , "AVEIN_DSYNC", "-"         ,
    "SDA1"      , "SA3"        , "LCD_VSYNC" , "AVEOUT_VSYNC"  , "AVEIN_VSYNC", "-"         ,
    "SCL1"      , "SA2"        , "LCD_HSYNC" , "AVEOUT_HSYNC"  , "AVEIN_HSYNC", "-"         ,
    "GPCLK0"    , "SA1"        , "DPI_D0"    , "AVEOUT_VID0"   , "AVEIN_VID0" , "ARM_TDI"   ,
    "GPCLK1"    , "SA0"        , "DPI_D1"    , "AVEOUT_VID1"   , "AVEIN_VID1" , "ARM_TDO"   ,
    "GPCLK2"    , "SOE_N_SE"   , "DPI_D2"    , "AVEOUT_VID2"   , "AVEIN_VID2" , "ARM_RTCK"  ,
    "SPI0_CE1_N", "SWE_N_SRW_N", "DPI_D3"    , "AVEOUT_VID3"   , "AVEIN_VID3" , "-"         ,
    "SPI0_CE0_N", "SD0"        , "DPI_D4"    , "AVEOUT_VID4"   , "AVEIN_VID4" , "-"         ,
    "SPI0_MISO" , "SD1"        , "DPI_D5"    , "AVEOUT_VID5"   , "AVEIN_VID5" , "-"         ,
    "SPI0_MOSI" , "SD2"        , "DPI_D6"    , "AVEOUT_VID6"   , "AVEIN_VID6" , "-"         ,
    "SPI0_SCLK" , "SD3"        , "DPI_D7"    , "AVEOUT_VID7"   , "AVEIN_VID7" , "-"         ,
    "PWM0"      , "SD4"        , "DPI_D8"    , "AVEOUT_VID8"   , "AVEIN_VID8" , "ARM_TMS"   ,
    "PWM1"      , "SD5"        , "DPI_D9"    , "AVEOUT_VID9"   , "AVEIN_VID9" , "ARM_TCK"   ,
    "TXD0"      , "SD6"        , "DPI_D10"   , "AVEOUT_VID10"  , "AVEIN_VID10", "TXD1"      ,
    "RXD0"      , "SD7"        , "DPI_D11"   , "AVEOUT_VID11"  , "AVEIN_VID11", "RXD1"      ,
    "FL0"       , "SD8"        , "DPI_D12"   , "CTS0"          , "SPI1_CE2_N" , "CTS1"      ,
    "FL1"       , "SD9"        , "DPI_D13"   , "RTS0"          , "SPI1_CE1_N" , "RTS1"      ,
    "PCM_CLK"   , "SD10"       , "DPI_D14"   , "I2CSL_SDA_MOSI", "SPI1_CE0_N" , "PWM0"      ,
    "PCM_FS"    , "SD11"       , "DPI_D15"   , "I2CSL_SCL_SCLK", "SPI1_MISO"  , "PWM1"      ,
    "PCM_DIN"   , "SD12"       , "DPI_D16"   , "I2CSL_MISO"    , "SPI1_MOSI"  , "GPCLK0"    ,
    "PCM_DOUT"  , "SD13"       , "DPI_D17"   , "I2CSL_CE_N"    , "SPI1_SCLK"  , "GPCLK1"    ,
    "SD0_CLK"   , "SD14"       , "DPI_D18"   , "SD1_CLK"       , "ARM_TRST"   , "-"         ,
    "SD0_CMD"   , "SD15"       , "DPI_D19"   , "SD1_CMD"       , "ARM_RTCK"   , "-"         ,
    "SD0_DAT0"  , "SD16"       , "DPI_D20"   , "SD1_DAT0"      , "ARM_TDO"    , "-"         ,
    "SD0_DAT1"  , "SD17"       , "DPI_D21"   , "SD1_DAT1"      , "ARM_TCK"    , "-"         ,
    "SD0_DAT2"  , "TE0"        , "DPI_D22"   , "SD1_DAT2"      , "ARM_TDI"    , "-"         ,
    "SD0_DAT3"  , "TE1"        , "DPI_D23"   , "SD1_DAT3"      , "ARM_TMS"    , "-"         ,
    "SDA0"      , "SA5"        , "PCM_CLK"   , "FL0"           , "-"          , "-"         ,
    "SCL0"      , "SA4"        , "PCM_FS"    , "FL1"           , "-"          , "-"         ,
    "TE0"       , "SA3"        , "PCM_DIN"   , "CTS0"          , "-"          , "CTS1"      ,
    "FL0"       , "SA2"        , "PCM_DOUT"  , "RTS0"          , "-"          , "RTS1"      ,
    "GPCLK0"    , "SA1"        , "RING_OCLK" , "TXD0"          , "-"          , "TXD1"      ,
    "FL1"       , "SA0"        , "TE1"       , "RXD0"          , "-"          , "RXD1"      ,
    "GPCLK0"    , "SOE_N_SE"   , "TE2"       , "SD1_CLK"       , "-"          , "-"         ,
    "SPI0_CE1_N", "SWE_N_SRW_N", "-"         , "SD1_CMD"       , "-"          , "-"         ,
    "SPI0_CE0_N", "SD0"        , "TXD0"      , "SD1_DAT0"      , "-"          , "-"         ,
    "SPI0_MISO" , "SD1"        , "RXD0"      , "SD1_DAT1"      , "-"          , "-"         ,
    "SPI0_MOSI" , "SD2"        , "RTS0"      , "SD1_DAT2"      , "-"          , "-"         ,
    "SPI0_SCLK" , "SD3"        , "CTS0"      , "SD1_DAT3"      , "-"          , "-"         ,
    "PWM0"      , "SD4"        , "-"         , "SD1_DAT4"      , "SPI2_MISO"  , "TXD1"      ,
    "PWM1"      , "SD5"        , "TE0"       , "SD1_DAT5"      , "SPI2_MOSI"  , "RXD1"      ,
    "GPCLK1"    , "SD6"        , "TE1"       , "SD1_DAT6"      , "SPI2_SCLK"  , "RTS1"      ,
    "GPCLK2"    , "SD7"        , "TE2"       , "SD1_DAT7"      , "SPI2_CE0_N" , "CTS1"      ,
    "GPCLK1"    , "SDA0"       , "SDA1"      , "TE0"           , "SPI2_CE1_N" , "-"         ,
    "PWM1"      , "SCL0"       , "SCL1"      , "TE1"           , "SPI2_CE2_N" , "-"         ,
    "SDA0"      , "SDA1"       , "SPI0_CE0_N", "-"             , "-"          , "SPI2_CE1_N",
    "SCL0"      , "SCL1"       , "SPI0_MISO" , "-"             , "-"          , "SPI2_CE0_N",
    "SD0_CLK"   , "FL0"        , "SPI0_MOSI" , "SD1_CLK"       , "ARM_TRST"   , "SPI2_SCLK" ,
    "SD0_CMD"   , "GPCLK0"     , "SPI0_SCLK" , "SD1_CMD"       , "ARM_RTCK"   , "SPI2_MOSI" ,
    "SD0_DAT0"  , "GPCLK1"     , "PCM_CLK"   , "SD1_DAT0"      , "ARM_TDO"    , "-"         ,
    "SD0_DAT1"  , "GPCLK2"     , "PCM_FS"    , "SD1_DAT1"      , "ARM_TCK"    , "-"         ,
    "SD0_DAT2"  , "PWM0"       , "PCM_DIN"   , "SD1_DAT2"      , "ARM_TDI"    , "-"         ,
    "SD0_DAT3"  , "PWM1"       , "PCM_DOUT"  , "SD1_DAT3"      , "ARM_TMS"    , "-"
};

static const char *gpio_alt_names_2711[54*6] =
{
    "SDA0"      , "SA5"        , "PCLK"      , "SPI3_CE0_N"    , "TXD2"            , "SDA6"        ,
    "SCL0"      , "SA4"        , "DE"        , "SPI3_MISO"     , "RXD2"            , "SCL6"        ,
    "SDA1"      , "SA3"        , "LCD_VSYNC" , "SPI3_MOSI"     , "CTS2"            , "SDA3"        ,
    "SCL1"      , "SA2"        , "LCD_HSYNC" , "SPI3_SCLK"     , "RTS2"            , "SCL3"        ,
    "GPCLK0"    , "SA1"        , "DPI_D0"    , "SPI4_CE0_N"    , "TXD3"            , "SDA3"        ,
    "GPCLK1"    , "SA0"        , "DPI_D1"    , "SPI4_MISO"     , "RXD3"            , "SCL3"        ,
    "GPCLK2"    , "SOE_N_SE"   , "DPI_D2"    , "SPI4_MOSI"     , "CTS3"            , "SDA4"        ,
    "SPI0_CE1_N", "SWE_N_SRW_N", "DPI_D3"    , "SPI4_SCLK"     , "RTS3"            , "SCL4"        ,
    "SPI0_CE0_N", "SD0"        , "DPI_D4"    , "I2CSL_CE_N"    , "TXD4"            , "SDA4"        ,
    "SPI0_MISO" , "SD1"        , "DPI_D5"    , "I2CSL_SDI_MISO", "RXD4"            , "SCL4"        ,
    "SPI0_MOSI" , "SD2"        , "DPI_D6"    , "I2CSL_SDA_MOSI", "CTS4"            , "SDA5"        ,
    "SPI0_SCLK" , "SD3"        , "DPI_D7"    , "I2CSL_SCL_SCLK", "RTS4"            , "SCL5"        ,
    "PWM0_0"    , "SD4"        , "DPI_D8"    , "SPI5_CE0_N"    , "TXD5"            , "SDA5"        ,
    "PWM0_1"    , "SD5"        , "DPI_D9"    , "SPI5_MISO"     , "RXD5"            , "SCL5"        ,
    "TXD0"      , "SD6"        , "DPI_D10"   , "SPI5_MOSI"     , "CTS5"            , "TXD1"        ,
    "RXD0"      , "SD7"        , "DPI_D11"   , "SPI5_SCLK"     , "RTS5"            , "RXD1"        ,
    "-"         , "SD8"        , "DPI_D12"   , "CTS0"          , "SPI1_CE2_N"      , "CTS1"        ,
    "-"         , "SD9"        , "DPI_D13"   , "RTS0"          , "SPI1_CE1_N"      , "RTS1"        ,
    "PCM_CLK"   , "SD10"       , "DPI_D14"   , "SPI6_CE0_N"    , "SPI1_CE0_N"      , "PWM0_0"      ,
    "PCM_FS"    , "SD11"       , "DPI_D15"   , "SPI6_MISO"     , "SPI1_MISO"       , "PWM0_1"      ,
    "PCM_DIN"   , "SD12"       , "DPI_D16"   , "SPI6_MOSI"     , "SPI1_MOSI"       , "GPCLK0"      ,
    "PCM_DOUT"  , "SD13"       , "DPI_D17"   , "SPI6_SCLK"     , "SPI1_SCLK"       , "GPCLK1"      ,
    "SD0_CLK"   , "SD14"       , "DPI_D18"   , "SD1_CLK"       , "ARM_TRST"        , "SDA6"        ,
    "SD0_CMD"   , "SD15"       , "DPI_D19"   , "SD1_CMD"       , "ARM_RTCK"        , "SCL6"        ,
    "SD0_DAT0"  , "SD16"       , "DPI_D20"   , "SD1_DAT0"      , "ARM_TDO"         , "SPI3_CE1_N"  ,
    "SD0_DAT1"  , "SD17"       , "DPI_D21"   , "SD1_DAT1"      , "ARM_TCK"         , "SPI4_CE1_N"  ,
    "SD0_DAT2"  , "-"          , "DPI_D22"   , "SD1_DAT2"      , "ARM_TDI"         , "SPI5_CE1_N"  ,
    "SD0_DAT3"  , "-"          , "DPI_D23"   , "SD1_DAT3"      , "ARM_TMS"         , "SPI6_CE1_N"  ,
    "SDA0"      , "SA5"        , "PCM_CLK"   , "-"             , "MII_A_RX_ERR"    , "RGMII_MDIO"  ,
    "SCL0"      , "SA4"        , "PCM_FS"    , "-"             , "MII_A_TX_ERR"    , "RGMII_MDC"   ,
    "-"         , "SA3"        , "PCM_DIN"   , "CTS0"          , "MII_A_CRS"       , "CTS1"        ,
    "-"         , "SA2"        , "PCM_DOUT"  , "RTS0"          , "MII_A_COL"       , "RTS1"        ,
    "GPCLK0"    , "SA1"        , "-"         , "TXD0"          , "SD_CARD_PRES"    , "TXD1"        ,
    "-"         , "SA0"        , "-"         , "RXD0"          , "SD_CARD_WRPROT"  , "RXD1"        ,
    "GPCLK0"    , "SOE_N_SE"   , "-"         , "SD1_CLK"       , "SD_CARD_LED"     , "RGMII_IRQ"   ,
    "SPI0_CE1_N", "SWE_N_SRW_N", "-"         , "SD1_CMD"       , "RGMII_START_STOP", "-"           ,
    "SPI0_CE0_N", "SD0"        , "TXD0"      , "SD1_DAT0"      , "RGMII_RX_OK"     , "MII_A_RX_ERR",
    "SPI0_MISO" , "SD1"        , "RXD0"      , "SD1_DAT1"      , "RGMII_MDIO"      , "MII_A_TX_ERR",
    "SPI0_MOSI" , "SD2"        , "RTS0"      , "SD1_DAT2"      , "RGMII_MDC"       , "MII_A_CRS"   ,
    "SPI0_SCLK" , "SD3"        , "CTS0"      , "SD1_DAT3"      , "RGMII_IRQ"       , "MII_A_COL"   ,
    "PWM1_0"    , "SD4"        , "-"         , "SD1_DAT4"      , "SPI0_MISO"       , "TXD1"        ,
    "PWM1_1"    , "SD5"        , "-"         , "SD1_DAT5"      , "SPI0_MOSI"       , "RXD1"        ,
    "GPCLK1"    , "SD6"        , "-"         , "SD1_DAT6"      , "SPI0_SCLK"       , "RTS1"        ,
    "GPCLK2"    , "SD7"        , "-"         , "SD1_DAT7"      , "SPI0_CE0_N"      , "CTS1"        ,
    "GPCLK1"    , "SDA0"       , "SDA1"      , "-"             , "SPI0_CE1_N"      , "SD_CARD_VOLT",
    "PWM0_1"    , "SCL0"       , "SCL1"      , "-"             , "SPI0_CE2_N"      , "SD_CARD_PWR0",
    "SDA0"      , "SDA1"       , "SPI0_CE0_N", "-"             , "-"               , "SPI2_CE1_N"  ,
    "SCL0"      , "SCL1"       , "SPI0_MISO" , "-"             , "-"               , "SPI2_CE0_N"  ,
    "SD0_CLK"   , "-"          , "SPI0_MOSI" , "SD1_CLK"       , "ARM_TRST"        , "SPI2_SCLK"   ,
    "SD0_CMD"   , "GPCLK0"     , "SPI0_SCLK" , "SD1_CMD"       , "ARM_RTCK"        , "SPI2_MOSI"   ,
    "SD0_DAT0"  , "GPCLK1"     , "PCM_CLK"   , "SD1_DAT0"      , "ARM_TDO"         , "SPI2_MISO"   ,
    "SD0_DAT1"  , "GPCLK2"     , "PCM_FS"    , "SD1_DAT1"      , "ARM_TCK"         , "SD_CARD_LED" ,
    "SD0_DAT2"  , "PWM0_0"     , "PCM_DIN"   , "SD1_DAT2"      , "ARM_TDI"         , "-"           ,
    "SD0_DAT3"  , "PWM0_1"     , "PCM_DOUT"  , "SD1_DAT3"      , "ARM_TMS"         , "-"           ,
};

static const char *gpio_fsel_alts[8] =
{
    " ", " ", "5", "4", "0", "1", "2", "3"
};

static const char *gpio_pull_names[4] =
{
    "NONE", "DOWN", "UP", "?"
};

/* 0 = none, 1 = down, 2 = up */
static const int gpio_default_pullstate[54] =
{
    2,2,2,2,2,2,2,2,2, /*GPIO0-8 UP*/
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /*GPIO9-27 DOWN*/
    0,0, /*GPIO28-29 NONE*/
    1,1,1,1, /*GPIO30-33 DOWN*/
    2,2,2, /*GPIO34-36 UP*/
    1,1,1,1,1,1,1, /*GPIO37-43 DOWN*/
    0,0, /*GPIO44-45 NONE*/
    2,2,2,2,2,2,2,2 /*GPIO46-53 UP*/
};

#define GPIO_BASE_OFFSET  0x00200000

#define BLOCK_SIZE  (4*1024)

#define GPSET0    7
#define GPSET1    8
#define GPCLR0    10
#define GPCLR1    11
#define GPLEV0    13
#define GPLEV1    14
#define GPPUD     37
#define GPPUDCLK0 38
#define GPPUDCLK1 39

/* 2711 has a different mechanism for pin pull-up/down/enable  */
#define GPPUPPDN0                57        /* Pin pull-up/down for pins 15:0  */
#define GPPUPPDN1                58        /* Pin pull-up/down for pins 31:16 */
#define GPPUPPDN2                59        /* Pin pull-up/down for pins 47:32 */
#define GPPUPPDN3                60        /* Pin pull-up/down for pins 57:48 */


/*define GPIO_PWM_Register*/
/*add to GPIO_base   */
#define GPIO_PWM_BASE        0x20C000
#define PWM_CTL              0
#define PWM_STA              1
#define PWM_DMAC             2
#define PWM0_RANGE           4
#define PWM0_DATA            5
#define PWM_FIF1             6
#define PWM1_RANFE           8
#define PWM1_DATA            9
/*define Clock base to GPIO_base*/
#define CLOCK_BASE          0X101000//(0x200000-0x101000)/4
#define PWM_CLK_CNTL        40
#define PWM_CLK_DIV         41
#define PWM_PASSWRD         (0x5A <<24)

/* Pointer to HW */
static volatile uint32_t *gpio_base;
static int is_2711;
static const char **gpio_alt_names;

void print_gpio_alts_info(int gpio)
{
    int alt;
    printf("%d", gpio);
    if (gpio_default_pullstate[gpio] == 0)
        printf(", NONE");
    else if (gpio_default_pullstate[gpio] == 1)
        printf(", DOWN");
    else
        printf(", UP");
    for (alt=0; alt < 6; alt++)
    {
        printf(", %s", gpio_alt_names[gpio*6+alt]);
    }
    printf("\n");
}

void delay_us(uint32_t delay)
{
    struct timespec tv_req;
    struct timespec tv_rem;
    int i;
    uint32_t del_ms, del_us;
    del_ms = delay / 1000;
    del_us = delay % 1000;
    for (i=0; i<=del_ms; i++)
    {
        tv_req.tv_sec = 0;
        if (i==del_ms) tv_req.tv_nsec = del_us*1000;
        else          tv_req.tv_nsec = 1000000;
        tv_rem.tv_sec = 0;
        tv_rem.tv_nsec = 0;
        nanosleep(&tv_req, &tv_rem);
        if (tv_rem.tv_sec != 0 || tv_rem.tv_nsec != 0)
            printf("timer oops!\n");
    }
}
void delay_ms(uint32_t msdelay)
{
    for(int i=0;i<msdelay;i++)
    {
        delay_us(1000);
    }
}
uint32_t get_hwbase(void)
{
    const char *ranges_file = "/proc/device-tree/soc/ranges";
    uint8_t ranges[12];
    FILE *fd;
    uint32_t ret = 0;

    memset(ranges, 0, sizeof(ranges));

    if ((fd = fopen(ranges_file, "rb")) == NULL)
    {
        printf("Can't open '%s'\n", ranges_file);
    }
    else if (fread(ranges, 1, sizeof(ranges), fd) >= 8)
    {
        ret = (ranges[4] << 24) |
              (ranges[5] << 16) |
              (ranges[6] << 8) |
              (ranges[7] << 0);
        if (!ret)
            ret = (ranges[8] << 24) |
                  (ranges[9] << 16) |
                  (ranges[10] << 8) |
                  (ranges[11] << 0);
        if ((ranges[0] != 0x7e) ||
                (ranges[1] != 0x00) ||
                (ranges[2] != 0x00) ||
                (ranges[3] != 0x00) ||
                ((ret != 0x20000000) && (ret != 0x3f000000) && (ret != 0xfe000000)))
        {
            printf("Unexpected ranges data (%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x)\n",
                   ranges[0], ranges[1], ranges[2], ranges[3],
                   ranges[4], ranges[5], ranges[6], ranges[7],
                   ranges[8], ranges[9], ranges[10], ranges[11]);
            ret = 0;
        }
    }
    else
    {
        printf("Ranges data too short\n");
    }

    fclose(fd);

    return ret;
}

int get_gpio_fsel(int gpio)
{
    /* GPIOFSEL0-GPIOFSEL5 with 10 sels per 32 bit reg,
       3 bits per sel (so bits 0:29 used) */
    uint32_t reg = gpio / 10;
    uint32_t sel = gpio % 10;
    if (gpio < GPIO_MIN || gpio > GPIO_MAX) return -1;
    /*printf("reg = %d, sel = %d ", reg, sel);*/
    return (int)((*(gpio_base+reg))>>(3*sel))&0x7;
}

int set_gpio_fsel(int gpio, int fsel)
{
   // printf("info[set_gpio]:%x",gpio_base);
    static volatile uint32_t *tmp;
    uint32_t reg = gpio / 10;
    uint32_t sel = gpio % 10;
    uint32_t mask;
    if (gpio < GPIO_MIN || gpio > GPIO_MAX) return -1;
    tmp = gpio_base+reg;
    mask = 0x7<<(3*sel);
    mask = ~mask;
    /*printf("reg = %d, sel = %d, mask=%08X\n", reg, sel, mask);*/
    tmp = gpio_base+reg;
    *tmp = *tmp & mask;
    *tmp = *tmp | ((fsel&0x7)<<(3*sel));
    return (int)((*tmp)>>(3*sel))&0x7;
}

int get_gpio_level(int gpio)
{
    if (gpio < GPIO_MIN || gpio > GPIO_MAX) return -1;
    if (gpio < 32)
    {
        return ((*(gpio_base+GPLEV0))>>gpio)&0x1;
    }
    else
    {
        gpio = gpio-32;
        return ((*(gpio_base+GPLEV1))>>gpio)&0x1;
    }
}

int set_gpio_value(int gpio, int value)
{
    if (gpio < GPIO_MIN || gpio > GPIO_MAX) return -1;
    if (value != 0)
    {
        if (gpio < 32)
        {
            *(gpio_base+GPSET0) = 0x1<<gpio;
        }
        else
        {
            gpio -= 32;
            *(gpio_base+GPSET1) = 0x1<<gpio;
        }
    }
    else
    {
        if (gpio < 32)
        {
            *(gpio_base+GPCLR0) = 0x1<<gpio;
        }
        else
        {
            gpio -= 32;
            *(gpio_base+GPCLR1) = 0x1<<gpio;
        }
    }
    return 0;
}

int gpio_fsel_to_namestr(int gpio, int fsel, char *name)
{
    int altfn = 0;
    if (gpio < GPIO_MIN || gpio > GPIO_MAX) return -1;
    switch (fsel)
    {
    case 0:
        return sprintf(name, "INPUT");
    case 1:
        return sprintf(name, "OUTPUT");
    case 2:
        altfn = 5;
        break;
    case 3:
        altfn = 4;
        break;
    case 4:
        altfn = 0;
        break;
    case 5:
        altfn = 1;
        break;
    case 6:
        altfn = 2;
        break;
    default:  /*case 7*/
        altfn = 3;
        break;
    }
    return sprintf(name, "%s", gpio_alt_names[gpio*6 + altfn]);
}

void print_raw_gpio_regs(void)
{
    int i;
    int end = is_2711 ? GPPUPPDN3 : GPPUDCLK1;

    for (i = 0; i <= end; i++)
    {
        /* Skip over non-GPIO registers on Pi4 models */
        if (i == (GPPUDCLK1 + 1))
        {
            i = GPPUPPDN0;
            printf("%02x:%*s", i * 4, (i & 3) * 9, "");
        }

        uint32_t val = *(gpio_base + i);
        if ((i & 3) == 0)
            printf("%02x:", i * 4);
        printf(" %08x", val);
        if ((i & 3) == 3)
            printf("\n");
    }
    if (i & 3)
        printf("\n");
}

void print_help()
{
    char *name = "raspi-gpio"; /* in case we want to rename */
    printf("\n");
    printf("WARNING! %s set writes directly to the GPIO control registers\n", name);
    printf("ignoring whatever else may be using them (such as Linux drivers) -\n");
    printf("it is designed as a debug tool, only use it if you know what you\n");
    printf("are doing and at your own risk!\n");
    printf("\n");
    printf("The %s tool is designed to help hack / debug BCM283x GPIO.\n", name);
    printf("Running %s with the help argument prints this help.\n", name);
    printf("%s can get and print the state of a GPIO (or all GPIOs)\n", name);
    printf("and can be used to set the function, pulls and value of a GPIO.\n");
    printf("%s must be run as root.\n", name);
    printf("Use:\n");
    printf("  %s get [GPIO]\n", name);
    printf("OR\n");
    printf("  %s set <GPIO> [options]\n", name);
    printf("OR\n");
    printf("  %s funcs [GPIO]\n", name);
    printf("OR\n");
    printf("  %s raw\n", name);
    printf("\n");
    printf("GPIO is a comma-separated list of pin numbers or ranges (without spaces),\n");
    printf("e.g. 4 or 18-21 or 7,9-11\n");
    printf("Note that omitting [GPIO] from %s get prints all GPIOs.\n", name);
    printf("%s funcs will dump all the possible GPIO alt funcions in CSV format\n", name);
    printf("or if [GPIO] is specified the alternate funcs just for that specific GPIO.\n");
    printf("Valid [options] for %s set are:\n", name);
    printf("  ip      set GPIO as input\n");
    printf("  op      set GPIO as output\n");
    printf("  a0-a5   set GPIO to alternate function alt0-alt5\n");
    printf("  pu      set GPIO in-pad pull up\n");
    printf("  pd      set GPIO pin-pad pull down\n");
    printf("  pn      set GPIO pull none (no pull)\n");
    printf("  dh      set GPIO to drive to high (1) level (only valid if set to be an output)\n");
    printf("  dl      set GPIO to drive low (0) level (only valid if set to be an output)\n");
    printf("Examples:\n");
    printf("  %s get              Prints state of all GPIOs one per line\n", name);
    printf("  %s get 20           Prints state of GPIO20\n", name);
    printf("  %s get 20,21        Prints state of GPIO20 and GPIO21\n", name);
    printf("  %s set 20 a5        Set GPIO20 to ALT5 function (GPCLK0)\n", name);
    printf("  %s set 20 pu        Enable GPIO20 ~50k in-pad pull up\n", name);
    printf("  %s set 20 pd        Enable GPIO20 ~50k in-pad pull down\n", name);
    printf("  %s set 20 op        Set GPIO20 to be an output\n", name);
    printf("  %s set 20 dl        Set GPIO20 to output low/zero (must already be set as an output)\n", name);
    printf("  %s set 20 ip pd     Set GPIO20 to input with pull down\n", name);
    printf("  %s set 35 a0 pu     Set GPIO35 to ALT0 function (SPI_CE1_N) with pull up\n", name);
    printf("  %s set 20 op pn dh  Set GPIO20 to ouput with no pull and driving high\n", name);
}

/*
 * type:
 *   0 = no pull
 *   1 = pull down
 *   2 = pull up
 */
int gpio_set_pull(int gpio, int type)
{
    if (gpio < GPIO_MIN || gpio > GPIO_MAX) return -1;
    if (type < 0 || type > 2) return -1;

    if (is_2711)
    {
        int pullreg = GPPUPPDN0 + (gpio>>4);
        int pullshift = (gpio & 0xf) << 1;
        unsigned int pullbits;
        unsigned int pull;

        switch (type)
        {
        case PULL_NONE:
            pull = 0;
            break;
        case PULL_UP:
            pull = 1;
            break;
        case PULL_DOWN:
            pull = 2;
            break;
        default:
            return 1; /* An illegal value */
        }

        pullbits = *(gpio_base + pullreg);
        pullbits &= ~(3 << pullshift);
        pullbits |= (pull << pullshift);
        *(gpio_base + pullreg) = pullbits;
    }
    else
    {
        int clkreg = GPPUDCLK0 + (gpio>>5);
        int clkbit = 1 << (gpio & 0x1f);

        *(gpio_base + GPPUD) = type;
        delay_us(10);
        *(gpio_base + clkreg) = clkbit;
        delay_us(10);
        *(gpio_base + GPPUD) = 0;
        delay_us(10);
        *(gpio_base + clkreg) = 0;
        delay_us(10);
    }

    return 0;
}

int get_gpio_pull(int pinnum)
{
    int pull = PULL_UNSET;
    if (is_2711)
    {
        int pull_bits = (*(gpio_base + GPPUPPDN0 + (pinnum >> 4)) >> ((pinnum & 0xf)<<1)) & 0x3;
        switch (pull_bits)
        {
        case 0:
            pull = PULL_NONE;
            break;
        case 1:
            pull = PULL_UP;
            break;
        case 2:
            pull = PULL_DOWN;
            break;
        default:
            pull = PULL_UNSET;
            break; /* An illegal value */
        }
    }
    return pull;
}

int gpio_get(int pinnum)
{
    char name[512];
    char pullstr[12];
    int level;
    int fsel;
    int pull;
    int n;

    fsel = get_gpio_fsel(pinnum);
    gpio_fsel_to_namestr(pinnum, fsel, name);
    level = get_gpio_level(pinnum);
    pullstr[0] = '\0';
    pull = get_gpio_pull(pinnum);
    if (pull != PULL_UNSET)
        sprintf(pullstr, " pull=%s", gpio_pull_names[pull & 3]);
    if (fsel < 2)
        printf("GPIO %d: level=%d fsel=%d func=%s%s\n",
               pinnum, level, fsel, name, pullstr);
    else
        printf("GPIO %d: level=%d fsel=%d alt=%s func=%s%s\n",
               pinnum, level, fsel, gpio_fsel_alts[fsel], name, pullstr);
    return 0;
}

int gpio_set(int pinnum, int fsparam, int drive, int pull)
{
    /* set function */
    if (fsparam != FUNC_UNSET)
        set_gpio_fsel(pinnum, fsparam);

    /* set output value (check pin is output first) */
    if (drive != DRIVE_UNSET)
    {
        if (get_gpio_fsel(pinnum) == 1)
        {
            set_gpio_value(pinnum, drive);
        }
        else
        {
            printf("Can't set pin value, not an output\n");
            return 1;
        }
    }

    /* set pulls */
    if (pull != PULL_UNSET)
        return gpio_set_pull(pinnum, pull);

    return 0;
}


int gpio_init()
{

    uint32_t hwbase = get_hwbase();
    is_2711 = (hwbase == 0xfe000000);
    printf("[info_gpio_init]: Is_raspi_4B %d\n",is_2711);
    /*check for /dev/gpiomem,else we need root access to /dev/mem*/
    if((memfd = open("/dev/gpiomem",O_RDWR | O_SYNC | O_CLOEXEC)) >=0 )
    {
        gpio_base = (uint32_t *)mmap(0,BLOCK_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED,memfd,0);
        printf("[info_gpio_init]:gpio_base %x\n",gpio_base);

    }
    else
    {
        if(geteuid())
        {
            printf("Must be root\n");
            return 0;
        }
        if(!hwbase) return 1;

        if ((memfd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) >= 0)
        {
            printf("Unable to open /dev/mem: %s\n", strerror (errno));
            return 1;
        }

        gpio_base = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, GPIO_BASE_OFFSET+hwbase);
        printf("info_gpio_init: gpio_base %x\n",gpio_base);
    }   
    if(gpio_base == (uint32_t *)-1)
    {
        printf("mmap(GPIO) is failed? :%s\n",strerror(errno));
    }
}
/*init the pwm mem*/
/**************************
 *非root环境只能使用gpiomem
 *要想或得完整的外设情况必须要 
 *root环境 
 *************************/

/*! On all recent OSs, the base of the peripherals is read from a /proc file */
#define BMC2835_RPI2_DT_FILENAME "/proc/device-tree/soc/ranges"

#define BCM2835_RPI4_BASE 0xFE000000
off_t bcm2835_peripherals_base = 0x20000000;
size_t bcm2835_peripherals_size = 0x01000000;
uint32_t *bcm_peripherals;
/* Map 'size' bytes starting at 'off' in file 'fd' to memory.
// Return mapped address on success, MAP_FAILED otherwise.
// On error print message.
*/
static void *mapmem(const char *msg, size_t size, int fd, off_t off)
{
    void *map = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, off);
    if (map == MAP_FAILED)
	fprintf(stderr, "bcm2835_init: %s mmap failed: %s\n", msg, strerror(errno));
    return map;
}

void PWM_init()
{
    FILE *fp;
    if ((fp = fopen(BMC2835_RPI2_DT_FILENAME, "rb")))
    {
        unsigned char buf[16];
        uint32_t base_address;
        uint32_t peri_size;
        if (fread(buf, 1, sizeof(buf), fp) >= 8)
        {
            base_address = (buf[4] << 24) |
              (buf[5] << 16) |
              (buf[6] << 8) |
              (buf[7] << 0);
            
            peri_size = (buf[8] << 24) |
              (buf[9] << 16) |
              (buf[10] << 8) |
              (buf[11] << 0);
            
            if (!base_address)
            {
                /* looks like RPI 4 */
                base_address = (buf[8] << 24) |
                      (buf[9] << 16) |
                      (buf[10] << 8) |
                      (buf[11] << 0);
                      
                peri_size = (buf[12] << 24) |
                (buf[13] << 16) |
                (buf[14] << 8) |
                (buf[15] << 0);
            }
            /* check for valid known range formats */
            if ((buf[0] == 0x7e) &&
                    (buf[1] == 0x00) &&
                    (buf[2] == 0x00) &&
                    (buf[3] == 0x00) &&
                    ((base_address == 0x20000000) || (base_address == 0x3F000000) || (base_address == BCM2835_RPI4_BASE)))
            {
                bcm2835_peripherals_base = (off_t)base_address;
                bcm2835_peripherals_size = (size_t)peri_size;
                if( base_address == BCM2835_RPI4_BASE )
                {
                    printf("[info]:device is Rasberry Pi-4\n");
                }
            }
        
        }
        
	fclose(fp);
    }
    if(getuid() != 0)
    {
        printf("[info]:error must be root\n");
        return;
    }
    /*open the master /dev/mem device */
    static int basefd=-1;
    if((basefd = open("/dev/mem",O_RDWR|O_SYNC))<0)
    {
        printf("[info]:error while open /dev/mem\n");
        return;        
    }
    bcm_peripherals = mapmem("gpio",
                                bcm2835_peripherals_size,
                                basefd,
                                bcm2835_peripherals_base);
    if(bcm_peripherals == MAP_FAILED) 
    {
        printf("[info]:error while mmap the VM \n");
        return;
    }
    printf("[info]: bcm_perpherals is %x\n",bcm_peripherals);
    printf("[info]:bcm gpio base is %x\n",(bcm_peripherals+0x200000/4));
}
/*PWM clock setting*/
void PWM_set_clock(uint32_t divisor)
{
    if(bcm_peripherals== MAP_FAILED) 
    { 
        printf("[info]:run the PWM_init() first \n");
        return;
    }
    divisor &=0xfff;
    /* stop PWM clock */
    static volatile uint32_t *bcm_clk ;
    bcm_clk = bcm_peripherals + CLOCK_BASE/4;
   // printf("[info]:debug bcm_clk&bcm_pwm %x",bcm_clk);

    *(bcm_clk+PWM_CLK_CNTL)=(uint32_t)(PWM_PASSWRD|0x01);
    /*prevents clock going slow*/
    delay_ms(110);
    /*wait for the clock to be not busy*/
    while(((*(bcm_clk+PWM_CLK_CNTL))&0x80) !=0)
    delay_ms(1);
    /*set the clock divider & enable the PWM clock*/
    *(bcm_clk+PWM_CLK_DIV)=(PWM_PASSWRD|(divisor<<12));
    *(bcm_clk+PWM_CLK_CNTL)=(PWM_PASSWRD|0x11);
    //source= osc &enabel
}
/*PWM set mode*/
void PWM_set_mode(uint8_t channel,uint8_t markspace,
                  uint8_t enable)
{
    if(bcm_peripherals == MAP_FAILED)
    {
        printf("[info]:run the pwm_init() first \n");
        return;
    }
   // uint32_t *bcm_clk = gpio_base +CLOCK_BASE;
    static volatile uint32_t *bcm_pwm; 
    bcm_pwm = bcm_peripherals + GPIO_PWM_BASE/4;
    
  //  printf("[info]:the bcm_pwm is %x\n",bcm_pwm);
    
    uint32_t control;
    control = *(bcm_pwm+PWM_CTL);
  //  printf("[info]:%x \n ",control);
    if(channel == 0)
    {
        if(markspace) control |=0x0080;
        else control &=~(0x0080);
        if(enable) control |=0x0001;
        else control &=~(0x0001);
    }
    else if(channel==1)
    {
        if(markspace) control |=0x8000;
        else control &=~(0x8000);
        if(enable) control |=0x0100;
        else control &=~(0x0100);
    }
    /*Write the CTL register*/
    *(bcm_pwm+PWM_CTL)=control;
  //  printf("[info]:control is %x\n",*(bcm_pwm+PWM_CTL));
}

void PWM_set_range(uint8_t channel,uint32_t range)
{
    if(bcm_peripherals == MAP_FAILED)
    {
        printf("[info]: run the pwm_init() first\n");
        return;
    }
    static volatile uint32_t *bcm_pwm;
    bcm_pwm = bcm_peripherals + GPIO_PWM_BASE/4;
    if(channel == 0) *(bcm_pwm+PWM0_RANGE)=range;
    else if (channel ==1) *(bcm_pwm+PWM1_RANFE)=range;

}

void PWM_set_data(uint8_t channel,uint32_t data)
{
    if(bcm_peripherals == MAP_FAILED)
    {
        printf("[info]: run the pwm_init() first\n");
        return;
    }
    static volatile uint32_t *bcm_pwm;
    bcm_pwm = bcm_peripherals + GPIO_PWM_BASE/4;
    if(channel == 0) *(bcm_pwm+PWM0_DATA)=data;
    else if (channel ==1) *(bcm_pwm+PWM1_DATA)=data;

}
void bcm_set_gpio_fsel(uint8_t pin,uint8_t mode)
{
    static volatile uint32_t *gpio_base_addr;
    gpio_base_addr = bcm_peripherals+(0x200000)/4;
    static volatile uint32_t *temp;
    uint32_t reg= pin/10;
    uint8_t shift =(pin%10)*3;
    uint32_t mask = 0x7<<shift;
  //  uint32_t value = mode << shift;
   // __sync_synchronize();
    temp = gpio_base_addr+reg;
   // __sync_synchronize();

    *temp=(*temp&~mask);
    *temp=*temp|(mode&0x7)<<shift;
  //  __sync_synchronize();

  //  __sync_synchronize();

}