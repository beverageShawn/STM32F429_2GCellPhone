/*
 * Copyright (c) 2012, 2013, Joel Bodenmann aka Tectu <joel@unormal.org>
 * Copyright (c) 2012, 2013, Andrew Hannam aka inmarket
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the <organization> nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Kernel includes. */
#include "FreeRTOS.h"

/* Hardware and starter kit includes */
#include "stm32f4xx.h"

/* Demo application includes. */
#include "partest.h"
#include "flash.h"

/* uGFX includes. */
#include "gfx.h"

/* Tasks */
#include "phone.h"
#include "mylib.h"
#include "simcom.h"

#include "defines.h"
/* Include fatfs and usb libraries */
#include "tm_stm32f4_fatfs.h"
#include "tm_stm32f4_usb_msc_host.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_usart.h"
#include "tm_stm32f4_i2c.h"
#include <stdio.h>
/* Task priorities. */
#define mainFLASH_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )
void fileSystemTask();
static void prvSetupHardware(void)
{
    /* Setup STM32 system (clock, PLL and Flash configuration) */
    SystemInit();

    TM_USB_MSCHOST_Init();

    TM_DELAY_Init();

    TM_DISCO_LedInit(); //conclict with vpartestinit()

    /* Ensure all priority bits are assigned as preemption priority bits. */
    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

    /* Setup the LED outputs. */
    //vParTestInitialise();
}

uint8_t locking = 0;

extern TickType_t ticks_now, ticks_last, ticks_to_sleep;
extern TaskHandle_t Phone_Handle;
extern TaskHandle_t USB_Handle;
extern TaskHandle_t LCD_Handle;
extern enum State next, current;

void lockphone(void)
{
	if (current == MAIN) {
		LCD_DisplayOff();
		locking = 1;
		vTaskSuspend( Phone_Handle );
		vTaskSuspend( LCD_Handle );
	} else if (current != DURING && current != INCOMING) {
		next = MAIN;
	}
}

void wakeup(void)
{
	BaseType_t xYieldRequired;	
	locking = 0;
	LCD_DisplayOn();
	ticks_last = xTaskGetTickCount();
	ticks_to_sleep = SLEEP_TICKS;
	xYieldRequired = xTaskResumeFromISR( Phone_Handle );
	xYieldRequired = xTaskResumeFromISR( LCD_Handle );
	//taskYIELD();
	if (xYieldRequired == pdTRUE) {
		portYIELD_FROM_ISR(xYieldRequired);
	}
}

/* Set button as interrupt */
void initButton(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef gpio_init;
	GPIO_StructInit(&gpio_init);
	gpio_init.GPIO_Mode = GPIO_Mode_IN;
	gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gpio_init.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOA, &gpio_init);


	EXTI_InitTypeDef exti_init;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,EXTI_PinSource0);
	exti_init.EXTI_Line = EXTI_Line0;
	exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_init.EXTI_Trigger = EXTI_Trigger_Rising;
	exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

	NVIC_InitTypeDef nvic_init;
	nvic_init.NVIC_IRQChannel = EXTI0_IRQn;
	nvic_init.NVIC_IRQChannelPreemptionPriority = 0x0F;
	nvic_init.NVIC_IRQChannelSubPriority = 0x0F;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init);

/*    GPIO_InitTypeDef gpio_init2;
    GPIO_StructInit(&gpio_init2);
    gpio_init2.GPIO_Mode = GPIO_Mode_AF;
    gpio_init2.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpio_init2.GPIO_OType = GPIO_OType_PP;
    gpio_init2.GPIO_Pin = GPIO_Pin_6;
    gpio_init2.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init2);
    TM_I2C_Init(I2C1, TM_I2C_PinsPack_1, TM_I2C_CLOCK_FAST_MODE);
*/

}

void EXTI0_IRQHandler(void)
{
	uint32_t i;
	if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
		if (locking == 0) {
			lockphone();
		} else {
			wakeup();
		}
		i = 100000;
		while (i > 0)
			i--;
		
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

int main(void)
{
	/* Configure the hardware ready to run the test. */
	prvSetupHardware();

	/* Initialize USB MSC HOST */
    
	initButton();

	/* Start the Phone task */
	xTaskCreate( prvPhoneTask, "Phone", configMINIMAL_STACK_SIZE * 2, NULL, mainPhone_TASK_PRIORITY, &Phone_Handle);
    //fatfstest();//good
	//xTaskCreate( prvFileSystemTask, "USB", configMINIMAL_STACK_SIZE * 303, NULL, mainPhone_TASK_PRIORITY, &USB_Handle);
	
	/* Start the scheduler. */
	vTaskStartScheduler();
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
	(void)pxTask;
	(void)pcTaskName;

	for (;;);
}

void vApplicationTickHook(void)
{
}



void fatfstest(void)
{
        FATFS USB_Fs;
    FIL USB_Fil;
    FIL* f;    
    char buffer[50];
    uint8_t write = 1;
    uint32_t free, total;
    uint32_t i, width = 320, height = 240, size_in_file = 2 * width * height;
    unsigned char info[54];
    //unsigned char* data_from_file = (unsigned char*) malloc (size_in_file + 64);
    unsigned char temp0; 
    unsigned char temp1;
    uint16_t pixel_data;
       
    while (1) 
    {
        /* Host Task handler */
        /* This have to be called periodically as fast as possible */
        TM_USB_MSCHOST_Process();
        /* Device is connected and ready to use */
        if (TM_USB_MSCHOST_Device() == TM_USB_MSCHOST_Result_Connected) 
        {
            /* If we didn't write data already */
            if (write) 
            {
                /* Try to mount USB device */
                /* USB is at 1: */
                if (f_mount(&USB_Fs, "1:", 1) == FR_OK) 
                {
                    TM_DISCO_LedOn(LED_GREEN);
                    /* Mounted ok */
                    /* Try to open USB file */
                    if (f_open(&USB_Fil, "1:usb_file.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) 
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
                        TM_DISCO_LedOn(LED_GREEN);
                        TM_DISCO_LedOff(LED_RED);


                        //f_open(f, "ssk.bmp", FA_READ | FA_OPEN_ALWAYS);
                        //f_read(f, info, 70, &i); // read the 54-byte header
//                        f_read(f, data_from_file, size_in_file + 64, &i); // read the rest
                        //f_close(f);

                        //unsigned char pixels[240 * 320][3];
                                      
                        i = 0;
/*
                        for(int y ; y < 240 ; y++)
                        {
                            for(int x = 0 ; x < 320 ; x++)
                            {
                                temp0 = data_from_file[i * 2 + 0 + 64];
                                temp1 = data_from_file[i * 2 + 1 + 64];
                                pixel_data = temp0 << 8 | temp1;

                                TM_ILI9341_DrawPixel(x, y, pixel_data);
                                i += 2;
                            }
                        }
  */                                              
                    }
         
                }
                /* Unmount USB */
                f_mount(0, "1:", 1);
            }
        } 
        else 
        {
            /* Not inserted, turn on RED led */
            TM_DISCO_LedOn(LED_RED);
            TM_DISCO_LedOff(LED_GREEN);
            
            /* Ready to write next time */
            write = 1;
        }
    }
}