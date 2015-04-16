/******************************************************************************
MSP430G2553 Project Creator

GE 423  - Dan Block
        Spring(2012)

        Written(by) : Steve(Keres)
College of Engineering Control Systems Lab
University of Illinois at Urbana-Champaign
*******************************************************************************/

#include "msp430g2553.h"
#include "UART.h"

// my local defns for calculating LED output
#define IT_IS 	 1<<0
#define FIVE_MIN 1<<1
#define TEN_MIN  1<<2
#define QUARTER  1<<3
#define TWENTY   1<<4
#define HALF 	 1<<5
#define PAST 	 1<<6
#define TO 		 1<<7
#define ONE 	 1<<8
#define TWO 	 1<<9
#define THREE 	 1<<10
#define FOUR 	 1<<11
#define FIVE_HR  1<<12
#define SIX 	 1<<13
#define SEVEN 	 1<<14
#define EIGHT 	 1<<15
#define NINE 	 1<<16
#define TEN_HR 	 1<<17
#define ELEVEN   1<<18
#define TWELVE   1<<19
#define OCLK     1<<20

// register maps
#define REG_SECONDS 0x00
#define REG_MINUTES 0x01
#define REG_HOURS   0x02

// current time
// see how to format in calibrate() function
#define CURR_SEC 
#define CURR_MIN
#define CURR_HRS

// globals
char newprint = 0;           // UART print flag
unsigned long timecnt = 0;  
unsigned char seconds = 0; 
unsigned char minutes = 0;
unsigned char hours = 0;
double wordArray = 0;

// function definitions
void config(void);
void pulseClk(void);
void shiftData(double wordArray);
void idle(int us);
void setRegister(unsigned char address, unsigned char value);
unsigned char getRegister(unsigned char address);

void main(void) {

	Init_UART(9600,1);	// Initialize UART for 9600 baud serial communication

	_BIS_SR(GIE); 		// Enable global interrupt

	// check to see if inital cal needs to be performed
	if (getRegister(REG_MINUTES) & 0x80) 
	{
		// NEEDS TO BE CUSTOMIZED EACH TIME
		// 0 - [3 bit BCD Ten's Minute] - [4 bit BCD One's Minute]
		// 0 - 0 - [2 bit BCD Ten's Hour] - [4 bit BCD One's Hour]
		// use 24 hour format
		unsigned char min = ;
		unsigned char hour = 
		setRegister(REG_MINUTES, );
		setRegister(REG_HOURS, ) 
	}

	while(1) 
	{
		if(newmsg) 
		{	
			newmsg = 0;
		}

		if (newprint)  
		{	
			P1OUT ^= 0x1;		// Blink LED
			UART_printf("Hello %d\n\r",(int)(timecnt/500));
			newprint = 0;
		}
	}
}


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	timecnt++; // Keep track of time for main while loop. 

	if ((timecnt%500) == 0) {
		newprint = 1;  // flag main while loop that .5 seconds have gone by.  
		timecnt = 0;
	}

}

// USCI Transmit ISR - Called when TXBUF is empty (ready to accept another character)
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void) {

	if(IFG2&UCA0TXIFG) {		// USCI_A0 requested TX interrupt
		if(printf_flag) {
			if (currentindex == txcount) {
				senddone = 1;
				printf_flag = 0;
				IFG2 &= ~UCA0TXIFG;
			} else {
				UCA0TXBUF = printbuff[currentindex];
				currentindex++;
			}
		} else if(UART_flag) {
			if(!donesending) {
				UCA0TXBUF = txbuff[txindex];
				if(txbuff[txindex] == 255) {
					donesending = 1;
					txindex = 0;
				}
				else txindex++;
			}
		} else {  // interrupt after sendchar call so just set senddone flag since only one char is sent
			senddone = 1;
		}

		IFG2 &= ~UCA0TXIFG;
	}

	if(IFG2&UCB0TXIFG) {	// USCI_B0 requested TX interrupt (UCB0TXBUF is empty)

		IFG2 &= ~UCB0TXIFG;   // clear IFG
	}
}


