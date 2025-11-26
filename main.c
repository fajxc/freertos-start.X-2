/*
 * File:   main.c
 * Author: ENCM 511
 *
 * Countdown Timer Application with FreeRTOS
 * 
 * Description:
 *   This application implements a countdown timer with three main operational
 *   states: Waiting, Time Input, and Countdown. The user interacts via UART
 *   terminal and physical buttons.
 * 
 * Features:
 *   - Smooth LED pulsing in waiting state (software PWM)
 *   - UART-based time input in MM:SS format
 *   - Accurate countdown with LED blinking
 *   - Variable brightness LED controlled by potentiometer
 *   - Pause/Resume/Reset functionality
 *   - Extended display mode ('i' key toggle)
 *   - Blink/Solid mode toggle ('b' key)
 * 
 * Hardware:
 *   - PB1: Initiate time entry (from waiting state)
 *   - PB2+PB3: Start countdown / Reset time entry
 *   - PB3: Pause/Resume/Abort (long press)
 *   - LED0: Completion indication
 *   - LED1: Countdown blink indicator
 *   - LED2: PWM brightness controlled by potentiometer
 *   - Potentiometer: ADC input for brightness control
 *   - UART2: Terminal communication
 *
 * Created on Oct 22, 2025
 * Updated on Nov 2025
 */

/*============================================================================
 * CONFIGURATION BITS
 *============================================================================*/

// FSEC
#pragma config BWRP = OFF    //Boot Segment Write-Protect bit->Boot Segment may be written
#pragma config BSS = DISABLED    //Boot Segment Code-Protect Level bits->No Protection (other than BWRP)
#pragma config BSEN = OFF    //Boot Segment Control bit->No Boot Segment
#pragma config GWRP = OFF    //General Segment Write-Protect bit->General Segment may be written
#pragma config GSS = DISABLED    //General Segment Code-Protect Level bits->No Protection (other than GWRP)
#pragma config CWRP = OFF    //Configuration Segment Write-Protect bit->Configuration Segment may be written
#pragma config CSS = DISABLED    //Configuration Segment Code-Protect Level bits->No Protection (other than CWRP)
#pragma config AIVTDIS = OFF    //Alternate Interrupt Vector Table bit->Disabled AIVT

// FBSLIM
#pragma config BSLIM = 8191    //Boot Segment Flash Page Address Limit bits->8191

// FOSCSEL
#pragma config FNOSC = FRC    //Oscillator Source Selection->Internal Fast RC (FRC)
#pragma config PLLMODE = PLL96DIV2    //PLL Mode Selection->96 MHz PLL. Oscillator input is divided by 2 (8 MHz input)
#pragma config IESO = OFF    //Two-speed Oscillator Start-up Enable bit->Start up with user-selected oscillator source

// FOSC
#pragma config POSCMD = NONE    //Primary Oscillator Mode Select bits->Primary Oscillator disabled
#pragma config OSCIOFCN = ON    //OSC2 Pin Function bit->OSC2 is general purpose digital I/O pin
#pragma config SOSCSEL = OFF    //SOSC Power Selection Configuration bits->Digital (SCLKI) mode
#pragma config PLLSS = PLL_FRC    //PLL Secondary Selection Configuration bit->PLL is fed by the on-chip Fast RC (FRC) oscillator
#pragma config IOL1WAY = ON    //Peripheral pin select configuration bit->Allow only one reconfiguration
#pragma config FCKSM = CSECMD    //Clock Switching Mode bits->Clock switching is enabled,Fail-safe Clock Monitor is disabled

// FWDT
#pragma config WDTPS = PS32768    //Watchdog Timer Postscaler bits->1:32768
#pragma config FWPSA = PR128    //Watchdog Timer Prescaler bit->1:128
#pragma config FWDTEN = OFF    //Watchdog Timer Enable bits->WDT DISABLED to prevent random resets
#pragma config WINDIS = OFF    //Watchdog Timer Window Enable bit->Watchdog Timer in Non-Window mode
#pragma config WDTWIN = WIN25    //Watchdog Timer Window Select bits->WDT Window is 25% of WDT period
#pragma config WDTCMX = WDTCLK    //WDT MUX Source Select bits->WDT clock source is determined by the WDTCLK Configuration bits
#pragma config WDTCLK = LPRC    //WDT Clock Source Select bits->WDT uses LPRC

// FPOR
#pragma config BOREN = ON    //Brown Out Enable bit->Brown Out Enable Bit
#pragma config LPCFG = OFF    //Low power regulator control->No Retention Sleep
#pragma config DNVPEN = ENABLE    //Downside Voltage Protection Enable bit->Downside protection enabled using ZPBOR when BOR is inactive

