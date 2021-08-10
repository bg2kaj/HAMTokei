#include <REGX52.H>
#include "DS1302.h"
#include <intrins.h>

sbit RST=P3^5;
sbit IO=P3^6;
sbit SCK=P3^4;


//DS1302初始化函数
void ds1302_init(void) 
{
	RST=0;			//RST脚置低
	SCK=0;			//SCK脚置低
}
//向DS1302写入一字节数据
void ds1302_write_byte(unsigned char addr, unsigned char d) 
{
	unsigned char i;
	RST=1;					//启动DS1302总线	
	//写入目标地址：addr
	addr = addr & 0xFE;   //最低位置零，寄存器0位为0时写，为1时读
	for (i = 0; i < 8; i ++) {
		if (addr & 0x01) {
			IO=1;
			}
		else {
			IO=0;
			}
		SCK=1;      //产生时钟
		SCK=0;
		addr = addr >> 1;
		}	
	//写入数据：d
	for (i = 0; i < 8; i ++) {
		if (d & 0x01) {
			IO=1;
			}
		else {
			IO=0;
			}
		SCK=1;    //产生时钟
		SCK=0;
		d = d >> 1;
		}
	RST=0;		//停止DS1302总线
}

//从DS1302读出一字节数据
unsigned char ds1302_read_byte(unsigned char addr) {

	unsigned char i,temp;	
	RST=1;					//启动DS1302总线
	//写入目标地址：addr
	addr = addr | 0x01;    //最低位置高，寄存器0位为0时写，为1时读
	for (i = 0; i < 8; i ++) {
		if (addr & 0x01) {
			IO=1;
			}
		else {
			IO=0;
			}
		SCK=1;
		SCK=0;
		addr = addr >> 1;
		}	
	//输出数据：temp
	for (i = 0; i < 8; i ++) {
		temp = temp >> 1;
		if (IO) {
			temp |= 0x80;
			}
		else {
			temp &= 0x7F;
			}
		SCK=1;
		SCK=0;
		}	
	RST=0;					//停止DS1302总线
	return temp;
}

