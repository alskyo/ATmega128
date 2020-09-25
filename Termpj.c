//************************************************************************
//	Term Project
//  작성자 : 15010799 정민교
//	Due Date : 6/14
//************************************************************************

#define TRUE	1
#define FALSE	0
#define VREF	5.0
#define TIME	60.0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>										
#include "tpk_lcd8m.h"																								//	LCD사용을 위한 HEADER

volatile int Intflag = 0, Keyflag = 0, ADval = 0, Fndflag = 0, Pwlock = 0;
volatile int Cmd_U0 = 0, Tn = 0, Interval = 0, N_cnt = 0;
volatile float Vin = 0.0;
volatile long A_cnt = 0;
volatile char MSG[16];																								// LCD Msg
volatile char NOWACT[4][7] = {"  Up  ", " Stop ", " Down ", "Reset "};
volatile unsigned char FND[4];
volatile unsigned char SEG[16] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e};	// CA

void txd(char ch);
void txd_string(char *str);
void IntMsg(void);
							                     
int main(void)
{
	lcd_init();
	int pcmd_U0 = 0, flagbuf = 0, check = 0, tm_h = 0, tm_m = 0, tm_s = 0, a_switch = 0, lockck = 0;
	int vbuf_m = 0, vbuf_s = 0;
	long vbuf_h = 0;
	int pw1 = 0, pw2 = 0, pw3 = 0, pw4 = 0, pw5 = 0, pw6 = 0, pw7 = 0, pw8 = 0;
	unsigned char ch = 0, chbuf = 0, chck = 0;
	DDRA = 0xFF;														// PA0~PA7 -> output (FND)
	DDRB = 0x0F;														// PB0~PB3 -> output (FND)
	DDRE = 0x02;														// USRAT & SW INTERRUPT
	DDRF = 0x00;														// A/D Conv. 
	
	PORTE = 0xF0;														// SW INTERRUPT (Pull up)	PORTB = 0x00;														// FND S0~S3 사용	PORTA = 0xFF;														// FND 초기화(모두 끄기)
	
	ADMUX = 0x00;														// AD
	ADCSRA = 0x8f;
	
	EICRB = 0xAA;	EIMSK = 0xF0;														// INT4~7 인터럽트 처리

	TCCR1A = 0x00;														// Timer/Counter 1 
	TCCR1B = 0x04;														// Normal mode														
	TCCR1C = 0x00;			
	
	TCNT1 = 0xE980;
	TIMSK = 0x04;
	
	TCCR3A = 0x00;														// Timer/Counter 3
	TCCR3B = 0x0D;														// CTC mode
	TCCR3C = 0x00;
	
	OCR3A = 0x3F;
	ETIMSK = 0x10;
	UCSR0A = 0x00;														// Hyper terminal
	UCSR0B = 0x98;														// RXCIE0=1, TXEN0=1, RXEN0=1
	UCSR0C = 0x06; 														// 비동기 통신, 패리티 없음, 데이터 비트 : 8비트, 정지비트 : 1비트
	UBRR0H = 0;
	UBRR0L = 0x5F;														// fosc=14.7456MHz, BAUD=9600bps
	
	sei();
	
	IntMsg();
	lcd_display_position(1,3);															// LCD initial output	lcd_string("Term Project");	lcd_display_position(2,3);	lcd_string("by M.K.Jeong");
	
	while(1)																			// While문을 이용한 main함수 내 반복실행
	{
		if(pcmd_U0 != Cmd_U0 || flagbuf != Intflag || lockck == 1)						// 키보드 입력, 스위치 입력, Lock이 잠겨있다 풀릴경우(1회) 실행
		{
			lockck = 0;																	// Lock check flag initialization
			if((flagbuf == 0) && (Intflag != 0) && (check == 0))
			{																			// 키보드를 안누른상태에서 스위치만 누를경우
				lcd_display_clear();													// 최초 1회만 동작하는 구문이기 떄문에 clear 사용
				lcd_display_position(1,1);												
				lcd_string("Press Keyboard..");
				flagbuf = Intflag;														// 스위치 입력값을 저장해논 후 추후에 새로운 스위치가 입력됬는지 비교에 사용
			}
			else
			{
				lcd_display_position(1,1);												// 출력동작 설명 첫번째줄 지정
				if (Pwlock == 0) ch = Cmd_U0;											// 잠겨있지 않을 경우 키보드 입력 ch 변수에 저장
				txd(ch);											
				if (ch == 'l') chck = ch;												// 'l'이 들어온 경우 chck에 'l' 저장	
				else	
				{																		
					chbuf = ch;															// 'l'이 아닌경우 ch 버퍼에 ch를 저장 chck는 초기화
					chck = 0;
				}
				if (ch == 'u' || ch == 's' || ch == 'd' || ch == 'r')					// 타이머 관련 입력이 들어온 경우
				{
					check = 1;
					if (Intflag == 1) lcd_string("LCD : Timer     ");					// INT4 인터럽트 플래그에 해당하는 내용 LCD에 출력
					else if (Intflag == 2) lcd_string("FND : Timer     ");				// INT5
					else if (Intflag == 3) lcd_string("USART : Timer   ");				// INT6
					else lcd_string("ALL : Timer     ");								// INT7			
				}
				if (ch == '1' || ch == '2' || ch == '3' || ch == '4')					// ADC 관련 입력이 들어온 경우
				{
					check = 1;
					if (Intflag == 1) lcd_string("LCD : A/D Conv.  ");					// INT4
					else if (Intflag == 2) lcd_string("FND : A/D Conv.  ");				// INT5
					else if (Intflag == 3) lcd_string("USART: A/D Conv.");				// INT6
					else lcd_string("ALL : A/D Conv.  ");								// INT7
				}
				if (ch == 'a')															// 첫번째 부가기능 Analog Setting Alarm
				{
					check = 1;
					if (Intflag == 1) lcd_string("LCD :AnalogAlarm");					// INT4
					else if (Intflag == 2) lcd_string("FND :     EMPTY   ");			// INT5
					else if (Intflag == 3) lcd_string("USART :   Alarm   ");			// INT6
					else lcd_string("ALL :AnalogAlarm");								// INT7
					Interval = 0;
				}
				if (chck == 'l')														// 세번째 부가기능 Lock Atmega128(Keyboard)
				{
					check = 1;
					if (Intflag == 1) lcd_string("LCD :  L O C K  ");					// INT4
					else if (Intflag == 2) lcd_string("FND :  L O C K  ");				// INT5
					else if (Intflag == 3) lcd_string("USART : L O C K ");				// INT6
					else lcd_string("ALL :  L O C K  ");								// INT7
				}
				if (ch == 'p')															// 두번째 부가기능 Password Setting
				{
					check = 1;
					if (Intflag == 1) lcd_string("LCD : Password  ");					// INT4
					else if (Intflag == 2) lcd_string("FND : Password  ");				// INT5
					else if (Intflag == 3) lcd_string("USART : Password");				// INT6
					else lcd_string("ALL : Password  ");								// INT7
				}
				if (ch == 'u') Interval = 1, Keyflag = 0;								// Up counting (0.1s) condition		시간나면 정수형으로 바꿔보자 지금은 바쁘다
				if (ch == 's') Interval = 0, Keyflag = 1;								// Stop counting
				if (ch == 'd') Interval = -1, Keyflag = 2;								// Down counting (0.1s)
				if (ch == 'r') Interval = 0, N_cnt = 0, Keyflag = 3;					// Reset (timer = 0000)
				if (ch == '1') ADMUX = 0x00, Keyflag = 4;								// ADC0
				if (ch == '2') ADMUX = 0x01, Keyflag = 5;								// ADC1
				if (ch == '3') ADMUX = 0x02, Keyflag = 6;								// ADC2
				if (ch == '4') ADMUX = 0x03, Keyflag = 7;								// ADC3									
				if (ch == 'c') IntMsg();												// 네번째 부가기능 Usart clear
				if (Intflag == 1 || Intflag == 2)								
				{																		
					txd_string("\r                                    ");				// LCD, FND의 경우 기존에 있는 USART 출력을 지우고,
					txd_string("\r Enter Key : ");										// 입력을 받기 위한 코드
				}												
				pcmd_U0 = Cmd_U0;														// 기존 키보드 입력 저장
				flagbuf = Intflag;														// 기존 스위치 입력 저장
			}
		}
		lcd_display_position(2,1);														// 여기서 부터 동작에 의한 출력 두번째 줄 및 USART
		if(Intflag == 0 && (ch != 0))
		{																				// 스위치가 안눌린 상태에서 키보드만 누를경우
			lcd_display_position(1,1);						
			lcd_string("Press switch... ");
			lcd_display_position(2,1);
			lcd_string("                ");
		}
		else if (ch == 'u' || ch == 's' || ch == 'd' || ch == 'r')						// 타이머 
		{
			sprintf(MSG, "%s= %5.1f     ", NOWACT[Keyflag], (float)(N_cnt)/10.0);		// 0.1초 단위의 타이머 값 출력
			if (Intflag == 1 || Intflag == 4)
			{
				lcd_string(MSG);														// LCD와 ALL일 경우
				if (Intflag == 1)
				{ 
					Fndflag = 0;														// LCD의 경우 기존에 켜져있을 FND를 종료
					_delay_ms(10);
				}
			}
			if (Intflag == 2 || Intflag == 4)											// FND와 ALL일 경우
			{				Fndflag = 1;															// FND플래그를 통해 타이머카운터3에서 동작
			}
			if (Intflag == 3 || Intflag == 4)											// USART와 ALL일 경우
			{
				txd_string("\r");														
				txd_string(MSG);														// 타이머 결과 출력 후 키보드 입력을 받는 곳인 엔터키 출력
				txd_string("   Enter Key : ");
				if (Intflag == 3) Fndflag = 0;											// USART의 경우 FND off
			}
		}
		else if (ch == '1' || ch == '2' || ch == '3' || ch == '4')						// ADC
		{
			ADCSRA |= 0x40;																// 단일변환, ADSC 변경
			Vin = (float)ADC*VREF/1023.;												
			sprintf(MSG, " ADC%d = %.2f[V]   ", ch - '1', Vin);							
			if (Intflag == 1 || Intflag == 4)
			{
				lcd_string(MSG);
			}
			if (Intflag == 2 || Intflag == 4)
			{
				Fndflag = 2;
			}
			if (Intflag == 3 || Intflag == 4)
			{
				txd_string("\r");
				txd_string(MSG);
				txd_string("   Enter Key : ");
			}
		}
		else if (ch == 'a')
		{
			ADCSRA |= 0x60;																// 프리러닝
			ADMUX = 0x03;
			_delay_ms(20);
			a_switch = ADC*2/1023;														// ADC3를 0에서 1로 나오게 설정
			if (a_switch == 0)															// ADC3에서 나온 값이 0일 경우 타이머 설정 중
			{
				ADMUX = 0x00;
				_delay_ms(20);
				vbuf_h = (long)((float)ADC*TIME/1023.+ 0.5)*TIME*TIME;					// 시, 0에서 60까지 
				ADMUX = 0x01;
				_delay_ms(20);
				vbuf_m = (int)((float)ADC*TIME/1023. + 0.5)*TIME;						// 분
				ADMUX = 0x02;
				_delay_ms(20);
				vbuf_s = (int)((float)ADC*TIME/1023. + 0.5);							// 초
				A_cnt = vbuf_h + vbuf_m + vbuf_s;
			}
			else Interval = -1;															// ADC3값이 0이 아닐 경우 타이머 설정 종료 및 1초 카운트 다운 시작
			tm_h = (int)(A_cnt / 3600);
			tm_m = (int)((A_cnt % 3600) / 60);
			tm_s = (int)(A_cnt % 60);
			sprintf(MSG, "  %2dh %2dm %2ds     ", tm_h, tm_m, tm_s);					// 시 분 초 나타내기
			if (Intflag == 1 || Intflag == 4)
			{
				lcd_string(MSG);
			}
			if (Intflag == 2 || Intflag == 4)
			{
				Fndflag = 0;
			}
			if (Intflag == 3 || Intflag == 4)
			{
				txd_string("\r");
				txd_string(MSG);
				txd_string("   Enter Key : ");
			}
		}
		else if (ch == 'p')																// Password Setting
		{
			ADCSRA |= 0x60;																// 프리러닝
			ADMUX = 0x03;																// ADC3
			_delay_ms(20);
			pw4 = (int)((float)ADC*10./1023.- 0.5);										// 4번째 자리 암호 값 설정, 0 ~ 9
			FND[3] = SEG[pw4];
			ADMUX = 0x02;
			_delay_ms(20);
			pw3 = (int)((float)ADC*10./1023.- 0.5);										// 3번째
			FND[2] = SEG[pw3];
			ADMUX = 0x01;
			_delay_ms(20);
			pw2 = (int)((float)ADC*10./1023.- 0.5);										// 2번째
			FND[1] = SEG[pw2];
			ADMUX = 0x00;
			_delay_ms(20);
			pw1 = (int)((float)ADC*10./1023.- 0.5);										// 1번째
			FND[0] = SEG[pw1];
			sprintf(MSG, " PW :   %d %d %d %d   ", pw1, pw2, pw3, pw4);					// 현재 저장되어 있는 패스워드를 계속하여 출력
			if (Intflag == 1 || Intflag == 4)
			{
				lcd_string(MSG);	
			}
			if (Intflag == 2 || Intflag == 4)
			{
				Fndflag = 3;
			}
			if (Intflag == 3 || Intflag == 4)
			{
				txd_string("\r");
				txd_string(MSG);
				txd_string("   Enter Key : ");
			}
		}
		if (chck == 'l')																// Atmega128 Lock
		{
			Pwlock = 1;																	// 잠금 여부를 파악할 플래그
			ADCSRA |= 0x60;																// 프리러닝
			ADMUX = 0x03;											
			_delay_ms(20);
			pw8 = (int)((float)ADC*10./1023.- 0.5);										// Password Setting과 동일한 방법으로 패스워드 변수 저장 및 출력
			FND[3] = SEG[pw8];															// FND 출력을 위한 캐릭터 배열 저장
			ADMUX = 0x02;		
			_delay_ms(20);
			pw7 = (int)((float)ADC*10./1023.- 0.5);
			FND[2] = SEG[pw7];
			ADMUX = 0x01;
			_delay_ms(20);
			pw6 = (int)((float)ADC*10./1023.- 0.5);
			FND[1] = SEG[pw6];
			ADMUX = 0x00;
			_delay_ms(20);
			pw5 = (int)((float)ADC*10./1023.- 0.5);
			FND[0] = SEG[pw5];
			sprintf(MSG, " PW :   %d %d %d %d   ", pw5, pw6, pw7, pw8);				
			if (Intflag == 1 || Intflag == 4)
			{
				lcd_string(MSG);
			}
			if (Intflag == 2 || Intflag == 4)
			{
				Fndflag = 3;
			}
			if (Intflag == 3 || Intflag == 4)
			{
				txd_string("\r");
				txd_string(MSG);
				txd_string("   Enter Key : ");
			}
			if ((pw1 == pw5) && (pw2 == pw6) && (pw3 == pw7) && (pw4 == pw8))			// 패스워드가 일치할 경우
			{
				Pwlock = 0;																// 잠금 여부 False로 변경
				Cmd_U0 = chbuf;															// 기존 진행하던 작업값을 저장해놓은 chbuf를 Cmd_U0에 입력
				lockck = 1;																// 잠금해제시 1회 진행을 위한 check
			}
		}
	}
}
ISR(USART0_RX_vect)
{
	Cmd_U0 = UDR0;
}
ISR(TIMER1_OVF_vect)																	// A_cnt는 1초, N_cnt는 0.1초 타이머
{
	TCNT1 = 0xE980;
	static int j = 0;
	N_cnt = N_cnt + Interval;
	if (N_cnt < 0 ) N_cnt = 9999; if (N_cnt >= 10000) N_cnt = 0; 
	if (j == 0)
	{
		A_cnt = A_cnt + Interval;
		if (A_cnt < 0) Interval = 0, A_cnt = 0.0;		
	}
	j++;
	j%=10;
}
ISR(TIMER3_COMPA_vect)
{
	static char i = 0;																	// FND 출력부분
	if (Intflag == 2 || Intflag ==4)
	{
		if (Fndflag == 1)																// 0.1초 타이머
		{
			PORTB |= 0x0F;			switch (i%4)
			{
				case 0 :
				PORTA = SEG[N_cnt/1000];
				PORTB &= 0xF7;
				break;
				case 1 :
				PORTA = SEG[(N_cnt/100)%10];
				PORTB &= 0xFB;
				break;
				case 2 :
				PORTA = SEG[(N_cnt/10)%10] & 0x7F;			// 점 출력
				PORTB &= 0xFD;
				break;
				case 3 :
				PORTA = SEG[N_cnt%10];
				PORTB &= 0xFE;
				break;
				default : break;
			}
		}
		else if (Fndflag == 2)															// ADC
		{
			PORTB |= 0x0F;
			switch(i%4)
			{
				case 0 :
				if (Keyflag == 4) PORTA = SEG[10];
				if (Keyflag == 5) PORTA = SEG[11];
				if (Keyflag == 6) PORTA = SEG[12];
				if (Keyflag == 7) PORTA = SEG[13];
				PORTB &= 0xF7;
				break;
				case 1 :
				PORTA = SEG[(int)(Vin)] & 0x7F;
				PORTB &= 0xFB;
				break;
				case 2 :
				PORTA = SEG[(int)(Vin*10)%10];
				PORTB &= 0xFD;
				break;
				case 3 :
				PORTA = SEG[(int)(Vin*100+0.5)%10];
				PORTB &= 0xFE;
				break;
				default : break;
			}
		}
		else if (Fndflag == 3)															// PW
		{
			PORTB |= 0x0F;
			switch(i%4)
			{
				case 0 :
				PORTA = FND[0];
				PORTB &= 0xF7;
				break;
				case 1 :
				PORTA = FND[1];
				PORTB &= 0xFB;
				break;
				case 2 :
				PORTA = FND[2];
				PORTB &= 0xFD;
				break;
				case 3 :
				PORTA = FND[3];
				PORTB &= 0xFE;
				break;
				default : break;
			}
		}
		else PORTA = 0xFF;																// FND off
		i++;
		i %= 4;		
	}
	else PORTA = 0xFF;
}