// FICD
#pragma config ICS = PGD1    //ICD Communication Channel Select bits->Communicate on PGEC1 and PGED1
#pragma config JTAGEN = OFF    //JTAG Enable bit->JTAG is disabled

// FDEVOPT1
#pragma config ALTCMPI = DISABLE    //Alternate Comparator Input Enable bit->C1INC, C2INC, and C3INC are on their standard pin locations
#pragma config TMPRPIN = OFF    //Tamper Pin Enable bit->TMPRN pin function is disabled
#pragma config SOSCHP = ON    //SOSC High Power Enable bit (valid only when SOSCSEL = 1->Enable SOSC high power mode (default)
#pragma config ALTI2C1 = ALTI2CEN    //Alternate I2C pin Location->SDA1 and SCL1 on RB9 and RB8

/*============================================================================
 * INCLUDES
 *============================================================================*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "app.h"
#include "hw_config.h"
#include "uart.h"
#include "buttons.h"
#include "pwm.h"

/*============================================================================
 * FREERTOS OBJECT DEFINITIONS
 * 
 * These are declared extern in app.h and defined here.
 *============================================================================*/

/* Queues */
QueueHandle_t xButtonQueue = NULL;
QueueHandle_t xUartRxQueue = NULL;

/* Semaphores for synchronization */
SemaphoreHandle_t xStartInputSem = NULL;
SemaphoreHandle_t xStartCountdownSem = NULL;

/* Mutexes for shared resource protection */
SemaphoreHandle_t xUartMutex = NULL;
SemaphoreHandle_t xStateMutex = NULL;
SemaphoreHandle_t xCountdownMutex = NULL;

/* Global shared state */
volatile SystemState_t g_SystemState = STATE_WAITING;

/* Countdown data */
volatile uint16_t g_CountdownSeconds = 0;

/*============================================================================
 * FREERTOS HOOKS
 *============================================================================*/

void vApplicationIdleHook(void)
{
    /* Clear watchdog timer to prevent system reset */
    ClrWdt();
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;
    
    /* Stack overflow detected - halt system */
    taskDISABLE_INTERRUPTS();
    for(;;);
}

/*============================================================================
 * MODIFIED UART RX ISR
 * 
 * Sends received characters to the UART queue for processing by tasks.
 *============================================================================*/

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    UartCmd_t cmd;
    char received;
    
    /* Clear interrupt flag */
    IFS1bits.U2RXIF = 0;
    
    /* Read received character */
    received = U2RXREG;
    
    /* Clear overflow error if set */
    if (U2STAbits.OERR) {
        U2STAbits.OERR = 0;
    }
    
    /* Categorize the received character */
    if (received == '\r' || received == '\n') {
        cmd.type = UART_CMD_ENTER;
        cmd.character = received;
    } else if (received == 0x08 || received == 0x7F) {  /* Backspace or DEL */
        cmd.type = UART_CMD_BACKSPACE;
        cmd.character = received;
    } else {
        cmd.type = UART_CMD_CHAR;
        cmd.character = received;
    }
    
    /* Send to queue (non-blocking from ISR) */
    if (xUartRxQueue != NULL) {
        xQueueSendFromISR(xUartRxQueue, &cmd, &xHigherPriorityTaskWoken);
    }
    
    /* Yield if a higher priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* TX ISR - unchanged */
void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void)
{
    IFS1bits.U2TXIF = 0;
}

/*============================================================================
 * HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Thread-safe UART string transmission
 */
static void SafeDisp2String(const char *str)
{
    if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        Disp2String((char*)str);
        xSemaphoreGive(xUartMutex);
    }
}


/*============================================================================
 * WAITING STATE TASK
 * 
 * Initial state: LED pulsing, waiting for PB1 click.
 * Upon PB1 click, transitions to TIME_INPUT state.
 *============================================================================*/

void vWaitingTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime;
    ButtonEvent_t buttonEvent;
    
    for(;;) {
        /* Wait until we're in WAITING state */
        while (g_SystemState != STATE_WAITING) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        /* Clear button queue */
        while (xQueueReceive(xButtonQueue, NULL, 0) == pdTRUE) { }
        vTaskDelay(pdMS_TO_TICKS(100));
        
        /* Display welcome message */
        SafeDisp2String("\r\n\n========================================\r\n");
        SafeDisp2String("      COUNTDOWN TIMER APPLICATION\r\n");
        SafeDisp2String("========================================\r\n");
        SafeDisp2String("Press PB1 to enter time...\r\n\n");
        
        /* Start LED pulsing */
        PWM_ResetPulse();
        PWM_Start();
        PWM_SetOutputEnabled(true);
        
        xLastWakeTime = xTaskGetTickCount();
        
        /* Wait for PB1 click to enter time input */
        while (g_SystemState == STATE_WAITING) {
            PWM_UpdatePulse(20, PULSE_PERIOD_MS);
            
            /* Check for PB1 click */
            if (xQueueReceive(xButtonQueue, &buttonEvent, pdMS_TO_TICKS(20)) == pdTRUE) {
                if (buttonEvent.button == BUTTON_PB1 && buttonEvent.event == EVENT_CLICK) {
                    PWM_Stop();
                    /* Change state to ENTER_TIME */
                    if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        g_SystemState = STATE_TIME_INPUT;
                        xSemaphoreGive(xStateMutex);
                    }
                    /* Signal time input task to start */
                    xSemaphoreGive(xStartInputSem);
                    break;  /* Exit inner loop, will restart outer loop */
                }
            }
            
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));
        }
    }
}

/*============================================================================
 * TIME INPUT TASK
 * 
 * Handles UART input for countdown time entry (MM:SS format).
 *============================================================================*/

void vTimeInputTask(void *pvParameters)
{
    (void)pvParameters;
    char input_buffer[16];
    uint8_t input_index;
    UartCmd_t uartCmd;
    uint16_t minutes, seconds;
    char *colon_pos;
    bool got_valid_time;
    
    for(;;) {
        /* Wait for signal to start time input */
        xSemaphoreTake(xStartInputSem, portMAX_DELAY);
        
        bool repeat_input = true;
        
        while (repeat_input) {
            repeat_input = false;
            
            /* Clear stale events */
            while (xQueueReceive(xUartRxQueue, &uartCmd, 0) == pdTRUE) { }
            vTaskDelay(pdMS_TO_TICKS(100));
            
            /* Display prompt */
            SafeDisp2String("\r\nEnter countdown time (MM:SS): ");
            
            /* Reset input buffer */
            memset(input_buffer, 0, sizeof(input_buffer));
            input_index = 0;
            got_valid_time = false;
            
            /* Get input until Enter pressed */
            while (!got_valid_time) {
            if (xQueueReceive(xUartRxQueue, &uartCmd, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (uartCmd.type == UART_CMD_CHAR) {
                    if ((uartCmd.character >= '0' && uartCmd.character <= '9') || uartCmd.character == ':') {
                        if (input_index < sizeof(input_buffer) - 1) {
                            input_buffer[input_index++] = uartCmd.character;
                            input_buffer[input_index] = '\0';
                            XmitUART2(uartCmd.character, 1);
                        }
                    }
                } else if (uartCmd.type == UART_CMD_BACKSPACE) {
                    if (input_index > 0) {
                        input_index--;
                        input_buffer[input_index] = '\0';
                        SafeDisp2String("\b \b");
                    }
                } else if (uartCmd.type == UART_CMD_ENTER) {
                    if (input_index > 0) {
                        colon_pos = strchr(input_buffer, ':');
                        if (colon_pos != NULL) {
                            *colon_pos = '\0';
                            minutes = atoi(input_buffer);
                            seconds = atoi(colon_pos + 1);
                            if (seconds < 60 && (minutes > 0 || seconds > 0)) {
                                got_valid_time = true;
                                
                                /* Store countdown time */
                                uint16_t total_seconds = (minutes * 60) + seconds;
                                if (xSemaphoreTake(xCountdownMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                                    g_CountdownSeconds = total_seconds;
                                    xSemaphoreGive(xCountdownMutex);
                                }
                                
                                /* Change state to READY */
                                if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                                    g_SystemState = STATE_READY;
                                    xSemaphoreGive(xStateMutex);
                                }
                                
                                SafeDisp2String("\r\nTime set! Press PB2+PB3 to start (long press to clear).\r\n");
                            } else {
                                SafeDisp2String("\r\nInvalid time.\r\n");
                            }
                        } else {
                            SafeDisp2String("\r\nInvalid format. Use MM:SS\r\n");
                        }
                    }
                }
            }
        }
        
            /* Now in READY state - wait for PB3+PB2 click to start countdown */
            ButtonEvent_t buttonEvent;
            bool start_triggered = false;
            
            while (g_SystemState == STATE_READY && !start_triggered && !repeat_input) {
                /* Check for PB3+PB2 combo */
                if (Buttons_IsPB2Pressed() && Buttons_IsPB3Pressed()) {
                    /* Both pressed - wait to see if it's click or long press */
                    TickType_t holdStart = xTaskGetTickCount();
                    while (Buttons_IsPB2Pressed() && Buttons_IsPB3Pressed() && 
                           g_SystemState == STATE_READY) {
                        vTaskDelay(pdMS_TO_TICKS(50));
                        if ((xTaskGetTickCount() - holdStart) >= pdMS_TO_TICKS(1000)) {
                            /* Long press - clear time, re-enter */
                            SafeDisp2String("\r\nTime cleared. Re-enter value.\r\n");
                            if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                                g_SystemState = STATE_TIME_INPUT;
                                xSemaphoreGive(xStateMutex);
                            }
                            repeat_input = true;
                            break;
                        }
                    }
                    
                    if (!repeat_input && g_SystemState == STATE_READY) {
                        /* Short press - start countdown */
                        start_triggered = true;
                    }
                }
                
                /* Also check button queue for combo events */
                while (xQueueReceive(xButtonQueue, &buttonEvent, 0) == pdTRUE) {
                    if (buttonEvent.button == BUTTON_PB2_AND_PB3) {
                        if (buttonEvent.event == EVENT_CLICK) {
                            start_triggered = true;
                            break;
                        } else if (buttonEvent.event == EVENT_LONG_PRESS) {
                            SafeDisp2String("\r\nTime cleared. Re-enter value.\r\n");
                            if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                                g_SystemState = STATE_TIME_INPUT;
                                xSemaphoreGive(xStateMutex);
                            }
                            repeat_input = true;
                            break;
                        }
                    }
                }
                
                if (!start_triggered && !repeat_input) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
            
            if (start_triggered) {
                /* Transition to COUNTDOWN */
                if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_SystemState = STATE_COUNTDOWN;
                    xSemaphoreGive(xStateMutex);
                }
                /* Signal countdown task to start */
                xSemaphoreGive(xStartCountdownSem);
                break;  /* Exit repeat_input loop */
            }
        }
    }
}

