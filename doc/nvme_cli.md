# NVME-CLI

用户态读写NVMe磁盘, 慢盘检测问题

HB08 RSSD 后端集群发现一块磁盘读正常， 写延时非常高 4k写5ms左右， 256k写300多ms

慢盘检测考虑chunk 定期上报磁盘的iostat信息，按带宽设置告警阈值。

慢盘smart-log

![image](image/slow_ssd_smatr_log1.png)
![image](image/slow_ssd_smatr_log2.png)

异常盘perf

![image](image/bad_ssd_perf.png)

正常盘perf

![image](image/normal_ssd_perf.png)
