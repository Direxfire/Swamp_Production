#include <msp430.h> 

#define Red_LED BIT1;
#define Green_LED BIT0;
#define Blue_LED BIT0;

#define Green_SW BIT7;
#define Red_SW BIT6;

#define Duty_Cycle 32768;


#define Relay BIT2;
#define On 1
#define Off 0

void Setup_GPIO(void);
void Setup_Timers(void);
void Setup_A2D(void);

int i;
int Water_Height;
unsigned Start_Loop;
unsigned Mode;
int New_Data;
int Time_Out;
int Reset;
void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    Mode = 0;
    New_Data = 0;
    Time_Out = 0;
    Start_Loop = 0;
    Setup_GPIO();
    Setup_Timers();
    Setup_A2D();
    __enable_interrupt();
    TB1CTL |= MC__UP;
    while(Start_Loop < 6){

        //Wait for setup to finish
    }

    P2IE |= Green_SW;
    P2IE |= Red_SW;

    while(1){
        if(New_Data == 1){
        switch(Mode){
        case 0:     //Idle
            P2OUT ^= Blue_LED;

        break;

        case 1: //Waiting to fill tank
            P2OUT &= ~Blue_LED;
            P1OUT &= ~Red_LED;
            P1OUT &= ~Relay; //Turn off relay
            //P1OUT |= Relay;
            if(Water_Height > 90){
                Mode = 2;
            }
            else{
                P1OUT ^= Green_LED;
            }
        break;

        case 2: //Actively Pumping
            P1OUT |= Relay;
            P2OUT &= ~Blue_LED;
            P1OUT &= ~Red_LED;
            P1OUT |= Green_LED;
            if(Water_Height < 90){
                Mode = 1;
            }
        break;

        case 3: //Off
            P1OUT |= Red_LED;
            P2OUT &= ~Blue_LED;
            P1OUT &= ~Green_LED;
            P1OUT &= ~Relay;
            Time_Out++;
            if(Time_Out >= 20){
                Mode = 4;
                Time_Out = 0;
                P1OUT |= Red_LED;
                P1OUT |= Green_LED;
            }
       break;

        case 4:
            P1OUT ^= Red_LED;
            P1OUT ^= Green_LED;
            Reset++;
            if(Reset > 10){
                Mode = 0;
                Reset = 0;
                P1OUT &= ~Red_LED;
                P1OUT &= ~Green_LED;
            }
        default:
       break;
        }
        New_Data = 0;
        }
    }
}




void Setup_GPIO(void){
    //Setup LEDS
    P1DIR |= BIT0;
    P1DIR |= BIT1;
    P1OUT &= ~Red_LED;
    P1OUT &= ~Green_LED;
    P2DIR |= Blue_LED;
    P2OUT &= ~Blue_LED;
    //Setup Relay pin
    P1DIR |= Relay;
    P1OUT &= ~Relay;

    //Setup input Switches
    P2DIR &= ~Green_SW;
    P2DIR &= ~Red_SW;
    P2IES &= ~Green_SW;
    P2IES &= ~Red_SW;

    P2IFG &= ~Green_SW;
    P2IFG &= ~Red_SW;

    P1SEL1 |= BIT6;
    P1SEL0 |= BIT6;

    //Enable IO (take out of low power mode)
    PM5CTL0 &= ~LOCKLPM5;

}

void Setup_Timers(void){
    TB1CTL |= TBCLR;
    TB1CTL |= TBSSEL__ACLK;
    TB1CTL |= MC__STOP;
    TB1CCR0 |= Duty_Cycle;               //Set to .5 seconds interrupt to trigger.....

    TB1CCTL0 |= CCIE;
    TB1CCTL0 &= ~CCIFG;
}

void Setup_A2D(void){
    /*
     // configure ADC
    ADCCTL0 &= ~ADCENC;                                     // Disable ADC
    ADCCTL0 |= ADCSHT_2 | ADCON;                            // Sampling time and ADC on
    ADCCTL1 |= ADCSHP | ADCSSEL_2 | ADCDIV_7 | ADCSHP;      // Signal source
    ADCCTL2 |= ADCRES;                                      //sets the ADC resolution to 10 bits.
    ADCIE |= ADCIE0;                                        // Enable interrupt
    //ADCMCTL0 |= LM19_PIN | ADCINCH_0;                       // Vref+ and input channel
    ADCMCTL0 |= LM19_PIN | ADCINCH_8;                       //Vref+ and input channel
    ADCCTL0 |= ADCENC;                                      // Enable ADC
    */

   ADCCTL0 &= ~ADCSHT;                                     //Clear ADCSHT from def. of ADCHST =01
   ADCCTL0 |= ADCSHT_2;                                    //Conversion cycles = 16 <- Might be a bit over kill but eh
   ADCCTL0 |= ADCON;                                        //Turn ADC ON <-  This is fine as we won't start it until the timer triggers

   ADCCTL1 |= ADCSSEL_2;                                    //ADC Clock Source = SMCLK
   ADCCTL1 |= ADCSHP;                                       //Sample Signal Source = Sampling timer

   ADCCTL2 &= ~ADCRES;                                      //Clear ADCRES from def. of ADCRES = 01
   ADCCTL2 |= ADCRES_2;                                     //Resolution = 8 bit (+/- .1 c this is plenty)

   ADCMCTL0 |= ADCINCH_6;                                   //Set ADC input channel to 8, P5.0
}


//Interrupts here
//Need GPIO Interrupts for SW1 and SW2
//Need Timer Interrupt for the Pump control (Maybe use a sensor of some kind??)



//Port 2 interrupt (Green and Red Switch)

#pragma vector = PORT2_VECTOR
__interrupt void ISR_PORT2_Switches(void){
    switch(P2IV){
    case 16:
        P2IE &= ~Green_SW;
        for(i = 0; i < 10000; i++);
        for(i = 0; i < 10000; i++);
        Mode = 1;
        //Send_UART_Message(2);
        break;
    case 14:
         P2IE &= ~Red_SW;
         for(i = 0; i < 10000; i++);
         for(i = 0; i < 10000; i++);
         Mode = 3;
        //Send_UART_Message(3);

         break;
    default:
        break;
    }
    P2IE |= Green_SW;
    P2IE |= Red_SW;
}

//Timer interrupt
//When this triggers, toggle the pump??
//Right now testing with LEDs.
#pragma vector = TIMER1_B0_VECTOR
__interrupt void Pump_Timer(void)
{
    TB0CCTL0 &= ~CCIFG;
    New_Data = 1;
    if(Start_Loop < 6){
        Start_Loop++;
        P2OUT ^= Blue_LED;
    }
    ADCCTL0 |= ADCENC | ADCSC;
    while (ADCCTL1 & ADCBUSY);
    Water_Height = ADCMEM0;


}
