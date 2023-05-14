一不小心有一个星期没更新了……今天整个短一点的。我们聊聊网络编程。

如何选定网络编程中服务器端所需要的处理多任务多请求的模式是我们常会遇到的问题，常见方式有

直接tcp+多线程；libevent+多线程等，但是每一种方案其实都是对性能与资源的取舍。而且它们都脱离不了原始的网络编程api，即selet、poll与epoll。

下面，我们将简单的介绍其原理与区别

```c++
selet代码
//创建一个socket服务端
//创建文件描述符数组【fds】
//使用select函数在内核态查找哪一个文件被修改【其实是查找bitmap中哪一个位置被修改】
//找到这个文件，处理数据变动
//[缺点]1.bitmap大小有限；2.fds不可重用；
//3.用户态拷贝数据到内核态还是有开销的；4.O(n)时间复杂度的轮询   
///////////////////////////////////////////////////////////////////////////////
//经典的select应用
sockfd=socket(AF_INET,SOCK_STREAM,0);
memset(&addr,0,sizeof(addr));
addr.sin_family=AF_INET;
addr.sin_port=htons(2000);
bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
listen(sockfd,5);
for(i=0;i<5;i++)
{
    memset(&client,0,sizeof(client));
    addrlen=sizeof(client);
    fds[i]=accept(sockfd,(struct sockaddr*)&client,&addrlen);
    if(fds[i]>max)
    {
        max=fds[i];
    }
}

while(1)
{
    FD_ZERO(&rset);
    for(int i=0;i<5;i++)
    {
        FD_SET(fds[i],&rset);
    }
    puts("round again");
//max+1表示在bitmap中那些位数中去找
//rset：1024位的bitmap，记录信息【fds】；
//这个函数是一个阻塞函数，运行在内核态，判断哪一个文件有数据来了，找出之后返回
    select(max+1,&rset,NULL,NULL,NULL);
    for(int i=0;i<5;i++)
    {
        if(FD_ISSET(fds[i],&rset))
        {
            memset(buffer,0,MAXBUF);
            rend(fds[i],buffer,MAXBUF);
            puts(buffer);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////   
```

```c++
poll代码
//缺点和select相比少了bitmap大小固定和fds不可重用两个；
//原因是它使用了结构体，而且并不直接修改fd,而是使用revents代替
///////////////////////////////////////////////////////////////////////////////
//poll应用
struct pollfds
{
    int fd;
    short events;
    short revents;
}
for(i=0;i<5;i++)
{
    memset(&client,0,sizeof(client));
    addrlen=sizeof(client);
    pollfds[i].fd=accept(sockfd,(struct sockaddr*)&client,&addrlen);
    pollfds[i].event=POLLIN;
}

while(1)
{
    puts("round again");
    poll(pollfds,5,50000);

    for(int i=0;i<5;i++)
    {
        if(pollfds[i].revents & POLLIN)
        {
            pollfds[i].revents=0;
            memset(buffer,0,MAXBUF);
            rend(pollfds[i].fd,buffer,MAXBUF);
            puts(buffer);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
```

```c++
epoll代码
//它较好的解决了所有的问题（暂时）
//不需要轮询，时间复杂度为O(1)
//epoll_create  创建一个白板 存放fd_events
//epoll_ctl 用于向内核注册新的描述符或者是改变某个文件描述符的状态。
// 已注册的描述符在内核中会被维护在一棵红黑树上
//epoll_wait 通过回调函数内核会将 I/O 准备好的描述符加入到一个链表中管理，
// 进程调用 epoll_wait() 便可以得到事件完成的描述符
///////////////////////////////////////////////////////////////////////////////
//epoll
struct eventpoll {
　　...
　　/*红黑树的根节点，这棵树中存储着所有添加到epoll中的事件，
　　也就是这个epoll监控的事件*/
　　struct rb_root rbr;
　　/*双向链表rdllist保存着将要通过epoll_wait返回给用户的、满足条件的事件*/
　　struct list_head rdllist;
　　...
};    

struct epoll_event events[5];
int epfd=epoll_create(10);//epoll_create创建eventpoll
//内核帮我们在epoll文件系统里建了个file结点，在内核cache里建了个红黑树用于存储以后epoll_ctl传来的socket，还会再建立一个rdllist双向链表，用于存储准备就绪的事件。
...
...
for(int i=0;i<5;i++)
{
    static struct epoll_event ev;
    memset(&client,0,sizeof(client));
    addrlen=sizeof(client);
    ev.data.fd=accept(sockfd,(struct sockaddr*)&client,&addrlen);
    ev.events=EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,ev.data.fd,&ev);
}

while(1)
{
    puts("round again");
    nfds=epoll_wait(epfd,events,5,10000);
    for(int i=0;i<nfds;i++)
    {
        memset(buffer,0,MAXBUF);
        rend(pollfds[i].fd,buffer,MAXBUF);
        puts(buffer);
    }
}
///////////////////////////////////////////////////////////////////////////////
//同时，他也含有两种触发模式
//LT:水平触发
// 当 epoll_wait() 检测到描述符事件到达时，将此事件通知进程。
//进程可以不立即处理该事件，下次调用 epoll_wait() 会再次通知进程。
//是默认的一种模式，并且同时支持 Blocking 和 No-Blocking。
//ET:边缘触发
// 和 LT 模式不同的是，通知之后进程必须立即处理事件。
// 下次再调用 epoll_wait() 时不会再得到事件到达的通知。很大程度上减少了 epoll 事件被重复触发的次数。
// 因此效率要比 LT 模式高。只支持 No-Blocking，以避免由于一个文件句柄的阻塞读/阻塞写操作把处理多个文件描述符的任务饿死。
```

