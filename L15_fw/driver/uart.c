/*-----------------------------------------------------------------------------

   1  ABSTRACT

   1.1 Module Type

      UART I/O Driver

   1.2 Functional Description

      The UART I/O Interface routines are contained in this module.

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
         7.1   uart_init()
         7.2   uart_isr()
         7.3   uart_tx()
         7.4   uart_cmio()
         7.5   uart_msgtx()
         7.6   uart_pipe()
         7.7   uart_report()

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

   static   uint8_t        cm_port = CM_PORT_NONE;

   static   UART_HandleTypeDef   *uart;

   static   uart_txq_t     txq;
   static   uart_rxq_t     rxq;

// 7 MODULE CODE

// ===========================================================================

// 7.1

uint32_t uart_init(UART_HandleTypeDef *huart, uint32_t ext_int, uint8_t port) {

/* 7.1.1   Functional Description

   The UART Interface is initialized in this routine.

   7.1.2   Parameters:

   module  Module Base Address
   ext_int External Interrupt Number
   port    COM Port

   7.1.3   Return Values:

   result   CFG_ERROR_OK

-----------------------------------------------------------------------------
*/

// 7.1.4   Data Structures

   uint32_t    result = CFG_ERROR_OK;
   uint32_t    j, status;

// 7.1.5   Code

   // Store the UART Module
   uart = huart;

   // Initialize the RX Queue
   memset(&rxq, 0, sizeof(uart_rxq_t));
   rxq.state  = UART_RX_IDLE;
   rxq.count  = 0;
   rxq.slotid = 0;
   rxq.slot   = NULL;
   rxq.msg    = NULL;

   // Initialize the TX Queue
   memset(&txq, 0, sizeof(uart_txq_t));
   for (j=0;j<UART_TX_QUE;j++) {
      txq.buf[j] = NULL;
   }
   txq.state  = UART_TX_IDLE;
   txq.head   = 0;
   txq.tail   = 0;
   txq.slots  = UART_TX_QUE;
   txq.count  = 0;

   // Register the I/O Interface callback for CM
   cm_ioreg(uart_cmio, port, CM_MEDIA_OPTO);

   // Update CM Port
   cm_port = port;
   
   // Flush DR
   __HAL_UART_FLUSH_DRREGISTER(huart);
   __HAL_UART_FLUSH_DRREGISTER(huart);

   // Enable Interrupts
   __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
   __HAL_UART_ENABLE_IT(huart, UART_IT_TC);

   // Report H/W Details
   if (gc.trace & CFG_TRACE_ID) {
      xlprint("%-13s base:irq %08X:%d\n", "/dev/uart", uart, ext_int);
      xlprint("%-13s rate:   %d bps\n", "/dev/uart", 115200);
      xlprint("%-13s port:   %d\n", "/dev/uart", cm_port);
   }

   return result;

}  // end uart_init()


// ===========================================================================

// 7.2

