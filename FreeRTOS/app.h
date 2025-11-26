/*
 * File:   app.h
 * Author: ENCM 511
 * 
 * Application Header
 * 
 * Description: Defines system states, shared data structures, and declares
 *              all FreeRTOS objects used for inter-task communication.
 * 
 * Created on Nov 2025
 */

#ifndef APP_H
#define APP_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * SYSTEM STATE DEFINITIONS
 * 
 * The application operates as a state machine with the following states:
 * 
 * WAITING    -> Initial state, LED pulsing, waiting for PB1 click
 * TIME_INPUT -> User enters countdown time via UART
 * READY      -> Time captured, waiting for PB2+PB3 start combo
 * COUNTDOWN  -> Timer counting down with LED blinking
 * PAUSED     -> Countdown paused (PB3 click)
 * COMPLETED  -> Countdown finished, completion indication
 *============================================================================*/

typedef enum {
    STATE_WAITING,      /* Initial waiting state - LED pulsing */
    STATE_TIME_INPUT,   /* Waiting for user to input time via UART */
    STATE_READY,        /* Time set, waiting for PB2+PB3 combo */
    STATE_COUNTDOWN,    /* Countdown active */
    STATE_PAUSED,       /* Countdown paused */
    STATE_COMPLETED     /* Timer completed - showing completion indication */
} SystemState_t;

/*============================================================================
 * BUTTON EVENT DEFINITIONS
 * 
 * Button events are sent via queue from ISR/polling to tasks.
 * Each event indicates which button and what type of action occurred.
 *============================================================================*/

typedef enum {
    BUTTON_NONE = 0,
    BUTTON_PB1,
    BUTTON_PB2,
    BUTTON_PB3,
    BUTTON_PB2_AND_PB3  /* Both PB2 and PB3 pressed together */
} ButtonId_t;

typedef enum {
    EVENT_NONE = 0,
    EVENT_CLICK,        /* Short press and release */
    EVENT_LONG_PRESS,   /* Held for >1 second */
    EVENT_PRESSED,      /* Button just pressed down */
    EVENT_RELEASED      /* Button just released */
} ButtonEventType_t;

typedef struct {
    ButtonId_t button;
    ButtonEventType_t event;
} ButtonEvent_t;

/*============================================================================
 * UART COMMAND DEFINITIONS
 * 
 * Commands received via UART during countdown operation.
 *============================================================================*/

typedef enum {
    UART_CMD_NONE = 0,
    UART_CMD_CHAR,          /* Regular character for time input */
    UART_CMD_ENTER,         /* Enter key pressed */
    UART_CMD_BACKSPACE,     /* Backspace key pressed */
    UART_CMD_TOGGLE_INFO,   /* 'i' key - toggle info display */
    UART_CMD_TOGGLE_BLINK   /* 'b' key - toggle LED2 blink mode */
} UartCmdType_t;

typedef struct {
    UartCmdType_t type;
    char character;         /* The actual character if CMD_CHAR */
} UartCmd_t;

/*============================================================================
 * SHARED DATA STRUCTURES
 * 
 * These structures hold data shared between tasks.
 * Protected by mutexes where necessary.
 *============================================================================*/

/* Countdown timer data - protected by xCountdownMutex */
typedef struct {
    uint16_t total_seconds;     /* Total countdown time in seconds */
    uint16_t remaining_seconds; /* Remaining time in seconds */
    bool is_running;            /* True if countdown is active */
    bool is_paused;             /* True if countdown is paused */
} CountdownData_t;

/* Display mode settings */
typedef struct {
    bool show_extended_info;    /* 'i' toggle: show ADC/intensity info */
    bool led2_solid_mode;       /* 'b' toggle: LED2 solid vs blinking */
} DisplaySettings_t;

/* PWM/Brightness data - protected by xBrightnessMutex */
typedef struct {
    uint16_t adc_value;         /* Raw ADC reading (0-1023) */
    uint8_t duty_cycle;         /* Calculated duty cycle (0-100%) */
    bool led2_state;            /* Current LED2 on/off state for blinking */
} BrightnessData_t;

