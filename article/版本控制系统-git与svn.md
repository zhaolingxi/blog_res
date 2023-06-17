# 主流版本控制系统（SVN|GIT）
文章图片来源于网络和《pro git》

## git

### git的历史
自从软件工程成为一种多人协作的工作以来，对于团队协作、版本管理的需求就自然而然的产生了。

在2002年以前，世界各地的贡献者把源代码文件通过diff的方式发给Linus，然后由Linus本人通过手工方式合并代码，即使Linus天赋异禀，这样干还是有些不干人事。
这将会导致贡献者的提交反馈被延迟，维护者更是麻烦，linux社区和Lunus意识到使用VCS已经迫在眉睫了。

Linus 想要一款开源的，分布式的版本控制系统。在当时的软件中，他选择了BitKeeper，它虽然不是开源的，但是给予了linux社区免费的使用许可。

但是在2005年，社区中有个叫Andrew Tridgell 的家伙逆向了BitKeeper，写了一个叫SourcePuller的破解版。

BitKeeper的拥有者随即收回了Linux社区免费使用BitKeeper的许可证，此时，linus开始准备自己开发一款VCS

2005年4月3日开始开发
2005年4月6日对外公布他正在开发Git
2005年4月7日已经实现了使用Git管理Git自己的源码了
2005年4月18日已经实现了多个分支合并功能了
2005年4月29日将性能提升到了他最初的目标。别忘了，市面上开发了很多年的CVS都无法达到这个性能目标
2005年6月16日Linux 内核2.6.12已经用上了Git

### git的特点

#### 1.使用快照管理版本信息
与某些版本控制系统使用每个文件的版本号区分版本信息不同（说的就是你，svn），git使用了快照的形式管理不同版本之间的差异。

同一个的文件在不同的版本里面可能内容不同，Git 并不保存这些前后变化的差异数据。实际上，Git 更像是把变化的文件作快照后，
记录在一个微型的文件系统中。

每次提交更新时，它会纵览一遍所有文件的指纹信息并对文件作一快照，然后保存一个指向这次快照的索引。为提高性能，若文件没有变化，Git 不会再次保存，而只对上次保存的快照作一连接。

这样的好处就是，在文件库体量巨大的时候，我们更新一次代码，git只会下载和更新改动的部分，其余部分甚至无需花费时间检查。

#### 2.强调本地的功能完整
git的最明显特点即为分散式或是分布式版本控制。只要求他在本地就必须完成整个版本控制流程，而不需要时时刻刻链接远端服务器。所以，本地的修改、保存、提交、合并、历史功能均无需
链接服务器。

这样做的好处也十分明显，我们并不会时时刻刻都在线，在移动办公、出差的情况下也可以先在本地对分支进行开发与维护。

#### 3.使用hash值进行身份验证
有没有什么办法分辨文件A与文件B呢？

用名字？那如果同名怎么算？

用版本号？不同的主机上可能存在相同的版本号。

uuid？那玩意儿携带不了任何文件的信息。

文件本质是承载信息的载体，那信息本身就是文件的“指纹”，我们需要把文件信息提取为一行编码就可以抓住本质，解决上述问题。

在git中，我们使用了SHA-1算法，将文件的信息计算为一行hash值。我们可以通过这枚“指纹”找到文件本身。

当文件发生更改时，指纹也会随之更改，方便我们记录版本。

#### 4.git文件的状态
这其实不应该算是git的特点，但是它对于git的理解和使用十分的重要，所以我们也把它放到特点里面。

对于任何一个文件，在 Git 内都只有三种状态：已提交（committed），已修改（modified）和已暂存（staged）。

注意，git目录下的文件还有track与untrack的区别，这里仅仅讨论被git管理的文件

他们可以分别对应命令（commit）(--) (add)

一旦我们将文件添加到git的管理中，管理数据将在Git 的本地数据目录（track but not update），工作目录(commit)以及暂存区域(add)中流动

### git的底层数据结构
git的开发者说它是一个文件管理系统，市场则认为他是一个版本控制系统。但是它归根结底就是一个数据表，使用简单的 key-value 数据存储。

它允许插入任意类型的内容，并会返回一个键值，通过该键值可以在任何时候再取出该内容。可以通过底层命令 hash-object 来示范这点。

在此基础中，为了实现版本管理所需的功能，git还额外实现了一些数据结构
#### 数据结构 blob对象

#### commit (提交) 对象
在 Git 中提交时，会保存一个提交（commit）对象，它包含一个指向暂存内容快照的指针，作者和相关附
属信息，以及一定数量（也可能没有）指向该提交对象直接祖先的指针：第一次提交是没有直接祖先的，普通
提交有一个祖先，由两个或多个分支合并产生的提交则有多个祖先。
同时提交对象，除了包含相关提交信息以外，还包含着指向这个树对象（项目根目录）的指针，如此它就可以在将来需要的时候，重现此次快照的内容
了。

#### 管理结构 tree (树) 对象
tree对象类似windows中的文件夹或者stl中的容器，他可以存放其规定的元素或者其他容器

