/******************************************************************************
WORD CLOCK Controller
Brady Salz
GE423 HW Project
4/16/15

Template Code made by:
GE 423  - Dan Block
        Spring(2012)

        Written(by) : Steve(Keres)
College of Engineering Control Systems Lab
University of Illinois at Urbana-Champaign
*******************************************************************************/

#include "msp430g2553.h"
#include "UART.h"

// my local defns for calculating LED output
#define IT_IS    0x0000001
#define FIVE_MIN 0x0000002
#define TEN_MIN  0x0000004
#define QUARTER  0x0000008
#define TWENTY   0x0000010
#define HALF     0x0000020
#define PAST     0x0000040
#define TO       0x0000080
#define ONE      0x0000100
#define TWO      0x0000200
#define THREE    0x0000400
#define FOUR     0x0000800
#define FIVE_HR  0x0001000
#define SIX      0x0002000
#define SEVEN    0x0004000
#define EIGHT    0x0008000
#define NINE     0x0100000
#define TEN_HR   0x0200000
#define ELEVEN   0x0400000
#define TWELVE   0x0800000
#define OCLK     0x1000000

// register maps
#define REG_SECONDS 0x00
#define REG_MINUTES 0x01
#define REG_HOURS   0x02

// current time
// NEEDS TO BE CUSTOMIZED EACH TIME
// 0 - [3 bit BCD Ten's Second] - [4 bit BCD One's Second]
// 0 - [3 bit BCD Ten's Minute] - [4 bit BCD One's Minute]
// 0 - 0 - [2 bit BCD Ten's Hour] - [4 bit BCD One's Hour]
// use 24 hour format
unsigned char CURR_SEC = 0;
unsigned char CURR_MIN = 0;
unsigned char CURR_HRS = 0;

// globals
char newprint = 0;           // UART print flag
unsigned long timecnt = 0;  
unsigned char seconds = 0; 
unsigned char minutes = 0;
unsigned char hours = 0;

// function definitions
void config(void);
void pulseClk(void);
void shiftData(double wordArray);
void idle(int us);
void setRegister(unsigned char address, unsigned char value);
unsigned char getRegister(unsigned char address);
void calibrate(void);
double setLEDBits(void);
void setServoPos(void);

void main(void) {

    Init_UART(9600,1);  // Initialize UART for 9600 baud serial communication

    _BIS_SR(GIE);       // Enable global interrupt

    // check to see if inital cal needs to be performed
    if (getRegister(REG_MINUTES) & 0x80) 
    {
        calibrate();
    }

    while(1) 
    {
        if (newprint)  
        {   
            seconds = getRegister(REG_SECONDS);
            if(seconds & 0x80)
            {
                calibrate();
            }

            minutes = getRegister(REG_MINUTES);
            hours   = getRegister(REG_HOURS);

            double wordArray = IT_IS + OCLK;  //  intialize array 

            // convert from register to decimal
            minutes = (minutes & 0x0F) + 10 * (minutes>>4); 
            hours = (hours & 0x0F) + 10 * (hours>>4);      
            
            if (hours > 12) 
            {
                hours -= 12; // convert from 24hr to 12hr
            }

            wordArray = setLEDBits();

            setServoPos();

            shiftData(wordArray);
        }
    }
}


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    timecnt++; // Keep track of time for main while loop. 

    if ((timecnt%5000) == 0) {
        newprint = 1;  // flag main while loop that 15 seconds have gone by.  
        timecnt = 0;
    }

}

