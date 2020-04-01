#include "raspi_ov7670.h"
#include "stdio.h"
extern "C"
{
#include "raspi_io.h"   
}
#define SCCB_SIOD 19 //sda
#define SCCB_SIOC 26 //scl

#define SCCB_DELAY 100
#define SCCB_MIN 50

#define SCCB_SIOD_H set_gpio_value(SCCB_SIOD,1)
#define SCCB_SIOC_H set_gpio_value(SCCB_SIOC,1)
#define SCCB_SIOD_L set_gpio_value(SCCB_SIOD,0)
#define SCCB_SIOC_L set_gpio_value(SCCB_SIOC,0)

#define SCCB_SIOD_IN  set_gpio_fsel(SCCB_SIOD,FUNC_IP)
#define SCCB_SIOD_OUT set_gpio_fsel(SCCB_SIOD,FUNC_OP)

#define SCCB_SIOD_STATE get_gpio_level(SCCB_SIOD)
static int DataLine[8]={
    17,27,22,10,9,11,0,5
};

/*定义SCCB　XCLK*/

#define SCCB_XCLK_PIN 18
#define PLCK_PIN 1

#define PWDN 20
#define RESET 21

/*定义VSYN*/
#define VSYN_PIN 16
/*定义HREF*/
#define HREF_PIN 23
struct regval_list{
	uint8_t reg_num;
	uint8_t value;
};
static const struct regval_list vga_ov7670[]= {
	{REG_HREF,0xF6},	// was B6  
	{0x17,0x13},		// HSTART
	{0x18,0x01},		// HSTOP
	{0x19,0x02},		// VSTART
	{0x1a,0x7a},		// VSTOP
	{REG_VREF,0x0a},	// VREF
	{0xff, 0xff},		/* END MARKER */
};
static const struct regval_list qvga_ov7670[]= {
	{REG_COM14, 0x19},
	{0x72, 0x11},
	{0x73, 0xf1},
	{REG_HSTART,0x16},
	{REG_HSTOP,0x04},
	{REG_HREF,0x24},
	{REG_VSTART,0x02},
	{REG_VSTOP,0x7a},
	{REG_VREF,0x0a},
	{0xff, 0xff},	/* END MARKER */
};
static const struct regval_list qqvga_ov7670[]  = {
	{REG_COM14, 0x1a},	// divide by 4
	{0x72, 0x22},		// downsample by 4
	{0x73, 0xf2},		// divide by 4
	{REG_HSTART,0x16},
	{REG_HSTOP,0x04},
	{REG_HREF,0xa4},		   
	{REG_VSTART,0x02},
	{REG_VSTOP,0x7a},
	{REG_VREF,0x0a},
	{0xff, 0xff},	/* END MARKER */
};
static const struct regval_list yuv422_ov7670[] = {
	{REG_COM7, 0x0},	/* Selects YUV mode */
	{REG_RGB444, 0},	/* No RGB444 please */
	{REG_COM1, 0},
	{REG_COM15, COM15_R00FF},
	{REG_COM9, 0x6A},	/* 128x gain ceiling; 0x8 is reserved bit */
	{0x4f, 0x80},		/* "matrix coefficient 1" */
	{0x50, 0x80},		/* "matrix coefficient 2" */
	{0x51, 0},		/* vb */
	{0x52, 0x22},		/* "matrix coefficient 4" */
	{0x53, 0x5e},		/* "matrix coefficient 5" */
	{0x54, 0x80},		/* "matrix coefficient 6" */
	{REG_COM13,/*COM13_GAMMA|*/COM13_UVSAT},
	{0xff, 0xff},		/* END MARKER */
};
static const struct regval_list rgb565_ov7670[] = {
	{REG_COM7, COM7_RGB}, /* Selects RGB mode */
	{REG_RGB444, 0},	  /* No RGB444 please */
	{REG_COM1, 0x0},
	{REG_COM15, COM15_RGB565|COM15_R00FF},
	{REG_COM9, 0x6A},	 /* 128x gain ceiling; 0x8 is reserved bit */
	{0x4f, 0xb3},		 /* "matrix coefficient 1" */
	{0x50, 0xb3},		 /* "matrix coefficient 2" */
	{0x51, 0},		 /* vb */
	{0x52, 0x3d},		 /* "matrix coefficient 4" */
	{0x53, 0xa7},		 /* "matrix coefficient 5" */
	{0x54, 0xe4},		 /* "matrix coefficient 6" */
	{REG_COM13, /*COM13_GAMMA|*/COM13_UVSAT},
	{0xff, 0xff},	/* END MARKER */
};
static const struct regval_list bayerRGB_ov7670[] = {
	{REG_COM7, COM7_BAYER},
	{REG_COM13, 0x08}, /* No gamma, magic rsvd bit */
	{REG_COM16, 0x3d}, /* Edge enhancement, denoise */
	{REG_REG76, 0xe1}, /* Pix correction, magic rsvd */
	{0xff, 0xff},	/* END MARKER */
};
static const struct regval_list ov7670_default_regs[] = {//from the linux driver
	{REG_COM7, COM7_RESET},
	{REG_TSLB,  0x04},	/* OV */
	{REG_COM7, 0},	/* VGA */
	/*
	 * Set the hardware window.  These values from OV don't entirely
	 * make sense - hstop is less than hstart.  But they work...
	 */
	{REG_HSTART, 0x13},	{REG_HSTOP, 0x01},
	{REG_HREF, 0xb6},	{REG_VSTART, 0x02},
	{REG_VSTOP, 0x7a},	{REG_VREF, 0x0a},

	{REG_COM3, 0},	{REG_COM14, 0},
	/* Mystery scaling numbers */
	{0x70, 0x3a},		{0x71, 0x35},
	{0x72, 0x11},		{0x73, 0xf0},
	{0xa2,/* 0x02 changed to 1*/1},{REG_COM10, COM10_VS_NEG},
	/* Gamma curve values */
	{0x7a, 0x20},		{0x7b, 0x10},
	{0x7c, 0x1e},		{0x7d, 0x35},
	{0x7e, 0x5a},		{0x7f, 0x69},
	{0x80, 0x76},		{0x81, 0x80},
	{0x82, 0x88},		{0x83, 0x8f},
	{0x84, 0x96},		{0x85, 0xa3},
	{0x86, 0xaf},		{0x87, 0xc4},
	{0x88, 0xd7},		{0x89, 0xe8},
	/* AGC and AEC parameters.  Note we start by disabling those features,
	   then turn them only after tweaking the values. */
	{REG_COM8, COM8_FASTAEC | COM8_AECSTEP},
	{REG_GAIN, 0},	{REG_AECH, 0},
	{REG_COM4, 0x40}, /* magic reserved bit */
	{REG_COM9, 0x18}, /* 4x gain + magic rsvd bit */
	{REG_BD50MAX, 0x05},	{REG_BD60MAX, 0x07},
	{REG_AEW, 0x95},	{REG_AEB, 0x33},
	{REG_VPT, 0xe3},	{REG_HAECC1, 0x78},
	{REG_HAECC2, 0x68},	{0xa1, 0x03}, /* magic */
	{REG_HAECC3, 0xd8},	{REG_HAECC4, 0xd8},
	{REG_HAECC5, 0xf0},	{REG_HAECC6, 0x90},
	{REG_HAECC7, 0x94},
	{REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_AGC|COM8_AEC},
	{0x30,0},{0x31,0},//disable some delays
	/* Almost all of these are magic "reserved" values.  */
	{REG_COM5, 0x61},	{REG_COM6, 0x4b},
	{0x16, 0x02},		{REG_MVFP, 0x07},
	{0x21, 0x02},		{0x22, 0x91},
	{0x29, 0x07},		{0x33, 0x0b},
	{0x35, 0x0b},		{0x37, 0x1d},
	{0x38, 0x71},		{0x39, 0x2a},
	{REG_COM12, 0x78},	{0x4d, 0x40},
	{0x4e, 0x20},		{REG_GFIX, 0},
	/*{0x6b, 0x4a},*/		{0x74,0x10},
	{0x8d, 0x4f},		{0x8e, 0},
	{0x8f, 0},		{0x90, 0},
	{0x91, 0},		{0x96, 0},
	{0x9a, 0},		{0xb0, 0x84},
	{0xb1, 0x0c},		{0xb2, 0x0e},
	{0xb3, 0x82},		{0xb8, 0x0a},

	/* More reserved magic, some of which tweaks white balance */
	{0x43, 0x0a},		{0x44, 0xf0},
	{0x45, 0x34},		{0x46, 0x58},
	{0x47, 0x28},		{0x48, 0x3a},
	{0x59, 0x88},		{0x5a, 0x88},
	{0x5b, 0x44},		{0x5c, 0x67},
	{0x5d, 0x49},		{0x5e, 0x0e},
	{0x6c, 0x0a},		{0x6d, 0x55},
	{0x6e, 0x11},		{0x6f, 0x9e}, /* it was 0x9F "9e for advance AWB" */
	{0x6a, 0x40},		{REG_BLUE, 0x40},
	{REG_RED, 0x60},
	{REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_AGC|COM8_AEC|COM8_AWB},

	/* Matrix coefficients */
	{0x4f, 0x80},		{0x50, 0x80},
	{0x51, 0},		{0x52, 0x22},
	{0x53, 0x5e},		{0x54, 0x80},
	{0x58, 0x9e},

	{REG_COM16, COM16_AWBGAIN},	{REG_EDGE, 0},
	{0x75, 0x05},		{REG_REG76, 0xe1},
	{0x4c, 0},		{0x77, 0x01},
	{REG_COM13, /*0xc3*/0x48},	{0x4b, 0x09},
	{0xc9, 0x60},		/*{REG_COM16, 0x38},*/
	{0x56, 0x40},

	{0x34, 0x11},		{REG_COM11, COM11_EXP|COM11_HZAUTO},
	{0xa4, 0x82/*Was 0x88*/},		{0x96, 0},
	{0x97, 0x30},		{0x98, 0x20},
	{0x99, 0x30},		{0x9a, 0x84},
	{0x9b, 0x29},		{0x9c, 0x03},
	{0x9d, 0x4c},		{0x9e, 0x3f},
	{0x78, 0x04},

	/* Extra-weird stuff.  Some sort of multiplexor register */
	{0x79, 0x01},		{0xc8, 0xf0},
	{0x79, 0x0f},		{0xc8, 0x00},
	{0x79, 0x10},		{0xc8, 0x7e},
	{0x79, 0x0a},		{0xc8, 0x80},
	{0x79, 0x0b},		{0xc8, 0x01},
	{0x79, 0x0c},		{0xc8, 0x0f},
	{0x79, 0x0d},		{0xc8, 0x20},
	{0x79, 0x09},		{0xc8, 0x80},
	{0x79, 0x02},		{0xc8, 0xc0},
	{0x79, 0x03},		{0xc8, 0x40},
	{0x79, 0x05},		{0xc8, 0x30},
	{0x79, 0x26},

	{0xff, 0xff},	/* END MARKER */
};
//数据总线初始化
void SCCB_GPIO_init()
{
    set_gpio_fsel(SCCB_SIOC,FUNC_OP);
    set_gpio_fsel(SCCB_SIOD,FUNC_OP);
    gpio_set_pull(SCCB_SIOC,PULL_UP);
    gpio_set_pull(SCCB_SIOD,PULL_UP);
}
/*
--------------------
    功能：start命令
    参数：无
    返回值：无
--------------------
*/
void Start_SCCB()
{
    SCCB_SIOD_H;
    delay_us(SCCB_DELAY);
    SCCB_SIOC_H;
    delay_us(SCCB_DELAY);
    SCCB_SIOD_L;
    delay_us(SCCB_DELAY);
    SCCB_SIOC_L;
    delay_us(SCCB_DELAY);

}
/*
--------------------
    功能：stop命令
    参数：无
    返回值：无
--------------------
*/
void Stop_SCCB()
{
    SCCB_SIOD_L;
    delay_us(SCCB_DELAY);
    SCCB_SIOC_H;
    delay_us(SCCB_DELAY);
    SCCB_SIOD_H;
    delay_us(SCCB_DELAY);
}

