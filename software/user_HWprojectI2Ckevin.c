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
#define VL6180x_FAILURE_RESET  -1

#define VL6180X_IDENTIFICATION_MODEL_ID              0x0000
#define VL6180X_IDENTIFICATION_MODEL_REV_MAJOR       0x0001
#define VL6180X_IDENTIFICATION_MODEL_REV_MINOR       0x0002
#define VL6180X_IDENTIFICATION_MODULE_REV_MAJOR      0x0003
#define VL6180X_IDENTIFICATION_MODULE_REV_MINOR      0x0004
#define VL6180X_IDENTIFICATION_DATE                  0x0006 //16bit value
#define VL6180X_IDENTIFICATION_TIME                  0x0008 //16bit value

#define VL6180X_SYSTEM_MODE_GPIO0                    0x0010
#define VL6180X_SYSTEM_MODE_GPIO1                    0x0011
#define VL6180X_SYSTEM_HISTORY_CTRL                  0x0012
#define VL6180X_SYSTEM_INTERRUPT_CONFIG_GPIO         0x0014
#define VL6180X_SYSTEM_INTERRUPT_CLEAR               0x0015
#define VL6180X_SYSTEM_FRESH_OUT_OF_RESET            0x0016
#define VL6180X_SYSTEM_GROUPED_PARAMETER_HOLD        0x0017

#define VL6180X_SYSRANGE_START                       0x0018
#define VL6180X_SYSRANGE_THRESH_HIGH                 0x0019
#define VL6180X_SYSRANGE_THRESH_LOW                  0x001A
#define VL6180X_SYSRANGE_INTERMEASUREMENT_PERIOD     0x001B
#define VL6180X_SYSRANGE_MAX_CONVERGENCE_TIME        0x001C
#define VL6180X_SYSRANGE_CROSSTALK_COMPENSATION_RATE 0x001E
#define VL6180X_SYSRANGE_CROSSTALK_VALID_HEIGHT      0x0021
#define VL6180X_SYSRANGE_EARLY_CONVERGENCE_ESTIMATE  0x0022
#define VL6180X_SYSRANGE_PART_TO_PART_RANGE_OFFSET   0x0024
#define VL6180X_SYSRANGE_RANGE_IGNORE_VALID_HEIGHT   0x0025
#define VL6180X_SYSRANGE_RANGE_IGNORE_THRESHOLD      0x0026
#define VL6180X_SYSRANGE_MAX_AMBIENT_LEVEL_MULT      0x002C
#define VL6180X_SYSRANGE_RANGE_CHECK_ENABLES         0x002D
#define VL6180X_SYSRANGE_VHV_RECALIBRATE             0x002E
#define VL6180X_SYSRANGE_VHV_REPEAT_RATE             0x0031

#define VL6180X_SYSALS_START                         0x0038
#define VL6180X_SYSALS_THRESH_HIGH                   0x003A
#define VL6180X_SYSALS_THRESH_LOW                    0x003C
#define VL6180X_SYSALS_INTERMEASUREMENT_PERIOD       0x003E
#define VL6180X_SYSALS_ANALOGUE_GAIN                 0x003F
#define VL6180X_SYSALS_INTEGRATION_PERIOD            0x0040

#define VL6180X_RESULT_RANGE_STATUS                  0x004D
#define VL6180X_RESULT_ALS_STATUS                    0x004E
#define VL6180X_RESULT_INTERRUPT_STATUS_GPIO         0x004F
#define VL6180X_RESULT_ALS_VAL                       0x0050
#define VL6180X_RESULT_HISTORY_BUFFER                0x0052
#define VL6180X_RESULT_RANGE_VAL                     0x0062
#define VL6180X_RESULT_RANGE_RAW                     0x0064
#define VL6180X_RESULT_RANGE_RETURN_RATE             0x0066
#define VL6180X_RESULT_RANGE_REFERENCE_RATE          0x0068
#define VL6180X_RESULT_RANGE_RETURN_SIGNAL_COUNT     0x006C
#define VL6180X_RESULT_RANGE_REFERENCE_SIGNAL_COUNT  0x0070
#define VL6180X_RESULT_RANGE_RETURN_AMB_COUNT        0x0074
#define VL6180X_RESULT_RANGE_REFERENCE_AMB_COUNT     0x0078
#define VL6180X_RESULT_RANGE_RETURN_CONV_TIME        0x007C
#define VL6180X_RESULT_RANGE_REFERENCE_CONV_TIME     0x0080

