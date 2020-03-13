#ifndef _RASPI_IO_H_
#define _RASPI_IO_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#define DRIVE_UNSET -1
#define DRIVE_LOW    0
#define DRIVE_HIGH   1

#define PULL_UNSET  -1
#define PULL_NONE    0
#define PULL_DOWN    1
#define PULL_UP      2

//GPFSEL的值
#define FUNC_UNSET  -1
#define FUNC_IP      0
#define FUNC_OP      1
#define FUNC_A0      4
#define FUNC_A1      5
#define FUNC_A2      6
#define FUNC_A3      7
#define FUNC_A4      3
#define FUNC_A5      2

#define GPIO_MIN     0
#define GPIO_MAX     53


//读取虚拟地址映射到真实地址
int gpio_init();
//延时
void delay_us(uint32_t delay);
void delay_ms(uint32_t msdelay);
//通过读取GPLEV0&GPLEV1来获取电平高低
int get_gpio_level(int gpio);
//通过设置GPFSEL来设置ＩＯ功能
int set_gpio_fsel(int gpio,int fsel);
//通过设置GPSET来设置ＩＯ输出电平
int set_gpio_value(int gpio,int value);
//通过设置GPPUD&GPPUDCLK来设置上拉下拉
//type 0 nopull
//     1 pull down
//     2 pull up
int gpio_set_pull(int gpio, int type);
//int gpio_set(int pinnum,int fsparam,int drive,int pull);
/*pwm functions*/
void PWM_init();
void PWM_set_clock(uint32_t divisor);
void PWM_set_mode(uint8_t channel,uint8_t markspace,
                  uint8_t enable);
void PWM_set_range(uint8_t channel,uint32_t range);
void PWM_set_data(uint8_t channel,uint32_t data);

//test
void bcm_set_gpio_fsel(uint8_t pin,uint8_t mode);
#endif /*_RASPI_IO_H_*/