/*
--------------------
    功能：noACK用于连续读取中最后一个结束周期
    参数：无
    返回值：无
--------------------
*/
void NoAck()
{
    SCCB_SIOD_H;
    delay_us(SCCB_DELAY);

    SCCB_SIOC_H;
    delay_us(SCCB_DELAY);

    SCCB_SIOC_L;
    delay_us(SCCB_DELAY);

    SCCB_SIOD_L;
    delay_us(SCCB_DELAY);

}
/*
------------------
    功能：写入一个字节到SCBB
    参数：写入数据mdate
    返回值：发送成功返回１　失败返回０
-------------------
*/
uint8_t SCCBWriteB(uint8_t m_data)
{
    uint8_t j,temp;
    SCCB_SIOD_OUT;
    for(j=0;j<8;j++)
    {
        if((m_data<<j)&0x80)
        {
            SCCB_SIOD_H;
        }
        else
        {
            SCCB_SIOD_L;
        }
        delay_us(SCCB_DELAY);
        SCCB_SIOC_H;
        delay_us(SCCB_DELAY);
        SCCB_SIOC_L;
        delay_us(SCCB_DELAY);
    }
    delay_us(SCCB_DELAY);
    SCCB_SIOD_IN;//设置ＳＤＡ为输入
    delay_us(SCCB_DELAY);
    SCCB_SIOC_H;
    delay_us(SCCB_DELAY);
  /*  printf("[info]:this value of SCCB_SIOD_STATE %d\n",
    SCCB_SIOD_STATE);
    */
    if(SCCB_SIOD_STATE) temp=0;//SDA发送失败
    else temp=1;//ＳＤＡ发送成功
    SCCB_SIOC_L;
    delay_us(SCCB_DELAY);
    SCCB_SIOD_OUT;

    return temp;

}
/*
-----------------------------------------------
   功能: 一个字节数据读取并且返回
   参数: 无
 返回值: 读取到的数据
-----------------------------------------------
*/
uint8_t SCCBReadB(void)
{
    uint8_t read,j;
    read=0x00;

    SCCB_SIOD_IN;
    delay_us(SCCB_DELAY);
    for(j=8;j>0;j--)
    {
        delay_us(SCCB_DELAY);
        SCCB_SIOC_H;
        delay_us(SCCB_DELAY);
        read=read<<1;
        if(SCCB_SIOD_STATE)
        {
            read=read+1;
        }
        SCCB_SIOC_L;
        delay_us(SCCB_DELAY);
    }
    SCCB_SIOD_OUT;
    return (read);
}
/****************
 *功能：提供OV7670的时钟信号 
 *     信号输出方式PWM %50占空比 1.2*16/2 Mhz GPIO18
 *参数：无
 *返回值：无 
 ****************/