// USCI Receive ISR - Called when shift register has been transferred to RXBUF
// Indicates completion of TX/RX operation
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {

	if(IFG2&UCB0RXIFG) {  // USCI_B0 requested RX interrupt (UCB0RXBUF is full)

		IFG2 &= ~UCB0RXIFG;   // clear IFG
	}

	if(IFG2&UCA0RXIFG) {  // USCI_A0 requested RX interrupt (UCA0RXBUF is full)

		IFG2 &= ~UCA0RXIFG;
	}

}


// set the configuration registers
// specifically focused on I2C, UART, UART 
void config(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

	if (CALBC1_16MHZ ==0xFF || CALDCO_16MHZ == 0xFF) while(1);

	DCOCTL = CALDCO_16MHZ;    // Set uC to run at approximately 16 Mhz
	BCSCTL1 = CALBC1_16MHZ; 

	// Timer A Config
	TACCTL0 = CCIE;       		// Enable Periodic interrupt
	TACCR0 = 16000;                // period = 1ms   
	TACTL = TASSEL_2 + MC_1; // source SMCLK, up mode

	// I2C Config
	P1SEL |= BIT6 + BIT7;   // Assign I2C pins to USCI_B0
  	P1SEL2|= BIT6 + BIT7;   // Assign I2C pins to USCI_B0

	UCB0I2CSA = 0x68; // slave address (110 1000)
  	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	CB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset, 7 bit
	CB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0CTL1 &= ~UCSWRST; // start state machine
	IFG2 &= ~UCB0RXIE;    // clear interrupt flag, ready to Rx/Tx

	// GPIO Config
	P1DIR |= BIT1 + BIT0;    // shift register pins (1.1, 1.2) are outputs
	P1OUT = 0; 				
	P1OUT |= BIT2;   // enable pullup for input
	P1REN |= BIT2;   // 1.3 is button input

}


// create a rising edge pulse for the shift register
// shouldn't need any delay in this
void pulseClk(void)
{
	P1OUT &= ~BIT0;
	P1OUT |= BIT1;
	P1OUT &= ~BIT0;
}


// add a small delay
// used to delay while the chip processes the I2C data
// before TX/RX any more data
void idle(int us)
{
	us *= 16; // 16MHz clock
	int delay;
	for(delay = 0; delay < us; delay++);
}

// I2C Write
// set a register on the chip using I2C
void setRegister(unsigned char address, unsigned char value)
{
	UCB0CTL1 |= UCTXSTT + UCTR;   	  // generate start condition in transmit mode
	
	UCB0TXBUF = address & 0xFF;  	  // send address
	while(UCB0CTL1 & UCTXSTT);		  // wait until start condition is sent
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); // wait for ACK
	idle(15); // idle to wait for transfer to finish


	UCB0TXBUF = value & 0xFF;
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); // wait for ACK
	idle(15); // idle to wait for transfer to finish

	UCB0CTL1 |= UCTXSTP;        //send stop condition after 2 byte of data
	while(UCB0CTL1 & UCTXSTP); //wait for stop condition to sent before moving on
}

// I2C Read
// get a register on the chip using I2C
unsigned char getRegister(unsigned char address)
{
	UCB0CTL1 |= UCTXSTT + UCTR;   	  // generate start condition in transmit mode
	
	UCB0TXBUF = (address) & 0xFF;   // send addresss
	while(UCB0CTL1 & UCTXSTT);		// wait until start condition is sent
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); // wait for ACK
	idle(15);//idle to wait for transfer to finish

	//Recieve
	UCB0CTL1 &= ~UCTR;			//receive
	UCB0CTL1 |= UCTXSTT;		//send another start condition in receive mode
	idle(15);
	while(UCB0CTL1 & UCTXSTT);	//wait until start condition is sent

	UCB0CTL1 |= UCTXSTP;        //send stop condition after 1 byte of data
	while(!IFG2  & UCB0RXIFG);  //wait for master to finish receiving
	idle(15);

	unsigned char value = UCB0RXBUF;
	while(UCB0CTL1 & UCTXSTP); //wait for stop condition to sent before moving on
	return value;
}