# COM机制浅析_内燃机时代的汗血宝马

图片挂载与github，无法查看时请检查网络原因

文章未完成，持续更新中。

## 前言

写这篇文章的原因是我和一位朋友聊天，不知道咋的就说到了微软这家公司的产品，我们都认为这是一家伟大的软件公司，也都认为它的产品过于臃肿。各种技术路线不管有没有过时，都稳稳的粘在微软的产品中，各种特性和屎山让人欲仙欲死。但是从侧面来说历史的变革进程它一个也没有错过。在花里胡哨的技术中，你从能看见他的身影。

从跨平台语言来说，即使是当今JAVA遍地开花的年代，微软家的.NET也是硬生生的从市场里啃下来一块蛋糕。

不过这和我们搞CPP的人有个毛线关系，不，还真能扯得上一点关系，原因就在于windows厚重的历史感。.NET的基石是跨语言，而所有的兼容性问题，最终都会落到二进制兼容的头上，这个时候，JAVA、.NET选手小手一摊，就该让苦逼的C艹选手上场了。

其实在大概十年前，C++有过一阵很风光的时候，当时的兼容性、跨平台的需求已经是大型软件的必备要求了。所以各个厂商、平台也提出了自己的解决方案。微软提出的这一坨，就叫COM机制。

很多程序员都不知道这个是啥，有的工作中不接触windows，有的是压根不关心操作系统。更多的，是找不到合适的教材。是的，COM不仅在企业开发中几乎消失，同时连培训教材都在2000年前后断档了。

不过，这种机制仍然活跃于windows的系统API中，甚至比他更早的OLE机制也在最新的windows中发光发热。它或许不能帮助你找到一份好工作，但对于你实现自己的跨平台方法、理解操作系统的运行逻辑，还是大有好处的。

## 什么是COM

搜索引擎给的回答是：（Component Object Model，组件对象模型）是微软公司于1993年提出的一种组件技术，它是一种平台无关、语言中立、位置透明、支持网络的中间件技术。

下面是chatgpt的答案：

