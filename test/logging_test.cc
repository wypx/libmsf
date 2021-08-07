#include "../Timestamp.h"
#include "../AsyncLogging.h"
#include "../Logging.h"
#include "../ThreadPool.h"
#include "../LogStream.h"
#include "../Mutex.h"

#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>

int kRollSize = 500*1000*1000;

void* g_asyncLog = NULL;

template<typename ASYNC>
void asyncOutput(const char* msg, int len)
{
  ASYNC* log = static_cast<ASYNC*>(g_asyncLog);
  log->append(msg, len);
}



muduo::AtomicInt64 bytesWriten;


template<typename ASYNC>
void doBench(ASYNC* log)
{

  int cnt = 0;
  const int kBatch = 1000;
  const bool kLongLog = false;

  muduo::string empty("");
  muduo::string smallStr(1, 'X');
  muduo::string midStr(110, 'X');
  muduo::string longStr(3000, 'X');
  longStr += " ";

    int runs = 512;
    for (int t = 0; t < runs; ++t)
  {
    muduo::Timestamp start = muduo::Timestamp::now();
    for (int i = 0; i < kBatch; ++i)
    {
      // kLongLog用来尝试4K大小的日志和一般的小日志时候的bench mark
      LOG_INFO << (kLongLog ? longStr : midStr);
      ++cnt;
    }
    muduo::Timestamp end = muduo::Timestamp::now();

    // 下面表示这一次: 每条数据需要消耗多少ms
    printf("%f\n", timeDifference(end, start) * 1000000 / kBatch);
  }

    LOG_INFO << (kLongLog ? longStr : "ABCDEFG");
    LOG_INFO << (kLongLog ? longStr : "1234567");

    muduo::Timestamp end = muduo::Timestamp::now();

    // 计算每s能写多少字节的数据
    long MsgSize = (kLongLog ? 3000 : 110); //Byte
    long Bytes = MsgSize * cnt;// 这一次写入的数量
    bytesWriten.add(Bytes);
}

template<typename ASYNC>
void benchPrepare(ASYNC* log){
    g_asyncLog = log;
    log->start();
    muduo::Logger::setOutput(asyncOutput<ASYNC>);
}

template<typename ASYNC>
void bench(ASYNC* log){
    benchPrepare(log);
    doBench(log);
    printf("total = %f MB\n",((bytesWriten.get() + 0.0) / (1024 * 1024)));
}


// 多线程的情况下
template<typename ASYNC>
void mutilBench(ASYNC* log, int numThreads ){
    benchPrepare(log);
    muduo::ThreadPool pool("logPool");
    pool.start(numThreads);
    for(int i = 0; i < numThreads; i++){
        pool.run(std::bind<void(ASYNC*)>(doBench, log));
    }
    usleep(100000);

    pool.stop();

    printf("total = %f MB\n",((bytesWriten.get() + 0.0) / (1024 * 1024)));
}


