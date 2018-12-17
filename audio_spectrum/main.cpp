/*
 * audio_spectrum.cpp
 *
 * Created: 01-Jun-17 6:36:44 PM
 * Author : Mahathir
 */ 

#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define F_CPU 16000000 //Clock Frequency 16 MHz
#include <util/delay.h>

#define WELCOME 1
#define GAME 2
#define SPECTRUM 3
int mode=WELCOME;

#define N 32 // 32 point DFT
#define _N32_
#include "DFT.h" //cos,sine and degree lookup for DFT
int32_t Fourier[N/2][2];//for sine and cosine values
uint8_t buff[N/2];// N/2 points taken for output
int32_t adcRes[N];//sound wave captured N times
void spectrum_display();
void initial_display();
void Fourier_TRANSFORM();


unsigned int update_time;
unsigned int timer;
unsigned int speed;
int flag;
int score_card;
int new_block;
int player_index;
void game_initialize();
int hurdle_update();
int player_update();
int score_update();
int merge(int h,int p,int s);
void game_display();


unsigned int hurdle[16][3]={{0b0000000011100000,0b0000000011100000,0b0000000011100000},
{0b0000000001110000,0b0000000001110000,0b0000000001110000},
{0b0000000000111000,0b0000000000111000,0b0000000000111000},
{0b0000000000011100,0b0000000000011100,0b0000000000011100},
{0b0000000000001110,0b0000000000001110,0b0000000000001110},
{0b0000000000000111,0b0000000000000111,0b0000000000000111},
{0b1000000000000011,0b1000000000000011,0b1000000000000011},
{0b1100000000000001,0b1100000000000001,0b1100000000000001},
{0b1110000000000000,0b1110000000000000,0b1110000000000000},
{0b0111000000000000,0b0111000000000000,0b0111000000000000},
{0b0011100000000000,0b0011100000000000,0b0011100000000000},
{0b0001110000000000,0b0001110000000000,0b0001110000000000},
{0b0000111000000000,0b0000111000000000,0b0000111000000000},
{0b0000011100000000,0b0000011100000000,0b0000011100000000},
{0b0000001110000000,0b0000001110000000,0b0000001110000000},
{0b0000000111000000,0b0000000111000000,0b0000000111000000}};

unsigned int player[7][16]={{0b0001110000000000,0b0001110000000000,0b0000100000000000,0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0b0001110000000000,0b0001110000000000,0b0000100000000000,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0b0001110000000000,0b0001110000000000,0b0000100000000000,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0b0001110000000000,0b0001110000000000,0b0000100000000000,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0b0001110000000000,0b0001110000000000,0b0000100000000000,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0b0001110000000000,0b0001110000000000,0b0000100000000000,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0b0001110000000000,0b0001110000000000,0b0000100000000000,0,0,0,0,0,0,0}};

unsigned int score[3]={0b0000000011100000,0b0000000011000000,0b0000000010000000};

unsigned int out[16][16];
unsigned int i,j,x;
unsigned char ch;

int main(void)
{
	 /* Replace with your application code */
	 
	DDRB=0b00001111;// PORTB=i means ROW i is selected
	DDRD=0b00001111;// PORTD=i means COLUMN i is selected
	//ADC Initialization
	ADMUX = 0b11000000;//Internal 2.56 volt reference(REFS0=1,REFS1=1), ADC0 as input
	ADCSRA =0b10000010;//ADCSRA=(1<<ADEN)|(1<<ADPS1) , Prescalar=4
	
	//Timer Initialization
	TCCR1B = 0b00001001; //(1<<WGM12)|(1<<CS10)
	OCR1A = 99;// Count to 100
	
    while (1) 
    {
		ch=PIND;
		if(ch&0b00010000)
		{
			mode=SPECTRUM;
		}
		else if(ch&0b00100000)
		{
			if(mode!=GAME)game_initialize();
			else
			{
				player_index=player_update();
			}
			mode=GAME;
		}
		
		
		if(mode==SPECTRUM)
		{		
			TCNT1 = 0;
			TIFR |= 1<<OCF1A;
			for(i=0;i<N;i++)
			{
				while((TIFR & (1<<OCF1A)) == 0);
				ADCSRA |= 1<<ADSC;//Start conversion
				while(!ADIF);
				ADCSRA |= 1<<ADIF;
				uint16_t low,high;
				low = ADCL;
				high = ADCH;
				high=(high<<8)|low;
				adcRes[i] = ((int16_t)high);
				if(adcRes[i]<0)
				{
					adcRes[i]=-adcRes[i];
					adcRes[i]*=2;
				}
				
				TIFR |= 1<<OCF1A;
			}
			
			Fourier_TRANSFORM();
			
			for(i =1; i<N/2; i++)
			{
				if(Fourier[i][0]<0)Fourier[i][0]*=-1;
				if(Fourier[i][1]<0)Fourier[i][1]*=-1;
				uint8_t mag;
				mag = (uint8_t)(Fourier[i][0] + Fourier[i][1])/4;//magnitude of i-th resolution
				if((mag)>16) {
					buff[i-1] = 16;
				}
				else {
					buff[i-1] = mag;
				}
			}
			buff[15]=0;
			//_delay_ms(5);
			spectrum_display();	
		}
		
		else if(mode==GAME)
		{
			int hurdle_index=hurdle_update();
			if( (update_time!=0) )
			{
				player_index=player_update();
			}
			int status=merge(hurdle_index,player_index,score_card);
			if(status==0)
			{
				score_card=score_update();
				if(score_card==3)
				{
					mode=WELCOME;
					initial_display();
				}
			}
			if(mode==GAME)game_display();
		}
		
		else if(mode==WELCOME)
		{
			initial_display();
		}
		
	}
}