/*============================================================================
 * FREERTOS OBJECT DECLARATIONS
 * 
 * All FreeRTOS objects are declared extern here and defined in main.c
 *============================================================================*/

/* Queues for event communication */
extern QueueHandle_t xButtonQueue;      /* Button events: ISR/Task -> Tasks */
extern QueueHandle_t xUartRxQueue;      /* UART chars: ISR -> Tasks */

/* Semaphores for state synchronization */
extern SemaphoreHandle_t xStartInputSem;    /* Signal to start time input */
extern SemaphoreHandle_t xStartCountdownSem;/* Signal to start countdown */
extern SemaphoreHandle_t xCompletionSem;    /* Signal countdown complete */

/* Mutexes for shared resource protection */
extern SemaphoreHandle_t xUartMutex;        /* Protect UART transmissions */
extern SemaphoreHandle_t xCountdownMutex;   /* Protect countdown data */
extern SemaphoreHandle_t xBrightnessMutex;  /* Protect brightness data */
extern SemaphoreHandle_t xStateMutex;       /* Protect system state */

/* Global shared data */
extern volatile SystemState_t g_SystemState;
extern volatile CountdownData_t g_CountdownData;
extern volatile DisplaySettings_t g_DisplaySettings;
extern volatile BrightnessData_t g_BrightnessData;

/*============================================================================
 * QUEUE SIZES
 *============================================================================*/

#define BUTTON_QUEUE_SIZE       10
#define UART_RX_QUEUE_SIZE      32

/*============================================================================
 * TASK PRIORITIES
 * 
 * Priority scheme (higher number = higher priority):
 * Note: configMAX_PRIORITIES is 4, so valid priorities are 0-3
 *============================================================================*/

#define PRIORITY_PWM            3   /* Highest - timing critical */
#define PRIORITY_COUNTDOWN      2   /* High - accuracy important */
#define PRIORITY_BUTTON_HANDLER 2   /* High - responsiveness */
#define PRIORITY_TIME_INPUT     1   /* Medium */
#define PRIORITY_WAITING        1   /* Medium */
#define PRIORITY_ADC            0   /* Low - not time critical */
#define PRIORITY_IDLE           0   /* Lowest */

/*============================================================================
 * TASK STACK SIZES
 *============================================================================*/

#define STACK_SIZE_WAITING      configMINIMAL_STACK_SIZE
#define STACK_SIZE_TIME_INPUT   (configMINIMAL_STACK_SIZE + 50)
#define STACK_SIZE_COUNTDOWN    (configMINIMAL_STACK_SIZE + 50)
#define STACK_SIZE_PWM          configMINIMAL_STACK_SIZE
#define STACK_SIZE_BUTTON       configMINIMAL_STACK_SIZE
#define STACK_SIZE_ADC          configMINIMAL_STACK_SIZE

/*============================================================================
 * FUNCTION PROTOTYPES - Task functions
 *============================================================================*/

/* Main application tasks */
void vWaitingTask(void *pvParameters);
void vTimeInputTask(void *pvParameters);
void vCountdownTask(void *pvParameters);

/* Supporting tasks */
void vButtonTask(void *pvParameters);
void vPwmTask(void *pvParameters);
void vAdcTask(void *pvParameters);

/*============================================================================
 * FUNCTION PROTOTYPES - Initialization
 *============================================================================*/

/* Initialize all application FreeRTOS objects */
void App_InitRTOSObjects(void);

/* Initialize hardware peripherals */
void App_InitHardware(void);

/*============================================================================
 * HELPER MACROS
 *============================================================================*/

/* Safe state access with mutex */
#define GET_SYSTEM_STATE() (g_SystemState)

/* Convert milliseconds to ticks */
#define MS_TO_TICKS(ms) pdMS_TO_TICKS(ms)

#endif /* APP_H */

