//
// Created by yijunwu on 2018/10/17.
//

#include <libavcodec/avcodec.h>
#include <pthread.h>
#include "ff_packet_queue.h"
#include "log.h"

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
    if (data==NULL){ return;}
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
    LOGE("deQueue  %d", queue->size);
    AVPacket *data;
    if (queue->head == NULL) {//空队列
        data = NULL;
    } else if (queue->head != NULL && queue->tail == NULL) {//1个
        Node *head = queue->head;
        data = head->data;
        queue->head = NULL;
        queue->tail = NULL;
        free(head);
    } else {//多个
        Node *head = queue->head;
        data = head->data;
        queue->head = head->next;
        if (queue->head == queue->tail) {
            queue->tail = NULL;
        }
        free(head);
    }
    queue->size--;
    return data;
}


//将packet压入队列,生产者
int putQueue(Queue *queue, AVPacket *avPacket, pthread_mutex_t *mutex, pthread_cond_t *cond) {
    LOGE("插入队列")
    AVPacket *avPacket1 = av_packet_alloc();
    //克隆
    if (av_packet_ref(avPacket1, avPacket)) {
        //克隆失败
        return 0;
    }
    pthread_mutex_lock(mutex);
    LOGE("插入队列 %d ", queue->size);
    //push的时候需要锁住，有数据的时候再解锁
    enQueue(queue, avPacket1);//将packet压入队列
    //压入过后发出消息并且解锁
    pthread_cond_signal(cond);
    pthread_mutex_unlock(mutex);
    return 1;
}

//将packet弹出队列
int getQueue(Queue *queue, AVPacket *avPacket) {
    LOGE("取出队列")
    //如果队列中有数据可以拿出来
    AVPacket *ptk = deQueue(queue);
    if (ptk == NULL) {
        LOGE("取出队列失败")
        return 0;
    }
    if (av_packet_ref(avPacket, ptk) != 0) {//失败
        return 0;
    }
    //取成功了，弹出队列，销毁packet
    av_packet_unref(ptk);
    av_packet_free(&ptk);
    return 1;
}

int cleanQueue(Queue *queue) {
    AVPacket *pkt = deQueue(queue);
    while (pkt != NULL) {
        LOGE("销毁帧%d", 1);
        av_packet_unref(pkt);
        av_packet_free(&pkt);
        pkt = deQueue(queue);
    }
    return 1;
}