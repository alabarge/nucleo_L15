#pragma once

#include <stdint.h>

#define  CP_RX_QUE      8

// CP Service Provider Data Structure
typedef struct _cp_t {
   uint8_t           srvid;
   uint8_t           handle;
} cp_t, *pcp_t;

// Receive Queue
typedef struct _cp_rxq_t {
   uint32_t         *buf[CP_RX_QUE];
   uint8_t           head;
   uint8_t           tail;
   uint8_t           slots;
} cp_rxq_t, *pcp_rxq_t;

uint32_t cp_init(void);
uint32_t cp_msg(pcm_msg_t msg);
uint32_t cp_timer(pcm_msg_t msg);
uint32_t cp_tick(void);
uint32_t cp_qmsg(pcm_msg_t msg);
void     cp_thread(void);