// USCI Transmit ISR - Called when TXBUF is empty (ready to accept another character)
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void) {

    if(IFG2&UCA0TXIFG) {        // USCI_A0 requested TX interrupt
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

    if(IFG2&UCB0TXIFG) {    // USCI_B0 requested TX interrupt (UCB0TXBUF is empty)

        IFG2 &= ~UCB0TXIFG;   // clear IFG
    }
}


// USCI Receive ISR - Called when shift register has been transferred to RXBUF
// Indicates completion of TX/RX operation
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) 
{
    if(IFG2&UCB0RXIFG)  // USCI_B0 requested RX interrupt (UCB0RXBUF is full)
    {
        IFG2 &= ~UCB0RXIFG;   // clear IFG
    }

    if(IFG2&UCA0RXIFG)  // USCI_A0 requested RX interrupt (UCA0RXBUF is full)
    {
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
    TACCTL0 = CCIE;             // Enable Periodic interrupt
    TACCR0 = 16000;                // period = 1ms   
    TACTL = TASSEL_2 + MC_1; // source SMCLK, up mode

    // I2C Config
    P1SEL |= BIT6 + BIT7;   // Assign I2C pins to USCI_B0
    P1SEL2|= BIT6 + BIT7;   // Assign I2C pins to USCI_B0

    UCB0I2CSA = 0x68; // slave address (110 1000)
    UCB0CTL1 |= UCSWRST;                      // Enable SW reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset, 7 bit
    UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST; // start state machine
    IFG2 &= ~UCB0RXIE;    // clear interrupt flag, ready to Rx/Tx

    // GPIO Config
    P1DIR |= BIT1 + BIT0;    // shift register pins (1.1, 1.2) are outputs
    P1OUT = 0;              
    P1OUT |= BIT2;   // enable pullup for input
    P1REN |= BIT2;   // 1.3 is button input

    // Timer pin output setup
    P2SEL = 0x14;
    P2SEL2 = 0;
    P2DIR = 0x14;

    // PWM configuration
    TA1CTL = ID_3 + MC_1 + TASSEL_2;
    TA1CCR0 = 40000;
    TA1CCTL1 = OUTMOD_7;
    TA1CCTL2 = OUTMOD_7;

}


// create a rising edge pulse for the shift register
// shouldn't need any delay in this
void pulseClk(void)
{
    P1OUT &= ~BIT0;
    P1OUT |= BIT1;
    idle(5);
    P1OUT &= ~BIT0;
    idle(5);
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
    UCB0CTL1 |= UCTXSTT + UCTR;       // generate start condition in transmit mode
    
    UCB0TXBUF = address & 0xFF;       // send address
    while(UCB0CTL1 & UCTXSTT);        // wait until start condition is sent
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
    UCB0CTL1 |= UCTXSTT + UCTR;       // generate start condition in transmit mode
    
    UCB0TXBUF = (address) & 0xFF;   // send addresss
    while(UCB0CTL1 & UCTXSTT);      // wait until start condition is sent
    while((!UCB0STAT & UCNACKIFG) && (!IFG2 & UCB0TXIFG)); // wait for ACK
    idle(15);//idle to wait for transfer to finish

    //Recieve
    UCB0CTL1 &= ~UCTR;          //receive
    UCB0CTL1 |= UCTXSTT;        //send another start condition in receive mode
    idle(15);
    while(UCB0CTL1 & UCTXSTT);  //wait until start condition is sent

    UCB0CTL1 |= UCTXSTP;        //send stop condition after 1 byte of data
    while(!IFG2  & UCB0RXIFG);  //wait for master to finish receiving
    idle(15);

    unsigned char value = UCB0RXBUF;
    while(UCB0CTL1 & UCTXSTP); //wait for stop condition to sent before moving on
    return value;
}

void calibrate(void)
{
    setRegister(REG_SECONDS, CURR_SEC);
    setRegister(REG_MINUTES, CURR_MIN);
    setRegister(REG_HOURS, CURR_HRS);
}

void shiftData(double wordArray)
{
    // shift out LSB first
    int i;
    for(i = 20; i >= 0; i--)
    {
        if( (int) wordArray>>i & 0x01) // need to cast for if statement
        {
            P1OUT |= BIT1;
        }       
        else
        {
            P1OUT &= ~BIT1;
        }
        pulseClk();
    }
}

double setLEDBits(void)
{
    wordArray = 0;

    if ((3 <= minutes) && (minutes <= 7))
    {
        wordArray += FIVE_MIN;
        wordArray += PAST;
    }
    else if ((8 <= minutes) && (minutes <= 12))
    {
        wordArray += TEN_MIN;
        wordArray += PAST;
    }
    else if ((13 <= minutes) && (minutes <= 17))
    {
        wordArray += QUARTER;
        wordArray += PAST;
    }
    else if ((18 <= minutes) && (minutes <= 22))
    {
        wordArray += TWENTY;
        wordArray += PAST;
    }
    else if ((23 <= minutes) && (minutes <= 27))
    {
        wordArray += (TWENTY + FIVE_MIN);
        wordArray += PAST;
    }
    else if ((27 <= minutes) && (minutes <= 32))
    {
        wordArray += HALF;
        wordArray += PAST;
    }
    else if ((33 <= minutes) && (minutes <= 37))
    {
        wordArray += (TWENTY + FIVE_MIN);
        wordArray += TO;
    }
    else if ((38 <= minutes) && (minutes <= 42))
    {
        wordArray += TWENTY;
        wordArray += TO;
    }
    else if ((43 <= minutes) && (minutes <= 47))
    {
        wordArray += QUARTER;
        wordArray += TO;
    }
    else if ((48 <= minutes) && (minutes <= 52))
    {
        wordArray += TEN_MIN;
        wordArray += TO;
    }
    else if ((53 <= minutes) && (minutes <= 57))
    {
        wordArray += FIVE_MIN;
        wordArray += TO;
    }

    switch (hours)
    {
        case 0:
            wordArray += TWELVE;
            break;

        case 1:
            wordArray += ONE;
            break;

        case 2:
            wordArray += TWO;
            break;

        case 3:
            wordArray += THREE;
            break;

        case 4:
            wordArray += FOUR;
            break;

        case 5:
            wordArray += FIVE_HR;
            break;

        case 6:
            wordArray += SIX;
            break;

        case 7:
            wordArray += SEVEN;
            break;

        case 8:
            wordArray += EIGHT;
            break;

        case 9:
            wordArray += NINE;
            break;

        case 10:
            wordArray += TEN_HR;
            break;

        case 11:
            wordArray += ELEVEN;
            break;

        case 12:
            wordArray += TWELVE;
            break;
    }

    return wordArray;
}