#include <REGX52.H>
#include "LCD1602.h"
#include "DS1302.h"
#include <string.h>
#include <stdio.h>
#include "dht111.h"

#define LCD 2004   	//Choose your display LCD parameter now 1602&2004 supported
#define UTC	8 		//UNUSED

#define WARNINGHIGHTEMP 37 	//Environment temp warning limit
#define WARNINGHIGHHUMI 80  //Environment humidity warning limit. Use with temp warning
#define WARNINGLOWTEMP 5    //Environment low temp warning limit
#define GPSUPDATEINTERVAL 3600 	//Seconds to sync local time with GPS
#define GPSGRIDINTERVAL 300	 	//Seconds to sync grid locator with GPS

#define FOSC 11059200L      //System frequency
#define BAUD 9600           //UART baudrate

/*Define UART parity mode*/
#define NONE_PARITY     0   //None parity
#define ODD_PARITY      1   //Odd parity
#define EVEN_PARITY     2   //Even parity
#define MARK_PARITY     3   //Mark parity
#define SPACE_PARITY    4   //Space parity

#define PARITYBIT NONE_PARITY   //Testing even parity

typedef unsigned char BYTE;
typedef unsigned int WORD;


unsigned char time_buf[8] = {0x20,0x21,0x08,0x03,0x20,0x00,0x00,0x02};	
unsigned char dis_time_buf[20]={0};
unsigned char dis_banner_buf[20];
unsigned char test_buf[20];

unsigned char ifGPSLocked=0,ifInWarning=0,ifFlashed=0;
unsigned char *Callsign="BG2KAJ"; 		//Modify here to change your own callsign
unsigned char xdata recv_buffer[100];
unsigned char Grid[6];
int recv_counter=0,data update_counter=0,grid_counter=0,date_counter=0;
float lat=0,lon=0;

bit busy,firstPWR=1;



unsigned char hour,minute;
data int temp=25,humi=50;
extern int data temp_value, humi_value;
int comma_counter=0;

void ds1302_write_time(void);
void ds1302_read_time(void);
void SendData(BYTE dat);
void SendString(char *s);
void calcLocator(char *dst, float lat, float lon);

