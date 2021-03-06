# SSDB - A fast NoSQL database for storing big list of data

[![Author](https://img.shields.io/badge/author-@ideawu-blue.svg?style=flat)](http://www.ideawu.net/) [![Platform](https://img.shields.io/badge/platform-Linux,%20BSD,%20OS%20X,%20Windows-green.svg?style=flat)](https://github.com/ideawu/ssdb) [![NoSQL](https://img.shields.io/badge/db-NoSQL-pink.svg?tyle=flat)](https://github.com/ideawu/ssdb) [![License](https://img.shields.io/badge/license-New%20BSD-yellow.svg?style=flat)](LICENSE)


## 目的

```
为了学习key-value这类系统的实现细节，就拿SSDB作为学习对象，至少SSDB得代码组织是相对友好的&非常易读，使用的是libuv作为网络框架,其他均使用SSDB原生结构&命名方式，目前
只实现了SET/GET/DEL操作&仅支持redis协议

```

## Master-Slave

```

SSDB采用的是Master-Slave模式，Slave会存储一个last_seq序号来确定下次同步master的数据区间, Master上会存储了binlog日志,每个binlog会有具体seq和command操作,同步的时候根据这个从原始存储结构中
读取具体的数据,然后发送给Salve，Slave依据发送的命令redo就行并且last_seq的值，Slave不会去向Master汇报这次操作的成功与否的状态，因为SSDB的Slave遇到redo不成功的时候就停止操作了，这样就保证了redo
的原子性，所以SSDB这点上很黄很暴力。

```

## TODO
1. 补充一下自己对SSDB内部存储结构的实现原理包括日志的记录 
2. 弄懂ssdb的slave实现原理&分布式存储
3. 准备使用zeromq或者nanomsg来实现分布式通讯