/*============================================================================
 * COUNTDOWN TASK
 * 
 * Main countdown logic - displays time, blinks LED1, controls LED2 PWM.
 *============================================================================*/

static void FormatTime(uint16_t seconds, char *buffer)
{
    uint16_t mins = seconds / 60;
    uint16_t secs = seconds % 60;
    
    /* Manual formatting to avoid sprintf issues */
    buffer[0] = '0' + (mins / 10);
    buffer[1] = '0' + (mins % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (secs / 10);
    buffer[4] = '0' + (secs % 10);
    buffer[5] = '\0';
}

void vCountdownTask(void *pvParameters)
{
    (void)pvParameters;
    uint16_t remaining;
    char time_str[8];
    char display_buffer[32];
    bool led1_on = false;
    
    for(;;) {
        /* Wait for signal to start countdown */
        xSemaphoreTake(xStartCountdownSem, portMAX_DELAY);
        
        /* Get the countdown time */
        if (xSemaphoreTake(xCountdownMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            remaining = g_CountdownSeconds;
            xSemaphoreGive(xCountdownMutex);
        } else {
            remaining = 0;
        }
        
        if (remaining == 0) {
            SafeDisp2String("[ERROR: No time set]\r\n");
            if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_SystemState = STATE_WAITING;
                xSemaphoreGive(xStateMutex);
            }
            continue;
        }
        
        SafeDisp2String("\r\n[COUNTDOWN STARTED]\r\n");
        SafeDisp2String("TEST1: Simple string works\r\n");
        
        /* Test: Can we print remaining as simple number? */
        if (remaining == 0) {
            SafeDisp2String("ERROR: remaining is ZERO!\r\n");
        } else {
            SafeDisp2String("TEST2: remaining is NOT zero\r\n");
        }
        
        /* Initialize LEDs */
        LED1_Off();
        PWM_Start();
        PWM_SetOutputEnabled(true);
        PWM_SetDutyCycle(50);  /* 50% brightness for now */
        SafeDisp2String("TEST3: After LED init\r\n");
        
        /* Format and display time */
        FormatTime(remaining, time_str);
        SafeDisp2String("\r\nTime: ");
        SafeDisp2String(time_str);
        SafeDisp2String("\r\n");
        
        /* Countdown loop */
        SafeDisp2String("TEST8: Before loop, remaining=");
        /* Try to print remaining value simply */
        if (remaining > 0) {
            SafeDisp2String(">0, entering loop\r\n");
        } else {
            SafeDisp2String("=0, NOT entering loop\r\n");
        }
        
        while (remaining > 0) {
            SafeDisp2String("LOOP: Inside countdown\r\n");
            /* Wait 1 second */
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            /* Decrement */
            remaining--;
            
            /* Display updated time */
            FormatTime(remaining, time_str);
            SafeDisp2String("Time: ");
            SafeDisp2String(time_str);
            SafeDisp2String("\r\n");
            
            /* Toggle LED1 every second */
            led1_on = !led1_on;
            if (led1_on) {
                LED1_On();
            } else {
                LED1_Off();
            }
            
            /* Toggle LED2 PWM (sync with LED1) */
            PWM_SetOutputEnabled(led1_on);
        }
        
        /* Countdown complete */
        LED1_Off();
        PWM_Stop();
        SafeDisp2String("\r\n\nCOUNTDOWN COMPLETE!\r\n\n");
        
        /* Return to WAITING */
        if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_SystemState = STATE_WAITING;
            xSemaphoreGive(xStateMutex);
        }
    }
}