void XCLK_Init_On()
{
    PWM_init();
    set_gpio_fsel(SCCB_XCLK_PIN,FUNC_A5);
    delay_ms(10);
    PWM_set_clock(2);
    PWM_set_mode(0,1,1);
    PWM_set_range(0,2);
    PWM_set_data(0,2>>1);  
   // PWM_set_data(0,100);
}
/********************
 * 功能：关闭时钟信号
 * 参数：无
 * 返回值：无
 *********************/
void XCLK_Init_Off()
{
    set_gpio_fsel(SCCB_XCLK_PIN,FUNC_OP);
    set_gpio_value(SCCB_XCLK_PIN,0);
} 
/***************
 * 功能设置XCL_line为高电平
 * 参数：无 
 * 返回值：无
 ****************/
void XCLK_H()
{
    set_gpio_value(SCCB_XCLK_PIN,1);
}

void XCLK_L()
{
    set_gpio_value(SCCB_XCLK_PIN,0);
}

/******************
 * 功能：初始化数据总线
 * 参数：无
 * 返回值：无
 ***************/ 
void Ov7670_Gpio_Init()
{
    for(int i=0;i<8;i++)
    {
        set_gpio_fsel(DataLine[i],FUNC_IP);
        gpio_set_pull(DataLine[i],PULL_DOWN);
    }
}
/***********************
 * 功能：写OV7670的寄存器
 * 参数：uint8_t reg uint8_t data
 * 返回值：1 success 0 failed
 ***********************/
