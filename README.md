
# 处理思路
> 优先推荐 URL 上传版本，其次使用 JNI 方式调用

## 自定义实现| 尝试未果
- 尝试自行对文件内容分块进行 SHA1 计算，但与企微版本偏差较多且无法排查

## 代码转换| 不推荐
> 将 c++ 代码直接翻译为 Java 版本
- 如 c++ 版本中的 unsigned char* 类型，没办法用 java 的 short/ char 来对应转换
    - 尤其在进行位运算后，值变化非常大，同步无法排查

## 封装调用| 可用
> JNI 形式
- 在提供的 c++ 版本基础上，再次抽象、封装
    - 仅传入地址，并输出 SHA1 值的拼接文本

## URL 上传| 推荐
> 与企微产品同学确认，预期于 `2022.09` 月底上线
- 如果不是一定掐点上线，建议等该接口

# 实现
## 封装定义
> 详见 DiskSHA1Encoder.java | liDiskSHA1Encoder.so
> 需特别注意，JNI 调用时两者全路径要匹配

```sh
javac .\DiskSHA1Encoder.java
# 生成 DiskSHA1Encoder.class 文件

javah -jni DiskSHA1Encoder
# 生成 DiskSHA1Encoder.h 头文件（定义）
# 如设置 package com.yao 则对应文件名和类名定义应为 com_yao_DiskSHA1Encoder.h 和 Java_com_yao_DiskSHA1Encoder_encode 
```

## 接口实现
> 详见 DiskSHA1Encoder.cpp

- 主要实现来源于 `test_file_block_digest.cpp` 文件
    - 需要注意 jstring 和 char* 的不同语言中文本类型的转换

## 功能验证
```sh

# STEP-0：功能验证
# 生成可执行文件（源版本）
# **根据环境可能需要安装 automake、gcc、gcc-c++**
aclocal
automake --add-missing
./configure CXX=g++ CPPFLAGS=-std=c++11
make
./test_file_block_digest test.pptx

# STEP-1：打包
# 需先移除包含 main 函数的 test_file_block_digest.cpp 文件
# **由于涉及 JNI，需额外包含 JDK 路径下的相关文件**
rm test_file_block_digest.cpp
g++ -fpic -shared -o libDiskSHA1Encoder.so  *.cpp  -std=c++11 -I  $JAVA_HOME/include -I  $JAVA_HOME/include/linux

# STEP-3：JNI 调用验证
# **Linux 场景下，需将当前路径配置至环境变量中**
# **生成的 lib{libName=DiskSHA1Encoder}.so 文件需以 lib 开头，而调用时仅需 DiskSHA1Encoder**
java -Djava.library.path=. DiskSHA1Encoder test.pptx
```

# 扩展知识

## java.library.path
> 用于指定非 Java 类包（如 dll、so 等）的位置

### 对照变量
系统    | 变量
--------|------------------
Windows | PATH
Linux   | LD_LIBRARY_PATH
Mac     | JAVA_LIBRARY_PATH

### 配置优先级
- LD_LIBRARY_PATH
    - 设置多目录时，以`:`间隔
