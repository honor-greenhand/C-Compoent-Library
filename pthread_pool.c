/* 创建一个手写的线程池需要一些基础模块单元
 * 1、事件的集合（用来接收和存储任务）
 * 2、线程的集合（即执行任务单元）
 * 3、管理的单元
 *
 * 除此之外还需要一些方法：
 * 如加入、销毁任务，创建线程池，增加、销毁线程，查询线程状态等等
 * */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* 宏定义：简单的函数用宏定义的形式写上，在预处理阶段会直接放到代码中，不经过任何解释 
 * 在C语言中经常会使用宏定义处理，和inline的方式差不多，但是宏定义用的更多*/

#define LL_ADD(item, list) do { \
    item->prev = NULL;          \
    item->next = list;          \
    if(list != NULL) list->prev = item; \
    list = item;                \
}while(0)      

#define LL_REMOVE(item, list) do{   \
    if(item->prev != NULL) item->prev->next = item->next;   \
    if(item->next != NULL) item->next->prev = item->prev;   \
    if(list == item) list = item->next;         \
    item->prev = item->next = NULL;              \
}while(0)

/* 基础模块: 是组成一个组件的元素 */
// 执行模块
typedef struct NWORKER{
    pthread_t thread;
    int terminate;
    struct NTHREADPOOL *pool;
    struct NWORKER *prev;
    struct NWORKER *next;
}nworker;

// 事务模块
typedef struct NJOB{
    void (*job_func)(struct NJOB *job);
    void *user_data;
    struct NJOB *prev;
    struct NJOB *next;
} njob;

// 管理单元模块
typedef struct NTHREADPOOL{
    struct NWORKER *workers;
    struct NJOB *waiting_jobs;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
}nthreadpool;

// callback
void *thread_callback(void *arg){
    // 是一个线程的执行入口
    nworker *worker = (nworker*)arg;   // 不太懂arg是什么

    while(1){
        pthread_mutex_lock(&worker->pool->mtx);
        while(worker->pool->waiting_jobs == NULL){
            if(worker->terminate) break;
            pthread_cond_wait(&worker->pool->cond, &worker->pool->mtx);
        }
        if(worker->terminate){
            pthread_mutex_unlock(&worker->pool->mtx);
            break;
        }

        njob *job = worker->pool->waiting_jobs;
        // job只是一次任务，从列表中移除证明要实现
        if(job){
            LL_REMOVE(job, worker->pool->waiting_jobs);
        }

        pthread_mutex_unlock(&worker->pool->mtx);
        // job在这里实现
        if(job == NULL) continue;
        job->job_func(job);
    }

    free(worker);
}

/* 基本API：是提供给外界的接口，属于组件的功能函数 */
// create/init 
int thread_pool_create(nthreadpool *pool, int num){
    
    if(pool == NULL) return -1;
    
    // cond:条件变量(互斥条件)
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    // 这种方法创建的是一种全局静态变量
    memcpy(&pool->cond, &cond, sizeof(pthread_cond_t));

    // mutex：线程自带的互斥锁
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool->mtx, &mtx, sizeof(pthread_mutex_t));

    int idx = 0;
    for(; idx < num; ++idx){
        nworker *work = (nworker*)malloc(sizeof(nworker));
        if(work == NULL){perror("malloc"); return idx;}
        memset(work, 0, sizeof(nworker));

        work->pool = pool;
        int ret = pthread_create(&work->thread,
                                 NULL,
                                 thread_callback,
                                 work);
        if(ret){perror("pthread"); free(work);return idx;}
        LL_ADD(work, pool->workers);
    }
    return idx;
}

// push_task：将任务加入
int thread_pool_push_task(nthreadpool *pool, njob *job){
    // 加锁
    pthread_mutex_lock(&pool->mtx);
    LL_ADD(job, pool->waiting_jobs);
    pthread_cond_signal(&pool->cond); // 空闲条件
    // 解锁
    pthread_mutex_unlock(&pool->mtx);
}
// delete_task
int thread_pool_destory(nthreadpool *pool){
    // 这个是销毁所有的线程
    nworker *work = NULL;
    for(work = pool->workers; work != NULL; work=work->next){
        work->terminate = 1;
    }
    pthread_mutex_lock(&pool->mtx);
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mtx);
}

void counter(njob *job){
    if(job == NULL) return;
    int idx = *(int*)job->user_data; // 取出来线程的编号
    printf("idx : %d , selfid : %lu\n", idx,pthread_self());
    free(job->user_data);
    free(job);
}

#define TASK_COUNT 1000
int main(){
    nthreadpool pool = {0};
    int num_thread = 50;

    thread_pool_create(&pool, num_thread);

    int i = 0;
    for(; i < TASK_COUNT; ++i){
        njob *job = (njob*)malloc(sizeof(njob));
        if(job == NULL) exit(1);

        job->job_func = counter;
        job->user_data = malloc(sizeof(int));
        *(int*)job->user_data = i;

        thread_pool_push_task(&pool, job);
    }
    getchar();
}