uint8_t Write_Ov7670_Reg(uint8_t reg,uint8_t data)
{
    Start_SCCB();
    if(0==SCCBWriteB(0x42))
    {
        Stop_SCCB();
        printf("[info]:write reg fail\n");
        return 0;
    }
    delay_us(SCCB_MIN);
    if(0==SCCBWriteB(reg))
    {
        Stop_SCCB();
        return 0;
    }
    delay_us(SCCB_MIN);
    if(0==SCCBWriteB(data))
    {
        Stop_SCCB();
        return 0;
    }
    Stop_SCCB();
    return 1;
}
/**********************
 * 功能：读取OV7670的寄存器
 * 参数：uint8_t reg,uint8_t data
 * 返回值：1-success 0-fail
 ***********************/
uint8_t Read_Ov7670_Reg(uint8_t reg,uint8_t *data)
{
    //通过写操作设置寄存器
    Start_SCCB();
    if(0==SCCBWriteB(0x42))
    {
        Stop_SCCB();
        printf("[info]:Re_Ov7670_Reg fail\n");
        return 0;
    }
    delay_us(SCCB_MIN);
    if(0==SCCBWriteB(reg))
    {
        Stop_SCCB();
        return 0;
    }
    Stop_SCCB();
    delay_us(SCCB_MIN);

    //设置寄存器地址之后才是读寄存器
    Start_SCCB();
    if(0==SCCBWriteB(0x43))
    {
        Stop_SCCB();
        printf("[info:] reWrite failed\n");
        return 0;
    }
    delay_us(SCCB_MIN);
    *data = SCCBReadB();
    NoAck();
    Stop_SCCB();
    return 1;
}
static void WriteSensorRegs(const struct regval_list reglist[])
{
    const struct regval_list *next = reglist;
	for(;;){
		uint8_t reg_addr = (next->reg_num);
		uint8_t reg_val = (next->value);
		if((reg_addr==255)&&(reg_val==255))
			break;
		while(0==Write_Ov7670_Reg(reg_addr, reg_val))
        {
            printf("[error]:failed while write the reg\n");
            printf("[error]:reg is %x data is %x \n",reg_addr,reg_val);
        }
		next++;
	}
}
void All_gpio_set_Op()
{
    for(int i=0;i<27;i++)
    {
        set_gpio_fsel(i,FUNC_OP);
        delay_ms(100);
        set_gpio_value(i,0);
    }
}
void Set_PWDN_RESET_v()
{
    set_gpio_fsel(PWDN,FUNC_OP);
    set_gpio_value(PWDN,0);
    set_gpio_fsel(RESET,FUNC_OP);
    set_gpio_value(RESET,1);
}

uint8_t useVga()
{
    while(0==Write_Ov7670_Reg(REG_COM3,0))
    {
        return 0;
    }
    WriteSensorRegs(vga_ov7670);
  //  WriteSensorRegs(bayerRGB_ov7670);
    WriteSensorRegs(rgb565_ov7670);
  //  WriteSensorRegs(yuv422_ov7670);
 
   //设置内部时钟分频
    Write_Ov7670_Reg(0x11,2);
    return 1;
}

void useQvga()
{
   Write_Ov7670_Reg(REG_COM3,4);
   WriteSensorRegs(qvga_ov7670);
   //WriteSensorRegs(yuv422_ov7670);
   WriteSensorRegs(rgb565_ov7670);
   //设置内部时钟分频
   Write_Ov7670_Reg(0x11,4); 
}
uint8_t setTestMode()
{
    Write_Ov7670_Reg(0x12,0x00);    
    Write_Ov7670_Reg(0x40,0xc0);
    Write_Ov7670_Reg(0x70,0x80);
    Write_Ov7670_Reg(0x71,0x00);
    //设置手动uv值
    Write_Ov7670_Reg(0x67,0x80);
    Write_Ov7670_Reg(0x68,0x00);
    Write_Ov7670_Reg(0x3A,0x14);
    Write_Ov7670_Reg(0x3D,0x88);
  
    return 1;
}

