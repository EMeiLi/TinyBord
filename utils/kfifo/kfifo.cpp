#include "kfifo.h"
#include <cstring>

#define IS_POWER_OF_2(n)    ((n) != 0 && (((n) & ((n) - 1)) == 0))
#define MIN(a, b)           ((a) < (b)? (a): (b))


void kfifo_init(kfifo_t *fifo, uint8_t *buf, uint32_t size)
{
    // 1> check size 2
    // static_assert(IS_POWER_OF_2(size), "size must be the power of 2");
    fifo->buf = buf;
    fifo->size = size;
    fifo->in = 0;
    fifo->out = 0;
    fifo->mask = size - 1;
#ifdef SUPPORT_MULTITHREAD
    fifo->mutex = osMutexNew(NULL);
#endif

}

uint32_t kfifo_in(kfifo_t *fifo, const uint8_t *buf, uint32_t len)
{
    uint32_t l;
    len = MIN(len, fifo->size - fifo->in + fifo->out);
#ifdef SUPPORT_MULTITHREAD
    osMutexAcquire(fifo->mutex, osWaitForever);
#endif
    l = MIN(len, fifo->size - (fifo->in & fifo->mask));
    memcpy(fifo->buf + (fifo->in & fifo->mask), buf, l);
    memcpy(fifo->buf, buf + l, len - l);
    fifo->in += len;

#ifdef SUPPORT_MULTITHREAD
    osMutexRelease(fifo->mutex);
#endif
    return len;
}

uint32_t kfifo_out(kfifo_t *fifo, void *buf, uint32_t len)
{
    uint32_t l;
    len = MIN(len, fifo->in - fifo->out);

    l = MIN(len, fifo->size - (fifo->out & fifo->mask));
    memcpy(buf, fifo->buf + (fifo->out & fifo->mask), l);
    memcpy(buf + l, fifo->buf, len -l);

    return len;
}

uint32_t kfifo_len(kfifo_t *fifo)
{
    return (fifo->in - fifo->out);
}

#ifdef KFIFO_TEST

void kfifo_print_info(kfifo_t *fifo)
{
    printf("kfifo> out=%d -->> in=%d, len=%d, size=%d\r\n", fifo->out, fifo->in, kfifo_len(fifo), fifo->size);
}
kfifo_t kfifo;
uint8_t kfifo_buf[16];
void kfifo_test()
{
    uint8_t buf[5];
    uint32_t len;
    kfifo_init(&kfifo, kfifo_buf, 16);
    kfifo_print_info(&kfifo);
    kfifo_in(&kfifo, "hello", 5);
    kfifo_print_info(&kfifo);
    len = kfifo_out(&kfifo, buf, 10);
    kfifo_print_info(&kfifo);
    printf("test> read from kfifo readlen=%d\r\n", len);

    kfifo_in(&kfifo, "hello", 5);
    kfifo_in(&kfifo, "hello", 5);
    kfifo_in(&kfifo, "hello", 5);
    kfifo_in(&kfifo, "hello", 5);

}
#endif