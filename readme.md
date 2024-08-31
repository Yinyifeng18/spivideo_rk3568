# 一、Linux编译

### 1、osip2编译

[osip2库下载地址](https://ftp.gnu.org/gnu/osip/libosip2-3.6.0.tar.gz)

```
tar -zxvf libosip2-3.6.0.tar.gz

cd libosip2-3.6.0

./configure 

make && sudo make install
```



### 2、eXosip2编译

[eXosip2库下载地址](http://download.savannah.nongnu.org/releases/exosip/libeXosip2-3.5.0.tar.gz)

```
tar -zxvf libeXosip2-3.6.0.tar.gz
cd libeXosip2-3.6.0
mkdir output

./configure --disable-openssl

make && sudo make install

```





# 二、测试

### 1、demo测试

 编译代码uac.cpp和 uas.cpp

```
gcc uac.cpp -o uac -losip2 -leXosip2 -losipparser2 -lpthread
gcc uas.cpp -o uas -losip2 -leXosip2 -losipparser2 -lpthread	
```

 测试运行uas 和 uac，最好打开两个shell来运行

```
./uas
./uac
```

​    注意：如果在运行过程中提示找不到libosip2.so.6等类似的提示，说明osip动态库的路径可能还没有包含进去，可以使用下面的命令手动包含动态库的路径

```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```





# 三、编译spiclient

**编译**

```
clear

gcc sipclient.c audio_rtp_recv.c audio_rtp_send.c g711.c g711codec.c video_rtp_pack.c video_rtp_recv.c video_rtp_send.c -o sipclient  -losip2 -leXosip2 -losipparser2 -lpthread

```

### 1、安装软件

1、先安装SIP服务器，win10下安装miniSIPServer V26 

2、安装MicroSIP电话机

**服务器软件设置：**

1、服务器IP地址

![image-20240831160413198](G:\hjb\参考资料\images\image-20240831160413198.png)

2、添加分机

![image-20240831160557972](G:\hjb\参考资料\images\image-20240831160557972.png)



**MicroSIP电话机帐号添加：**

![image-20240831160808974](G:\hjb\参考资料\images\image-20240831160808974.png)



### 2、测试

![image-20240831161010640](G:\hjb\参考资料\images\image-20240831161010640.png)





# 四、交叉编译

### 1、osip2编译

[osip2库下载地址](https://ftp.gnu.org/gnu/osip/libosip2-3.6.0.tar.gz)

```
#记得export一下交叉编译工具链
export PATH=t/atk-dlrk356x-toolchain/usr/bin:$PATH
```

```
tar -zxvf libosip2-3.6.0.tar.gz
cd libosip2-3.6.0
mkdir output
使用自己需要的交叉编译工具链
./configure --host=arm-linux --target=arm CC=/opt/atk-dlrk356x-toolchain/usr/bin/aarch64-buildroot-linux-gnu-gcc CXX=/opt/atk-dlrk356x-toolchain/usr/bin/aarch64-buildroot-linux-gnu-g++ --prefix=/home/alientek/Download/libosip2-3.6.0/output/ --enable-static
make && make install
```

### 2、eXosip2编译

[eXosip2库下载地址](http://download.savannah.nongnu.org/releases/exosip/libeXosip2-3.5.0.tar.gz)

```
tar -zxvf libeXosip2-3.6.0.tar.gz
cd libeXosip2-3.6.0
mkdir output
使用自己需要的交叉编译工具链
./configure --host=arm-linux --target=arm CC=/opt/atk-dlrk356x-toolchain/usr/bin/aarch64-buildroot-linux-gnu-gcc CXX=/opt/atk-dlrk356x-toolchain/usr/bin/aarch64-buildroot-linux-gnu-g++ --prefix=/home/alientek/Download/libeXosip2-3.6.0/output/ CPPFLAGS='-I/home/alientek/Download/libosip2-3.6.0/output/include/' LDFLAGS='-L/home/alientek/Download/libosip2-3.6.0/output/lib/' --enable-static  --disable-openssl

make && make install
```




