在git中所有内容以 tree 或 blob 对象存储，其中 tree 对象对应于 UNIX 中的
目录，blob 对象则大致对应于 inodes 或文件内容。一个单独的 tree 对象包含一条或多条 tree 记录，每
一条记录含有一个指向 blob 或子 tree 对象的 SHA-1 指针，并附有该对象的权限模式 (mode)、类型和文件
名信息。

当使用 git commit 新建一个提交对象前，Git 会先计算每一个子目录（本例中就是项目根目录）的校验和，
然后在 Git 仓库中将这些目录保存为树（tree）对象。

#### log结构


### git的常用命令

#### init
init的工作就是在空目录下新建一个git的管理系统，使用init会生成一个.git目录，里面的object表示文件对象bolb
```
$ mkdir runoob
$ cd runoob/
$ git init
Initialized empty Git repository in /Users/tianqixin/www/runoob/.git/
# 初始化空 Git 仓库完毕。
```

#### clone

克隆一个远端的仓库，使用clone的时候，需要输入远端仓库的链接地址，改地址可以是http协议的网址或者使用git智能匹配的链接（支持SSH等协议）
```
$ git clone https://github.com/tianqixin/runoob-git-test
Cloning into 'runoob-git-test'...
remote: Enumerating objects: 12, done.
remote: Total 12 (delta 0), reused 0 (delta 0), pack-reused 12
Unpacking objects: 100% (12/12), done.
```

#### checkout
checkout作为一个创建和下载的命令，常用于创建一个新分支中，它将完成branch\switch\pull\等一系列组合操作。
```
git checkout [-q] [-f] [-m] [<branch>]
git checkout [-q] [-f] [-m] --detach [<branch>]
git checkout [-q] [-f] [-m] [--detach] <commit>
git checkout [-q] [-f] [-m] [[-b|-B|--orphan] <new_branch>] [<start_point>]
git checkout [-f|--ours|--theirs|-m|--conflict=<style>] [<tree-ish>] [--] <pathspec>…​
git checkout [-f|--ours|--theirs|-m|--conflict=<style>] [<tree-ish>] --pathspec-from-file=<file> [--pathspec-file-nul]
git checkout (-p|--patch) [<tree-ish>] [--] [<pathspec>…​]
```


#### pull
用于拉取远端仓库的代码和资源，但是pull会自己做一次merge，如果你有还没有提交的代码，记得先commit
除此之外，我们还是建议使用fench+rebase的方式拉取，或者使用pull --rebase实现
```
git checkout [-q] [-f] [-m] [<branch>]
git checkout [-q] [-f] [-m] --detach [<branch>]
git checkout [-q] [-f] [-m] [--detach] <commit>
git checkout [-q] [-f] [-m] [[-b|-B|--orphan] <new_branch>] [<start_point>]
git checkout [-f|--ours|--theirs|-m|--conflict=<style>] [<tree-ish>] [--] <pathspec>…​
git checkout [-f|--ours|--theirs|-m|--conflict=<style>] [<tree-ish>] --pathspec-from-file=<file> [--pathspec-file-nul]
git checkout (-p|--patch) [<tree-ish>] [--] [<pathspec>…​]
```

#### push
push是将本地分支的代码推送到远端。需要注意的是，一般工作环境下，我们会创建一个新的远端分支，这个分支一般与当前的本地分支同名。
```
git push [--all | --mirror | --tags] [--follow-tags] [--atomic] [-n | --dry-run] [--receive-pack=<git-receive-pack>]
           [--repo=<repository>] [-f | --force] [-d | --delete] [--prune] [-v | --verbose]
           [-u | --set-upstream] [-o <string> | --push-option=<string>]
           [--[no-]signed|--signed=(true|false|if-asked)]
           [--force-with-lease[=<refname>[:<expect>]] [--force-if-includes]]
           [--no-verify] [<repository> [<refspec>…​]]
```

#### diff\difftool
git本身的diff功能是用来区分不同版本文件内容的。通常使用箭头和版本内容相加表示，在处理大量的区别时不够直观和清晰。
我们推荐使用文件比对工具进行辅助识别，这里推荐使用Beyondcompare4
具体配置工具的方法我们在git常用工具中讲述。
```
git diff [<options>] [<commit>] [--] [<path>…​]
git diff [<options>] --cached [--merge-base] [<commit>] [--] [<path>…​]
git diff [<options>] [--merge-base] <commit> [<commit>…​] <commit> [--] [<path>…​]
git diff [<options>] <commit>…​<commit> [--] [<path>…​]
git diff [<options>] <blob> <blob>
git diff [<options>] --no-index [--] <path> <path>
```

