## 1.软件与服务器

首先，你需要一台服务器，我们使用的实例为阿里云的轻量级服务器，运行在Alibaba Cloud Linux  3.2104 LTS 64位（cent os8）上，如果使用其他的linux系统，比如ubantu，可能在某些命令行上有细微的差别。

为了方便后续的代码书写查询，建议先补全man手册，**yum install -y man-pages**

好了，开始安装libevent

```
//下载openssl
wget https://www.openssl.org/source/openssl-1.1.1l.tar.gz

//解压并且安装openssl
tar -zxvf openssl-1.1.1l.tar.gz
cd openssl-1.1.1l/
./config
make -j16
sudo make install

//检查是否安装libevent
ls -al /usr/lib | grep libevent

//没有安装过，下载最新版本【版本改为自己的】，解压【注意选一个自己顺手的目录】
wget https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
tar -zxvf libevent-2.1.12-stable.tar.gz
cd libevent-2.1.12-stable/

//切换到解压后的 libevent 主目录，准备好root权限
./configure –prefix=/usr （或 ./configure --program-prefix=/usr）  
make  
make install  

//测试安装是否成功
ls -al /usr/lib | grep libevent（或 ls -al /usr/local/lib | grep libevent）
安装成功会显示一张图


//如果你的目录安装在了/usr/local/lib 需要建立一个软连接【版本改为自己的】
ln -s /usr/local/lib/libevent-2.0.so.22  /usr/lib/libevent-2.0.so.22
```

## 2.简单的函数介绍与使用

![结构体](/upload/2022/05/%E7%BB%93%E6%9E%84%E4%BD%93-ceb1f2c4bd894d6fb2b14ad61bdab585.png)

![基础函数](/upload/2022/05/%E5%9F%BA%E7%A1%80%E5%87%BD%E6%95%B0-c4d01a9fecf849258004d19e68dfbc68.png)

基础函数的具体解释见头文件：

使用基础函数实现进程间通信：

GitHub地址（头文件与实例）：https://github.com/zhaolingxi/web_program1

接收端：

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<stdlib.h>
#include<event.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<unistd.h>
void fifo_read(evutil_socket_t fd, short events, void * arg)
{
	char buf[32]={0};
	int ret=read(fd,buf ,sizeof(buf));
	if(-1==ret)
        {
                perror("read");
                exit(1);
        }
	printf("read form pipe %s\n",buf);
}
int main()
{
	int ret=mkfifo("fifo.temp",00700);
	if(-1==ret)
	{
		perror("mkfifo");
		exit(1);
	}

	int fd=open("fifo.temp",O_RDONLY);
	if(-1==fd)
        {
                perror("open");
                exit(1);
        }
	
	struct event ev;
	
	//init
	event_init();
	
	event_set(&ev,fd,EV_READ | EV_PERSIST,fifo_read,NULL);
	
	event_add(&ev,NULL);

	event_dispatch();
	return 0;
}

```

发送端：

```c
#include <sys/stat.h>
#include<stdlib.h>
#include<event.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<unistd.h>
#include<string.h>

int main()
{

	int fd = open("fifo.temp", O_WRONLY);
	if (-1 == fd)
	{
		perror("open");
		exit(1);
	}

	char buf[32] = { 0 };
	while (1)
	{
		scanf("%s", buf);
		int ret = write(fd, buf, strlen(buf));
		if (-1 == ret)
		{
			perror("write");
			exit(1);
		}
		if (!strcmp(buf, "bye"))
		{
			break;
		}
		memset(buf, 0, sizeof(buf));
	}
	return 0;
}

```



## 3.封装函数简介与使用

![封装接口](/upload/2022/05/%E5%B0%81%E8%A3%85%E6%8E%A5%E5%8F%A3-56d47c0f0a5f43f19fce9aae42158762.png)

使用封装函数的实例：

GitHub地址（头文件与实例）：https://github.com/zhaolingxi/web_program1

```c
#include<stdio.h>
#include<stdlib.h>
#include<event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<string.h>
#include<event2/listener.h>

void read_cb(struct bufferevent *bev, void *ctx)
{
	int fd=*(int *)ctx;
	char * buf[128]={0}; 
	size_t ret=bufferevent_read(bev,buf,sizeof(buf));
	if(ret<0)
	{
		printf("read_fail\n");
		exit(1);
	}
	else
	{
		printf("read form: %d %s\n",fd,buf);
	}
}

void event_cb(struct bufferevent *bev, short what, void *ctx)
{
	if(what &BEV_EVENT_EOF)
	{
		int fd1=*(int*)ctx; 
		printf("BEV_EVENT_EOF %d \n",fd1);
		bufferevent_free(bev);
	}
	else   {
                printf("error\n");
        }
}

void listenercb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *arg)
{
	printf("connect with:",fd);
	
	struct event_base* base=arg;

	struct bufferevent * BV=bufferevent_socket_new(base,fd,BEV_OPT_CLOSE_ON_FREE);
	
	if(!BV)
	{
		printf("BV fail\n");
		exit(1);
	}

	bufferevent_setcb(BV, read_cb,NULL,event_cb,&fd);

	bufferevent_enable(BV,EV_READ);
}


int main()
{
	struct event_base* base=event_base_new();
	if(!base)
	{
		printf("constrct fail\n");
		exit(1);
	}
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=8000;
	server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");

	struct evconnlistener *listener= evconnlistener_new_bind(base,listenercb,NULL,LEV_OPT_CLOSE_ON_FREE |LEV_OPT_REUSEABLE,10,
			(struct sockaddr*)&server_addr,sizeof(server_addr));
	if(!listener)
	{
		printf("construct listener fail\n");
		exit(1);
	}

	event_base_dispatch(base);
	
	evconnlistener_free(listener);

	event_base_free(base);

	return 0;
}

```



## 

