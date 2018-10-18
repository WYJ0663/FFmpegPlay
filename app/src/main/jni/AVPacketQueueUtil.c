//
// Created by yijunwu on 2018/10/17.
//

#include <libavcodec/avcodec.h>
#include <pthread.h>
#include "AVPacketQueueUtil.h"
#include "Log.h"

//初始化队列
Queue *createQueue() {
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

void freeQueue(Queue *queue) {
    if (queue) {
        free(queue);
    }
}

//入队
void enQueue(Queue *queue, AVPacket *data) {
    Node *node = (Node *) malloc(sizeof(Node));
    node->data = data;
    if (queue->head == NULL) {//空队列
        queue->head = node;
        LOGE("enQueue 0 %d", queue->size);
    } else if (queue->head != NULL && queue->tail == NULL) {//1个
        queue->tail = node;
        queue->head->next = node;
        LOGE("enQueue 1 %d", queue->size);
    } else {//多个
        Node *tail = queue->tail;
        queue->tail = node;
        tail->next = node;
        LOGE("enQueue n %d", queue->size);
    }
    queue->size++;
}

//出队
AVPacket *deQueue(Queue *queue) {
    AVPacket *data;
    if (queue->head == NULL) {//空队列
        data = NULL;
    } else if (queue->head != NULL && queue->tail == NULL) {//1个
        Node *head = queue->head;
        data = head->data;
        queue->head = NULL;
        free(head);
    } else {//多个
        Node *head = queue->head;
        data = head->data;
        queue->head = head->next;
        free(head);
    }
    queue->size--;
    return data;
}

void freeAll(Queue *queue) {
    while (deQueue(queue) != NULL);
    freeQueue(queue);
    return;
}