#### merge\mergetool
merge工具能辅助我们对冲突进行处理，特别实在多人协作的项目中，极有可能会出现同一个文件不同人修改的情况
所以我们需要在无损他人成果的情况下修改代码，这个时候，需要文件文本比对和手动逻辑合并，至于为什么要使用工具，与上文diff命令一样
工具方面依然推荐BC4具体配置工具的方法我们在git常用工具中讲述。
```
git merge [-n] [--stat] [--no-commit] [--squash] [--[no-]edit]
        [--no-verify] [-s <strategy>] [-X <strategy-option>] [-S[<keyid>]]
        [--[no-]allow-unrelated-histories]
        [--[no-]rerere-autoupdate] [-m <msg>] [-F <file>] [<commit>…​]
git merge (--continue | --abort | --quit)
```

#### add
add命令可以将我们本地的文件纳入git的管理体系，git将通过hash运算计算出对应文件的key值存入object文件夹中
同时，add也将本地代码纳入暂存区中，是一个相当重要的命令
```
git add [--verbose | -v] [--dry-run | -n] [--force | -f] [--interactive | -i] [--patch | -p]
          [--edit | -e] [--[no-]all | --[no-]ignore-removal | [--update | -u]]
          [--intent-to-add | -N] [--refresh] [--ignore-errors] [--ignore-missing] [--renormalize]
          [--chmod=(+|-)x] [--pathspec-from-file=<file> [--pathspec-file-nul]]
          [--] [<pathspec>…​]
```

#### commit
commit与add一样，都是可以改变当前文件状态的命令，commit将文件从暂存状态更改为已提交（committed）
这表示该文件的修改已经正式生效，我们将可以将修改推送到远端，同时，本次修改将会留下记录，也可以进行恢复等操作
```
git commit [-a | --interactive | --patch] [-s] [-v] [-u<mode>] [--amend]
           [--dry-run] [(-c | -C | --squash) <commit> | --fixup [(amend|reword):]<commit>)]
           [-F <file> | -m <msg>] [--reset-author] [--allow-empty]
           [--allow-empty-message] [--no-verify] [-e] [--author=<author>]
           [--date=<date>] [--cleanup=<mode>] [--[no-]status]
           [-i | -o] [--pathspec-from-file=<file> [--pathspec-file-nul]]
           [(--trailer <token>[(=|:)<value>])…​] [-S[<keyid>]]
           [--] [<pathspec>…​]
```


### git常用工具

#### bash
git bash是git 仿照bash给出的命令行shell,它并不是纯粹的命令行，当然也不是GUI工具，它将命令封装后以一种更加“友好”的方式提供给使用者（虽然我并不认为它很友好）

从某种意义上来说，git bash是最值得学习的git 工具，因为它有完善的git命令与功能，贴近git的设计使用逻辑，并且更加具有普适性。

#### tortures
拥有较为全面的命令和直观的操作逻辑，最重要的是，他和svn tortures简直一模一样，非常适合从SVN切换到git的初期选手
缺点就是软件比较慢,特别是在几十万或者百万代码量级的项目中，速度会带来比较糟糕的体验

#### github-desktop
有点不多说了，与github的操作逻辑相同，界面也差不多，适合社区用户。
缺点就是功能丰富性一般。

#### vs
如果你的项目使用VS开发，那么可以尝试一下VS自带的git管理，本人使用的是VS2019，基本的代码功能都有。
优点自不必说，它省事，直接集成到IDE中，同时兼顾开发与版本控制。
缺点就是功能还是偏少，同时自带的配置文件不好用，偶发更新不及时的问题。

### git的常用工作流程

#### 初始化、新建本地仓库与远端连接

#### 获取远端资源与代码

#### 提交本地修改、处理冲突、推送本地修改

#### 合并远端分支

#### 本地多分支管理

#### 历史版本的查询与恢复

## svn

### svn历史

### svn特点

### svn的底层数据结构

### svn常用工具

#### tortures

### svn的常用工作流程

#### 拉取远端代码

#### 修改代码、解决冲突、推送代码

#### 查看历史记录、回复历史版本

#### 清理无用数据、清理报错后的数据冗余

#### 合并远端分支代码

#### 解决远端合并冲突

## 总结
git就十全十美吗？并不是，至少作为一个在工作中前后使用过这两款版本控制系统的我来说，由于git与svn天生的设计思路不同，导致他们有着风格明显的偏向性。
例如，在大型项目中，我们需要集中一个总的分支进行编译发版，但是很多代码与我们目前动作的模块并没有直接关系，更新、编译都会消耗大量资源，
所以，我们希望灵活的更新其中一部分代码，动态的更新哪些对我们有用的部分，SVN由于集中式的设计思路，从本质上就支持以文件为单位的更新，但是git，不好意思
我们只提供一个稀疏检出，而且配置和使用远远谈不上顺手，在功能性上也是存在缺失的。

但是svn的集中式带来的问题也不少：不同机器版本号不同、连不上服务器就做不了新版本、新分支，一旦中央服务器故障，那就全公司骂娘……
所以，不存在百分之百完美的管理系统，得到一些就会失去另一些。总有人以为新的、流行的就是先进的、好的，其实不然。

适合公司办公场景的，才是合适的版本控制系统，适合自己的，才是正确的选择。
