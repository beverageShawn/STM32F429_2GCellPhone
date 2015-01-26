#include "phone.h"

/* SIMCOM module */
#include "simcom.h"

/* mylib include */
#include "mylib.h"

// Event Listener
GListener gl;

// Utility Tisk Variable
TickType_t ticks_to_sleep = SLEEP_TICKS, ticks_to_nextchar = NEXT_CHAR_TICKS;
TickType_t ticks_now, ticks_last, ticks_of_last_char;

// Container Handle
extern GHandle  MainMenuContainer, KeypadContainer, CallContainer, MsgContainer, CallOutContainer, CallInContainer, ReadMsgContainer, cameraContainer, photoContainer, imageContainer;

// Button & Label Handle
extern GHandle	RETURNBtn, PHONEBtn, READSMSBtn, WRITESMSBtn, CallBtn, CancelBtn, OneBtn, TwoBtn, ThreeBtn, FourBtn, FiveBtn, SixBtn, SevenBtn, EightBtn, NineBtn, StarBtn, ZeroBtn, JingBtn, AnswerBtn, DeclineBtn, HangoffBtn, BackspaceBtn, SendBtn, SwapBtn, cameraBtn, photoBtn, photoToCameraBtn, cameraToPhotoBtn, photoReturnBtn, cameraReturnBtn, rightGoBtn, leftGoBtn, readMsgToMain, writeMsgToMain,callToMain;
extern GHandle  NumLabel, MsgLabel[3], TargetLabel, IncomingLabel, OutgoingLabel, ReadMsgLabel[10];

extern GButtonObject right,left;

extern uint8_t locking;

extern FATFS fatfs_fs;


TaskHandle_t Phone_Handle;
TaskHandle_t USB_Handle;
TaskHandle_t LCD_Handle;
enum State next, current;

FATFS USB_Fs;
FIL USB_Fil;
GFILE * usbimage;

int cursor = -1;
char fileRecord[20][20];
bool_t filelistInitialDone = FALSE;


void prvPhoneTask(void *pvParameters)
{
    current = MAIN;
	next = MAIN;
    TickType_t xLastWakeTime;

    /* Initialize task and some utils */
	TM_DISCO_LedOn(LED_RED);
    SIMCOM_Init();//abc
	TM_DISCO_LedOff(LED_RED);
	gfxInit();
	gwinAttachMouse(0);
	createsUI();
	gwinShow(MainMenuContainer);
    // Create the check incoming task
 
    // Create the fatfs task
    xTaskCreate( prvUSBTask, "USB", configMINIMAL_STACK_SIZE, NULL, mainPhone_TASK_PRIORITY - 1, NULL);
	 // create phone call in task
    xTaskCreate( prvIncomingTask, "Check incoming", configMINIMAL_STACK_SIZE , NULL, mainCheck_TASK_PRIORITY, NULL);//abc
	 // creack check button task
    xTaskCreate( prvButtonTask, "Check button", configMINIMAL_STACK_SIZE * 4, NULL, mainButton_TASK_PRIORITY, &LCD_Handle);

    // Initialize the xLastWakeTime variable with current time.
    xLastWakeTime = xTaskGetTickCount(); 
	// counting time, used as make a phone sleep
	ticks_last = xLastWakeTime;
	ticks_to_sleep = SLEEP_TICKS;
    while(1) 
    {
			// delay the procedure, need not do the procedure all the time
        vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_DELAY);	
		ticks_now = xTaskGetTickCount();
		
		if ( current != DURING ) 
		{
			if ( ticks_to_sleep < (ticks_now - ticks_last)) 
			{
				LCD_DisplayOff();
				locking = 1;
				vTaskSuspend( LCD_Handle );
				vTaskSuspend( NULL );
				ticks_to_sleep = SLEEP_TICKS;
			} 
			else 
			{
				ticks_to_sleep -= (ticks_now - ticks_last);
			}
		} 
		else 
		{
			ticks_to_sleep = SLEEP_TICKS;
		}
        if(current == next) {
			ticks_to_sleep = SLEEP_TICKS;
            continue;
        }

        current = next;
		hideall();
        switch(current) {	// when event is happened, doing the following things
            // TODO : Main screen display
            case MAIN:
				gwinShow(MainMenuContainer);
                break;
            // TODO : Incoming screen display
            case INCOMING:
				gwinShow(CallInContainer);
                break;
            // TODO : During calling display
            case DURING:
				gwinShow(CallOutContainer);
                break;
            // TODO : Dial screen display
            case DIAL:
				gwinShow(CallContainer);
				gwinShow(KeypadContainer);
                break;
            // TODO : Send message screen display
            case SEND:
				gwinShow(MsgContainer);
				gwinShow(KeypadContainer);
                break;
            // TODO : Read message screen display
            case READ:
				gwinShow(ReadMsgContainer);
                break;
				case CAMERA:
				gwinShow(cameraContainer);
                break;
				case PHOTO:
				gwinShow(photoContainer);
                break;
            default:
				break;
        }
		ticks_last = ticks_now;
    }
}