/*============================================================================
 * BUTTON POLLING TASK
 * 
 * Periodically reads button states and sends events to queue.
 *============================================================================*/

void vButtonTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime;
    const TickType_t xPollPeriod = pdMS_TO_TICKS(10);  /* 100Hz polling rate */
    
    /* Initialize buttons */
    Buttons_Init();
    
    xLastWakeTime = xTaskGetTickCount();
    
    for(;;) {
        /* Update button states */
        Buttons_Update(10);  /* 10ms elapsed */
        
        /* Send any pending button events to queue */
        Buttons_SendPendingEvents();
        
        /* Wait for next poll cycle */
        vTaskDelayUntil(&xLastWakeTime, xPollPeriod);
    }
}


/*============================================================================
 * HARDWARE INITIALIZATION
 *============================================================================*/

void App_InitHardware(void)
{
    /* Initialize all GPIO pins */
    HW_InitAllPins();
    
    /* Initialize UART */
    InitUART2();
    
    /* Initialize PWM (but don't start yet) */
    PWM_Init();
}

/*============================================================================
 * RTOS OBJECTS INITIALIZATION
 *============================================================================*/

void App_InitRTOSObjects(void)
{
    /* Create queues */
    xButtonQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(ButtonEvent_t));
    xUartRxQueue = xQueueCreate(UART_RX_QUEUE_SIZE, sizeof(UartCmd_t));
    
    /* Create binary semaphores for synchronization */
    xStartInputSem = xSemaphoreCreateBinary();
    xStartCountdownSem = xSemaphoreCreateBinary();
    
    /* Create mutexes for shared resource protection */
    xUartMutex = xSemaphoreCreateMutex();
    xStateMutex = xSemaphoreCreateMutex();
    xCountdownMutex = xSemaphoreCreateMutex();
}

/*============================================================================
 * MAIN FUNCTION
 *============================================================================*/

int main(void)
{
    /*------------------------------------------------------------------------
     * Initialize hardware BEFORE creating FreeRTOS objects
     * This ensures peripherals are ready before tasks start.
     *------------------------------------------------------------------------*/
    App_InitHardware();
    
    /*------------------------------------------------------------------------
     * Initialize FreeRTOS objects (queues, semaphores, mutexes)
     * MUST be done BEFORE creating tasks that use them!
     *------------------------------------------------------------------------*/
    App_InitRTOSObjects();
    
    /*------------------------------------------------------------------------
     * Create application tasks
     *------------------------------------------------------------------------*/
    
    /* Button polling task */
    xTaskCreate(vButtonTask, "BTN", STACK_SIZE_BUTTON, 
                NULL, PRIORITY_BUTTON_HANDLER, NULL);
    
    /* Waiting task */
    xTaskCreate(vWaitingTask, "WAIT", STACK_SIZE_WAITING, 
                NULL, PRIORITY_WAITING, NULL);
    
    /* Time input task */
    xTaskCreate(vTimeInputTask, "INPUT", STACK_SIZE_TIME_INPUT, 
                NULL, PRIORITY_TIME_INPUT, NULL);
    
    /* Countdown task */
    xTaskCreate(vCountdownTask, "COUNT", STACK_SIZE_COUNTDOWN, 
                NULL, PRIORITY_COUNTDOWN, NULL);
    
    /*------------------------------------------------------------------------
     * Start the FreeRTOS scheduler
     * This function should never return.
     *------------------------------------------------------------------------*/
    vTaskStartScheduler();
    
    /* Should never reach here */
    for(;;);
    
    return 0;
}
