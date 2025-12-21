## 背景

在日常的开发过程中，为了实现对目标Docker容器的隔离化监控与深度性能分析，我们通常需要在一个独立的“监控容器”中部署诊断工具。针对这一需求，业界主要存在两种主流技术路径：一是在监控容器中利用经典的 perf 工具进行分析；二是在监控容器中部署 eBPF 程序进行观测。perf 作为Linux内核的原生工具，能够提供深入硬件底层的性能指标，但其运行强依赖于宿主机内核版本且需要极高的系统权限，带来了安全与可移植性的挑战。相比之下，eBPF 技术以其卓越的灵活性和内核内可编程能力，能够捕获更丰富的系统事件与上下文信息，但在安全权限和内核版本上同样存在严格要求，且技术门槛相对更高。由于常见的系统内核基本满足了内核大于4.15的要求，我们还是建议采用docker+ eBPF 组成sidecar模式来做性能调试。下文将穿插探讨这两种方案，分析其适用场景。

## 原理

**`perf` 的工作流程较为传统，可以粗略地看作：**

1. **内核态**：内核的性能事件子系统根据 `perf` 的指令，在采样事件发生时（比如时钟中断）捕获当时的指令指针（内存地址）和调用栈，并将这些原始数据写入一个环形缓冲区 (Ring Buffer)。
2. **用户态**：用户态的 `perf` 工具从这个缓冲区读取大量原始二进制数据，保存为 `perf.data` 文件。
3. **事后分析**：`perf script` 命令读取 `perf.data`，然后开始漫长的符号解析过程，它需要逐个查找地址对应的文件和函数名。**整个符号解析过程是完全在事后、在用户态完成的。**

