# Windbg内存问题排查

## 前言：

## windbg简介：

作为一名程序员，平时的本质工作就是写自己的bug，改别人的bug……

bug就像是空气，伴随我们的整个职业生涯……

快速精准的定位问题，不仅可以在项目发版日期的抢救上力挽狂澜，也是定位责任人、组合周报的好帮手……

对大多数的CPP工程师来说，宇宙第一大IDE VS就像是一个勤勤恳恳的保姆、从第一个helloworld到图像引擎、驱动开发都是在它的陪伴下完成的。得益于其对大型项目的优质支持，在很多千万级代码量的大项目中，有时候也仅需要这一个软件就能完成程序的搭建、编译、调试、打包，乃至于版本的管理、共享都可以交给他。

但是它也有力所不能及的时候，例如我们想要更贴近系统内核级别的调试，更自由的调试风格，直接操作寄存器，或者是扩展调试器本身，这些都是需要更加专业和贴近操作系统的工具来提供支持。

此时，在windows端下，windbg就成了不二之选。

它是微软团队用来开发内核的时候使用的调试工具，因此具备各种复杂的调试特性支持。我们上述的哪些VS难以企及的领域，在它面前都不是问题。

但也正是由于它贴近系统且较为专业小众，它的交互和使用体验远不如老少皆宜的集成IDE，许多的操作都需要使用命令行+参数完成，并且由于其使用场景，需要用户了解一些操作系统的基础知识，例如PE文件的格式与内容、寄存器的作用、简单的汇编语句等，这些都是普通用户使用的难点。

不过，工具是拿来用的，不是拿来做题的，我们不用了解太多它的特性与命令内容。只要掌握其大致的工作流程和常用命令就可以解决大部分的问题，其他功能可以在使用的过程中查阅其文档来辅助使用。

接下来是两个使用实例，分别针对栈内存溢出和堆内存溢出的问题定位与调试。

## 常见的内存问题：

由于CPP是一门相对自由的语言，我们可以使用指针指向任意地址的数据，同时我们也有分配内存和销毁内存的权限。在某种程度上，这也是C++常年被人诟病不够安全的问题所在，因为人不可能不犯错。而内存的错误往往都是致命的。

常见的内存错误主要集中于内存的错误使用（读写错误的地址），内存所有权的生命周期管理（内存泄漏）。

其中，很大一部分是由于程序员的失误导致分配内存不足或者是获取内存之后没有释放导致。

我们分别使用两个实例来模拟这两种情况。

示例代码我们放在了github上面，仓库链接如下：

同时，在仓库中我们也附上了对应的调试工具。

## 常见的问题定位方式：

对于栈内存越界的问题，大家会发现在VS中就能及时调试与纠正，但如果是在其他的三方库中出现的这个问题呢？我们在搭建项目的过程中，不一定会将所有的源代码都放在一起生成，考虑到不同的三方库使用了不同的编码规范和不同的编译器。我们通常不愿意也不能直接修改这些其他公司的代码。

如果确实在他们的代码中发现了这些低级错误，我们也常常会采用通知三方修改、规避出错的功能函数的形式来临时补救。且对于只有DLL的情况下，问题的定位也会相对复杂一些，我们这里还是使用含有PDB的形式来定位问题。

对于堆的内存错误，相对来说会麻烦一些，因为它的暴露有一定的滞后性，当然，栈也一样，但是由于栈本身较小，而且在子函数返回时会进行检查。而堆相对更大，只有在使用同一块内存，出现问题的时候，它才会暴露出来。

这个时候，单纯的调试器就难以帮上忙了，我们需要借助一下内存工具，比如gflags，开启页堆之后，才能较为准确的暴露问题。注意，开启页堆之后，仍然会有页对齐的问题，所以在64位系统中8个字节就是最小的暴露单位。

## 两个问题解决实例：

先简要的介绍一下流程：

通常情况下，我们对问题的定位有调试定位和dump定位两种方式，第一种常见于在开发环境中，对测试出来的问题定位修改，第二种常见于在客户现场出现问题之后，以发送dump的形式进行远程定位和处理。

我们先介绍前一种的大致流程，

第一步是将调试器连接到可执行文件上，使用open exe或者attach exe的形式来连接。这里我们建议连接的时间至少是要早于你需要调试的dll加载的时间，这样才不会丢失你需要的数据，也更加可靠一些。这一步你也需要让调试器附加符号和源码（如果有的话）。

