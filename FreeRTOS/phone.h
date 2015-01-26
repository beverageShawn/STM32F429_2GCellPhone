#ifndef _PHONE_H
#define _PHONE_H

#include <FreeRTOS.h>
#include <task.h>
#include "tm_stm32f4_fatfs.h"
#include "tm_stm32f4_usb_msc_host.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_usart.h"
#include "tm_stm32f4_i2c.h"
#include "OV7670FIFO.h"
#include "gfx.h"

/* Task priority */
#define mainPhone_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainButton_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )
#define mainCheck_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )

/* Main task delay period */
#define MAIN_TASK_DELAY (1000 / portTICK_PERIOD_MS)
#define USB_TASK_DELAY (50 / portTICK_PERIOD_MS)
#define INCOMING_TASK_DELAY (25000 / portTICK_PERIOD_MS)

/* Incoming call checking delay period */
//#define INCOMING_TASK_DELAY (10000 / portTICK_PERIOD_MS)

/* State Number
 * 0 : Main
 * 1 : Call incoming
 * 2 : During call
 * 3 : Dial keypad
 * 4 : Send SMS
 * 5 : Read SMS
 */
enum State {MAIN, INCOMING, DURING, DIAL, SEND, READ, CAMERA, PHOTO};

/* Phone main task */
void prvPhoneTask(void *pvParameters);

/* Touch panel event waiting task */
void prvButtonTask(void *pvParameters);

/* Check incoming call task */
void prvIncomingTask(void *pvParameters);

void prvFileSystemTask(void *pvParameters);

void prvUSBTask(void *pvParameters);

//void putImage();
void putImage(bool_t goRight);

void initialUSBfilelist(bool_t showOnlyBMP);

#endif