void Set_OV7670_Use_QiFei(void)
{
	Write_Ov7670_Reg(0x8c, 0x00);
	Write_Ov7670_Reg(0x3a, 0x04);
	Write_Ov7670_Reg(0x40, 0xd0);
	Write_Ov7670_Reg(0x8c, 0x00);
	Write_Ov7670_Reg(0x12, 0x14);
	Write_Ov7670_Reg(0x32, 0x80);
	Write_Ov7670_Reg(0x17, 0x16);
	Write_Ov7670_Reg(0x18, 0x04);
	Write_Ov7670_Reg(0x19, 0x02);
	Write_Ov7670_Reg(0x1a, 0x7b);
	Write_Ov7670_Reg(0x03, 0x06);
	Write_Ov7670_Reg(0x0c, 0x04);
	Write_Ov7670_Reg(0x3e, 0x00);
	Write_Ov7670_Reg(0x70, 0x3a);
	Write_Ov7670_Reg(0x71, 0x35); 
	Write_Ov7670_Reg(0x72, 0x11); 
	Write_Ov7670_Reg(0x73, 0x00);
	Write_Ov7670_Reg(0xa2, 0x00);
	Write_Ov7670_Reg(0x11, 0xff); 
	//Write_Ov7670_Reg(0x15 , 0x31);
	Write_Ov7670_Reg(0x7a, 0x20); 
	Write_Ov7670_Reg(0x7b, 0x1c); 
	Write_Ov7670_Reg(0x7c, 0x28); 
	Write_Ov7670_Reg(0x7d, 0x3c);
	Write_Ov7670_Reg(0x7e, 0x55); 
	Write_Ov7670_Reg(0x7f, 0x68); 
	Write_Ov7670_Reg(0x80, 0x76);
	Write_Ov7670_Reg(0x81, 0x80); 
	Write_Ov7670_Reg(0x82, 0x88);
	Write_Ov7670_Reg(0x83, 0x8f);
	Write_Ov7670_Reg(0x84, 0x96); 
	Write_Ov7670_Reg(0x85, 0xa3);
	Write_Ov7670_Reg(0x86, 0xaf);
	Write_Ov7670_Reg(0x87, 0xc4); 
	Write_Ov7670_Reg(0x88, 0xd7);
	Write_Ov7670_Reg(0x89, 0xe8);
	 
	Write_Ov7670_Reg(0x32,0xb6);
	
	Write_Ov7670_Reg(0x13, 0xff); 
	Write_Ov7670_Reg(0x00, 0x00);
	Write_Ov7670_Reg(0x10, 0x00);
	Write_Ov7670_Reg(0x0d, 0x00);
	Write_Ov7670_Reg(0x14, 0x4e);
	Write_Ov7670_Reg(0xa5, 0x05);
	Write_Ov7670_Reg(0xab, 0x07); 
	Write_Ov7670_Reg(0x24, 0x75);
	Write_Ov7670_Reg(0x25, 0x63);
	Write_Ov7670_Reg(0x26, 0xA5);
	Write_Ov7670_Reg(0x9f, 0x78);
	Write_Ov7670_Reg(0xa0, 0x68);
//	Write_Ov7670_Reg(0xa1, 0x03);//0x0b,
	Write_Ov7670_Reg(0xa6, 0xdf);
	Write_Ov7670_Reg(0xa7, 0xdf);
	Write_Ov7670_Reg(0xa8, 0xf0); 
	Write_Ov7670_Reg(0xa9, 0x90); 
	Write_Ov7670_Reg(0xaa, 0x94); 
	//Write_Ov7670_Reg(0x13, 0xe5); 
	Write_Ov7670_Reg(0x0e, 0x61);
	Write_Ov7670_Reg(0x0f, 0x43);
	Write_Ov7670_Reg(0x16, 0x02); 
	Write_Ov7670_Reg(0x1e, 0x37);
	Write_Ov7670_Reg(0x21, 0x02);
	Write_Ov7670_Reg(0x22, 0x91);
	Write_Ov7670_Reg(0x29, 0x07);
	Write_Ov7670_Reg(0x33, 0x0b);
	Write_Ov7670_Reg(0x35, 0x0b);
	Write_Ov7670_Reg(0x37, 0x3f);
	Write_Ov7670_Reg(0x38, 0x01);
	Write_Ov7670_Reg(0x39, 0x00);
	Write_Ov7670_Reg(0x3c, 0x78);
	Write_Ov7670_Reg(0x4d, 0x40);
	Write_Ov7670_Reg(0x4e, 0x20);
	Write_Ov7670_Reg(0x69, 0x00);
	Write_Ov7670_Reg(0x6b, 0x4a);
	Write_Ov7670_Reg(0x74, 0x19);
	Write_Ov7670_Reg(0x8d, 0x4f);
	Write_Ov7670_Reg(0x8e, 0x00);
	Write_Ov7670_Reg(0x8f, 0x00);
	Write_Ov7670_Reg(0x90, 0x00);
	Write_Ov7670_Reg(0x91, 0x00);
	Write_Ov7670_Reg(0x92, 0x00);
	Write_Ov7670_Reg(0x96, 0x00);
	Write_Ov7670_Reg(0x9a, 0x80);
	Write_Ov7670_Reg(0xb0, 0x84);
	Write_Ov7670_Reg(0xb1, 0x0c);
	Write_Ov7670_Reg(0xb2, 0x0e);
	Write_Ov7670_Reg(0xb3, 0x82);
	Write_Ov7670_Reg(0xb8, 0x0a);
	Write_Ov7670_Reg(0x43, 0x14);
	Write_Ov7670_Reg(0x44, 0xf0);
	Write_Ov7670_Reg(0x45, 0x34);
	Write_Ov7670_Reg(0x46, 0x58);
	Write_Ov7670_Reg(0x47, 0x28);
	Write_Ov7670_Reg(0x48, 0x3a);
	
	Write_Ov7670_Reg(0x59, 0x88);
	Write_Ov7670_Reg(0x5a, 0x88);
	Write_Ov7670_Reg(0x5b, 0x44);
	Write_Ov7670_Reg(0x5c, 0x67);
	Write_Ov7670_Reg(0x5d, 0x49);
	Write_Ov7670_Reg(0x5e, 0x0e);
	
	Write_Ov7670_Reg(0x64, 0x04);
	Write_Ov7670_Reg(0x65, 0x20);
	Write_Ov7670_Reg(0x66, 0x05);

	Write_Ov7670_Reg(0x94, 0x04);
	Write_Ov7670_Reg(0x95, 0x08);	 

	Write_Ov7670_Reg(0x6c, 0x0a);
	Write_Ov7670_Reg(0x6d, 0x55);
	Write_Ov7670_Reg(0x6e, 0x11);
	Write_Ov7670_Reg(0x6f, 0x9f); 

	Write_Ov7670_Reg(0x6a, 0x40);
	Write_Ov7670_Reg(0x01, 0x40);
	Write_Ov7670_Reg(0x02, 0x40);
	
	//Write_Ov7670_Reg(0x13, 0xe7);
	Write_Ov7670_Reg(0x15, 0x00);
	Write_Ov7670_Reg(0x4f, 0x80);
	Write_Ov7670_Reg(0x50, 0x80);
	Write_Ov7670_Reg(0x51, 0x00);
	Write_Ov7670_Reg(0x52, 0x22);
	Write_Ov7670_Reg(0x53, 0x5e);
	Write_Ov7670_Reg(0x54, 0x80);
	Write_Ov7670_Reg(0x58, 0x9e);

	Write_Ov7670_Reg(0x41, 0x08);
	Write_Ov7670_Reg(0x3f, 0x00);
	Write_Ov7670_Reg(0x75, 0x05);
	Write_Ov7670_Reg(0x76, 0xe1);

	Write_Ov7670_Reg(0x4c, 0x00);
	Write_Ov7670_Reg(0x77, 0x01);
	
	Write_Ov7670_Reg(0x3d, 0xc1);
	Write_Ov7670_Reg(0x4b, 0x09);
	Write_Ov7670_Reg(0xc9, 0x60);
	//Write_Ov7670_Reg(0x41, 0x38);	
	Write_Ov7670_Reg(0x56, 0x40);
	Write_Ov7670_Reg(0x34, 0x11);
	Write_Ov7670_Reg(0x3b, 0x02);
	Write_Ov7670_Reg(0xa4, 0x89);
	
	Write_Ov7670_Reg(0x96, 0x00);
	Write_Ov7670_Reg(0x97, 0x30);
	Write_Ov7670_Reg(0x98, 0x20);
	Write_Ov7670_Reg(0x99, 0x30);
	Write_Ov7670_Reg(0x9a, 0x84);
	Write_Ov7670_Reg(0x9b, 0x29);
	Write_Ov7670_Reg(0x9c, 0x03);
	Write_Ov7670_Reg(0x9d, 0x4c);
	Write_Ov7670_Reg(0x9e, 0x3f);	

	Write_Ov7670_Reg(0x09, 0x00);
	Write_Ov7670_Reg(0x3b, 0xc2);

}
void OV7670_config_window(unsigned int startx,unsigned int starty,unsigned int width, unsigned int height)
{
	uint16_t endx;
    uint16_t endy;// "v*2"必须
	uint8_t  temp_reg1, temp_reg2;
	uint8_t temp=0;
	
	endx=(startx+width);
	endy=(starty+height+height);// "v*2"必须
    Read_Ov7670_Reg(0x03, &temp_reg1 );
	temp_reg1 &= 0xf0;
	Read_Ov7670_Reg(0x32, &temp_reg2 );
	temp_reg2 &= 0xc0;
	
	// Horizontal
	temp = temp_reg2|((endx&0x7)<<3)|(startx&0x7);
	Write_Ov7670_Reg(0x32, temp );
	temp = (startx&0x7F8)>>3;
	Write_Ov7670_Reg(0x17, temp );
	temp = (endx&0x7F8)>>3;
	Write_Ov7670_Reg(0x18, temp );
	
	// Vertical
	temp =temp_reg1|((endy&0x3)<<2)|(starty&0x3);
	Write_Ov7670_Reg(0x03, temp );
	temp = starty>>2;
	Write_Ov7670_Reg(0x19, temp );
	temp = endy>>2;
	Write_Ov7670_Reg(0x1A, temp );
}
/*********************
 * 功能：初始化摄像头
 * 参数：无
 * 返回值：1-success 0-fail
 ********************/

