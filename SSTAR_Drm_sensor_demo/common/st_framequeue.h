#ifndef _ST_FRAMEQUEUE_H_
#define _ST_FRAMEQUEUE_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#if defined (__cplusplus)
extern "C" {
#endif


#define FRAME_QUEUE_SIZE    30

#define  SUCCESS            0
#define  FAIL               1


#define CheckFuncResult(result)\
    if (result != SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
    }\


/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct {
    char *frame;
    void *handle; //private data
    int buf_size;
    int64_t enqueueTime;
    int64_t DequeueTime;
    pthread_mutex_t mutex;
}   frame_t;


typedef struct {
    frame_t queue[FRAME_QUEUE_SIZE];
    int rindex;                     // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
    int windex;                     // 写索引
    int size;                       // 总帧数
    int max_size;                   // 队列可存储最大帧数
    int rindex_shown;               // 当前是否有帧在显示
    sem_t sem_avail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}   frame_queue_t;

int frame_queue_init(frame_queue_t *f, int max_size,int frame_size);
void frame_queue_putbuf(frame_queue_t *f,char *frame,int buf_len, void* handle);
void frame_queue_push(frame_queue_t *f);
frame_t *frame_queue_peek_last(frame_queue_t *f,int wait_ms);
void frame_queue_next(frame_queue_t *f,frame_t* pFrame);
void frame_queue_peek_end(frame_queue_t *f);
void frame_queue_flush(frame_queue_t *f);
void frame_queue_destory(frame_queue_t *f);

#if defined (__cplusplus)
}
#endif
#endif