#define VL6180X_READOUT_AVERAGING_SAMPLE_PERIOD      0x010A
#define VL6180X_FIRMWARE_BOOTUP                      0x0119
#define VL6180X_FIRMWARE_RESULT_SCALER               0x0120
#define VL6180X_I2C_SLAVE_DEVICE_ADDRESS             0x0212
#define VL6180X_INTERLEAVED_MODE_ENABLE              0x02A3

int version = 0,distance=0;


char newprint = 0;
int failure=0;
unsigned long timecnt = 0;  
char toggle=0;
int g_i = 0;
// Idle function
void idle(int us)
{
  us = 16*us;
  for(g_i=0;g_i<us;g_i++);
}


void setRegister(unsigned int address, unsigned char value) {
	//Transmit address
	UCB0CTL1 |= UCTXSTT + UCTR;   	//generate start condition in transmit mode
	UCB0TXBUF = (address>>8) & 0xFF;				//send memory location
	while(UCB0CTL1 & UCTXSTT);		//wait until start condition is sent
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); //wait for slave to acknowledge and master to finish transmitting
	idle(15);//idle to wait for transfer to finish
	//Transmit data
	UCB0TXBUF = (address) & 0xFF;
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); //wait for slave to acknowledge and master to finish transmitting
	idle(15);//idle to wait for transfer to finish

	UCB0TXBUF = (value) & 0xFF;
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); //wait for slave to acknowledge and master to finish transmitting
	idle(15);//idle to wait for transfer to finish

	UCB0CTL1 |= UCTXSTP;        //send stop condition after 2 byte of data
	while(UCB0CTL1 & UCTXSTP); //wait for stop condition to sent before moving on

}

unsigned char getRegister(unsigned int address) {
	//Transmit address
	UCB0CTL1 |= UCTXSTT + UCTR;   	//generate start condition in transmit mode
	UCB0TXBUF = (address>>8) & 0xFF;				//send memory location
	while(UCB0CTL1 & UCTXSTT);		//wait until start condition is sent
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); //wait for slave to acknowledge and master to finish transmitting
	idle(15);//idle to wait for transfer to finish
	//Transmit data
	UCB0TXBUF = (address) & 0xFF;
	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); //wait for slave to acknowledge and master to finish transmitting
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
void startDistance(void)
{
	setRegister(VL6180X_SYSRANGE_START, 0x01);
}
unsigned char readDistance(void)
{
	return getRegister(VL6180X_RESULT_RANGE_VAL);
}