uint8_t Ov7670_init()
{
    uint8_t temp,i;
    uint8_t MIDH_reg;
    uint8_t PID_reg;
    printf("\n init the camer \n");
    //gpio_init();
    Ov7670_Gpio_Init();
    SCCB_GPIO_init();
    XCLK_Init_On();//输出CLK
    
    temp = 0x80;

    printf("[info] set the regiter \n");
    delay_ms(500);
  //   Set_Ov7670_Reg();
  //  Ov7670_Config_Window(272,12,320,240);//设置像素为320*240
    if(0==Write_Ov7670_Reg(0x12,temp))//Reset the camera
    {
        printf("[info] failed in Write 0x12\n");
        return 0;
    }
    Set_OV7670_Use_QiFei();
    OV7670_config_window(272,12,320,240);
    if(0==Read_Ov7670_Reg(0x0B,&MIDH_reg))
    {
        printf("[info]:failed in write ox1c\n");
        return 0;
    }
    if(0x73!=MIDH_reg)
    {
        printf("[info] MIDH_reg %x\n",MIDH_reg);
        return 0;
    }
    if(0==Read_Ov7670_Reg(0x0A,&PID_reg))
    {
        printf("[info] PID reg read failed\n");
        return 0;
    }
    printf("[info] PID reg is %X\n",PID_reg);
    delay_ms(100);
    if(0==Write_Ov7670_Reg(0x48,0x11))
    {
        printf("[error] Write test failed \n");
        return 0;
    }
    uint8_t read_Reg_Temp=0x00;
    if(0==Read_Ov7670_Reg(0x48,&read_Reg_Temp))
    {
        printf("[error]:read reg test failed\n");
        return 0;
    }
    printf("[info]:read test reg is %X\n",read_Reg_Temp);
    delay_ms(100);
/*    WriteSensorRegs(ov7670_default_regs);   
    while(0==Write_Ov7670_Reg(REG_COM10,32))
    {
        return 0;
    }
  //  useQvga();
  useVga();
  setTestMode();
*/
    return 0x01;
}
void VSYN_IO_init()
{
    set_gpio_fsel(VSYN_PIN,FUNC_IP);
    gpio_set_pull(VSYN_PIN,PULL_DOWN);
}
void PLCK_IO_init()
{
    set_gpio_fsel(PLCK_PIN,FUNC_IP);
    gpio_set_pull(PLCK_PIN,PULL_DOWN);
}
uint8_t Get_VSYN_state()
{
    return get_gpio_level(VSYN_PIN);
}
uint8_t Get_PLCK_state()
{
    return get_gpio_level(PLCK_PIN);
}
void HREF_IO_init()
{
    set_gpio_fsel(HREF_PIN,FUNC_IP);
    gpio_set_pull(HREF_PIN,PULL_DOWN);
}
uint8_t Get_HREF_state()
{
    return get_gpio_level(HREF_PIN);
}
void captureImgQvgaRgb565(uint16_t wg,uint16_t hg,FILE *fp)
{
    uint16_t lg2;
    while(!Get_VSYN_state());
    while(Get_VSYN_state());
    while(hg--){
      lg2 = wg;
      while(Get_HREF_state()){
          printf("[info]:wait href low\n");
      }
      while(!Get_HREF_state())
      {
          printf("[info]wait href high\n");
      }
      while(lg2--){
          uint8_t data_low=0;
          uint8_t data_high=0;
          uint16_t data_temp=0;
          while(Get_PLCK_state());
          for(uint8_t i=0;i<8;i++)
          {
            data_high |=(get_gpio_level(DataLine[i])<<i);
          }
          while(!Get_PLCK_state());
          while(Get_PLCK_state());
          for(uint8_t i=0;i<8;i++)
          {
            data_low |=(get_gpio_level(DataLine[i])<<i);
          }
          while(!Get_PLCK_state());
          data_temp=((uint16_t)data_high)<<8|data_low;
          fwrite(&data_temp,sizeof(data_temp),1,fp);
        //  printf("temp: %d data is %x %x %x\n",lg2,data_temp,data_high,data_low);
      }   
      while(Get_HREF_state());
      while(!Get_HREF_state());
    }
}
void captureImgQvgaYuv422(uint16_t wg,uint16_t hg,FILE *fp)
{
    uint16_t lg2;
    while(!Get_VSYN_state());
    while(Get_VSYN_state());
    while(hg--){
      lg2 = wg;
      while(Get_HREF_state()){
          printf("[info]:wait href low\n");
      }
      while(!Get_HREF_state())
      {
          printf("[info]wait href high\n");
      }
      while(lg2--){
          uint8_t data_low=0;
          while(Get_PLCK_state());
          for(uint8_t i=0;i<8;i++)
          {
            data_low |=(get_gpio_level(DataLine[i])<<i);
          }
          while(!Get_PLCK_state());
          fwrite(&data_low,sizeof(data_low),1,fp);
         // printf("temp: %d data is %x\n",lg2,data_low);
      }   
      while(Get_HREF_state());
      while(!Get_HREF_state());
    }
}
void captureImgVga(uint16_t wg,uint16_t hg,FILE *fp)
{
    uint16_t lg2;
    while(!Get_VSYN_state());
    while(Get_VSYN_state());
    while(hg--){
      lg2 = wg;
      while(Get_HREF_state()){
          printf("[info]:wait href low\n");
      }
      while(!Get_HREF_state())
      {
          printf("[info]wait href high\n");
      }
      while(lg2--){
          uint8_t data_low=0;
          while(Get_PLCK_state());
          for(uint8_t i=0;i<8;i++)
          {
            data_low |=(get_gpio_level(DataLine[i])<<i);
          }
          while(!Get_PLCK_state());
          fwrite(&data_low,sizeof(data_low),1,fp);
          printf("temp: %d data is %x\n",lg2,data_low);
      }   
    }
}
void captureImgVgaRgb565(uint16_t wg,uint16_t hg,FILE *fp)
{
    uint16_t lg2;
    while(!Get_VSYN_state());
    while(Get_VSYN_state());
    while(hg--){
      lg2 = wg;
      while(Get_HREF_state()){
          printf("[info]:wait href low\n");
      }
      while(!Get_HREF_state())
      {
          printf("[info]wait href high\n");
      }
      while(lg2--){
          uint8_t data_low=0;
          uint8_t data_high=0;
          uint16_t data_temp=0;
          while(Get_PLCK_state());
          for(uint8_t i=0;i<8;i++)
          {
            data_high |=(get_gpio_level(DataLine[i])<<i);
          }
          while(!Get_PLCK_state());
          while(Get_PLCK_state());
          for(uint8_t i=0;i<8;i++)
          {
            data_low |=(get_gpio_level(DataLine[i])<<i);
          }
          while(!Get_PLCK_state());
          data_temp=((uint16_t)data_high)<<8|data_low;
          fwrite(&data_temp,sizeof(data_temp),1,fp);
        //  printf("temp: %d data is %x %x %x\n",lg2,data_temp,data_high,data_low);
      }   
    }
}
uint16_t OV7670_BGR2RGB(uint16_t c)
{
  uint16_t  r, g, b, rgb;

  b = (c>>0)  & 0x1f;
  g = (c>>5)  & 0x3f;
  r = (c>>11) & 0x1f;
  
  rgb =  (b<<11) + (g<<5) + (r<<0);

  return( rgb );
}

