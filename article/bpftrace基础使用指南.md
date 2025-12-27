# 一、bpftrace 概述与适用范围

- 概念：bpftrace 是基于 eBPF 的高级动态追踪工具，提供类似 AWK 的脚本语法，用于快速编写内核/用户态的观测脚本与一行命令。
- 适用范围：
    - 系统性能洞察：CPU 热点、I/O 延迟、网络重传、调度延迟等。
    - 应用故障定位：频繁系统调用、慢打开文件、慢查询（USDT）、内存分配/泄漏线索等。
    - 生产日常运维：轻量观察、短时采样、问题复现现场的即席分析。
- 前置条件：
    - Linux 内核通常建议 5.x+ 并开启 eBPF 与 BTF（/sys/kernel/btf/vmlinux），较新发行版默认具备。
    - root 权限或具备 CAP_BPF/CAP_SYS_ADMIN。容器内可能需要宿主机能力或 cgroup 过滤。
    - 安装 `bpftrace` 与（建议）`bpftool` 便于验证环境。
- 核心原理与优势

| 机制              | 实现方式                       | 性能收益               |
| :-------------- | :------------------------- | :----------------- |
| **内核态执行**       | BPF字节码JIT编译为机器码，直接在内核运行    | 零上下文切换，开销仅**1-5%** |
| **Ring Buffer** | 使用`perf_event_array`批量传递数据 | 减少90%用户态唤醒次数       |
| **过滤前置**        | 在**内核态**丢弃无关事件，不传递到用户态     | 避免strace式的数据风暴     |
| **聚合计算**        | `count()`、`hist()`在内核态完成统计 | 只传递最终结果，数据量减少千倍    |

**对比传统的方式**：strace基于`ptrace`，每次系统调用触发`SIGTRAP`暂停进程，开销达**10-100倍**。
# 二、使用环境检查

我们统一建议大家使用docker镜像来运行，作为一个诊断工具，应该要有“工具”的随取随用的样子，同时，很多情况下，我们都没有办法在某些服务器上安装软件。

这里我们我们给出一个使用docker进行进程调用栈监控的测试yaml：
```
version: "3.8"

services:
  bpftrace:
    image: quay.io/iovisor/bpftrace:latest
    privileged: true
    pid: host
    volumes:
      - /sys:/sys:ro
      - /sys/kernel/debug:/sys/kernel/debug
      - /sys/kernel/tracing:/sys/kernel/tracing   # 建议补充
      - /sys/fs/bpf:/sys/fs/bpf                    # 将来 pin map/程序更方便
      - /var/lib/docker:/var/lib/docker:ro
      - ./results:/data
    environment:
      - TARGET_PID=${TARGET_PID:-1}
      - BPFTRACE_PERF_RB_PAGES=4096                # 提升抗丢事件能力
      # - BPFTRACE_BTF=/sys/kernel/btf/vmlinux     # 若将来用到 kfunc/类型解码再开启
    command: >
      sh -c "
        echo 'Profiling PID=$TARGET_PID, output -> /data/profile.txt';
        bpftrace -p $TARGET_PID -e '
          profile:hz:999 /pid == $TARGET_PID/ { @[ustack(20)] = count(); }
          interval:s:30 { exit(); }
        ' > /data/profile.txt
      "
```

# 三、**适用范围与禁忌**

### **适合场景**

1. **生产环境问题诊断**（不能重启服务）
2. **跨进程通信追踪**（管道、Socket、信号）
3. **内核函数调用分析**（TCP/IP、文件系统、调度器）
4. **用户态函数监控**（malloc、SQL解析、GC事件）
5. **性能热点分析**（On-CPU火焰图）
6. **资源泄漏检测**（内存、文件句柄）

### **禁忌场景**

| 场景              | 原因                   | 替代工具                       |
| :-------------- | :------------------- | :------------------------- |
| **需要修改变量/内存**   | BPF程序**只读**，不能改变进程状态 | gdb                        |
| **共享内存通信**      | 无函数调用，无法插桩           | gdb + 手动日志                 |
| **C++虚函数/复杂模板** | 符号表解析困难，ustack可能不准   | perf + DWARF               |
| **反向执行调试**      | 无法回溯历史状态             | gdb rr                     |
| **长期指标存储**      | 不适合作为时序数据库           | eBPF Exporter + Prometheus |
### **同类工具对比矩阵**