void main(void) {

	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

	if (CALBC1_16MHZ ==0xFF || CALDCO_16MHZ == 0xFF) while(1);

	DCOCTL = CALDCO_16MHZ;    // Set uC to run at approximately 16 Mhz
	BCSCTL1 = CALBC1_16MHZ; 

	// Initialize Port 1
	P1SEL |= 0xc0;
	P1SEL2 |= 0xc0;
	P1REN = 0x00;
	P1DIR |= 0xc1;
	P1OUT &= ~0xc0;

	//initialize I2C
	UCB0I2CSA = 0x29;
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;        // 2-pin, 7-bit address I2C master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;               // SMCLK
	UCB0BR0 = 128;                               //
	UCB0BR1 = 0;                                 //
	UCB0CTL1 &= ~UCSWRST;                        // **Initialize USCI state machine**
	IFG2 &= ~UCB0RXIE;                           // Receive interrupt when buffer has 1 complete character, clears interrupt flag


	// Timer A Config
	TACCTL0 = CCIE;       		// Enable Periodic interrupt
	TACCR0 = 16000;                // period = 1ms   
	TACTL = TASSEL_2 + MC_1; // source SMCLK, up mode

	if(getRegister(VL6180X_SYSTEM_FRESH_OUT_OF_RESET)!=1)
		failure=VL6180x_FAILURE_RESET;
	else {
		failure=0;

		setRegister(0x0207, 0x01);
		setRegister(0x0208, 0x01);
		setRegister(0x0096, 0x00);
		setRegister(0x0097, 0xfd);
		setRegister(0x00e3, 0x00);
		setRegister(0x00e4, 0x04);
		setRegister(0x00e5, 0x02);
		setRegister(0x00e6, 0x01);
		setRegister(0x00e7, 0x03);
		setRegister(0x00f5, 0x02);
		setRegister(0x00d9, 0x05);
		setRegister(0x00db, 0xce);
		setRegister(0x00dc, 0x03);
		setRegister(0x00dd, 0xf8);
		setRegister(0x009f, 0x00);
		setRegister(0x00a3, 0x3c);
		setRegister(0x00b7, 0x00);
		setRegister(0x00bb, 0x3c);
		setRegister(0x00b2, 0x09);
		setRegister(0x00ca, 0x09);
		setRegister(0x0198, 0x01);
		setRegister(0x01b0, 0x17);
		setRegister(0x01ad, 0x00);
		setRegister(0x00ff, 0x05);
		setRegister(0x0100, 0x05);
		setRegister(0x0199, 0x05);
		setRegister(0x01a6, 0x1b);
		setRegister(0x01ac, 0x3e);
		setRegister(0x01a7, 0x1f);
		setRegister(0x0030, 0x00);

		setRegister(VL6180X_SYSTEM_FRESH_OUT_OF_RESET,0x00);
	}
//	//Transmit address
//	UCB0CTL1 |= UCTXSTT + UCTR;   	//generate start condition in transmit mode
//	UCB0TXBUF = 0x01;				//send memory location
//	while(UCB0CTL1 & UCTXSTT);		//wait until start condition is sent
//	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); //wait for slave to acknowledge and master to finish transmitting
//
//	//Transmit data
//	UCB0TXBUF = ;
//	while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); //wait for slave to acknowledge and master to finish transmitting
//	idle(15);					//idle to wait for transfer to finish
//	UCB0CTL1 |= UCTXSTP;        //send stop condition after 2 byte of data
//	while(UCB0CTL1 & UCTXSTP); //wait for stop condition to sent before moving on



	Init_UART(9600,1);	// Initialize UART for 9600 baud serial communication

	_BIS_SR(GIE); 		// Enable global interrupt


	while(1) {

		if(newmsg) {
			newmsg = 0;
		}


		if (newprint)  {

			P1OUT ^= 0x1;		// Blink LED
//			if(toggle==0)
//			{
//				startDistance();
//				toggle=1;
//			}
//			else
//			{
//				toggle=0;
//				distance=readDistance();
//			}
			//version=getRegister(VL6180X_IDENTIFICATION_MODEL_ID);



			if(failure!=0)
			{
				UART_printf("Failed to Initialize\n\r");
				failure=0;
			}
			else
			UART_printf("Hello %d\n\r",(int)(distance));

			newprint = 0;
		}

	}
}

int starttime = 100;
int readtime = 110;
int deltatime = 0;

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	timecnt++; // Keep track of time for main while loop. 
	deltatime++;
	if (deltatime == starttime) {
		startDistance();
	}
	if (deltatime == readtime) {
		distance=readDistance();
		deltatime = 0;
		newprint = 1;
	}
	if ((timecnt%500) == 0) {
	//newprint = 1;  // flag main while loop that .5 seconds have gone by.
	}

}


/*
// ADC 10 ISR - Called when a sequence of conversions (A7-A0) have completed
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {

}
*/


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

//    Uncomment this block of code if you would like to use this COM protocol that uses 253 as STARTCHAR and 255 as STOPCHAR
/*		if(!started) {	// Haven't started a message yet
			if(UCA0RXBUF == 253) {
				started = 1;
				newmsg = 0;
			}
		}
		else {	// In process of receiving a message		
			if((UCA0RXBUF != 255) && (msgindex < (MAX_NUM_FLOATS*5))) {
				rxbuff[msgindex] = UCA0RXBUF;

				msgindex++;
			} else {	// Stop char received or too much data received
				if(UCA0RXBUF == 255) {	// Message completed
					newmsg = 1;
					rxbuff[msgindex] = 255;	// "Null"-terminate the array
				}
				started = 0;
				msgindex = 0;
			}
		}
*/

		IFG2 &= ~UCA0RXIFG;
	}

}


