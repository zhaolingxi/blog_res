Linux，这玩意儿很早就认识了，但从来都谈不上熟悉。

不为啥，是真的不好用。

用一张图表达：
![linuxbirthdaycakejoke.jpg](/upload/2022/05/linux-birthday-cake-joke-458ce4b76737446ba928d745860e0eba.jpg)

以至于完全没有拿他当做主系统的想法。

但是，由于各大企业对于安全、开源与免费的不舍追求，特此在这里记录一些小tips ，希望不要反复的踩进同一个坑。

此文档会不断地更新。

## 安装一个软件的推荐流程：

```
//首先检查有没有还很久以前手贱安装过
ls -al /usr/lib | grep XXX

//我们再检查一次
 rpm -qal |grep XXX
 
 //确定了啊，没装过
 //推荐使用apt-get 或者 yum安装！！！
 //推荐使用apt-get 或者 yum安装！！！
 //推荐使用apt-get 或者 yum安装！！！
 yum install XXX
 
//实在不信邪的老铁也可以使用sftp加压缩包或者wget压缩包的方式
//没有安装过，下载最新版本【版本改为自己的】，解压【注意选一个自己顺手的目录】
wget https://github.com/XXX.tar.gz
tar -zxvf XXX.tar.gz
cd XXX/
 //切换到解压后的 libevent 主目录，准备好root权限
./configure –prefix=/usr （或 ./configure --program-prefix=/usr）  
make  
make install 

//安装完后记得检查一下装到哪里去了
//特别注意一下动态库so的方位
//不行你就再查一查
 rpm -qal |grep XXX
 //可能会安装到 /usr/lib  usr/local/lib  usr/lib64  usr/local/lib64等等位置
 //不对你就赶紧的软连接
ln -s /usr/local/lib/XXX  /usr/lib/XXX 


```

## 编译一个文件的注意事项

```
1.头文件默认读取的是/usr/include 其他的该写路径就写路径，不然编不过

2.编译如果报出找不到动态库的，用
ldd XXX可以查看哪一些动态库依赖找不到

3.使用gcc g++这些东西的时候，记得加依赖，什么 -lpthread -ljson -levent这些都是常客，用啥依赖啥要清楚

4.
```