void prvButtonTask(void *pvParameters)	// define button events
{
	GEvent* pe;
	SMS_STRUCT sms[3];

	char labeltext[16] = "";
	char msgbuffer[31] = "";
	char numbuffer[16] = "";
	char linebuffer[1][31];
	char last_char = 0;
	char *last_char_in_lb;
	uint32_t textindex = 0, msgindex = 0, currentline = 0, lineindex = 0, numindex = 0, changing = 0, msgORnum = 0, i, j, totalmsg, readline = 0;

	geventListenerInit(&gl);
	gwinAttachListener(&gl);
	
	ticks_to_sleep = SLEEP_TICKS;
	ticks_now = xTaskGetTickCount();
	ticks_of_last_char = 0;
	
	while (TRUE) {
		pe = geventEventWait(&gl, TIME_INFINITE); // wait forever, so if screen not touched, the task will be blocked
		ticks_now = xTaskGetTickCount();
		ticks_to_sleep = SLEEP_TICKS;
		// define when button is pushed, what should do next
		switch(pe->type) {
			case GEVENT_GWIN_BUTTON:
				if (((GEventGWinButton*)pe)->button == PHONEBtn) {
					// change status to DIAL
					next = DIAL;
				} else if (((GEventGWinButton*)pe)->button == RETURNBtn) {
					next = MAIN;
				} else if (((GEventGWinButton*)pe)->button == photoBtn) {
					next = PHOTO;
				} else if (((GEventGWinButton*)pe)->button == cameraBtn) {
					next = CAMERA;
				} else if (((GEventGWinButton*)pe)->button == cameraReturnBtn) {
					next = MAIN; 
					createsUI();
				} else if (((GEventGWinButton*)pe)->button == photoReturnBtn) {
					next = MAIN; 
					createsUI();
				} else if (((GEventGWinButton*)pe)->button == callToMain) {
					next = MAIN; 
				} else if (((GEventGWinButton*)pe)->button == writeMsgToMain) {
					next = MAIN; 
				} else if (((GEventGWinButton*)pe)->button == readMsgToMain) {
					next = MAIN; 
					createsUI();					
				} else if (((GEventGWinButton*)pe)->button == leftGoBtn) {
					next = PHOTO; 
					putImage(FALSE);	// FALSE means image go left

				} else if (((GEventGWinButton*)pe)->button == rightGoBtn) {
					next = PHOTO; 
					putImage(TRUE);	// TRUE means image go right

				} else if (((GEventGWinButton*)pe)->button == photoToCameraBtn) {
					next = CAMERA;
					createsUI();
				} else if (((GEventGWinButton*)pe)->button == cameraToPhotoBtn) {
					next = PHOTO; 
				} else if (((GEventGWinButton*)pe)->button == WRITESMSBtn) {
					// change status to READ
					msgORnum = 0;
					msgbuffer[0] = '\0';
					msgindex = 0;
					linebuffer[0][0] = '\0';
					currentline = 0;
					lineindex = 0;
					numbuffer[0] = '\0';
					numindex = 0;
					gwinSetText(MsgLabel[0], linebuffer[0], TRUE);
					gwinSetText(TargetLabel, numbuffer, TRUE);
					next = SEND;
				} else if (((GEventGWinButton*)pe)->button == READSMSBtn) {
					next = READ;
					//TODO: read sms here
					linebuffer[0][0] = '\0';
					lineindex = 0;
					totalmsg = SIMCOM_ReadSMS(sms);//abc
					if (totalmsg == 0) {
						gwinSetText(ReadMsgLabel[4], "NO MSG AVAILABLE", TRUE);
						continue;
					}
					readline = 0;
					currentline = 0;
					for (i = 0; i < totalmsg; i++) {
						if (i == 3)
							break;
						strcpy(linebuffer[0], "FROM: ");
						strcat(linebuffer[0], sms[i].number);
						gwinSetText(ReadMsgLabel[readline++], linebuffer[0], TRUE);
						strcpy(linebuffer[0], sms[i].content);
						gwinSetText(ReadMsgLabel[readline++], linebuffer[0], TRUE);
					}
					
				} else if (((GEventGWinButton*)pe)->button == CallBtn) {
					// change status to dial and call out
					next = DURING;
					SIMCOM_Dial(labeltext);//abc
					calling(labeltext);//abc
				} else if (((GEventGWinButton*)pe)->button == CancelBtn) {
					// Clear Number Label
					textindex = 0;
					labeltext[0] = '\0';
					changing = 1;
				} else if (((GEventGWinButton*)pe)->button == AnswerBtn) {
					SIMCOM_Answer();//abc
					next = DURING;
					calling("UNKNOWN");//abc
				} else if (((GEventGWinButton*)pe)->button == DeclineBtn) {
					SIMCOM_HangUp();//abc
					next = MAIN;
				} else if (((GEventGWinButton*)pe)->button == SendBtn) {
					// TODO: send message
					SIMCOM_SendSMS(numbuffer, msgbuffer);//abc
					next = MAIN;
				} else if (((GEventGWinButton*)pe)->button == BackspaceBtn) {
					if (msgORnum == 0) {
						if (msgindex == 0)
							continue;
						changing = 1;
						last_char = 0;
						msgindex--;
						*last_char_in_lb = '\0';
						if (lineindex == 0) {
							lineindex = 30;
							currentline--;
						} else {
							lineindex--;
						}
					} else {
						if (numindex == 0)
							continue;
						changing = 1;
						numindex--;
					}
				} else if (((GEventGWinButton*)pe)->button == SwapBtn) {
					if (msgORnum == 0)
						msgORnum = 1;
					else
						msgORnum = 0;
				} else if (((GEventGWinButton*)pe)->button == OneBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '1';
						labeltext[textindex] = '\0';
					} else if (current == SEND) { //if current is SEND MSG UI
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 1) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 1) + 1;
								if (i > 4 || char_of_button[1][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[1][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[1][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[1][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[1][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '1';
						}
					}
				} else if (((GEventGWinButton*)pe)->button == TwoBtn) {
					changing = 1;
					if (current == DIAL) {	
						labeltext[textindex++] = '2';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 2) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 2) + 1;
								if (i > 4 || char_of_button[2][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[2][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[2][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[2][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[2][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '2';
						}
					}
				} else if (((GEventGWinButton*)pe)->button == ThreeBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '3';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 3) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 3) + 1;
								if (i > 4 || char_of_button[3][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[3][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[3][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[3][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[3][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '3';
						}
					}
				} else if (((GEventGWinButton*)pe)->button == FourBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '4';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 4) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 4) + 1;
								if (i > 4 || char_of_button[4][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[4][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[4][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[4][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[4][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '4';
						}
						
					}
				} else if (((GEventGWinButton*)pe)->button == FiveBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '5';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 5) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 5) + 1;
								if (i > 4 || char_of_button[5][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[5][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[5][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[5][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[5][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '5';
						}
						
					}
				} else if (((GEventGWinButton*)pe)->button == SixBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '6';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 6) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 6) + 1;
								if (i > 4 || char_of_button[6][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[6][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[6][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[6][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[6][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '6';
						}
							
					}
				} else if (((GEventGWinButton*)pe)->button == SevenBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '7';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 7) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 7) + 1;
								if (i > 4 || char_of_button[7][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[7][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[7][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[7][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[7][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '7';
						}
							
					}
				} else if (((GEventGWinButton*)pe)->button == EightBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '8';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 8) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 8) + 1;
								if (i > 4 || char_of_button[8][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[8][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[8][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[8][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[8][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '8';
						}
						
					}
				} else if (((GEventGWinButton*)pe)->button == NineBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '9';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 9) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 9) + 1;
								if (i > 4 || char_of_button[9][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[9][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[9][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[9][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[9][0];
							}
						} else { //write to number
							numbuffer[numindex++] = '9';
						}
						
					}
				} else if (((GEventGWinButton*)pe)->button == ZeroBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '0';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							if (last_char != 0 && char_in_button(last_char, 0) != -1 && (ticks_now-ticks_of_last_char <= NEXT_CHAR_TICKS)) {
								i = char_in_button(last_char, 0) + 1;
								if (i > 4 || char_of_button[0][i] == 0) {
									i = 0;
								}
								last_char = char_of_button[0][i];
								msgbuffer[msgindex - 1] = last_char;
								*last_char_in_lb = char_of_button[0][i];
							} else {
								msgbuffer[msgindex++] = char_of_button[0][0];
								last_char = msgbuffer[msgindex - 1];
								linebuffer[currentline][lineindex++] = char_of_button[0][0];
							}	
						} else { //write to number
							numbuffer[numindex++] = '0';
						}
					}
				} else if (((GEventGWinButton*)pe)->button == StarBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '*';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							msgbuffer[msgindex++] = '*';
							linebuffer[currentline][lineindex++] = '*';
						} else { //write to number
							numbuffer[numindex++] = '*';
						}
					}
				} else if (((GEventGWinButton*)pe)->button == JingBtn) {
					changing = 1;
					if (current == DIAL) {
						labeltext[textindex++] = '#';
						labeltext[textindex] = '\0';
					} else if (current == SEND) {
						if (msgORnum == 0) { // write to msg
							msgbuffer[msgindex++] = '#';
							linebuffer[currentline][lineindex++] = '#';
						} else { //write to number
							numbuffer[numindex++] = '#';
						}
					}
				}
				break;
			default:
				break;
		}
		if (changing == 1) {
			if (current == DIAL) {
				gwinSetText(NumLabel, labeltext, TRUE);
				changing = 0;
			} else if (current == SEND) {
				if (msgORnum == 0) { // write to msg
					changing = 0;
					msgbuffer[msgindex] = '\0';
					linebuffer[currentline][lineindex] = '\0';
					ticks_of_last_char = ticks_now;

					if (lineindex > 0)
						last_char_in_lb = &(linebuffer[currentline][lineindex-1]);
					else if (currentline != 0)
						last_char_in_lb = &(linebuffer[currentline-1][0]);

					//gwinSetText(MsgLabel[1], linebuffer[1], TRUE);
					//gwinSetText(MsgLabel[2], linebuffer[2], TRUE);
					gwinSetText(MsgLabel[0], linebuffer[0], TRUE);
				} else { //write to number
					changing = 0;
					numbuffer[numindex] = '\0';
					gwinSetText(TargetLabel, numbuffer, TRUE);
				}
			}
		}
	}
	
}

/* This task is used for checking incoming call.
 * Because the SIMCOM module doesn't have interrupt
 * when calling income. So we check manually every
 * time period.
 */
void prvIncomingTask(void *pvParameters)
{
    TickType_t xLastWakeTime;

    // Initialize the xLastWakeTime variable with current time.
    xLastWakeTime = xTaskGetTickCount();
	//abc
    while(1) 
    {
        vTaskDelayUntil(&xLastWakeTime, INCOMING_TASK_DELAY);

        if(SIMCOM_CheckPhone()) 
        {
			if ( locking == 1) 
			{
				LCD_DisplayOn();
				ticks_last = xTaskGetTickCount();
				ticks_to_sleep = SLEEP_TICKS;
				locking = 0;
				vTaskResume( LCD_Handle );
				vTaskResume( Phone_Handle );
				vTaskResume( USB_Handle );
			}
            next = INCOMING;
			
        }

    }
	//abc
}


// define USB task
void prvUSBTask(void *pvParameters)
{
    TickType_t xLastWakeTime;
    uint8_t cam_pid = 0x00;

    // Initialize the xLastWakeTime variable with current time.
    xLastWakeTime = xTaskGetTickCount();

   
	while(1) 
   	 {
        vTaskDelayUntil(&xLastWakeTime, USB_TASK_DELAY);
			//  detect if usb is plugged in
        TM_USB_MSCHOST_Process();

		if (TM_USB_MSCHOST_Device() == TM_USB_MSCHOST_Result_Connected) 
        	{
        	if (f_mount(&fatfs_fs, "1:", 1) == FR_OK)
        		{
        		TM_DISCO_LedOn(LED_GREEN);

	        	initialUSBfilelist(TRUE);
	        	
        		}
		}
		else
		{
			//	;
			TM_DISCO_LedOff(LED_GREEN);		
		}
//			TM_DISCO_LedOff(LED_GREEN);		

		//TM_I2C_Write(I2C1, SLAVE_ADDRESS_WRITE, 0x8c, 2);
		cam_pid = TM_I2C_Read(I2C1, SLAVE_ADDRESS_READ, REG_PID);
		
		if(cam_pid == REG_REG76)
			TM_DISCO_LedOn(LED_GREEN);				
		else
			TM_DISCO_LedOff(LED_GREEN);
		
    	}
}


void prvFileSystemTask(void *pvParameters)
{

    FIL* f;    
    char buffer[50] = {'\0'};
    uint8_t write = 1;
    uint32_t free, total;
    uint32_t i, width = 320, height = 240, size_in_file = 2 * width * height;
    unsigned char info[54];
    //unsigned char* data_from_file = (unsigned char*) malloc (size_in_file + 64);
    unsigned char temp0; 
    unsigned char temp1;
    uint16_t pixel_data, a;
    //bool_t a;
    TickType_t xLastWakeTime;    
	xLastWakeTime = xTaskGetTickCount();

	                	

	

    //TM_DISCO_LedOn(LED_GREEN);
    while (1) 
    {
 		vTaskDelayUntil(&xLastWakeTime, MAIN_TASK_DELAY);
		
        /* Host Task handler */
        /* This have to be called periodically as fast as possible */
        //TM_USB_MSCHOST_Process();
        TM_DISCO_LedOn(LED_RED);
        /* Device is connected and ready to use */
		    		if (f_open(&USB_Fil, "1:usb.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) 
                    {
                        /* We want to write only once */
                        write = 0;
                        
                        /* Get total and free space on USB */
                        TM_FATFS_USBDriveSize(&total, &free);
                        
                        /* Put data */
                        f_puts("This is my first file with USB and FatFS\n", &USB_Fil);
                        f_puts("with USB MSC HOST library from stm32f4-discovery.com\n", &USB_Fil);
                        f_puts("----------------------------------------------------\n", &USB_Fil);
                        f_puts("USB total and free space:\n\n", &USB_Fil);
                        /* Total space */
                        sprintf(buffer, "Total: %8u kB; %5u MB; %2u GB\n", total, total / 1024, total / 1048576);
                        f_puts(buffer, &USB_Fil);
                        /* Free space */
                        sprintf(buffer, "Free:  %8u kB; %5u MB; %2u GB\n", free, free / 1024, free / 1048576);
                        f_puts(buffer, &USB_Fil);
                        f_puts("----------------------------------------------------\n", &USB_Fil);
                        /* Close USB file */
                        f_close(&USB_Fil);
 						
                        /* Turn GREEN LED On and RED LED Off */
                        /* Indicate successful write */
                        TM_DISCO_LedOff(LED_RED);
                	}
    }

}

void putImage(bool_t goRight)
{
	if (!filelistInitialDone) return;
	coord_t	swidth, sheight;
	GHandle ghImage1;
	gdispImage myImage;
	GWidgetInit wi;
	gwinWidgetClearInit(&wi);
	
	wi.g.x = 0; 	wi.g.y = 0; 
	wi.g.width = 240; 	wi.g.height = 320;
	wi.g.show = TRUE;
	wi.g.parent = imageContainer;
	char fileName[20];
	fileName[0] = '1';
	fileName[1] = ':';
	int  i = 0;
	if (goRight){
		cursor++;
		if (fileRecord[cursor][i] == '\0') cursor = 0;
	}
	else {
		cursor--;
		if (cursor < 0){ 
			cursor = 0;
			while (fileRecord[cursor][i] != '\0' ){
				cursor++;
			}
			cursor--;
		}
	}

	while (1)
	{
		fileName[i+2] = fileRecord[cursor][i];
		if (fileRecord[cursor][i] == '\0') break;
		i++; 
	}
	gdispImageOpenFile(&myImage, fileName);
	swidth = gdispGetWidth();
	sheight = gdispGetHeight();
	ghImage1 = gwinImageCreate(0, &wi.g);
	gdispImageDraw(&myImage, 0, 0, swidth, sheight, 0, 0);
	gdispImageClose(&myImage);
	createsPhotoUI();
}


/*
	read all the name of files in side the USB,
		if showOnlyBmp is TRUE, then read only bmp file, means only bmp file will be store into fileList
		else if FALSE, then every file name in the USB will be stored into fileList
*/
void initialUSBfilelist(bool_t showOnlyBMP)
{
	filelistInitialDone = TRUE;
	/*
		gfileOpenFileList('F',"1:",FALSE);
		F means FATFS, 
		1:  means the mount point
		FASLE measn not directory (dont read directory, read file only)
	*/
	gfileList *ptr = gfileOpenFileList('F',"1:",FALSE);
						
	int i = 0, j = 0;
	
	/* 
		read the USB
	*/
	while (1)
	{
		char *t = gfileReadFileList(ptr); // gfileReadFileList(ptr): read next filename with the pointer(ptr)
		bool_t skip = FALSE; // default set all filename should be record, so the skip should be false
		if (showOnlyBMP)	// if show only BMP file, the do the folling while(1)
		while(1)
		{
			if (t[j] == '\0' && j > 3)	// read the last three words, if is BMP or bmp
			{	
				if ( (t[j-1] == 'P' && t[j-2] == 'M' && t[j-3] == 'B') ||
					  (t[j-1] == 'p' && t[j-2] == 'm' && t[j-3] == 'b') 		)
				{
					break;
				}
			}
			
			if (t[j] == '\0') // end of the filename[j], if jth is the last word, and it is '\0' ( end of a string) 
			{
				skip = TRUE;
				break;
			}
			j++;
		}
		

		j = 0; // reset the cursor

		if (!skip) // TRUE means skip to reacord the filename into filelist
		while(1)	// record the filename into a filelist
		{
			if (t[j] == '\0') break;
			fileRecord[i][j] = t[j];				
			j++;
		}

		j = 0;
					
		if (t[j] == '\0') break;	// if t[0] == '\0', means ptr had already pointed to end of the filelist, so it return NULL
		if (!skip)			
		i++;
	}
}
