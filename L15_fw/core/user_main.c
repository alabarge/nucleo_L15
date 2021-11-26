/*-----------------------------------------------------------------------------

   1  ABSTRACT

   1.1 Module Type

      L15-I APPLICATION

   1.2 Functional Description

      This module is responsible for implementing the main embedded
      test application for the NUCLEO-L152RE board.

   1.3 Specification/Design Reference

      See fw_cfg.h under the share directory.

   1.4 Module Test Specification Reference

      None

   1.5 Compilation Information

      See fw_cfg.h under the share directory.

   1.6 Notes

      NONE

   2  CONTENTS

      1 ABSTRACT
        1.1 Module Type
        1.2 Functional Description
        1.3 Specification/Design Reference
        1.4 Module Test Specification Reference
        1.5 Compilation Information
        1.6 Notes

      2 CONTENTS

      3 VOCABULARY

      4 EXTERNAL RESOURCES
        4.1  Include Files
        4.2  External Data Structures
        4.3  External Function Prototypes

      5 LOCAL CONSTANTS AND MACROS

      6 MODULE DATA STRUCTURES
        6.1  Local Function Prototypes
        6.2  Local Data Structures

      7 MODULE CODE
        7.1  main()
        7.2  timer()

-----------------------------------------------------------------------------*/

// 3 VOCABULARY

// 4 EXTERNAL RESOURCES

// 4.1  Include Files

#include "user_main.h"

// message string table
#include "msg_str.h"

// 4.2   External Data Structures

   // global control
   gc_t     gc;

   // month table for date-time strings
   char  *month_table[] = {
            "JAN", "FEB", "MAR", "APR",
            "MAY", "JUN", "JUL", "AUG",
            "SEP", "OCT", "NOV", "DEC"
          };

// 4.3   External Function Prototypes
          
   extern TIM_HandleTypeDef htim2;
   extern TIM_HandleTypeDef htim5;
   extern UART_HandleTypeDef huart4;
   extern UART_HandleTypeDef huart2;
          
// 5 LOCAL CONSTANTS AND MACROS

// 6 MODULE DATA STRUCTURES

// 6.1  Local Function Prototypes
          
// 6.2  Local Data Structures

   // Heart Beat
   static   uint8_t  hb_led[]    = {1, 1, 0, 0, 1, 1, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 0};
   static   uint8_t  hb_cnt      = 0;
   static   uint8_t  led_cnt     = 0;

   static   char     clr_scrn[]  = {0x1B, '[', '2', 'J', 0x00};
   static   char     cur_home[]  = {0x1B, '[', 'H', 0x00};
   
// 7 MODULE CODE

// ===========================================================================

// 7.1

int user_main(void) {

/* 7.1.1   Functional Description

   This is the main entry point for the embedded application, it is called
   by the alt_main() function from HAL.

   7.1.2   Parameters:

   NONE

   7.1.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.1.4   Data Structures

   uint32_t       i;

// 7.1.5   Code

   // Clear GC
   memset(&gc, 0, sizeof(gc_t));

   // Initialize GC
   gc.feature   = CFG_FEATURE_DEFAULT;
   gc.trace     = CFG_TRACE_DEFAULT;
   gc.debug     = CFG_DEBUG_DEFAULT;
   gc.status    = CFG_STATUS_INIT;
   gc.error     = CFG_ERROR_CLEAR;
   gc.devid     = CM_DEV_L15;
   gc.winid     = CM_DEV_WIN;
   gc.com_port  = CM_PORT_COM0;
   gc.int_flag  = FALSE;
   gc.sw_reset  = FALSE;
   gc.sys_time  = 0;
   gc.ping_time = TIM5->CNT;
   gc.ping_cnt  = 0;
   gc.led_cycle = CFG_LED_CYCLE;
   gc.month     = month_table;
   gc.msg_table = msg_table;
   gc.msg_table_len = DIM(msg_table);

   //
   // INIT THE HARDWARE
   //

   // Start STM32 Timers
   HAL_TIM_Base_Start_IT(&htim2);
   HAL_TIM_Base_Start_IT(&htim5);

   // Open Debug Port
   xlprint_open(&huart2);
   
   // Clear the Terminal Screen and Home the Cursor
   xlprint(clr_scrn);
   xlprint(cur_home);

   // Display the Startup Banner
   xlprint("\nL15-I ARM, %s\n\n", BUILD_HI);
   xlprints(gc.dev_str, "L15-I ARM, %s", BUILD_STR);

   // CM Init
   gc.error |= cm_init();

   // COM Port
   gc.error |= uart_init(&huart4, UART4_IRQn, gc.com_port);

   // Report Versions
   version();

   //
   // START THE SERVICES
   //

   // Control Panel (CP)
   gc.error |= cp_hal_init();
   gc.error |= cp_init();

   // DAQ Controller (DAQ)
   gc.error |= daq_hal_init();
   gc.error |= daq_init();

   // Initialization Finished so
   // start Running
   gc.status &= ~CFG_STATUS_INIT;
   gc.status |=  CFG_STATUS_RUN;

   // Report Status and Error Results
   if (gc.trace & CFG_TRACE_POST) {
      xlprint("\n");
      xlprint("trace   :  %08X\n", gc.trace);
      xlprint("feature :  %08X\n", gc.feature);
      xlprint("status  :  %08X\n", gc.status);
      xlprint("error   :  %08X\n", gc.error);
      xlprint("\n");
   }

   // Init the Command Line Interpreter
   cli_init();
   
   //
   // BACKGROUND PROCESSING
   //
   // NOTE: All Background thread operations begin
   //       from this for-loop! Further, all foreground
   //       processing not done in the interrupt must
   //       start through this for-loop!
   //
   for (;;) {
      //
      // CM THREAD
      //
      cm_thread();
      //
      // CP THREAD
      //
      cp_thread();
      //
      // DAQ THREAD
      //
      daq_thread();
      //
      // CLI THREAD
      //
      cli_process(&gc.cli);
   }

   // Unreachable code
   return 0;

} // end main()


// ===========================================================================

// 7.2

void timer(void) {

/* 7.2.1   Functional Description

   This is the main system timer callback function for handling background
   periodic events.

   7.2.2   Parameters:

   arg   Callback parameter, unused

   7.2.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.2.4   Data Structures

// 7.2.5   Code

   // System Time Tick
   gc.sys_time++;

   // Period Service Ticks
   cp_tick();
   daq_tick();
   cm_tick();

   // Activity Indicator
   if (++led_cnt >= gc.led_cycle) {
      led_cnt = 0;
      // Heart Beat
      if (hb_led[(hb_cnt++ & 0xF)])
         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
      else
         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
   }

} // end HAL_TIM_PeriodElapsedCallback()


// ===========================================================================

// 7.3

void version(void) {

/* 7.2.1   Functional Description

   Report firmware and hardware version detail to STDOUT.

   7.2.2   Parameters:

   arg      Unused

   7.2.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.2.4   Data Structures

// 7.2.5   Code

   xlprint("\n");
   xlprint("\nL15-I ARM, %s\n\n", BUILD_HI);
   xlprint("\n");

} // end version()