ISR(INT4_vect)
{
	Intflag = 1;	
}
ISR(INT5_vect)
{
	Intflag = 2;
	lcd_display_position(2,1);
	lcd_string("                ");	
}
ISR(INT6_vect)
{
	Intflag = 3;
	lcd_display_position(2,1);
	lcd_string("                ");
}
ISR(INT7_vect)
{
	Intflag = 4;
}
ISR(ADC_vect)
{
	ADval = ADC = (int)ADCL + (int)ADCH*0x100;
}

void txd(char ch)
{
	while(!(UCSR0A & 0x20)); 
	UDR0 = ch;
} 
void txd_string(char *str)
{
	int i=0;
	while(1) {
	if (str[i] == '\0') break;
	txd(str[i++]);
	}
}
void IntMsg(void)																// Hyper terminal 초기화용
{
	txd_string("\n\n\n\n\n\n\n\n\n");
	txd_string("\n\n\n\n\n\n\n\n\n");
	txd_string("\n\n\n\n\n\n\n\n\n");
	txd_string("\n\n\n\n\n\n\n\n\n");
	txd_string("\n\n\n\n\n\n\n\n\n");
	txd_string("\n\r***********************************************");
	txd_string("\n\r******    MicroComputer Term Project     ******");
	txd_string("\n\r***********************************************");
	txd_string("\n\r");
	txd_string("\n\r   ------------- Timer Menu -------------------");
	txd_string("\n\r   'u' : up counting       'd' : down counting");
	txd_string("\n\r   's' : stop counting     'r' : reset counting");
	txd_string("\n\r");
	txd_string("\n\r   ---------- A/D Conversion Menu -------------");
	txd_string("\n\r   '1' : ADC CH 0          '2' : ADC CH 1");
	txd_string("\n\r   '3' : ADC CH 2          '4' : ADC CH 3");
	txd_string("\n\r");
	txd_string("\n\r   --------------Add-Ons Menu -----------------");
	txd_string("\n\r   'a' : Analog timer      'p' : Password Setting");
	txd_string("\n\r   'l' : Lock Atmega       'c' : Usart Clear");
	txd_string("\n\r");
	txd_string("\n\r   ---------- Display Menu --------------------");
	txd_string("\n\r   SW1(INT4) : LCD          SW2(INT5) : FND");
	txd_string("\n\r   SW3(INT6) : USART        SW4(INT7) : ALL");
	txd_string("\n\n");
	txd_string("\r Enter Key : ");
}