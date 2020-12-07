//Code Leveraged off of Chandra Kiran Saladi
// Edited by Project 4
//https://github.com/ChandraKiranSaladi/MSP432-Ultrasonic-UART


#include "msp.h"
#include <stdint.h>

int miliseconds = 0;
int distance = 0;
long sensor = 0;

static void Delay(uint32_t loop)
{
    volatile uint32_t i;

    for (i = 0 ; i < loop ; i++);
}

int main(void)
{
    uint32_t i;
    uint32_t error = 0;

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

    /*         UART                      */
    CS->KEY = CS_KEY_VAL;                   // Unlock CS module for register access
    CS->CTL0 = 0;                           // Reset tuning parameters
    CS->CTL0 = CS_CTL0_DCORSEL_3;           // Set DCO to 12MHz (nominal, center of 8-16MHz range)
    CS->CTL1 = CS_CTL1_SELA_2 |             // Select ACLK = REFO
    CS_CTL1_SELS_3 |                        // SMCLK = DCO
    CS_CTL1_SELM_3;                         // MCLK = DCO
    CS->KEY = 0;                            // Lock CS module from unintended accesses


    // Configure UART pins
     P1->SEL0 |= BIT2 | BIT3;                // set 2-UART pin as secondary function

    //OUR CODE
    P2->DIR &= ~BIT7;                   //Set Port 2 pin 7 as output
    P2->REN |= BIT7;
    P2->OUT &= ~BIT7;
    //OUR CODE
    
    //CHANDRA'S CODE
    P2->IFG = 0;
    P2->IE |= BIT7;
    P2->IES &= ~BIT7;
    //UART SETUP
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST;
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_SWRST |
    EUSCI_B_CTLW0_SSEL__SMCLK;      // Configure eUSCI clock source for SMCLK
    // Baud Rate calculation
    // 12000000/(16*9600) = 78.125
    // Fractional portion = 0.125
    // User's Guide Table 21-4: UCBRSx = 0x10
    // UCBRFx = int ( (78.125-78)*16) = 2
    EUSCI_A0->BRW = 78;                     // 12000000/16/9600
    EUSCI_A0->MCTLW = (2 << EUSCI_A_MCTLW_BRF_OFS) |
            EUSCI_A_MCTLW_OS16;

    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Initialize eUSCI
    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;    // Clear eUSCI RX interrupt flag
    EUSCI_A0->IE &= ~EUSCI_A_IE_RXIE;        // Disable USCI_A0 RX interrupt


    /*              TIMER A0            */
    TIMER_A1->CCTL[0]= TIMER_A_CCTLN_CCIE;       // CCR0 interrupt enabled
    TIMER_A1->CCR[0] = 1000;                 // 1ms at 1mhz
    TIMER_A1->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC__UP | TIMER_A_CTL_CLR;

    __enable_irq();
    NVIC->ISER[1] = 1 << ((PORT2_IRQn) & 31);       // Assign interrupts to the NVIC vector 
    NVIC->ISER[0] = 1 << ((TA0_0_IRQn) & 31);
    //CHANDRA'S CODE

//OUR CODE
    while(1){
        P2->DIR |= BIT6;
        P2->OUT |= BIT6;
        Delay(100);//This delay is for the 10us trigger.
        P2->OUT &= ~BIT6;
        P2->IFG = 0;
        P2->IES &= ~BIT7;
        Delay(30000);
        distance = sensor/58;

//OUR CODE
        char buffer[50];
        //printf(buffer, "Distance is %d \n", distance); This would print to the console
        sprintf(buffer,"The distance is %d millimeters\n",distance); //Using a serial communication port to display the values.
        uart_puts(buffer);
     }

}

//CHANDRA'S CODE
int uart_puts(const char *str)
{
    int status = -1;

    if (str != '\0') {
        status = 0;

        while (*str != '\0') {
            /* Wait for the transmit buffer to be ready */
            while (!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));

            /* Transmit data */
            EUSCI_A0->TXBUF  = *str;

            /* If there is a line-feed, add a carriage return */
            if (*str == '\n') {
                /* Wait for the transmit buffer to be ready */
                while (!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
                EUSCI_A0->TXBUF = '\r';
            }

            str++;
        }
    }

    return status;
}


void PORT2_IRQHandler()
{

    if(P2->IFG & BIT7)  //is there interrupt pending?
    {
        if(!(P2->IES & BIT7)) // is this the rising edge?
        {

            TIMER_A1->CTL |= TIMER_A_CTL_CLR;   // clears timer A
            miliseconds = 0;
            P2->IES |=  BIT7;  //falling edge
        }
        else
        {
            sensor = (long)miliseconds*1000 + (long)TIMER_A1->R;    //calculating ECHO length

            P2->IES &=  ~BIT7;  //falling edge

        }
        P2->IFG &= ~BIT7;             //clear flag
    }
}


void TA0_0_IRQHandler(void)
{
    //    Interrupt gets triggered for every clock cycle in SMCLK Mode counting number of pulses
    miliseconds++;
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}
//CHANDRA'S CODE