void captureImgQiFei(FILE *fp)
{
    uint32_t TimerCnt = 0;
    uint16_t temp7670 = 0;
    
    XCLK_Init_On();
    while(!Get_VSYN_state()){
        printf("wait VSYNC HIGH\n");
    }
    while(Get_VSYN_state())
    {
        printf("wait VSYNC LOW\n");
    }

    XCLK_Init_Off();
   
    uint16_t value,val,val1,val2;
    while(TimerCnt<76800)
    {
        uint8_t data_low=0;
        uint8_t data_high=0;
        XCLK_L();
        XCLK_H();
        for(uint8_t i=0;i<8;i++)
        {
            data_high |=(get_gpio_level(DataLine[i])<<i);
        }
        value = (uint16_t)data_high&0x00ff;
       // printf("[info] begin to read file\n");
        temp7670++;
        if(Get_HREF_state())//如果Href为高电平
        {
            if(temp7670 == 1 )
            { 
               val1 = value&0x00ff;
            }
            else
            {
               val2 = value<<8;
               val = OV7670_BGR2RGB(val1|val2);
               temp7670=0;
               TimerCnt++;
               fwrite(&val,sizeof(val),1,fp);
               printf("[info]:%d %x\n",TimerCnt,val);               
            }
        }


    }
}
//use Rgb565 useBayerRgb
int main(int argc,char **argv)
{
    printf("[info]:system init\n");
    gpio_init();
    All_gpio_set_Op();
    Set_PWDN_RESET_v();
    VSYN_IO_init();
    PLCK_IO_init();
    HREF_IO_init();
    while(!Ov7670_init())
    {
        printf("[info]:camer init failed\n");
        delay_ms(4000);
    }
    while(1){
        printf("[info]:sys init OK\n");
        uint8_t ID=0x00;
        Read_Ov7670_Reg(0x0B,&ID);
        printf("[INFO]:ID is %X\n",ID);
        delay_ms(1000);
        if(ID==0x73) break;
    }
    delay_ms(5000);
 //   CLK_Init_Off();
 //   CLK_Init_On();//XCLK开
    static int fileNum=0; 
    int captureNum=0;
    while(1)
    {
        uint16_t value_temp;
        char file_name[1024];
        captureNum++;
        //wait for vsync
    /*    while(!Get_VSYN_state())
        {
            printf("[info]:waitting VSYN IO high\n");//wait for high
        } 
        while(Get_VSYN_state())
        {
            printf("[info]:watting VSYN IO low\n");
        };//wait for low   
    */    
     //   CLK_Init_Off();
        //滤去前２０帧
        if(captureNum>=20){
            sprintf(file_name,"data/camerData-%d",captureNum);
            FILE *fp = fopen(file_name,"wb");
            if(!fp){
                return 0;
            }
           // captureImgQvgaYuv422(320*2,240,fp);
          //  captureImgVgaRgb565(640,480,fp);
           // captureImgQvgaRgb565(320,240,fp);
            captureImgQiFei(fp);
            printf("[info] can run out\n");
           // captureNum++;
            fclose(fp); 
        }
        if(captureNum>30){
                break;
        }
    }
    return 0;
}

