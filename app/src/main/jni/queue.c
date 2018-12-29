//
// Created by yijunwu on 2018/10/17.
//

#include <pthread.h>
#include <malloc.h>
#include <libavcodec/avcodec.h>
#include "queue.h"
#include "log.h"

//初始化队列
Queue *createQueue() {
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);

    queue->is_destroy = 0;
    return queue;
}

void freeQueue(Queue *queue) {
    if (queue) {
        queue->is_destroy = 1;
        cleanQueue(queue);
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->cond);
        free(queue);
    }
}

//入队
int enQueue(Queue *queue, void *data) {
    if (queue->is_destroy == 1) {
        return 0;
    }
    if (data == NULL) { return 0; }
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

    return 1;
}

//出队
void *deQueue(Queue *queue) {
    LOGE("deQueue  %d", queue->size);
    void *data;
    if (queue->head == NULL) {//空队列
        data = NULL;
        queue->size = 0;
    } else if (queue->head != NULL && queue->tail == NULL) {//1个
        Node *head = queue->head;
        data = head->data;
        queue->head = NULL;
        queue->tail = NULL;
        free(head);
        queue->size--;
    } else {//多个
        Node *head = queue->head;
        data = head->data;
        queue->head = head->next;
        if (queue->head == queue->tail) {
            queue->tail = NULL;
        }
        free(head);
        queue->size--;
    }
    return data;
}

//将packet压入队列,生产者
int putQueue(Queue *queue, void *avPacket) {
    if (queue->is_destroy == 1) {
        return 0;
    }
    LOGE("插入队列 %d ", queue->size);
    pthread_mutex_lock(&queue->mutex);

    //push的时候需要锁住，有数据的时候再解锁
    enQueue(queue, avPacket);//将packet压入队列
    //压入过后发出消息并且解锁
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return 1;
}

//将packet弹出队列
void *getQueue(Queue *queue) {
    LOGE("取出队列")
    pthread_mutex_lock(&queue->mutex);
    //如果队列中有数据可以拿出来
    void *ptk = NULL;
    while (1) {
        ptk = deQueue(queue);
        if (ptk != NULL) {
            //如果队列中有数据可以拿出来
            break;
        } else {
            LOGE("队列 wait")
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
    }
    //压入过后发出消息并且解锁
    pthread_mutex_unlock(&queue->mutex);
    return ptk;
}

int cleanQueue(Queue *queue) {
    pthread_mutex_lock(&queue->mutex);
    void *pkt = deQueue(queue);
    while (pkt != NULL) {
        LOGE("销毁%d", 1);
        pkt = deQueue(queue);
    }
    pthread_mutex_unlock(&queue->mutex);
    return 1;
}


//针对AVPacket销毁
int cleanAVPacketQueue(Queue *queue) {
    pthread_mutex_lock(&queue->mutex);
    AVPacket *pkt = deQueue(queue);
    while (pkt != NULL) {
        LOGE("销毁帧%d", 1);
        av_packet_unref(pkt);
        av_packet_free(&pkt);
        pkt = deQueue(queue);
    }
    pthread_mutex_unlock(&queue->mutex);
    return 1;
}

void freeAVPacketQueue(Queue *queue) {
    if (queue) {
        queue->is_destroy = 1;
        cleanAVPacketQueue(queue);
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->cond);
        free(queue);
    }
}
