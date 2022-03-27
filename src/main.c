
/*----------------------------------------------------------------------------
    In this  code for Embedded Systems Lab 5 
    
we have achieved	the	following	activities:	
1. Activity	1: Download,	read	and	run	the	project	using	a	terminal	emulator
2. Activity	2: Modify	the	project	to	be	increase	and	decrease	the	flash	rate
3. Activity	3:	Control	the	timing	more	accurately	when	the	rate	changes

 *---------------------------------------------------------------------------*/
 
#include "cmsis_os2.h"
#include "string.h"
#include <stdbool.h>
#include "gpio.h"
#include "serialPort.h"
#include <MKL25Z4.h>


#define RESET_EVT (1)
osEventFlagsId_t errorFlags ;       // id of the event flags
osMessageQueueId_t controlMsgQ ;    // id for the message queue


/*--------------------------------------------------------------
 *   Thread t_redLED
 *       Flash Red LED to show an error
 *       States are: ERRORON, ERROROFF and NOERROR
 *       A signal is set to change the error status
 *          - LED off when no error
 *          - LED flashes when there is an error
 *--------------------------------------------------------------*/



osThreadId_t thread_greenredLED;      // id of thread to toggle green led


//defining states for green LED
#define GREEN 0
#define RED 1
#define GERROR 2
enum controlMsg_t {faster, slower} ;  // type for the messages
int delayone;
int delaytwo;


void greenredLEDThread (void *arg) {
	int ledState = GREEN ;
	int DELAYS[8]={500,1000,1500,2000,2500,3000,3500,4000}; //array for delays 
	
	
	enum controlMsg_t msg ;
	int position=1;
	//uint32_t timeDifferent=delays[position];
    osStatus_t status ;   // returned by message queue get
    while (1) {
        // wait for message from queue
	uint32_t clock_checkbefore = osKernelGetTickCount();
        status = osMessageQueueGet(controlMsgQ, &msg, NULL, delaytwo);
	uint32_t clock_checkafter = osKernelGetTickCount();
        if (status == osOK) 
	{
		if (msg == faster)		//checking if input message is "faster"
		{
			position--;
			if(position<0)
			{
				position=7;
			}
			delayone=DELAYS[position]-clock_checkafter+clock_checkbefore;
					
			if(delayone>=0)
			{
				delaytwo=delayone;
			}
			else
			{
				delaytwo=0;
			}
		}
					
            if (msg == slower)			//checking if input message is "faster"
  	    {
		position++;
		if(position>7)
		{
			position=0;
		}
		delayone=DELAYS[position]-clock_checkafter+clock_checkbefore;
							
		if(delayone>=0)
		{
			delaytwo=delayone;
		}
		else
		{
			delaytwo=0;
		}
	     }
						
        }
	delaytwo=DELAYS[position];
	if(status==osErrorTimeout)
	{
		switch (ledState) 
		{
                case GREEN:
            		greenLEDOnOff(LED_ON);
			redLEDOnOff(LED_OFF);
			ledState=RED;
                    	break ;
                    
                case RED:      
			greenLEDOnOff(LED_OFF);
			redLEDOnOff(LED_ON);
			ledState=GREEN;
		        break ;
                    
                case GERROR: 
                    // ignore other messages - already in error state
                    break ;
	   
		}
    	}
   }
}





/*------------------------------------------------------------
 *  Thread t_command
 *      Request user command
 *      
 *
 *------------------------------------------------------------*/
osThreadId_t t_command;        /* id of thread to receive command */

/* const */ char prompt[] = "Command: on / off / reset>" ;
/* const */ char empty[] = "" ;

void commandThread (void *arg) {
    char response[6] ;  // buffer for response string
    enum controlMsg_t msg ;
    bool valid ;
    while (1) {
        //sendMsg(empty, CRLF) ;
        sendMsg(prompt, NOLINE) ;
        readLine(response, 6) ;  // 
        valid = true ;
        if (strcmp(response, "faster") == 0) {
            msg = faster ;
        } else if (strcmp(response, "slower") == 0) {
            msg = slower;
        } 
        else valid = false ;
        
        if (valid) {
            osMessageQueuePut(controlMsgQ, &msg, 0, NULL);  // Send Message
        } else {
            sendMsg(response, NOLINE) ;
            sendMsg(" not recognised", CRLF) ;
        }
    }
}

/*----------------------------------------------------------------------------
 * Application main
 *   Initialise I/O
 *   Initialise kernel
 *   Create threads
 *   Start kernel
 *---------------------------------------------------------------------------*/

int main (void) { 
    
    // System Initialization
    SystemCoreClockUpdate();

    // Initialise peripherals
    configureGPIOoutput();
    //configureGPIOinput();
    init_UART0(115200) ;

    // Initialize CMSIS-RTOS
    osKernelInitialize();
    
    // Create event flags
    errorFlags = osEventFlagsNew(NULL);
    
    // create message queue
    controlMsgQ = osMessageQueueNew(2, sizeof(enum controlMsg_t), NULL) ;

    // initialise serial port 
    initSerialPort() ;

    // Create threads
    //t_redLED = osThreadNew(redLEDThread, NULL, NULL); 
    thread_greenredLED = osThreadNew(greenredLEDThread, NULL, NULL);
    t_command = osThreadNew(commandThread, NULL, NULL); 
    
    osKernelStart();    // Start thread execution - DOES NOT RETURN
    for (;;) {}         // Only executed when an error occurs
}
