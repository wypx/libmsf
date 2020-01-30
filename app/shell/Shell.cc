#include <base/Version.h>
#include <base/Logger.h>
#include <base/Thread.h>
#include <base/Os.h>
#include <base/Time.h>
#include <base/Signal.h>
#include <event/Timer.h>
#include <client/AgentClient.h>

using namespace MSF::EVENT;

/* 这个表示一个方法的返回值只由参数决定, 如果参数不变的话,
    就不再调用此函数，直接返回值.经过我的尝试发现还是调用了,
    后又经查资料发现要给gcc加一个-O的参数才可以.
    是对函数调用的一种优化*/
__attribute__((const)) int test2()
{
    return 5;
}

/* 表示函数的返回值必须被检查或使用,否则会警告*/
__attribute__((unused)) int test3()
{
    return 5;
}



/* 这段代码能够保证代码是内联的,因为你如果只定义内联的话,
	编译器并不一定会以内联的方式调用,
	如果代码太多你就算用了内联也不一定会内联用了这个的话会强制内联*/
static inline __attribute__((always_inline)) void test5()
{

}

__attribute__((destructor)) void after_main()  
{  
//    ("after main.");  
} 


void  MSFLogTest(void) 
{
    std::string LogPath = "msf.log";
    std::string module = "MSF";
    Logger::getLogger().init(LogPath.c_str());
    MSF_INFO  << "Hello world1." << "yyyyyyyy" << ":" << 2;
 
    MSF_DEBUG << "Hello world2.";
    MSF_WARN  << "Hello world3.";
    MSF_ERROR << "Hello world4.";
    MSF_FATAL << "Hello world5.";
}

int fun1(uint32_t id, void* args) 
{
    MSF_FATAL << "Hello Timer = " << id;
    return TIMER_PERSIST;
}

int fun2(uint32_t id, void* args) 
{
    MSF_INFO << "Hello Timer = " << id;
    return TIMER_ONESHOT;
}

int main(int argc, char* argv[]) 
{
    MSF::MSF_BUILD_STATISTIC();

    MSFLogTest();

    // msf_os_init();

    MSF::TIME::msf_time_init();
    // MSF_WARN << "Time str: " << MSF::TIME::GetCurrentTimeString(NULL);

    HeapTimer * timer = new HeapTimer;
    timer->initTimer();
    timer->startTimer();
    
    timer->addTimer(1, TIMER_ONESHOT, 500, NULL, fun1);
    
    timer->addTimer(2, TIMER_PERSIST, 2000, NULL, fun2);

    // AgentConn *cli = new AgentConn();
    // cli->initAgent();
    // cli->startAgent();
    
    while (true) {
        sleep(1);
    }

    // TimerDestroy();

    return 0;
}
