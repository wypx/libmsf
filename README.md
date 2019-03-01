# libmsf
libmsf acting as mircro service process framework, provide infrastructure and provide service monitoring etc.

基于Linux C的为微服务管理框架

主要特性包括：
1. 为各个模块提供基础设施模块，比如内存，日志，网络，事件，信号等基础库
2. 提供微服务进程动态拉起，动态监控框架，实现微服务进程快速秒级拉起
3. 提供微服务so动态加载，动态卸载，做到插件化的即插即用
4. 提供各个微服务进程的内存监控等亚健康状态监控，达到实时告警通知
5. 提供微服务配置动态解析，热切换微服务进程，实现平滑升级过度


参考文章：
https://www.cnblogs.com/duanxz/p/3514895.html
https://www.cnblogs.com/SUNSHINEC/p/8628661.html
