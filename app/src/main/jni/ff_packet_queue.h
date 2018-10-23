//
// Created by yijunwu on 2018/10/17.
//
#ifndef AVPacketQueueUtilH
#define AVPacketQueueUtilH

#include <libavcodec/avcodec.h>

//存储的数量限制
#define MAX_QUEUE_SIZE 1000
#define MIN_QUEUE_SIZE 500

typedef struct item {
    AVPacket *data;
    struct item *next;
} Node;


typedef struct linkedlist {
    Node *head;
    Node *tail;
    int size;
} Queue;

//初始化队列
Queue *createQueue();

void freeQueue(Queue *queue);

//入队
void enQueue(Queue *queue, AVPacket *data);

//出队
AVPacket *deQueue(Queue *queue);


int putQueue(Queue *queue, AVPacket *avPacket, pthread_mutex_t *mutex, pthread_cond_t *cond);

int getQueue(Queue *queue, AVPacket *avPacket);

int cleanQueue(Queue *queue);

#endif