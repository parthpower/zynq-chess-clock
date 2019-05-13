#include "xparameters.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include <time.h>

// Parameter definitions
#define RESET_BUTTON		0x01
#define RIGHT_BUTTON		0x04
#define LEFT_BUTTON			0x02


#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define TMR_DEVICE_ID		XPAR_TMRCTR_0_DEVICE_ID
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_0_DEVICE_ID
#define LEDS_DEVICE_ID		XPAR_AXI_GPIO_1_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define INTC_TMR_INTERRUPT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR

#define BTN_INT 			XGPIO_IR_CH1_MASK
#define TMR_LOAD			XPAR_TMRCTR_0_CLOCK_FREQ_HZ

#define LEFT_TMR			0
#define RIGHT_TMR			1

XGpio LEDInst, BTNInst;
XScuGic INTCInst;
XTmrCtr TMRInst;
static int btn_value;
volatile int left_count=0;
volatile int right_count=0;

typedef enum current_state {INIT=0, PLAYER_LEFT=1,PLAYER_RIGHT=2} current_state;
volatile current_state state;

//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------
static void BTN_Intr_Handler(void *baseaddr_p);
static void TMR_Intr_Handler(void *baseaddr_p, u8 timerNumber);
static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr);

//----------------------------------------------------
// INTERRUPT HANDLER FUNCTIONS
// - called by the timer, button interrupt, performs
// - LED flashing
//----------------------------------------------------


void BTN_Intr_Handler(void *InstancePtr)
{
	// Disable GPIO interrupts
	XGpio_InterruptDisable(&BTNInst, BTN_INT);
//	xil_printf("BTN PRESSED!");
	// Ignore additional button presses
	if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) !=
			BTN_INT) {
			return;
		}
	btn_value = XGpio_DiscreteRead(&BTNInst, 1);

	// Do Stuff

	if(btn_value & LEFT_BUTTON ) {
		if(state == PLAYER_LEFT){
			// Pause Left Counter
			XTmrCtr_Stop(&TMRInst, LEFT_TMR);
			XTmrCtr_Reset(&TMRInst, LEFT_TMR);


			// Change state too PLAYER_RIGHT
			state = PLAYER_RIGHT;
			// Start timer for player 2
			XTmrCtr_Start(&TMRInst, RIGHT_TMR);
		} else if(state == INIT) {
			// For first click, start player's clock
			state = PLAYER_LEFT;
			XTmrCtr_Start(&TMRInst, LEFT_TMR);
		}

	}


	if(btn_value & RIGHT_BUTTON ) {
		if(state == PLAYER_RIGHT){
			// Pause Left Counter
			XTmrCtr_Stop(&TMRInst, RIGHT_TMR);
			XTmrCtr_Reset(&TMRInst, RIGHT_TMR);
			// Change state too PLAYER_RIGHT
			state = PLAYER_LEFT;
			// Start timer for player 2
			XTmrCtr_Start(&TMRInst, LEFT_TMR);
		}else if(state == INIT){
			// For first click, start player's clock
			state = PLAYER_RIGHT;
			XTmrCtr_Start(&TMRInst, RIGHT_TMR);
		}
	}

	if(btn_value & RESET_BUTTON){
		XTmrCtr_Stop(&TMRInst, RIGHT_TMR);
		XTmrCtr_Reset(&TMRInst, RIGHT_TMR);

		XTmrCtr_Stop(&TMRInst, LEFT_TMR);
		XTmrCtr_Reset(&TMRInst, LEFT_TMR);

		right_count =0;
		left_count =0;
		state = INIT;

	}
	// Increment counter based on button value
	// Reset if centre button pressed
//	if(btn_value != 1) led_data = led_data + btn_value;
//	else led_data = 0;
//    XGpio_DiscreteWrite(&LEDInst, 1, led_data);
    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}

void TMR_Intr_Handler(void *data, u8 timerNumber)
{
	if (XTmrCtr_IsExpired(&TMRInst,LEFT_TMR) == TRUE){
//		if(tmr_count == 3){
//			XTmrCtr_Stop(&TMRInst,0);
//			tmr_count = 0;
//			led_data++;
//			XGpio_DiscreteWrite(&LEDInst, 1, led_data);
//			XTmrCtr_Reset(&TMRInst,0);
//			XTmrCtr_Start(&TMRInst,0);
//
//		}
//		else tmr_count++;
		XTmrCtr_Reset(&TMRInst,LEFT_TMR);
		left_count++;

	}

	if (XTmrCtr_IsExpired(&TMRInst,RIGHT_TMR) == TRUE ){
			// Once timer has expired 3 times, stop, increment counter
			// reset timer and start running again
//			if(tmr_count == 3){
//				XTmrCtr_Stop(&TMRInst,0);
//				tmr_count = 0;
//				led_data++;
//				XGpio_DiscreteWrite(&LEDInst, 1, led_data);
//				XTmrCtr_Reset(&TMRInst,0);
//				XTmrCtr_Start(&TMRInst,0);
//
//			}
		XTmrCtr_Reset(&TMRInst,RIGHT_TMR);
		right_count++;
	}
}