void main()
{
	int i;
	for(i=0;i<100;i++)
		recv_buffer[i]=0;
#if (PARITYBIT == NONE_PARITY)
    SCON = 0x50;            //8-bit variable UART
#elif (PARITYBIT == ODD_PARITY) || (PARITYBIT == EVEN_PARITY) || (PARITYBIT == MARK_PARITY)
    SCON = 0xda;            //9-bit variable UART, parity bit initial to 1
#elif (PARITYBIT == SPACE_PARITY)
    SCON = 0xd2;            //9-bit variable UART, parity bit initial to 0
#endif
	//Init serial port
    TMOD = 0x20;            //Set Timer1 as 8-bit auto reload mode
    TH1 = TL1 = -(FOSC/12/32/BAUD); //Set auto-reload vaule
    TR1 = 1;                //Timer1 start run
    ES = 1;                 //Enable UART interrupt
    EA = 1;                 //Open master interrupt switch

	Initialize_LCD();
	ClearLine(1);
	ClearLine(2);
	ds1302_init();

	if(LCD==2004)
	{
	 	PutString(1,1,"                    ");
		PutString(2,1,"    Initializing    ");
		sprintf(dis_banner_buf,"    %s          ",Callsign);
		PutString(3,1,dis_banner_buf);
		PutString(4,1,"                    ");
	}
	else if(LCD==1602)
	{
	 	PutString(1,1,"  Initializing  ");
		sprintf(dis_banner_buf,"  %s        ",Callsign);
		PutString(2,1,dis_banner_buf);
	}
	Delay_1ms(1500);
//	ds1302_write_time();
	while(1)
	{
	if(recv_counter!=0)
	{
		for(i=0;i<100;i++)
		{
			if(recv_buffer[i]==',')
				comma_counter+=1;	
		}
		if((comma_counter==12)&&(recv_buffer[0]=='$'))
		{
			//Start parse GPS data
			update_counter+=1;
			grid_counter+=1;
			//For Debug	Use
//			SendString(recv_buffer);

			//Update GPS state
			if(recv_buffer[18]=='A')
				ifGPSLocked=1;
			else
				ifGPSLocked=0;
				
			if((update_counter>GPSUPDATEINTERVAL)||(firstPWR==1))
			{
				//Update time and date
				update_counter=0;
				for(i=0;i<100;i++)
				{
					if(recv_buffer[i]==',')
						comma_counter+=1;
					if(comma_counter==9)
						date_counter=i+1;		
				}
				time_buf[1]=(recv_buffer[61]-'0')*16+(recv_buffer[62]-'0'); 			//year
				time_buf[2]=(recv_buffer[59]-'0')*16+(recv_buffer[60]-'0');			//month
				time_buf[3]=(recv_buffer[57]-'0')*16+(recv_buffer[58]-'0'); 			//day
				time_buf[4]=(recv_buffer[7]-'0')*16+(recv_buffer[8]-'0');			//time
				time_buf[5]=(recv_buffer[9]-'0')*16+(recv_buffer[10]-'0'); 			//min
				time_buf[6]=(recv_buffer[11]-'0')*16+(recv_buffer[12]-'0');			//sec
				ds1302_write_time();
			}
			
			//Grid Calculate
			if((grid_counter>GPSGRIDINTERVAL)||(firstPWR==1))
			{
				//Update time and date
				grid_counter=0;
				firstPWR=0;
				lat=(recv_buffer[20]-'0')*10+(recv_buffer[21]-'0')*1+((recv_buffer[22]-'0')*10+(recv_buffer[23]-'0')*1+(recv_buffer[25]-'0')*0.1+(recv_buffer[26]-'0')*0.01)/60;
				if(recv_buffer[31]=='S')
					lat=0-lat;

				lon=(recv_buffer[33]-'0')*100+(recv_buffer[34]-'0')*10+(recv_buffer[35]-'0')*1+((recv_buffer[36]-'0')*10+(recv_buffer[37]-'0')*1+(recv_buffer[39]-'0')*0.1+(recv_buffer[40]-'0')*0.01)/60;
				if(recv_buffer[31]=='W')
					lon=0-lon;
				calcLocator(Grid,lat,lon);
			}
					
		}
		
		comma_counter=0;
		recv_counter=0;
		date_counter=0;
		for(i=0;i<100;i++)
			recv_buffer[i]=0;
	}
	 for(i=0;i<9999;i++);
		if(LCD==2004)
		{
			if(ifGPSLocked==1)
			{
				sprintf(dis_banner_buf," %s       [GPS] ",Callsign);
				PutString(1,1,dis_banner_buf);
				sprintf(dis_banner_buf," GRID: %s       ",Grid);
				PutString(3,1,dis_banner_buf);
			}
			else
			{
				sprintf(dis_banner_buf," %s             ",Callsign);
				PutString(1,1,dis_banner_buf);
				PutString(3,1," GRID: Waiting GPS  ");
			}
			
			ds1302_read_time();
			dis_time_buf[0]=' ';
		//	dis_time_buf[1]=(time_buf[0]>>4)+'0'; //年   
			dis_time_buf[1]='2'; //年  
		//	dis_time_buf[2]=(time_buf[0]&0x0f)+'0';
			dis_time_buf[2]='0';

			dis_time_buf[3]=(time_buf[1]>>4)+'0';   
			dis_time_buf[4]=(time_buf[1]&0x0f)+'0';
		
			dis_time_buf[5]=(time_buf[2]>>4)+'0'; //月  
			dis_time_buf[6]=(time_buf[2]&0x0f)+'0';
			
			dis_time_buf[7]=(time_buf[3]>>4)+'0'; //日   
			dis_time_buf[8]=(time_buf[3]&0x0f)+'0';
		   	
			dis_time_buf[9]=' ';
		
			dis_time_buf[10]=(time_buf[4]>>4)+'0'; //时   
			dis_time_buf[11]=(time_buf[4]&0x0f)+'0';   
			dis_time_buf[12]=':';
			dis_time_buf[13]=(time_buf[5]>>4)+'0'; //分   
			dis_time_buf[14]=(time_buf[5]&0x0f)+'0';   
			dis_time_buf[15]=':';	
			dis_time_buf[16]=(time_buf[6]>>4)+'0'; //秒 
			dis_time_buf[17]=(time_buf[6]&0x0f)+'0';
			PutString(2,1,dis_time_buf); 
			
			ES=0; 
			EA=0;
			if(DHT11_ReadTempAndHumi()==OK)
			{
				temp=temp_value/10;
				humi=humi_value/10;
			}
			ES=1;
			EA=1;
			if(temp<=WARNINGLOWTEMP)
			{
				ifInWarning=1;
				sprintf(dis_banner_buf," TEMP:%d'C HUMI:%d% ",temp,humi);
				PutString(3,1,dis_banner_buf);
				sprintf(dis_banner_buf,"  ! COLD WX WARN !   ",temp,humi);
				PutString(4,1,dis_banner_buf);
			}
			else if((temp>WARNINGLOWTEMP)&&(temp<WARNINGHIGHTEMP))
			{
				ifInWarning=0;
				sprintf(dis_banner_buf," TEMP:%d'C HUMI:%d% ",temp,humi);
				PutString(4,1,dis_banner_buf);
			}
			else if((temp>=WARNINGHIGHTEMP)&&(humi<=WARNINGHIGHHUMI))
			{
				ifInWarning=1;

				sprintf(dis_banner_buf," TEMP:%d'C HUMI:%d% ",temp,humi);
				PutString(3,1,dis_banner_buf);

				sprintf(dis_banner_buf," ! HIGH TEMP WARN ! ",temp,humi);
				PutString(4,1,dis_banner_buf);
			}
			else if((temp>=WARNINGHIGHTEMP)&&(humi>=WARNINGHIGHHUMI))
			{
				ifInWarning=1;

				sprintf(dis_banner_buf," TEMP:%d'C HUMI:%d% ",temp,humi);
				PutString(3,1,dis_banner_buf);

				sprintf(dis_banner_buf," !HEATSTROKE WARN!  ",temp,humi);
				PutString(4,1,dis_banner_buf);
			}	
		}
		else if(LCD==1602)
		{
			if(ifGPSLocked==1)
				sprintf(dis_banner_buf," %s  %02d'C G ",Callsign,temp);
			else
				sprintf(dis_banner_buf," %s  %02d'C   ",Callsign,temp);
			PutString(1,1,dis_banner_buf);

			ds1302_read_time();
			if(DHT11_ReadTempAndHumi()==OK)
			{
				temp=temp_value/10;
				humi=humi_value/10;
			}
			if(temp<=WARNINGLOWTEMP)
			{
				ifInWarning=1; 
				sprintf(dis_banner_buf,"%02d'C & %02d%% HUMI ",temp,humi);
				PutString(1,1,dis_banner_buf);
				sprintf(dis_banner_buf,"! COLD WX WARN !",temp,humi);
				PutString(2,1,dis_banner_buf);
			}
			else if((temp>WARNINGLOWTEMP)&&(temp<=WARNINGHIGHTEMP))
			{
				ifInWarning=0;
				if(ifGPSLocked==1)
					sprintf(dis_banner_buf," %c%c:%c%c   %s ",(time_buf[4]>>4)+'0',(time_buf[4]&0x0f)+'0',(time_buf[5]>>4)+'0',(time_buf[5]&0x0f)+'0',Grid);
				else
					sprintf(dis_banner_buf," %c%c:%c%c   No GPS ",(time_buf[4]>>4)+'0',(time_buf[4]&0x0f)+'0',(time_buf[5]>>4)+'0',(time_buf[5]&0x0f)+'0');
				PutString(2,1,dis_banner_buf);
			}
			else if((temp>=WARNINGHIGHTEMP)&&(humi<=WARNINGHIGHHUMI))
			{
				ifInWarning=1; 
				sprintf(dis_banner_buf,"%02d'C & %02d%% HUMI ",temp,humi);
				PutString(1,1,dis_banner_buf);
				sprintf(dis_banner_buf,"!HIGH TEMP WARN!",temp,humi);
				PutString(2,1,dis_banner_buf);
			}
			else if((temp>=WARNINGHIGHTEMP)&&(humi>=WARNINGHIGHHUMI))
			{
				ifInWarning=1;
				sprintf(dis_banner_buf,"%02d'C & %02d%% HUMI ",temp,humi);
				PutString(1,1,dis_banner_buf);
				sprintf(dis_banner_buf,"HEATSTROKE WARN!",temp,humi);
				PutString(2,1,dis_banner_buf);
			}
		} 
	}

}