void Fourier_TRANSFORM()
{
	
	int16_t count,degree;
	uint8_t u,k;
	count = 0;
	for (u=0; u<N/2; u++) {
		for (k=0; k<N; k++) {
			degree = (uint16_t)pgm_read_byte_near(degree_lookup + count)*2;
			count++;
			Fourier[u][0] +=  adcRes[k] * (int16_t)pgm_read_word_near(cos_lookup + degree);
			Fourier[u][1] += -adcRes[k] * (int16_t)pgm_read_word_near(sin_lookup + degree);
		}
		Fourier[u][0] /= N;
		Fourier[u][0] /= 10000;
		Fourier[u][1] /= N;
		Fourier[u][1] /= 10000;
	}
	
}

void spectrum_display()
{
	unsigned int i,j;
	for(i=0;i<16;i++)
	{
		for(j=0;j<8;j++)
		{
			if(buff[7-j]>i)
			{
				PORTB=i;
				PORTD=j;
			}
			_delay_us(20);
		}
		for(j=8;j<16;j++)
		{
			if(buff[23-j]>i)
			{
				PORTB=i;
				PORTD=j;
			}
			_delay_us(20);
		}
		_delay_us(10);
	}
}

void initial_display()
{
	int wel[16][16]={{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,1,1,0,0,1,1,0,1,0,0,0,1,1,1,1},
	{1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0},
	{1,0,0,0,1,0,0,1,1,0,1,0,1,1,1,1},
	{1,0,0,1,1,0,0,1,1,1,0,1,1,1,0,0},
	{0,1,1,0,0,1,1,0,1,0,0,0,1,1,1,1},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,1,0,1,0,0,1,1,1,1,0,1,1,1,1,0},
	{1,0,1,0,1,0,1,0,0,0,0,1,0,0,0,0},
	{1,0,0,0,1,0,1,1,1,1,0,1,0,0,0,0},
	{1,0,0,0,1,0,1,0,0,0,0,1,0,0,0,0},
	{1,0,0,0,1,0,1,1,1,1,0,1,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
		
	while(1)
	{
		ch=PIND;
		if(ch&0b00010000)
		{
			mode=SPECTRUM;
			return;
		}
		else if(ch&0b00100000)
		{
			game_initialize();
			mode=GAME;
			return;
		}
		for(i=0;i<16;i++)
		{
			for(j=0;j<8;j++)
			{
				if(wel[i][7-j]==1)
				{
					PORTB=i;
					PORTD=j;
				}
			}
			for (j=8;j<16;j++)
			{
				if(wel[i][23-j]==1)
				{
					PORTB=i;
					PORTD=j;
				}
			}
		}
	}
}

void game_initialize()
{
	update_time=0;
	timer=-1;
	speed=200;
	flag=0;
	score_card=0;
	new_block=0;
}

int hurdle_update()
{
	timer++;
	if(timer==16)
	{
		timer=0;
		new_block=1;
		if(speed>60)speed-=10;
	}
	return timer;
}

int player_update()
{
	if(update_time==6)flag=1;
	else if(update_time==0)flag=0;
	if(flag==1)update_time--;
	else update_time++;
	return update_time;
}

int score_update()
{
	score_card++;
	return score_card;
}

int merge(int h,int p,int s)
{
	unsigned int i,j,res,t=1;
	res=1;
	for(i=0;i<3;i++)
	{
		t=1;
		for(j=0;j<16;j++)
		{
			if(  ((t & hurdle[h][i])&&(!(t & player[p][i]))) || ((t & player[p][i])&&(!(t & hurdle[h][i])))  )
			{
				out[i][j]=1;
			}
			else if(  (t & hurdle[h][i]) && ((t & player[p][i]))  )
			{
				out[i][j]=1;
				if(new_block==1)
				{
					new_block=0;
					res=0;
				}
			}
			else
			{
				out[i][j]=0;
			}
			t<<=1;
		}
	}
	for(i=3;i<15;i++)
	{
		t=1;
		for(j=0;j<16;j++)
		{
			if( t&player[p][i]  )
			{
				out[i][j]=1;
			}
			else
			{
				out[i][j]=0;
			}
			t<<=1;
		}
	}
	i=15;
	t=1;
	for(j=0;j<16;j++)
	{
		if( t & score[s]  )
		{
			out[i][j]=1;
		}
		else
		{
			out[i][j]=0;
		}
		t<<=1;
	}
	return res;
}

void game_display()
{
	unsigned int i,j,x;
	for(x=0;x<speed;x++)
	{
		for(i=0;i<16;i++)
		{
			for(j=0;j<16;j++)
			{
				if(out[i][j]==1)
				{
					PORTB=i;
					PORTD=j;
				}
			}
		}
	}
	
}