第二步是确定断点的位置，在这之前，你可能已经根据VS调试或者对测试步骤的复现确定了大概区域，你需要在问题发生之前打上断点，然后执行软件，等待断点的命中。（你可以使用调试器在dll中查找你需要的函数信息，在其对应的地址上附加断点）

第三步是分步调试，等待问题的出现，在此期间可以查看寄存器、内存等信息，辅助问题的判断。

第四步在不出意外的情况下，你应该已经大致定位了问题，通常内存问题不会太复杂，此时你需要思考的是修改方案。

第二种情况就稍微比较麻烦了，

第一步同样需要添加符号和源码（注意，不要忘记windows的符号）

第二步我们常常以万能的 !analyze -v 入手，它会帮我们分析常见的信息并输出，注意，如果信息太多，你可以使用 .openlog <path> 的形式来进行调试信息的重定位，方便你日后的查看。

第三步就是找到问题线程，这个大概率在上一步已经做好了，但是也有可能软件有自己的dump输出线程，所以需要你使用 ~* 等命令自己查看一次，一般都是在主线程中出现的问题。

第四步是调用回溯堆栈的方法，这些方法通常是 k<args> ，不够的话，可以使用Dqs来辅助获取更多的信息。

第五步是检查问题出现的时候，其余的一些可疑现象，此时你差不多已经定位完成，需要开始复现问题了，那么就需要执行我们上面的那些步骤了。



怎么样，总的来说，对于不太复杂的问题，定位流程还是比较固定的。在实际的使用中，可能会出现各种奇怪的问题，变化永远都是调试的最大难点。

### 栈内存溢出定位：

代码示例

```
#include <iostream>

void overflowTest()
{
	char p[12] = {};
	for (int i = 0; i < 30; i++)
		p[i] = '1';
}

int main()
{
	overflowTest();
	return 0;
}
```

调试示例

```
Opened log file 'C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\222.txt0:000> lm
start             end                 module name
00007ff6`0fb50000 00007ff6`0fb77000   windbg_leak   (deferred)             
00007ffe`e90e0000 00007ffe`e9301000   ucrtbased   (deferred)             
00007ffe`e9310000 00007ffe`e93f3000   MSVCP140D   (deferred)             
00007fff`395e0000 00007fff`3960b000   VCRUNTIME140D   (deferred)             
00007fff`47190000 00007fff`4719f000   VCRUNTIME140_1D   (deferred)             
00007fff`4bd50000 00007fff`4c046000   KERNELBASE   (deferred)             
00007fff`4e0b0000 00007fff`4e16f000   KERNEL32   (deferred)             
00007fff`4e390000 00007fff`4e588000   ntdll      (pdb symbols)          c:\localsymbaols\ntdll.pdb\58AEFA7324FBDABAC35C72BBE3794AE51\ntdll.pdb
0:000> x windbg_leak!*overflow*
00007ff6`0fb61e90 windbg_leak!overflowTest (void)
0:000> bp 00007ff6`0fb61e90 windbg_leak!overflowTest
                                                   ^ Range error in 'bp 00007ff6`0fb61e90 windbg_leak!overflowTest'
0:000> bl
     0 d Enable Clear  00007ff6`0fb61e90     0001 (0001)  0:**** windbg_leak!overflowTest
0:000> be 0
0:000> bl
     0 e Disable Clear  00007ff6`0fb61e90     0001 (0001)  0:**** windbg_leak!overflowTest
