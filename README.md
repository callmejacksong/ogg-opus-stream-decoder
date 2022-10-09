# ogg-opus-stream-decoder
decode ogg opus in stream data

主要是针对 ogg-opus 音频边发送边解码的问题而编写的该库，音频需要单通道，16bit位深音频格式，默认解码成48k采样率。

## 安装第三方库
``` shell
cd libogg-1.3.5
./configure
make -j8 && make install 

cd ../opus-1.3.1
./configure
make -j8 && make install 

cd ../opusfile-0.12
./configure
make -j8 && make install
```

## 编译
```shell
mkdir build
cd build
cmake .. && make
```
```src/main.cc```模拟了流式传送数据工程
``` shell
./main ../16k16bit.opus out.pcm
```
