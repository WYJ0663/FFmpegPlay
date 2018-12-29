//
// Created by yijunwu on 2018/10/17.
//
#ifndef QUEUE_H
#define QUEUE_H
#include <pthread.h>

typedef struct item {
    void *data;
    struct item *next;
} Node;


typedef struct linkedlist {
    Node *head;
    Node *tail;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    int is_destroy;
} Queue;

//初始化队列
Queue *createQueue();

//void freeQueue(Queue *queue);

////入队
//void enQueue(Queue *queue, void *data);
//
////出队
//void *deQueue(Queue *queue);

int putQueue(Queue *queue, void *avPacket);

void *getQueue(Queue *queue);

//int cleanQueue(Queue *queue);

int cleanAVPacketQueue(Queue *queue);

void freeAVPacketQueue(Queue *queue);

#endif