| 特性        | **bpftrace** | **gdb** | **strace**  | **SystemTap** |
| :-------- | :----------- | :------ | :---------- | :------------ |
| **设计目标**  | 动态观测         | 交互式调试   | 系统调用追踪      | 动态追踪          |
| **侵入性**   | **无侵入**      | 断点暂停进程  | 进程冻结        | 需加载内核模块       |
| **开销**    | **1-5%**     | 高（断点影响） | **10-100倍** | 中等（脚本优化后）     |
| **监控范围**  | 内核+用户态       | 单个进程    | 仅系统调用       | 内核+用户态        |
| **跨进程追踪** | ✅ **原生支持**   | ❌ 仅当前进程 | ✅ 系统级       | ✅ 支持          |
| **实时性**   | ✅ **实时**     | 暂停后分析   | 实时但延迟高      | 实时            |
## 四、bpftrace 基础用法速览

- 列出探针：
    - 所有：`bpftrace -l`
    - 仅系统调用：`bpftrace -l 'tracepoint:syscalls:*'`
    - 仅内核函数（需 BTF）：`bpftrace -l 'kfunc:*'`
- 查看探针字段：`bpftrace -lv tracepoint:syscalls:sys_enter_openat`
- 运行一行命令：`sudo bpftrace -e 'BEGIN { printf("hello\\n"); }'`
- 运行脚本文件：
    - 脚本头：`#!/usr/bin/env bpftrace`
    - 执行：`sudo bpftrace ./script.bt`
- 常用内置：
    - 事件字段：`args->field`（tracepoint），`arg0..argN`（kprobe/uprobes），`retval`
    - 进程信息：`pid`, `tid`, `comm`
    - 栈与符号：`stack()`, `ustack()`, `ksym()`, `usym()`
    - 时间：`nsecs`, `time()`
    - 映射与聚合：`@map`, `count()`, `sum()`, `avg()`, `min()`, `max()`, `hist()`, `lhist()`, `quantize()`
    - 打印与清空：`print(@map)`, `clear(@map)`
    - 定时触发：`interval:s:5`（每 5 秒）
    - 
## 五、常见问题类型与基础命令

1. 系统调用与打开文件

- 统计各进程系统调用次数：
```
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[comm] = count(); } interval:s:5 { print(@); clear(@); }'
```

- 观察打开文件及耗时（openat）：
```
sudo bpftrace -e '
tracepoint:syscalls:sys_enter_openat { @ts[tid] = nsecs; printf("%-6d %-16s %s\n", pid, comm, str(args->filename)); }
tracepoint:syscalls:sys_exit_openat /@ts[tid]/ {
  @lat_us = hist((nsecs - @ts[tid]) / 1000);
  delete(@ts[tid]);
}
interval:s:5 { print(@lat_us); clear(@lat_us); }'
```

2. CPU 使用与热点

- 采样内核栈热点（99Hz）：
```
sudo bpftrace -e 'profile:hz:99 { @[kstack] = count(); }'
```

- 采样指定进程的用户栈热点（替换 1234 为目标 PID）：
```
sudo bpftrace -e 'profile:hz:99 /pid == 1234/ { @[ustack] = count(); }'
```

- 按进程统计 on-CPU 采样：
```
sudo bpftrace -e 'profile:hz:99 { @[comm] = count(); } interval:s:5 { print(@); clear(@); }'
```

3. 磁盘 I/O 与延迟

- 块层请求延迟直方图：
```
sudo bpftrace -e '
tracepoint:block:block_rq_issue { @start[args->rq] = nsecs; }
tracepoint:block:block_rq_complete /@start[args->rq]/ {
  @disk_lat_ms[args->dev] = hist((nsecs - @start[args->rq]) / 1000000);
  delete(@start[args->rq]);
}
interval:s:5 { print(@disk_lat_ms); clear(@disk_lat_ms); }'
```

- VFS 读写字节数（按进程）：
```
sudo bpftrace -e '
kprobe:vfs_read  { @rd_bytes[comm] = sum(arg2); }  // arg2 = count
kprobe:vfs_write { @wr_bytes[comm] = sum(arg2); }
interval:s:5 { printf("READ bytes by comm\\n"); print(@rd_bytes); clear(@rd_bytes);
               printf("WRITE bytes by comm\\n"); print(@wr_bytes); clear(@wr_bytes); }'
```

4. 网络连接与重传

- TCP 连接统计（目的 IP/端口）：
```
sudo bpftrace -e 'tracepoint:tcp:tcp_connect { @[comm, ntop(args->daddr), args->dport] = count(); } interval:s:5 { print(@); clear(@); }'
```

- TCP 重传计数：
```
sudo bpftrace -e 'tracepoint:tcp:tcp_retransmit_skb { @[comm] = count(); } interval:s:5 { print(@); clear(@); }'
```

- 网卡收发吞吐（每秒 Byte，按网卡）：
```
sudo bpftrace -e '
tracepoint:net:net_dev_queue       { @tx[args->name] = sum(args->len); }
tracepoint:net:netif_receive_skb   { @rx[args->name] = sum(args->len); }
interval:s:1 { printf("TX(B/s)\\n"); print(@tx); clear(@tx);
               printf("RX(B/s)\\n"); print(@rx); clear(@rx); }'
```