- /etc/ld.so.conf
    - /etc/ld.so.conf.d/*.conf
        - /etc/ld.so.conf.d/libc.conf-> /usr/local/lib

## JNA 调用

### 方法实现
- IDiskSHA1Encoder
```java
import com.sun.jna.Library;

public interface IDiskSHA1Encoder extends Library {
    
    // 此处直接调用的 JNI 生成版本，正常应该直接调用 c 版本
    String Java_DiskSHA1Encoder_encode(String fileUrl);
}
```

- DiskJNAUtils
```java
import cn.hutool.core.io.FileUtil;
import cn.hutool.core.io.resource.ClassPathResource;
import cn.hutool.core.io.resource.Resource;
import com.sun.jna.Native;
import org.apache.commons.io.FilenameUtils;

import java.io.File;

public class DiskJNAUtils {
    // 目标目录
    public static final String SO_FILE_PATH = "/tmp/libDiskSHA1Encoder.so";
    public static final String SO_RESOURCE = "libDiskSHA1Encoder.so";
    public static final String DLL_RESOURCE = "libDiskSHA1Encoder";
    
    public static IDiskSHA1Encoder INSTANCE;
    
    static {
        System.setProperty("jna.encoding", "UTF-8");
        String ops = System.getProperty("os.name");
        if (ops.toLowerCase().startsWith("win")) {
            INSTANCE = (IDiskSHA1Encoder) Native.loadLibrary(DLL_RESOURCE, IDiskSHA1Encoder.class);
        } else {
            File path = new File(FilenameUtils.getFullPath(SO_FILE_PATH));
            if (!path.exists()) {
                path.mkdirs();
            }
            File file = new File(SO_FILE_PATH);
            if (!file.exists()) {
                // 初始 libDiskSHA1Encoder.so 存放于项目的 /resources 目录下
                Resource classPathResource = new ClassPathResource(SO_RESOURCE);
                FileUtil.writeFromStream(classPathResource.getStream(), file);
            }
            INSTANCE = (IDiskSHA1Encoder) Native.loadLibrary(file.getAbsolutePath(), IDiskSHA1Encoder.class);
        }
    }
}
```

### 调用验证
> 验证调用时，需指定 Jna 的相关 jar 包
java -cp "/home/admin/code/file_block_digest/lib/jna-3.0.9.jar" DiskJNAUtils test.pptx

## 查看 *.so 的方法列表
- objdump -tT libDiskSHA1Encoder.so
    - JNA 调用时的方法名以此为准

# 踩坑
## libc.so.6: version `GLIBC_2.14' not found"
> 正常是由于不同环境的系统版本不一，推荐升级一致，其次考虑单纯升级 glibc 版本

- libc.so.6: version `GLIBC_2.14' not found
    - 由于Linux系统的glibc版本太低，而软件编译时使用了较高版本的glibc引起的
- rpm -qa |grep glibc
    - 通过上述指令确认其版本号信息

## JNA| undefined symbol
- 正常为方法名称未指定正确
    - 此处尤其不要将其与 JNI 生成的文件混淆
        - 如 javah 生成的定义方法为 Java_DiskSHA1Encoder_encode
            - JNI 中调用为 encode
            - JNA 调用则为 Java_DiskSHA1Encoder_encode
- 可配合 `objdump -tT libDiskSHA1Encoder.so` 指令确认方法列表

## JNA| Failed to write core dump. Core dumps have been disabled. - 验证中断
> Java 进程直接崩溃，使用推荐的 `ulimit -c unlimited` 指令
> 但再次尝试后，仍会触发 fatal error 导致服务异常（怀疑是由于 JNI 方式实现所置）

```log
Java HotSpot(TM) 64-Bit Server VM warning: You have loaded library /home/admin/tomcat/temp/jna308661952366919029.tmp which might have disabled stack guard. The VM will try to fix the stack guard now.
It's highly recommended that you fix the library with 'execstack -c <libfile>', or link it with '-z noexecstack'.
#
# A fatal error has been detected by the Java Runtime Environment:
#
#  SIGSEGV (0xb) at pc=0x00007f5aef7c1471, pid=23830, tid=0x00007f5afcc9a700
#
# JRE version: Java(TM) SE Runtime Environment (8.0_171-b11) (build 1.8.0_171-b11)
# Java VM: Java HotSpot(TM) 64-Bit Server VM (25.171-b11 mixed mode linux-amd64 compressed oops)
# Problematic frame:
# C  [libDiskSHA1Encoder.so+0x5471]  JNIEnv_::FindClass(char const*)+0x17
#
# Failed to write core dump. Core dumps have been disabled. To enable core dumping, try "ulimit -c unlimited" before starting Java again
#
# An error report file with more information is saved as:
# /home/admin/tomcat/hs_err_pid23830.log
#
# If you would like to submit a bug report, please visit:
#   http://bugreport.java.com/bugreport/crash.jsp
# The crash happened outside the Java Virtual Machine in native code.
# See problematic frame for where to report the bug.
#
```


# 参考
- [wecomopen/file_block_digest](https://github.com/wecomopen/file_block_digest)
    - 最新实现需要从此处获取
    - 其 SHA1 实现等细节与 Java 版本偏差较大
- [JNI中string 、 char* 和 jstring 两种转换](https://blog.csdn.net/xlxxcc/article/details/51106721)
- [升级glic： 解决"libc.so.6: version 'GLIBC_2.14' not found"问题](https://www.cnblogs.com/kevingrace/p/8744417.html)
    - [Linux下glibc升级](https://www.jianshu.com/p/1f434d0c11c3)
    - [Linux系统直接升级GLIBC-2.17版本](https://www.jianshu.com/p/cef630787d8e)
- [Ubuntu下JNI的简单使用](https://blog.csdn.net/fengqiaoyebo2008/article/details/6210499)
- [Failed to write core dump. Core dumps have been disabled](https://stackoverflow.com/questions/28982396/failed-to-write-core-dump-core-dumps-have-been-disabled)
    - JNA 调用报错
- [java.library.path是什么？](https://www.icode9.com/content-1-1128567.html)
- [java.library.path和LD_LIBRARY_PATH的介绍与区别](https://blog.csdn.net/gaussrieman123/article/details/106258530)
- [java jna调用so文件 undefined symbol xxx](https://blog.csdn.net/u013868665/article/details/115701264)