/**
 * @brief prints number on single 7seg
 */

static const u8 segDecodeTable[] = {
		0b1111110,
		0b0110000,
		0b1101101,
		0b1111001,
		0b0110011,
		0b1011011,
		0b1011111,
		0b1110000,
		0b1111111,
		0b1111011,
};

int print_7seg(u8 number, u8 msb) {
	u8 data;
	if(number > 9) {
		return -1;
	}

	data = segDecodeTable[number];

	if(msb) {
			data |= 1<<7;
	}
	XGpio_DiscreteWrite(&LEDInst, 1, data);

	return data;
}



//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void)
{
  int status;
  //----------------------------------------------------
  // INITIALIZE THE PERIPHERALS & SET DIRECTIONS OF GPIO
  //----------------------------------------------------
  // Initialise LEDs
  status = XGpio_Initialize(&LEDInst, LEDS_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;
  // Initialise Push Buttons
  status = XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;
  // Set LEDs direction to outputs
  XGpio_SetDataDirection(&LEDInst, 1, 0x00);
  // Set all buttons direction to inputs
  XGpio_SetDataDirection(&BTNInst, 1, 0xFF);


  //----------------------------------------------------
  // SETUP THE TIMER
  //----------------------------------------------------
  status = XTmrCtr_Initialize(&TMRInst, TMR_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;

  status = XTmrCtr_SelfTest(&TMRInst,LEFT_TMR);
  if(status != XST_SUCCESS) return XST_FAILURE;
  status = XTmrCtr_SelfTest(&TMRInst,RIGHT_TMR);
  if(status != XST_SUCCESS) return XST_FAILURE;
  XTmrCtr_SetHandler(&TMRInst, TMR_Intr_Handler, (void *)&TMRInst);

  XTmrCtr_SetResetValue(&TMRInst, LEFT_TMR, TMR_LOAD);
  XTmrCtr_SetResetValue(&TMRInst, RIGHT_TMR, TMR_LOAD);
  XTmrCtr_SetOptions(&TMRInst, LEFT_TMR, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);
  XTmrCtr_SetOptions(&TMRInst, RIGHT_TMR, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);


  // Initialize interrupt controller
  status = IntcInitFunction(INTC_DEVICE_ID, &TMRInst, &BTNInst);
  if(status != XST_SUCCESS) return XST_FAILURE;

//  XTmrCtr_Start(&TMRInst, LEFT_TMR);
//  XTmrCtr_Start(&TMRInst, RIGHT_TMR);


  state	= INIT;

  while(1){
	  xil_printf("state:%d tmr_count:%d tmr_count2:%d\r\n",state, left_count, right_count);
	  if(state == PLAYER_LEFT) {
		  if(left_count > 99) {
			  left_count = 0;
		  }
		  print_7seg((u8)(left_count % 10) ,0);
		  print_7seg((u8)(left_count % 100 - left_count % 10),1);
	  }

	  if(state == PLAYER_RIGHT) {
		  if(right_count > 99) {
			  right_count = 0;
		  }
		  print_7seg((u8)(right_count % 10) ,0);
		  print_7seg((u8)(right_count % 100 - left_count % 10),1);
	  }

	  if(state == INIT) {
		  print_7seg(0,0);
		  print_7seg(0,1);
	  }
  }

  return 0;
}


//----------------------------------------------------
// INITIAL SETUP FUNCTIONS
//----------------------------------------------------

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
	// Enable interrupt
	XGpio_InterruptEnable(&BTNInst, BTN_INT);
	XGpio_InterruptGlobalEnable(&BTNInst);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 	 	 	 	 	 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
			 	 	 	 	 	 XScuGicInstancePtr);
	Xil_ExceptionEnable();


	return XST_SUCCESS;

}

int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr)
{
	XScuGic_Config *IntcConfig;
	int status;

	// Interrupt controller initialisation
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Call to interrupt setup
	status = InterruptSystemSetup(&INTCInst);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Connect GPIO interrupt to handler
	status = XScuGic_Connect(&INTCInst,
					  	  	 INTC_GPIO_INTERRUPT_ID,
					  	  	 (Xil_ExceptionHandler)BTN_Intr_Handler,
					  	  	 (void *)GpioInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;


	// Connect timer interrupt to handler
	status = XScuGic_Connect(&INTCInst,
							 INTC_TMR_INTERRUPT_ID,
							 (Xil_ExceptionHandler)TMR_Intr_Handler,
							 (void *)TmrInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Enable GPIO interrupts interrupt
	XGpio_InterruptEnable(GpioInstancePtr, 1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr);

	// Enable GPIO and timer interrupts in the controller
	XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);

	XScuGic_Enable(&INTCInst, INTC_TMR_INTERRUPT_ID);


	return XST_SUCCESS;
}