![img](https://github.com/zhaolingxi/blog_res/blob/master/image/20250803/perf.PNG?raw=true)

**eBPF / bpftrace 的工作模式：**

`bpftrace` 的工作流程更现代化和高效，分为两个清晰的阶段：

1. **内核态 (eBPF 程序)**：
   2. 当采样事件发生时（例如 `profile:hz:99` 探针触发），附加在上面的 eBPF 程序会执行。
   3. 它会调用一个关键的 BPF 辅助函数，比如 `bpf_get_stackid`。这个函数在**内核中**安全地遍历调用栈，并将整个调用栈（一组内存地址）作为一个整体进行“哈希”，得到一个唯一的“堆栈ID”。
   4. eBPF 程序将这个 **堆栈ID** 和 **计数值** 存储在一个高效的 BPF `map`（一种内核里的键值存储）中。
   5. **关键区别**：内核态只负责收集和聚合堆栈的“指纹”（ID），而不是把海量的原始地址流扔给用户态。这极大地降低了内核与用户空间之间的数据传输量。
6. **用户态 (****`bpftrace`** **命令行工具)**：
   7. `bpftrace` 工具会定期从 BPF map 中读取数据。它拿到的是成对的 `(堆栈ID, 计数值)`。
   8. 同时，它也会读取一个由 `bpf_get_stackid` 维护的、从 `堆栈ID` 到**实际内存地址调用栈**的映射表。
   9. 现在，`bpftrace` 用户态工具拿到了具体的内存地址调用栈（例如 `[地址A, 地址B, 地址C]`），它才开始进行符号解析。

## 调试方法分析

1. ### 使用 Docker 内置命令及标准 Linux 工具 (宏观分析)

这是最简单、无侵入性的第一步，用于快速判断是哪种资源（CPU、内存、I/O）出现了瓶颈。

- **`docker stats`**: 这是一个实时展示容器资源使用情况的命令。它可以快速告诉您哪个容器是资源消耗大户。
  - **优点**: 无需安装任何东西，实时方便。
  - **缺点**: 提供的信息非常宏观，只能看到资源使用率，无法知道为什么高。
- **宿主机上的** **`top`****,** **`htop`****,** **`pidstat`**: 您可以在宿主机上运行这些工具来间接监控容器。首先，找到容器在宿主机上对应的进程ID（PID），然后针对性地进行监控。
  - 获取容器的完整ID: `docker inspect -f '{{.Id}}' <container_name_or_id>`
  - 找到容器的主进程PID: `docker inspect -f '{{.State.Pid}}' <container_name_or_id>`
  - 使用工具监控该PID: `htop -p <PID>` 或 `pidstat -u -p <PID> 1` (每秒采样一次CPU)
  - **优点**: 利用了宿主机自带工具，信息比 `docker stats` 更丰富（例如可以看到用户态/内核态CPU时间）。
  - **缺点**: 依然无法定位到具体的函数或代码行。
  - ###  2. 使用 eBPF 工具集 (微观分析)

  -  eBPF (Extended Berkeley Packet Filter) 允许在内核中运行一个安全的、沙箱化的“微型程序”，从而在不修改内核代码、不引入内核模块的情况下，实现极高性能的监控和追踪。

  - **原理**: eBPF 程序附加到内核的特定事件点（如系统调用、函数入口/返回、网络事件等）。当这些事件发生时，程序会执行并收集信息，几乎没有性能开销。
  - **常用工具集**:
    - **BCC (BPF Compiler Collection)**: 提供了大量基于 Python/C++ 的高级 eBPF 工具，可以直接使用。
    - **bpftrace**: 提供了一种类似 `awk` 和 `DTrace` 的高级追踪语言，编写自定义的追踪脚本非常方便。
  - **如何使用 eBPF 监控 Docker 容器？**
    - **安装**: 通常只需要在宿主机上安装 `bcc-tools` 或 `bpftrace` 即可
    - **定位**: 大多数 eBPF 工具可以通过 cgroup 来过滤，只监控特定 Docker 容器内的事件。
    - **常用工具示例**:
      1. `profile`: 采样分析指定进程的 CPU on-CPU 时间，找出热点函数（用户态和内核态）。 `profile -p <Container_PID> 10` (采样10秒)
      2. `funccount`: 统计一个函数被调用的次数。
      3. `opensnoop`: 追踪指定进程打开文件的操作，用于分析 I/O 瓶颈。
      4. `execsnoop`: 追踪进程执行新命令的操作。
      5. `tcplife`: 查看 TCP 连接的生命周期，分析网络问题。
  - **优点**: 功能极其强大，性能开销低。
  - **缺点**: 需要宿主机内核版本支持（通常是 4.1 或更高版本），但现在大多数主流发行版的内核都已支持。（ubuntu16以上都不用担心）
  - ###  3. 应用层性能剖析 (APM - 微观分析)

  -  这种方法将监控的重点从“容器”这个黑盒转向“容器内运行的应用程序”本身。通常这是定位代码层面瓶颈最精确的方法。

  - **原理**: 在应用程序代码中集成一个 Profiling 库或 Agent。这个 Agent 会在运行时收集应用的内部性能数据（如函数调用栈、内存分配、锁竞争等），然后通过网络发送出来。
  - **不同语言的方案**:
    - **Go**: `net/http/pprof` 是标准库自带的强大工具，只需在代码中匿名导入即可通过一个 HTTP 端点暴露性能数据。
    - **Java**: `async-profiler` 是一个非常优秀的低开销采样分析器。此外，商业的 APM 工具如 New Relic, Datadog, Dynatrace 的 Java Agent 功能也很强大。
    - **Python**: `py-spy` 和 `austin` 可以在不侵入代码的情况下，对正在运行的 Python 程序进行采样分析。`cProfile` 是内置的分析器，但性能开销较大。
    - **Node.js**: 内置了 V8 profiler，可以通过 `--prof` 标志生成分析日志，再用 `--prof-process` 解析。
  - **优点**: 可以直接定位到有问题的代码行、函数或类，信息最详细、最直接。
  - **缺点**: 需要对应用进行一定的配置或微小的代码修改。

## 代码工程

我们可以通过写BPFtrace代码的方式实现性能瓶颈分析，但是个人认为新启动一个docker才是正解，我们对于性能分析工具的定义首要是个工具，也就是即开即用，同时，由于客户允许运行我们的docker，那大概率也可以运行一个其他docker，但是更改内核、安装程序就不一定可以了。

综上，我们写一个yml去启动检查容器：

```YAML
version: "3.8"

services:
  bpftrace:
    image: quay.io/iovisor/bpftrace:latest
    privileged: true
    pid: host
    volumes:
      - /sys/kernel/debug:/sys/kernel/debug
      - /sys:/sys:ro
      - /var/lib/docker:/var/lib/docker:ro
      - ./results:/data
    environment:
      - TARGET_PID=${TARGET_PID:-1}
    # 修改 command 以将输出重定向到文件
    command: >
      sh -c "
        echo 'Profiling PID=$TARGET_PID, output will be saved to /data/profile.txt';
        bpftrace -p $TARGET_PID -e '
          profile:hz:99 /pid == $TARGET_PID/ {
            @[ustack] = count();
          }
          interval:s:10 {
            exit();
          }
        ' > /data/profile.txt
      "
# 准备好宿主机上的结果目录
mkdir -p ./results

# 导出PID
# export TARGET_PID=$(...)

# 直接启动即可
TARGET_PID=$TARGET_PID docker-compose -f ./bpf_test.yml up

# 运行结束后，容器会自动停止。
# 结果文件会出现在宿主机的 ./results/profile.txt
echo "分析完成！结果已保存在 ./results/profile.txt"
cat ./results/profile.txt

# 最后别忘了清理
docker-compose -f ./bpf_test.yml down
```

## 应用实例

把docker、启动脚本推送到目标服务器：

```C++
scp 本地文件 用户名@远端IP或域名:远端路径
```

获取容器 A（被监控容器） 的主进程 PID

```Bash
docker inspect --format '{{.State.Pid}}' <容器A名称或ID>
```

当然，如果容器内部有其他应用，我们推荐还是看仔细：

```C++
docker top <容器A名称>
```

```Bash
export TARGET_PID=<进程PID>
```

一键启动

```Bash
./bpf_out.bash
```

把输出重定向到文件即可用 [FlameGraph](https://github.com/brendangregg/FlameGraph) 生成 SVG 火焰图。(具体的处理代码我们可以从[brendangregg/FlameGraph: Stack trace visualizer](https://github.com/brendangregg/FlameGraph)  中获取)

```C++
https://www.speedscope.app/
./stackcollapse-bpftrace.pl ../../results/profile.txt > ../../results/profile.folded.txt
或者
./stackcollapse-bpftrace.pl ../../results/profile.txt | ./flamegraph.pl > ../../results/profile_flamegraph.svg
```

分析示例:![img](https://github.com/zhaolingxi/blog_res/blob/master/image/20250803/flame_work.png?raw=true)