/*test pwm mode*/
#ifdef PWM_TEST
#define pwm_pin 12
#define pwm_channel 0
int main(int argc,char **argv)
{
    gpio_init();
    for(int i=0;i<27;i++)
    {
        set_gpio_fsel(i,FUNC_OP);
        delay_ms(100);
        set_gpio_value(i,0);
    }
    //控制GPIO12 为alt0输出 PWM0
    //set_gpio_fsel(12,FUNC_A0);
    PWM_init();
    bcm_set_gpio_fsel(pwm_pin,FUNC_A0);
    set_gpio_fsel(18,FUNC_A5);
    delay_ms(10);
    
    PWM_set_clock(16);//19.2Mhz/16=1.2Mhz

    PWM_set_mode(pwm_channel,1,1);

    
    PWM_set_range(pwm_channel,1024);//每个周期是3s
    int direction = 1; // 1 is increase, -1 is decrease
    int data = 1;
    while (1)
    {
	if (data == 1)
	    direction = 1;   // Switch to increasing
	else if (data == 1024-1)
    {
	    direction = -1;  // Switch to decreasing
        printf("code change\n");
    }
	data += direction;
	PWM_set_data(pwm_channel, data);
	delay_ms(1);
    }

}
#endif
#ifdef IO_Test
int main(int argc,char **argv)
{
    gpio_init();    
    for(int i=0;i<27;i++)
    {
        set_gpio_fsel(i,FUNC_OP);
        delay_ms(100);
        set_gpio_value(i,0);
    }
    SCCB_GPIO_init();
}
#endif 