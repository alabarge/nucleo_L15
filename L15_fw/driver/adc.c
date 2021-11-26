/*-----------------------------------------------------------------------------

   1  ABSTRACT

   1.1 Module Type

      STM32 ADC Driver

   1.2 Functional Description

      The ADC Interface routines are contained in this module.

   1.3 Specification/Design Reference

      See fw_cfg.h under the BOOT/SHARE directory.

   1.4 Module Test Specification Reference

      None

   1.5 Compilation Information

      See fw_cfg.h under the BOOT/SHARE directory.

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
      7.1   adc_init()
      7.2   adc_isr()
      7.3   adc_intack()
      7.4   adc_run()
      7.5   adc_version()

-----------------------------------------------------------------------------*/

// 3 VOCABULARY

// 4 EXTERNAL RESOURCES

// 4.1  Include Files

#include "user_main.h"

// 4.2   External Data Structures

// 4.3 External Function Prototypes

// 5 LOCAL CONSTANTS AND MACROS

// 6 MODULE DATA STRUCTURES

// 6.1  Local Function Prototypes

// 6.2  Local Data Structures

// 7 MODULE CODE

// ===========================================================================

// 7.1

uint32_t adc_init(void) {

/* 7.1.1   Functional Description

   This routine is responsible for initializing the driver hardware.

   7.1.2   Parameters:

   NONE

   7.1.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.1.4   Data Structures

   uint32_t    result = CFG_STATUS_OK;

// 7.1.5   Code

   return result;

}  // end adc_init()


// ===========================================================================

// 7.2

void adc_isr(void *arg) {

/* 7.2.1   Functional Description

   This routine will service the hardware interrupt.

   7.2.2   Parameters:

   arg     IRQ arguments

   7.2.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.2.4   Data Structures

// 7.2.5   Code

} // end adc_isr()


// ===========================================================================

// 7.3

void adc_intack(uint8_t int_type) {

/* 7.3.1   Functional Description

   This routine will Acknowledge specific ADC Interrupts, or ALL.

   7.3.2   Parameters:

   intType  ADC_INT_RDY:   ADC Block Ready Interrupt
            ADC_INT_DONE:  ADC Done Interrupt
            ADC_INT_ALL:   All ADC Interrupts

   7.3.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.3.4   Data Structures

// 7.3.5   Code

} // end adc_intack()


// ===========================================================================

// 7.4

void adc_run(uint32_t flags, uint32_t packets) {

/* 7.4.1   Functional Description

   This routine will start or stop the ADC acquisition.

   7.4.2   Parameters:

   flags    Start/Stop flags
   blocks   Number of 1024-Byte packets to acquire

   7.4.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.4.4   Data Structures

// 7.4.5   Code

} // end adc_run()


// ===========================================================================

// 7.5

uint32_t adc_version(void) {

/* 7.5.1   Functional Description

   This routine will return the VERSION register value.

   7.5.2   Parameters:

   NONE

   7.5.3   Return Values:

   return   VERSION register

-----------------------------------------------------------------------------
*/

// 7.5.4   Data Structures

// 7.5.5   Code

   return 0;

} // end adc_version()