5. 调度/等待延迟

- 运行队列唤醒到运行的延迟（run queue latency）：
```
sudo bpftrace -e '
tracepoint:sched:sched_wakeup { @w[args->pid] = nsecs; }
tracepoint:sched:sched_switch /@w[args->next_pid]/ {
  @runqlat_us[args->next_comm] = hist((nsecs - @w[args->next_pid]) / 1000);
  delete(@w[args->next_pid]);
}
interval:s:5 { print(@runqlat_us); clear(@runqlat_us); }'
```

6. 用户态内存分配（libc）

- 统计 `malloc` 尺寸与 `free` 次数（替换库路径与 `-p` 目标 PID）：
```
sudo bpftrace -p 1234 -e '
uprobe:/lib/x86_64-linux-gnu/libc.so.6:malloc    { @malloc_sz = hist(arg0); }
uretprobe:/lib/x86_64-linux-gnu/libc.so.6:malloc { @malloc_ret[retval] = count(); }
uprobe:/lib/x86_64-linux-gnu/libc.so.6:free      { @free[arg0] = count(); }
interval:s:5 { print(@malloc_sz); }'
```

7. 应用 USDT（示例范式）

- 通用 USDT 起止事件（需应用启用 USDT）：
```
sudo bpftrace -e '
usdt:/usr/sbin/mysqld:query__start { @ts[tid] = nsecs; }
usdt:/usr/sbin/mysqld:query__done  /@ts[tid]/ {
  @q_lat_us = hist((nsecs - @ts[tid]) / 1000);
  delete(@ts[tid]);
}
interval:s:5 { print(@q_lat_us); clear(@q_lat_us); }'
```

提示：具体 USDT 名称请通过 `bpftrace -l 'usdt:/path/to/bin:*'` 查询。

## 六、脚本范式模板

- 文件 `opensnoop.bt`：
```
#!/usr/bin/env bpftrace

BEGIN { printf("opensnoop (openat)\\n"); }

tracepoint:syscalls:sys_enter_openat {
  printf("%-6d %-16s %s\n", pid, comm, str(args->filename));
}
```

- 运行：`sudo bpftrace ./opensnoop.bt`

## 七、探针类型与选择建议

- `tracepoint`：稳定字段，低风险，首选用于内核事件（syscalls、sched、net、block 等）。
- `kprobe/kretprobe`：钩住内核函数入口/返回，灵活但易随内核版本变化。新内核优先选 `kfunc`。
- `kfunc/kretfunc`：基于 BTF 的函数探针，更稳健（推荐）。
- `uprobe/uretprobe`：用户态二进制/库函数入口/返回；搭配 `-p PID` 可以限制进程。
- `usdt`：用户态静态探针（由应用暴露），字段稳定，适合重要业务指标。
- `profile/interval`：采样与定时触发，适合热点统计与周期打印。
- `rawtracepoint`/`perf` 事件：更底层或硬件计数，按需使用。
## 八、最佳实践与性能注意

- 优先使用 `tracepoint` 与 `kfunc`，减少版本漂移。
- 热路径中尽量聚合到 `@map`，避免频繁 `printf`。
- 使用过滤条件降低开销：例如 `/pid == 1234/`、`/comm == "nginx"/`、`/cgroupid == X/`。
- 使用 `interval` 周期打印并 `clear(@)`，防止映射无限增长。
- 针对高频采样（如 `profile:hz:999`）谨慎使用，先从低频开始。
- 指针解引用需要 `--unsafe`，生产环境尽量避免。
- 容器环境建议结合 cgroup 过滤或在宿主机运行并按容器进程过滤。

## 九、常见问题排查

- “BTF not found” 或类型解码失败：检查 `/sys/kernel/btf/vmlinux`，升级内核或安装带 BTF 的内核包；退回用 `kprobe`+原始参数。
- “permission denied”：缺少特权，使用 root 或配置 CAP_BPF/CAP_SYS_ADMIN。
- 用户态探针偏移不符：确认二进制路径与架构；对 PIE/ASLR 可使用 `-p` 目标 PID。
- 事件丢失/性能异常：降低频率、减少打印、改用聚合；检查 `perf_event_paranoid` 与资源限制。

## 十、参考工具集

- 自带示例：`/usr/share/bpftrace/tools/`（不同发行版路径可能不同），包含 `opensnoop.bt`、`biosnoop.bt` 等常用脚本。
- 官方文档与手册：`man bpftrace`、项目 Wiki/README。
- 结合 perf/BCC：当需要火焰图或复杂分析时，bpftrace 先定位，perf/BCC 后深化。