int logging_test(int argc, char* argv[])
{


  muduo::AsyncLoggingDoubleBufferingShards log5("log7", kRollSize);
  muduo::AsyncLoggingDoubleBufferingShards log6("log7", kRollSize);
  muduo::AsyncLoggingDoubleBufferingShards log7("log7", kRollSize);
  muduo::AsyncLoggingDoubleBuffering log8("log8", kRollSize);

  printf("pid = %d\n", getpid());
  int which = 8;
    muduo::Timestamp start = muduo::Timestamp::now();

    switch (which)
  {
      case 1:
          // 别的case
          break;
      case 5:
          /* 单线程,最原始版本的性能:
           * case1 在每次写入10字节的情况下,大约能写入170M - 200M
           * case2 在每次写入110字节的情况下,大约的能写入 200-220MB左右的速度
           * case3 每次写入3110字节的情况下,大约能写入200MB每S的速度
           * todo 整体来看, 每次append的耗时体现在 formateTime & Fmt::Fmt里面的snprintf
           * todo 想办法优化这个地方,可以有效降低延迟?增加吞吐(就一个线程，延迟降低吞吐肯定增加)
           *    利用特殊的FmtMicroSeconds直接优化到610M/s的速度
           *    将currentThread::tid也缓存起来之后,
           *    将Mutex换成SpinLock
           *    8线程情况下 可以达到900M/s的吞吐
           *    使用了writev对文件进行批量写入,减少系统调用,但是无明显变化(变化小于1%)
           *    后来卡在写入磁盘的速度上面了,这个没有太好办法。。因为磁盘的写入能力只有那么多(表现为消息堆积,这说明写入速度不够快)
           *
           * */
          bench(&log5);
          break;
      case 6:
          /*
           * case1 单线程情况, 写入速度220-230MB左右,估计和预热有关
           *       4线程情况,  写入速度180Mb左右
           *       profile之后进行对比,发现多出来的主要耗时在于mutex_wait上,也就是锁竞争激烈导致的
           *       光使用火焰图是不够的，
           *        1 因为上面没有显示实际调用耗时，而是使用的百分比(如果分子分母同时变大可能会不明显)
           *        2 如果使用成员函数，看不到占用的时间长度
           *       对比: append占比 6.4% -> 43%,增加了8倍
           *                unlock和lock_wait的耗时都显著增加了很多
           *            但是在实际使用中,日志库不存在这样的问题,因为业务写日志的时间肯定远远小于正常逻辑的执行(除非这个逻辑也特别短)
           *            所以发生碰撞的可能性非常的小了
           *
           * */
          mutilBench(&log6, 4);
          break;

      case 7:
          /*
           * 有2个比较致大的缺点:
           *    1. 多个localBuffer导致顺序不是按照时间来的, 比如线程A和B可能B会比A晚3s
           *    2. 如果中间刷入的压力不大的情况下, 会导致Buffer比较浪费(比如里面只有10条消息),不过既然压力不大就无所谓性能
           *    3. 如果线程数量很多(比如在64核机器上,我们开256个线程),那么实际上也不可能无限增加partition,
           *       这会导致后台线程过多的循环和加锁解锁,以及内存方面不好控制(比如1s一次,那么每秒都有256 * 4M的Buffer??太浪费)
           *    4. buffer太多可能导致空间局部性不够好,因为缓存可能还回去的时候会还到另外一个线程/cpu上,那么原来本来可能已经加载到L1的现在失效(对写入有影响)
           *       但是这个影响应该不是很大，因为L1一共也就4096 * 64Byte大小,很快就会重新写满了
           *    5. 整体一个大buffer对于mmap更友好,否则文件不好管理
           *
           *    考虑Disruptor来解决这几个问题
           * case0:  单线程情况下是640M左右,速度非常快,平均每10000条日志延迟在0.11-0.15ms左右
           * case1， 4线程的情况下是785M-870M/s
           * case2,  8线程下680M/S-900M/s
           * case3   16线程下440M，415M/s
           * case4:  32线程下，440M-460, 单个线程的延迟增加
           *         64线程 256-320MMB
           *         128线程 155M/s
           *         profile之后发现, append线程各个函数占比变化并不大(说明延迟增高是由于线程切换?)
           *         但是后台线程变化比较大,主要体现在: 释放内存大幅度增加(free_large,这显然会导致写入磁盘的效率降低),经过检查发现是写入太快导致的(buffer数量受限)
           *         通过提高内存中常驻buffer的数量 & 降低buffer刷入buffers的频率，可以略微提高性能
           * */
          //bench(&log7);
          mutilBench(&log7, 4);
          break;
      case 8:
          /*
           但是内存中的Buffer要少一些
           将Buffer做了如下改进:
               0 去掉了alloc_count和idle_count,并且使用四Buffer来进行交换,确保读数据的时候没有数据写入
               1 使用了读写自旋锁来保护对Buffer的assign 和 move
               2 将锁分段,写锁获取所有锁,读只获取单个锁。
               3 改为写者优先,确保不会饿死, 同时读发现写在等待的时候立刻进行线程切换,确保双方无意义的互斥(性能提高30%)
               4 将各个线程获取写锁的时机错开(性能提升10%),并在获取写锁后检查下是不是容量已经发生变化了(可能别的线程已经push了currentBuffer)
               5 和Buffer有关,将Buffer改小之后起到效果(pageFault造成的)
               6 分段 + CircularBuffer + emptyBuffers的情况下面对大量线程效果更好
          */

          /* 最终结论是, 在线程竞争非常激烈,活跃线程很多的情况下, 分段 + CircularBuffer性能最好,但是内存开销比较多
           * 1线程:  460M- 500M/s
           * 32线程: 786M
           * 64线程: 3791M
           * 128线程 : 700Mb/s左右
           * 256线程: 385M/s 左右
           *
           * */
          mutilBench(&log8, 1);//但是在多线程下会出现较多PageFault
          break;
  }


    // 最后总结
    muduo::Timestamp end = muduo::Timestamp::now();
    double Duration = timeDifference(end, start);
    log8.stop();
    printf("rate = %2fMb, duration: %2fms\n",(bytesWriten.get() / (1024 * 1024 * Duration)), 1000 * Duration);
}