//向DS302写入时钟数据
void ds1302_write_time(void) 
{
	ds1302_write_byte(ds1302_control_add,0x00);			//关闭写保护 
	ds1302_write_byte(ds1302_sec_add,0x80);				//暂停时钟 
	//ds1302_write_byte(ds1302_charger_add,0xa9);	    //涓流充电 
	ds1302_write_byte(ds1302_year_add,time_buf[1]);		//年 
	ds1302_write_byte(ds1302_month_add,time_buf[2]);	//月 
	ds1302_write_byte(ds1302_date_add,time_buf[3]);		//日 
	ds1302_write_byte(ds1302_hr_add,time_buf[4]);		//时 
	ds1302_write_byte(ds1302_min_add,time_buf[5]);		//分
	ds1302_write_byte(ds1302_sec_add,time_buf[6]);		//秒
	ds1302_write_byte(ds1302_day_add,time_buf[7]);		//周 
	ds1302_write_byte(ds1302_control_add,0x80);			//打开写保护     
}
//从DS302读出时钟数据
void ds1302_read_time(void)  
{
	time_buf[1]=ds1302_read_byte(ds1302_year_add);		//年 
	time_buf[2]=ds1302_read_byte(ds1302_month_add);		//月 
	time_buf[3]=ds1302_read_byte(ds1302_date_add);		//日 
	time_buf[4]=ds1302_read_byte(ds1302_hr_add);		//时 
	time_buf[5]=ds1302_read_byte(ds1302_min_add);		//分 
	time_buf[6]=(ds1302_read_byte(ds1302_sec_add))&0x7f;//秒，屏蔽秒的第7位，避免超出59
	time_buf[7]=ds1302_read_byte(ds1302_day_add);		//周 	
}
void Uart_Isr() interrupt 4 using 1
{
    if (RI)
    {
        RI = 0;             //Clear receive interrupt flag
//        P0 = SBUF;          //P0 show UART data
//        bit9 = RB8;         //P2.2 show parity bit
		recv_buffer[recv_counter]=SBUF;
		recv_counter+=1;
    }
    if (TI)
    {
        TI = 0;             //Clear transmit interrupt flag
        busy = 0;           //Clear transmit busy flag
    }
}

