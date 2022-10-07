#pragma once
#include <iostream>
#include <ogg/ogg.h>
#include <ogg/os_types.h>
#include <opus.h>
#include <string.h>
#include <opusfile.h>
#include <stdio.h>

#define OPUS_LENGTH (60*16000/3)
#define PCM_48K_SAMPLE_RATE (48000)

class StreamOggOpusDecoder
{
public:

    StreamOggOpusDecoder();
    ~StreamOggOpusDecoder()
    {
        reset();
    }
    void print_header(ogg_page *og);

    /*
    opus_data 输入的opus数据
    opus_length 输入的opus数据长度
    pcm_data 输入参数，解码后的pcm数据
    pcm_length pcm数据buffer的长度
    return 解码后的pcm数据长度 负数表示失败
    */
    int decode(unsigned char* opus_data, long opus_length, opus_int16 * pcm_data, long pcm_length);

private:
    ogg_sync_state * _ogg_sync_state = NULL;
    ogg_stream_state * _stream_state_struct = NULL;

    char * _opus_buffer = NULL;
    // 已经分配的内存大小
    int _buffer_size;
    // 存放数据的大小
    int _storaged;

    OpusDecoder * _opus_decoder;

// 整个解码过程中pcm数据的长度
    int total_pcm_length;

    bool _has_init;
    // opus的无用pcm数量
    int _pre_skip;
    
    bool _has_remove_skip;

    // 初始化状态
    int init();

        // 还原状态
    int reset();

    /*
    当opus流过于长时 _opus_buffer 不够用时，重新至少分配size大小的空间 默认是OPUS_LENGTH大小 并将原有数据重新拷贝过来
    返回分配的大小，负数表示错误
    */
    int re_malloc_opus_buffer(long size);

};