[**COM** 是 **组件对象模型**（Component Object Model）的缩写](https://learn.microsoft.com/zh-cn/windows/win32/com/com-technical-overview)
它是一种二进制互操作性标准，用于创建在运行时交互的可重用软件库。COM 可以让你使用 COM 库，而无需将它们编译到应用程序中。
COM 是许多 Microsoft 产品和技术的基础，例如 Windows 媒体播放器和 Windows Server

COM 定义了一个二进制标准，用于创建在运行时交互的可重用软件库，它可以在不同的操作系统和硬件平台上使用。COM 独立于实现语言，这意味着可以通过使用不同的编程语言（如 C++ 和 .NET Framework 中的编程语言）创建 COM 库。

COM 提供了支持跨平台软件重用的所有基本概念：

- 组件之间函数调用的二进制标准。
- 将函数强类型分组到接口中的预配。
- 提供多态性、特征发现和对象生存期跟踪的基接口。
- 唯一标识组件及其接口的机制。
- 从部署创建组件实例的组件加载程序。

COM 具有许多协同工作的部件，用于创建基于可重用组件构建的应用程序：

- 提供符合 COM 规范的运行时环境的主机系统。
- 定义功能协定的接口，以及实现接口的组件。
- 向系统提供组件的服务器以及使用组件提供的功能的客户端。
- 跟踪组件在本地和远程主机上的部署位置的注册表。
- 一个服务控制管理器，用于定位本地和远程主机上的组件并将服务器连接到客户端。
- 一种结构化存储协议，用于定义如何在主机文件系统上导航文件的内容。

COM 的核心是启用代码跨主机和平台重复使用。可重用接口实现称为**组件**、**组件对象**或**COM 对象**。组件实现一个或多个 COM 接口。通过设计库实现的接口来定义自定义 COM 库。库的使用者可以在不了解库的部署和实现详细信息的情况下发现和使用其功能。

COM 对象通过接口公开其功能，接口是成员函数的集合。COM 接口定义组件的预期行为和职责，并指定提供一小部分相关操作的强类型协定。COM 组件之间的所有通信都通过接口进行，组件提供的所有服务都通过其接口公开。调用方只能访问接口成员函数。内部状态对调用方不可用，除非它在接口中公开。接口是强类型。每个接口都有其自己唯一的接口标识符（名为 IID），这可以消除与人类可读名称可能发生的冲突。IID 是 GUID（全局唯一标识符），它与 Open Software Foundation (OSF) 分布式计算环境 (DCE) 定义的通用唯一 ID (UUID) 相同

其实上述的各种描述都在说明一件事情，COM是一个二进制层级的，实现程序复用的，规范，或者是机制。

## **TLB与IDL**

TLB 文件是 类型库（Type Library）它是一个说明文件，用于描述 DLL 中的 COM 接口和常量等信息。

具体的内容如下图（文件来自微软）：

![tlb](C:\Bolg\image\0924\tlb.png)

IDL 文件是 接口定义语言（Interface Definition Language）的缩写，它是一种描述性语言，用于描述对象实现的接口。IDL 文件通常包含在 COM 组件中，以便其他应用程序可以使用该组件的功能。

具体的内容如下图（文件来自达索）：

![](C:\Bolg\image\0924\idl.png)

我们可以看出，在上述的两种文件中，是有一些信息重复的，但是tlb是以二进制文件发布的，而idl则是文本形式。

## COM重要组成-QI|引用计数



## COM重要组成-CoCreateInstance|注册表

## 解释性语言下的COM调用（IDispatch）

我们知道，想要调用定义在其他PE文件（dll）中的函数，需要链接加载对应的代码段。常见的链接方式分为静态链接（把程序编译进静态库lib中，使用时加载lib）和动态链接（将程序编入dll中，将定义符号的文件编入def或者lib中，使用时动态的链接导入）。其中，动态链接又可以分为显示连接（ LoadLibrary+GetProcAddress的组合）隐式链接（使用lib+头文件，先跳转到lib中，再跳转到同名dll中）。

这些实现方式的前提都是指针，各种各样的指针，函数的、程序的。但是对于很多语言它们没有指针的概念，只有类和对象的概念，不仅如此，对于解释性语言他们也没有编译链接的概念，一切都交给类似JDK这样的平台去处理。

那如果我们想用python或者java这样的解释性语言，实现跨dll的调用，我们可以怎么去实现呢？

COM给出的答案是使用一套规则约束定义和调用过程，同时将调用的核心（寻找函数地址）交给操作系统去头疼。这样，无论什么语言，你需要告诉我你要什么（逻辑清楚）我就可以替你去寻找资源。

我们看看实际的案例（代码来源于网络，如有侵权，请联系我删除）

```
::CoInitialize( NULL );               // COM 初始化
 
CLSID clsid;                   // 通过 ProgID 得到 CLSID
HRESULT hr = ::CLSIDFromProgID( L"Simple8.DispSimple.1", &clsid );
ASSERT( SUCCEEDED( hr ) );     // 如果失败，说明没有注册组件
IDispatch * pDisp = NULL;  
// 由 CLSID 启动组件，并得到 IDispatch 指针
hr = ::CoCreateInstance( clsid, NULL, CLSCTX_ALL, IID_IDispatch, (LPVOID *)&pDisp );
ASSERT( SUCCEEDED( hr ) );     // 如果失败，说明没有初始化 COM
LPOLESTR pwFunName = L"Add";   // 准备取得 Add 函数的序号 DispID
DISPID dispID;                // 取得的序号，准备保存到这里
hr = pDisp->GetIDsOfNames(            // 根据函数名，取得序号的函数
               IID_NULL,
               &pwFunName,          // 函数名称的数组
               1,                   // 函数名称数组中的元素个数
               LOCALE_SYSTEM_DEFAULT,     // 使用系统默认的语言环境
               &dispID );          // 返回值
ASSERT( SUCCEEDED( hr ) );        // 如果失败，说明组件根本就没有 ADD 函数
VARIANTARG v[2];               // 调用 Add(1,2) 函数所需要的参数v[0].vt = VT_I4;    v[0].lVal = 2;    // 第二个参数，整数2v[1].vt = VT_I4;  v[1].lVal = 1; // 第一个参数，整数1
DISPPARAMS dispParams = { v, NULL, 2, 0 };    // 把参数包装在这个结构中
        VARIANT vResult;                      // 函数返回的计算结果
hr = pDisp->Invoke(                   // 调用函数
               dispID,               // 函数由 dispID 指定
               IID_NULL,
               LOCALE_SYSTEM_DEFAULT, // 使用系统默认的语言环境
               DISPATCH_METHOD,           // 调用的是方法，不是属性
               &dispParams,                // 参数
               &vResult,                          // 返回值
               NULL,                       // 不考虑异常处理
               NULL);                      // 不考虑错误处理
ASSERT( SUCCEEDED( hr ) );     // 如果失败，说明参数传递错误
CString str;                   // 显示一下结果
str.Format("1 + 2 = %d", vResult.lVal );
AfxMessageBox( str );
 
pDisp->Release();              // 释放接口指针
::CoUninitialize();            // 释放 COM

```

我们可以看出，所有的查找函数，他都是全局的函数，而数据来源，则是注册表。

当然，在实际的项目中，我们还是使用双接口会比较好，双接口的定义如下：

```
[
    uuid(1e196b20-1f3c-1069-996b-00dd010fe676), 
    oleautomation, dual
]
interface IHello : IDispatch
{
    //Diverse properties and methods defined here.
};
```

## COM有哪些可以改进的点

COM的

## 总结

参考资料：

[https://learn.microsoft.com/en-us/windows/win32/midl/dual](https://learn.microsoft.com/en-us/windows/win32/midl/dual)

部分引用来源于Microsoft copilot