0:000> g
Breakpoint 0 hit
windbg_leak!overflowTest:
00007ff6`0fb61e90 4055            push    rbp
0:000> pc
windbg_leak!overflowTest+0x7a:
00007ff6`0fb61f0a e87ff4ffff      call    windbg_leak!ILT+905(_RTC_CheckStackVars) (00007ff6`0fb6138e)
0:000> 
WARNING: This break is not a step/trace completion.
The last command has been cleared to prevent
accidental continuation of this unrelated event.
Check the event, location and thread before resuming.
(3a48.137c): Break instruction exception - code 80000003 (first chance)
windbg_leak!failwithmessage+0x234:
00007ff6`0fb63974 cc              int     3
0:000> r
rax=0000000000000001 rbx=00007ff60fb61f0f rcx=6d1b09de95b60000
rdx=0000000000000710 rsi=00000028af2fedc0 rdi=00007ff60fb61091
rip=00007ff60fb63974 rsp=00000028af2fe330 rbp=0000000000000002
 r8=00007fff4e3b6d0f  r9=00000173905e0000 r10=0000000050000163
r11=00000173905ecdc0 r12=0000000000000001 r13=0000000000000000
r14=0000000000000000 r15=00000028af2ff240
iopl=0         nv up ei pl nz na pe nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00000202
windbg_leak!failwithmessage+0x234:
00007ff6`0fb63974 cc              int     3
0:000> dd rbp
00000000`00000002  ???????? ???????? ???????? ????????
00000000`00000012  ???????? ???????? ???????? ????????
00000000`00000022  ???????? ???????? ???????? ????????
00000000`00000032  ???????? ???????? ???????? ????????
00000000`00000042  ???????? ???????? ???????? ????????
00000000`00000052  ???????? ???????? ???????? ????????
00000000`00000062  ???????? ???????? ???????? ????????
00000000`00000072  ???????? ???????? ???????? ????????
0:000> k
 # Child-SP          RetAddr               Call Site
00 00000028`af2fe330 00007ff6`0fb635da     windbg_leak!failwithmessage+0x234 [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\rtc\error.cpp @ 213] 
01 00000028`af2ff220 00007ff6`0fb628c2     windbg_leak!_RTC_StackFailure+0xaa [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\rtc\error.cpp @ 263] 
02 00000028`af2ff660 00007ff6`0fb61f0f     windbg_leak!_RTC_CheckStackVars+0x62 [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\rtc\stack.cpp @ 69] 
03 00000028`af2ff690 00007ff6`0fb620c0     windbg_leak!overflowTest+0x7f [C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\main.c @ 11] 
04 00000028`af2ff7e0 00007ff6`0fb631b9     windbg_leak!main+0x20 [C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\main.c @ 16] 
05 00000028`af2ff8e0 00007ff6`0fb6305e     windbg_leak!invoke_main+0x39 [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 79] 
06 00000028`af2ff930 00007ff6`0fb62f1e     windbg_leak!__scrt_common_main_seh+0x12e [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288] 
07 00000028`af2ff9a0 00007ff6`0fb6324e     windbg_leak!__scrt_common_main+0xe [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 331] 
08 00000028`af2ff9d0 00007fff`4e0c7614     windbg_leak!mainCRTStartup+0xe [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp @ 17] 
09 00000028`af2ffa00 00007fff`4e3e26b1     KERNEL32!BaseThreadInitThunk+0x14
0a 00000028`af2ffa30 00000000`00000000     ntdll!RtlUserThreadStart+0x21
0:000> lsa windbg_leak!overflowTest+0x7a
     7: 	HANDLE hHeap;
     8: 	hHeap = HeapCreate(0, 1000, 0);
     9: 	p = (char*)HeapAlloc(hHeap, 0, 9);
    10: 	for (int i = 0; i < 20; i++)
>   11: 		*(p + i) = '1';
    12: 
    13: 	if (!HeapFree(hHeap, 0, p))
    14: 		std::cout << "Free " << p << " from " << hHeap << " failed" << std::endl;
    15: 
    16: 	if (!HeapDestroy(hHeap))
0:000> .logclose
Closing open log file C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\222.tx
```



### 堆内存溢出定位：

代码示例

```
#include <iostream>
#include <windows.h>

int main()
{
	char* p;
	HANDLE hHeap;
	hHeap = HeapCreate(0, 1000, 0);
	p = (char*)HeapAlloc(hHeap, 0, 9);
	for (int i = 0; i < 20; i++)
		* (p + i) = '1';

	if (!HeapFree(hHeap, 0, p))
		std::cout << "Free " << p << " from " << hHeap << " failed" << std::endl;

	if (!HeapDestroy(hHeap))
		std::cout << "Destroy heap " << hHeap << " failed " << std::endl;

	return 0;
}
```

调试示例

```
************* Path validation summary **************
Response                         Time (ms)     Location
OK                                             C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\x64\Deb
Deferred                                       SRV*c:\localsymbaols*http://msdl.microsoft.com/download/symbols
OK                                             C:\localsymbaols
Symbol search path is: C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\x64\Debug;SRV*c:\localsymbaols*http://msdl.microsoft.com/download/symbols;C:\localsymbaolExecutable search path is: 
ModLoad: 00007ff7`8b8f0000 00007ff7`8b917000   windbg_leak.exe
ModLoad: 00007fff`4e390000 00007fff`4e588000   ntdll.dll
ModLoad: 00007fff`06ef0000 00007fff`06f64000   C:\WINDOWS\System32\verifier.dll
Page heap: pid 0x6FD4: page heap enabled with flags 0x3.
ModLoad: 00007fff`4e0b0000 00007fff`4e16f000   C:\WINDOWS\System32\KERNEL32.DLL
ModLoad: 00007fff`4bd50000 00007fff`4c046000   C:\WINDOWS\System32\KERNELBASE.dll
ModLoad: 00007fff`47190000 00007fff`4719f000   C:\WINDOWS\SYSTEM32\VCRUNTIME140_1D.dll
ModLoad: 00007fff`04d00000 00007fff`04de3000   C:\WINDOWS\SYSTEM32\MSVCP140D.dll
ModLoad: 00007ffe`e91d0000 00007ffe`e93f1000   C:\WINDOWS\SYSTEM32\ucrtbased.dll
ModLoad: 00007fff`395e0000 00007fff`3960b000   C:\WINDOWS\SYSTEM32\VCRUNTIME140D.dll
(6fd4.c30): Break instruction exception - code 80000003 (first chance)
ntdll!LdrpDoDebuggerBreak+0x30:
00007fff`4e460910 cc              int     3
0:000> !gflag
Current NtGlobalFlag contents: 0x02000000
    hpa - Place heap allocations at ends of pages
0:000> lm
start             end                 module name
00007ff7`8b8f0000 00007ff7`8b917000   windbg_leak   (deferred)             
00007ffe`e91d0000 00007ffe`e93f1000   ucrtbased   (deferred)             
00007fff`04d00000 00007fff`04de3000   MSVCP140D   (deferred)             
00007fff`06ef0000 00007fff`06f64000   verifier   (deferred)             
00007fff`395e0000 00007fff`3960b000   VCRUNTIME140D   (deferred)             
00007fff`47190000 00007fff`4719f000   VCRUNTIME140_1D   (deferred)             
00007fff`4bd50000 00007fff`4c046000   KERNELBASE   (deferred)             
00007fff`4e0b0000 00007fff`4e16f000   KERNEL32   (deferred)             
00007fff`4e390000 00007fff`4e588000   ntdll      (pdb symbols)          c:\localsymbaols\ntdll.pdb\58AEFA7324FBDABAC35C72BBE3794AE51\ntdll.pdb
0:000> !heap
        Heap Address      NT/Segment Heap

         2b0a3bb0000              NT Heap
         2b0a3ae0000              NT Heap
         2b0a1190000              NT Heap
0:000> g
(6fd4.c30): Access violation - code c0000005 (first chance)
First chance exceptions are reported before any exception handling.
This exception may be expected and handled.
*** WARNING: Unable to verify checksum for windbg_leak.exe
windbg_leak!main+0x64:
00007ff7`8b902434 c6040131        mov     byte ptr [rcx+rax],31h ds:000002b0`a4d1a000=??
0:000> !heap
        Heap Address      NT/Segment Heap

         2b0a3bb0000              NT Heap
         2b0a3ae0000              NT Heap
         2b0a1190000              NT Heap
         2b0a3b90000              NT Heap
0:000> !heap -a 2b0a3b90000 
HEAPEXT: Unable to get address of ntdll!RtlpHeapInvalidBadAddress.
Index   Address  Name      Debugging options enabled
  4:   2b0a3b90000 
    Segment at 000002b0a3b90000 to 000002b0a3b9f000 (00002000 bytes committed)
    Flags:                00001002
    ForceFlags:           00000000
    Granularity:          16 bytes
    Segment Reserve:      00100000
    Segment Commit:       00002000
    DeCommit Block Thres: 00000100
    DeCommit Total Thres: 00001000
    Total Free Size:      00000177
    Max. Allocation Size: 00007ffffffdefff
    Lock Variable at:     000002b0a3b902c0
    Next TagIndex:        0000
    Maximum TagIndex:     0000
    Tag Entries:          00000000
    PsuedoTag Entries:    00000000
    Virtual Alloc List:   2b0a3b90110
    Uncommitted ranges:   2b0a3b900f0
            2b0a3b92000: 0000d000  (53248 bytes)
    FreeList[ 00 ] at 000002b0a3b90150: 000002b0a3b90860 . 000002b0a3b90860  
        000002b0a3b90850: 00110 . 01770 [100] - free

    Segment00 at a3b90000:
        Flags:           00000000
        Base:            2b0a3b90000
        First Entry:     a3b90740
        Last Entry:      2b0a3b9f000
        Total Pages:     0000000f
        Total UnCommit:  0000000d
        Largest UnCommit:00000000
        UnCommitted Ranges: (1)

    Heap entries for Segment00 in Heap 000002b0a3b90000
                 address: psize . size  flags   state (requested size)
        000002b0a3b90000: 00000 . 00740 [101] - busy (73f)
        000002b0a3b90740: 00740 . 00110 [101] - busy (10f) Internal 
        000002b0a3b90850: 00110 . 01770 [100]
        000002b0a3b91fc0: 01770 . 00040 [111] - busy (3d)
        000002b0a3b92000:      0000d000      - uncommitted bytes.
0:000> dv
              i = 0n16
          hHeap = 0x000002b0`a4cb0000
              p = 0x000002b0`a4d19ff0 "1111111111111111"
0:000> dd p
000000f4`e573f638  a4d19ff0 000002b0 d29ec82d 0000b7bb
000000f4`e573f648  00000002 00000000 00000000 00000000
000000f4`e573f658  a4cb0000 000002b0 00000000 000000f4
000000f4`e573f668  00000000 00000000 a2abef00 00000010
000000f4`e573f678  0000004c 00000000 00000000 000002b0
000000f4`e573f688  e92003c8 00007ffe e573f6b0 000000f4
000000f4`e573f698  e9262df9 00007ffe e93dae50 00007ffe
000000f4`e573f6a8  a2d77fb0 000002b0 00000000 000002b0
0:000> dd 0x000002b0`a4d19ff0
000002b0`a4d19ff0  31313131 31313131 31313131 31313131
000002b0`a4d1a000  ???????? ???????? ???????? ????????
000002b0`a4d1a010  ???????? ???????? ???????? ????????
000002b0`a4d1a020  ???????? ???????? ???????? ????????
000002b0`a4d1a030  ???????? ???????? ???????? ????????
000002b0`a4d1a040  ???????? ???????? ???????? ????????
000002b0`a4d1a050  ???????? ???????? ???????? ????????
000002b0`a4d1a060  ???????? ???????? ???????? ????????
0:000> dd 0x000002b0`a4d19ff0-0x9
000002b0`a4d19fe7  00000000 babbbb00 313131dc 31313131
000002b0`a4d19ff7  31313131 31313131 ???????? ????????
000002b0`a4d1a007  ???????? ???????? ???????? ????????
000002b0`a4d1a017  ???????? ???????? ???????? ????????
000002b0`a4d1a027  ???????? ???????? ???????? ????????
000002b0`a4d1a037  ???????? ???????? ???????? ????????
000002b0`a4d1a047  ???????? ???????? ???????? ????????
000002b0`a4d1a057  ???????? ???????? ???????? ????????
0:000> !address a4d1a007

Usage:                  Free
Base Address:           00000000`7ffee000
End Address:            000000f4`e5640000
Region Size:            000000f4`65652000 ( 977.584 GB)
State:                  00010000          MEM_FREE
Protect:                00000001          PAGE_NOACCESS
Type:                   <info not present at the target>


Content source: 0 (invalid), length: f440925ff9
0:000> g
(6fd4.c30): Access violation - code c0000005 (!!! second chance !!!)
windbg_leak!main+0x64:
00007ff7`8b902434 c6040131        mov     byte ptr [rcx+rax],31h ds:000002b0`a4d1a000=??
0:000> k
 # Child-SP          RetAddr               Call Site
00 000000f4`e573f610 00007ff7`8b9031b9     windbg_leak!main+0x64 [C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\main.c @ 11] 
01 000000f4`e573f770 00007ff7`8b90305e     windbg_leak!invoke_main+0x39 [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 79] 
02 000000f4`e573f7c0 00007ff7`8b902f1e     windbg_leak!__scrt_common_main_seh+0x12e [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288] 
03 000000f4`e573f830 00007ff7`8b90324e     windbg_leak!__scrt_common_main+0xe [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 331] 
04 000000f4`e573f860 00007fff`4e0c7614     windbg_leak!mainCRTStartup+0xe [d:\a01\_work\12\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp @ 17] 
05 000000f4`e573f890 00007fff`4e3e26b1     KERNEL32!BaseThreadInitThunk+0x14
06 000000f4`e573f8c0 00000000`00000000     ntdll!RtlUserThreadStart+0x21
0:000> lsa windbg_leak!main+0x64
     7: 	HANDLE hHeap;
     8: 	hHeap = HeapCreate(0, 1000, 0);
     9: 	p = (char*)HeapAlloc(hHeap, 0, 9);
    10: 	for (int i = 0; i < 20; i++)
>   11: 		*(p + i) = '1';
    12: 
    13: 	if (!HeapFree(hHeap, 0, p))
    14: 		std::cout << "Free " << p << " from " << hHeap << " failed" << std::endl;
    15: 
    16: 	if (!HeapDestroy(hHeap))
0:000> .logclose
Closing open log file C:\Users\zhaolingxi\Desktop\临时\leak\windbg_leak\111.tx
```

