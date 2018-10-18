//
// Created by yijunwu on 2018/10/17.
//
#ifndef _TEST_H_
#define _TEST_H_

#include <libavcodec/avcodec.h>

//存储的数量限制
#define MAX_SIZE 1000;
#define MIN_SIZE 500;

typedef struct itemyijun {
    AVPacket *data;
    struct itemyijun *next;
} Node;


typedef struct linkedlist {
    Node *head;
    Node *tail;
    uint32_t size;
} Queue;

//初始化队列
Queue *createQueue();

void freeQueue(Queue *queue);
//入队
void enQueue(Queue *queue, AVPacket *data);

//出队
AVPacket *deQueue(Queue *queue);

void freeAll(Queue *queue);

#endif