/*----------------------------
Send a byte data to UART
Input: dat (data to be sent)
Output:None
----------------------------*/
void SendData(BYTE dat)
{
    while (busy);           //Wait for the completion of the previous data is sent
    ACC = dat;              //Calculate the even parity bit P (PSW.0)
    if (P)                  //Set the parity bit according to P
    {
#if (PARITYBIT == ODD_PARITY)
        TB8 = 0;            //Set parity bit to 0
#elif (PARITYBIT == EVEN_PARITY)
        TB8 = 1;            //Set parity bit to 1
#endif
    }
    else
    {
#if (PARITYBIT == ODD_PARITY)
        TB8 = 1;            //Set parity bit to 1
#elif (PARITYBIT == EVEN_PARITY)
        TB8 = 0;            //Set parity bit to 0
#endif
    }
    busy = 1;
    SBUF = ACC;             //Send data to UART buffer
}

/*----------------------------
Send a string to UART
Input: s (address of string)
Output:None
----------------------------*/
void SendString(char *s)
{
    while (*s)              //Check the end of the string
    {
        SendData(*s++);     //Send current char and increment string ptr
    }
}

void calcLocator(char *dst, float lat, float lon) {
  int o1, o2, o3;
  int a1, a2, a3;
  float remainder;
  // longitude
  remainder = lon + 180.0;
  o1 = (int)(remainder / 20.0);
  remainder = remainder - (float)o1 * 20.0;
  o2 = (int)(remainder / 2.0);
  remainder = remainder - 2.0 * (float)o2;
  o3 = (int)(12.0 * remainder);

  // latitude
  remainder = lat + 90.0;
  a1 = (int)(remainder / 10.0);
  remainder = remainder - (float)a1 * 10.0;
  a2 = (int)(remainder);
  remainder = remainder - (float)a2;
  a3 = (int)(24.0 * remainder);
  dst[0] = (char)o1 + 'A';
  dst[1] = (char)a1 + 'A';
  dst[2] = (char)o2 + '0';
  dst[3] = (char)a2 + '0';
  dst[4] = (char)o3 + 'A';
  dst[5] = (char)a3 + 'A';
  dst[6] = (char)0;
}