void uart_isr(void) {

/* 7.2.1   Functional Description

   This routine will service the UART Interrupt.

   7.2.2   Parameters:

   NONE

   7.2.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.2.4   Data Structures

   uint8_t       c;

// 7.2.5   Code

   // report interrupt request
   if (gc.trace & CFG_TRACE_IRQ) {
      xlprint("uart_isr() %08X\n", uart->Instance->SR);
   }

   //
   // RX INTERRUPT
   //
   if (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) == 1) {
      // always read the RX register
      c = uart->Instance->DR;
      // handle restart outside of state machine
      if (c == UART_START_FRAME) {
          rxq.state = UART_RX_IDLE;
          if (rxq.slot != NULL) {
              cm_free((pcm_msg_t)rxq.slot->buf);
              rxq.slot = NULL;
          }
      }
      switch (rxq.state) {
         case UART_RX_IDLE :
            // all messages begin with UART_START_FRAME
            // drop all characters until start-of-frame
            if (c == UART_START_FRAME) {
               // allocate slot from cmq
               rxq.slot = cm_alloc();
               if (rxq.slot != NULL) {
                  rxq.msg = (pcm_msg_t)rxq.slot->buf;
                  // preserve q slot id
                  rxq.slotid = rxq.msg->h.slot;
                  rxq.count  = 0;
                  rxq.state  = UART_RX_MSG;
               }
               else {
                  // no room at the inn, drop the entire message
                  rxq.state  = UART_RX_IDLE;
                  rxq.count  = 0;
               }
            }
            break;
         case UART_RX_MSG :
            // unstuff next octet
            if (c == UART_ESCAPE) {
               rxq.state = UART_RX_ESCAPE;
            }
            // end-of-frame
            else if (c == UART_END_FRAME) {
               // restore q slot id
               rxq.msg->h.slot = rxq.slotid;
               rxq.slot->msglen = rxq.msg->h.msglen;
               cm_qmsg(rxq.msg);
               rxq.state = UART_RX_IDLE;
               rxq.slot  = NULL;
            }
            else {
               // store message octet
               rxq.slot->buf[rxq.count++] = c;
            }
            break;
         case UART_RX_ESCAPE :
            // unstuff octet
            rxq.slot->buf[rxq.count++] = c ^ UART_STUFFED_BIT;
            rxq.state = UART_RX_MSG;
            break;
      }
   }
   //
   // TX INTERRUPT
   //
   else if (__HAL_UART_GET_FLAG(uart, UART_FLAG_TC) == 1) {
      __HAL_UART_CLEAR_FLAG(uart, UART_FLAG_TC);
      switch (txq.state) {
         case UART_TX_IDLE :
            // check for non-empty TX queue
            uart_msgtx();
            break;
         case UART_TX_MSG :
            // next character to send
            c = txq.uint8[txq.count];
            if (c == UART_START_FRAME ||
                c == UART_END_FRAME   ||
                c == UART_ESCAPE) {
               uart->Instance->DR = UART_ESCAPE;
               txq.state = UART_TX_ESCAPE;
            }
            else {
               uart->Instance->DR = c;
               txq.count++;
            }
            // end message?
            txq.state = (txq.count == txq.msglen) ? UART_TX_EOF : txq.state;
            break;
         case UART_TX_ESCAPE :
            // get character to escape
            c = txq.uint8[txq.count++];
            uart->Instance->DR = c ^ UART_STUFFED_BIT;
            txq.state = UART_TX_MSG;
            // end message?
            txq.state = (txq.count == txq.msglen) ? UART_TX_EOF : txq.state;
            break;
         case UART_TX_EOF :
            uart->Instance->DR = UART_END_FRAME;
            txq.state = UART_TX_LAST;
            // free control messages only, not pipe
            if (txq.msglen != sizeof(cm_pipe_t)) cm_free((pcm_msg_t)txq.uint8);
            txq.msglen = 0;
            txq.count  = 0;
            break;
         case UART_TX_LAST :
            txq.state = UART_TX_IDLE;
            // check for non-empty TX queue
            uart_msgtx();
            break;
      }
   }

} // end uart_isr()


// ===========================================================================

// 7.3

void uart_tx(pcm_msg_t msg, uint16_t msglen) {

/* 7.3.1   Functional Description

   This routine will transmit the message.

   7.3.2   Parameters:

   msg     Message to send.

   7.3.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.3.4   Data Structures

   uint8_t  u8 = UART_START_FRAME;

// 7.4.5   Code

   // Trace Entry
   if (gc.trace & CFG_TRACE_UART) {
      xlprint("uart_tx() msg:msglen = %08X:%d\n", (uint32_t)msg, msglen);
      dump((uint8_t *)msg, msglen, LIB_ASCII, 0);
   }

   uart->Instance->DR = UART_START_FRAME;

   txq.state  = UART_TX_MSG;
   txq.count  = 0;
   txq.msglen = msglen;
   txq.uint8  = (uint8_t *)msg;

} // end uart_tx()


// ===========================================================================

// 7.4

void uart_cmio(uint8_t op_code, pcm_msg_t msg) {

/* 7.4.1   Functional Description

   OPCODES

   CM_IO_TX : The transmit queue index will be incremented,
   this causes the top of the queue to be transmitted.

   7.4.2   Parameters:

   msg     Message Pointer
   opCode  CM_IO_TX

   7.4.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.4.4   Data Structures

// 7.4.5   Code

   if (gc.trace & CFG_TRACE_UART) {
      xlprint("uart_cmio() op_code:msg = %02X:%08X\n", op_code, (uint32_t)msg);
   }

   // disable UART interrupts
   HAL_NVIC_DisableIRQ(UART4_IRQn);

   // place in transmit queue
   txq.buf[txq.head] = (uint8_t *)msg;
   if (++txq.head == txq.slots) txq.head = 0;

   // try to transmit message
   uart_msgtx();

   // enable UART interrupts
   HAL_NVIC_EnableIRQ(UART4_IRQn);

} // end uart_cmio()


// ===========================================================================

// 7.5

void uart_msgtx(void) {

/* 7.5.1   Functional Description

   This routine will check for outgoing messages and route them to the
   transmitter uart_tx().

   7.5.2   Parameters:

   NONE

   7.5.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.5.4   Data Structures

   pcm_msg_t   msg;

// 7.5.5   Code

   // check for message in queue
   if (txq.head != txq.tail && txq.state == UART_TX_IDLE) {
      msg = (pcm_msg_t)txq.buf[txq.tail];
      // control or pipe message
      if (msg->h.dst_cmid == CM_ID_PIPE)
          uart_tx(msg, sizeof(cm_pipe_t));
      else
          uart_tx(msg, msg->h.msglen);
      // next slot in queue
      if (++txq.tail == txq.slots) txq.tail = 0;
   }

} // end uart_msgtx()


// ===========================================================================

// 7.6

void uart_pipe(pcm_pipe_t pipe, uint16_t msglen) {

/* 7.6.1   Functional Description

   This routine will place a pipe message on the transmit queue.

   7.6.2   Parameters:

   pipe       Pointer to pipe message
   msglen     Pipe message length

   7.6.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.6.4   Data Structures

// 7.6.5   Code

   if (gc.trace & CFG_TRACE_PIPE) {
      xlprint("uart_pipe() pipe:msglen = %08X:%d\n", pipe, msglen);
   }

   // disable UART interrupts
   HAL_NVIC_DisableIRQ(UART4_IRQn);

   // place in transmit queue
   txq.buf[txq.head] = (uint8_t *)pipe;
   if (++txq.head == txq.slots) txq.head = 0;

   // try transmitting
   uart_msgtx();

   // enable UART interrupts
   HAL_NVIC_EnableIRQ(UART4_IRQn);

} // end uart_pipe()


// ===========================================================================

// 7.7

void uart_report(void) {

/* 7.7.1   Functional Description

   This routine will report the txq and rxq state.

   7.7.2   Parameters:

   NONE

   7.7.3   Return Values:

   NONE

-----------------------------------------------------------------------------
*/

// 7.7.4   Data Structures

// 7.7.5   Code

   // txq
   xlprint("\n");
   xlprint("txq.state  : %d\n", txq.state);
   xlprint("txq.head   : %d\n", txq.head);
   xlprint("txq.tail   : %d\n", txq.tail);
   xlprint("txq.buf[]  : %08X\n", txq.buf[txq.head]);
   xlprint("txq.slots  : %d\n", txq.slots);
   xlprint("txq.msglen : %d\n", txq.msglen);
   xlprint("txq.count  : %d\n", txq.count);
   xlprint("txq.uint8  : %08X\n\n", txq.uint8);

   // rxq
   xlprint("\n");
   xlprint("rxq.state  : %d\n", rxq.state);
   xlprint("rxq.count  : %d\n", rxq.count);
   xlprint("rxq.slotid : %d\n", rxq.slotid);
   xlprint("rxq.slot   : %d\n", rxq.slot);
   xlprint("rxq.msg    : %08X\n\n", rxq.msg);

} // end uart_report()
