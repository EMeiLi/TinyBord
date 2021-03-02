#ifndef __KFIFO_H__
#define __KFIFO_H__

#include <stdint.h>

#define SUPPORT_MULTITHREAD
#define KFIFO_TEST

#ifdef SUPPORT_MULTITHREAD
#include "cmsis_os2.h"
#endif

#ifdef KFIFO_TEST
#include <cstdio>
#endif

/*
 * kfifo struct
*/
typedef struct
{
    uint8_t     *buf;
    uint32_t    size;
    uint32_t    in;
    uint32_t    out;
    uint32_t    mask;
#ifdef SUPPORT_MULTITHREAD
    osMutexId_t mutex;
#endif

}kfifo_t;


/*
 * kfifo interface
*/
extern void kfifo_init(kfifo_t *fifo, uint8_t *buf, uint32_t size);

extern uint32_t kfifo_in(kfifo_t *fifo, const uint8_t *buf, uint32_t len);

extern uint32_t kfifo_out(kfifo_t *fifo, void *buf, uint32_t len);


#endif