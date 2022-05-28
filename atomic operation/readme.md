1、互斥锁 mutex
2、自旋锁 spinlock
3、原子操作
4、线程私有空间 ，pthread_key
5、共享内存
6、CPU的亲缘性， affinity
7、setjmp/longjmp


mutex和spinlock的区别：
mutex：锁被占用，引起线程切换
spinlock：线程会一直等待、
使用场景：1、临界资源操作简单/没有系统调用，可以优先选择spinlock
          2、操作复杂/有系统调用， 使用mutex，主要 开销在线程切换上（复杂程度超过切换）
          
原子操作的作用场景，必须要CPU指令集支持的情况下才可以。

CAS：compare and swap

1. try、catch嵌套如何解决
2. 线程安全问题怎么解决？

进程线程黏合
