#include "stream_opus_decoder.h"

StreamOggOpusDecoder::StreamOggOpusDecoder():_buffer_size(0),_storaged(0),
total_pcm_length(0),_has_init(false),_pre_skip(0),_has_remove_skip(false)
{
    init();
}

void StreamOggOpusDecoder::print_header(ogg_page *og)
{
  int j;
  fprintf(stderr,"\nHEADER:\n");
  fprintf(stderr,"  capture: %c %c %c %c  version: %d  flags: %x\n",
          og->header[0],og->header[1],og->header[2],og->header[3],
          (int)og->header[4],(int)og->header[5]);

  fprintf(stderr,"  granulepos: %d  serialno: %d  pageno: %ld\n",
          (og->header[9]<<24)|(og->header[8]<<16)|
          (og->header[7]<<8)|og->header[6],
          (og->header[17]<<24)|(og->header[16]<<16)|
          (og->header[15]<<8)|og->header[14],
          ((long)(og->header[21])<<24)|(og->header[20]<<16)|
          (og->header[19]<<8)|og->header[18]);

  fprintf(stderr,"  checksum: %02x:%02x:%02x:%02x\n  segments: %d (",
          (int)og->header[22],(int)og->header[23],
          (int)og->header[24],(int)og->header[25],
          (int)og->header[26]);

  for(j=27;j<og->header_len;j++)
    fprintf(stderr,"%d ",(int)og->header[j]);
  fprintf(stderr,")\n\n");
}

int StreamOggOpusDecoder::reset()
{
    if(_ogg_sync_state != NULL)
    {
        ogg_sync_destroy(_ogg_sync_state);
        _ogg_sync_state = NULL;
    }

    if(_opus_decoder != NULL)
    {
        opus_decoder_destroy(_opus_decoder);
        _opus_decoder = NULL;
    }
    if(_stream_state_struct != NULL)
    {
        ogg_stream_destroy(_stream_state_struct);
        _stream_state_struct = NULL;
    }
    _opus_buffer = NULL;
    _buffer_size = 0;
    _storaged = 0;
    _has_init = false;
    return 0;
    
}

int StreamOggOpusDecoder::init()
{
    _ogg_sync_state = (ogg_sync_state *)malloc(sizeof(ogg_sync_state));
    // 初始化存放opus数据
    ogg_sync_init(_ogg_sync_state);

    _stream_state_struct = (ogg_stream_state *)malloc(sizeof(ogg_stream_state));
    ogg_stream_init(_stream_state_struct, -1);

    _opus_buffer = ogg_sync_buffer(_ogg_sync_state, OPUS_LENGTH);
    if(_opus_buffer==NULL)
    {
        std::cout << "init opus buffer error" << std::endl;
        reset();
        return -1;
    }
    _buffer_size = OPUS_LENGTH;

    // 初始化opus解码器
    int error;

    _opus_decoder = opus_decoder_create(PCM_48K_SAMPLE_RATE, 1, &error);

    if(error != OPUS_OK)
    {
        std::cout << "init opus decoder error" << std::endl;
        _opus_decoder = NULL;
        reset();
        return -1;
    }
    _has_init = true;
    return 0;
}

int StreamOggOpusDecoder::re_malloc_opus_buffer(long size)
{
    ogg_sync_state * tmp_ogg_state = (ogg_sync_state *)malloc(sizeof(ogg_sync_state));
        // 初始化存放opus数据
    ogg_sync_init(tmp_ogg_state);
    long tmp_size = OPUS_LENGTH > size ? _buffer_size + OPUS_LENGTH : _buffer_size + size;
    char * tmp_buffer = ogg_sync_buffer(tmp_ogg_state, tmp_size);

    memcpy(tmp_buffer, _opus_buffer, _buffer_size);
    tmp_ogg_state->fill = _ogg_sync_state->fill;
    tmp_ogg_state->returned = _ogg_sync_state->returned;
    tmp_ogg_state->unsynced = _ogg_sync_state->unsynced;
    tmp_ogg_state->headerbytes = _ogg_sync_state->headerbytes;
    tmp_ogg_state->bodybytes = _ogg_sync_state->bodybytes;

    if(_ogg_sync_state != NULL)
    {
        ogg_sync_destroy(_ogg_sync_state);
    }
    _ogg_sync_state = tmp_ogg_state;
    _opus_buffer = tmp_buffer;
    _buffer_size = tmp_size;

    return 0;
}




int StreamOggOpusDecoder::decode(unsigned char* opus_data, long opus_length, opus_int16 * pcm_data, long pcm_length)
{

    int ret;
    if (!_has_init)
    {
        ret = init();
        if(ret < 0)
        {
            std::cout << "decoder params has not init!" << std::endl;
            return -1;
        }
    }
    // 校验是否数据不够存放
    // std::cout << "_buffer_size: "<< _buffer_size<< " _storaged:"<<_storaged << " total_pcm_length:"<< total_pcm_length << " _has_init: "<< _has_init << " _pre_skip:" << _pre_skip << std::endl;
    if(_storaged + opus_length > _buffer_size)
    {
        std::cout << "not re_malloc_opus_buffer" << std::endl;
        re_malloc_opus_buffer(opus_length);
    }

    // 将opus数据放入opus buffer中

    memcpy(_opus_buffer+_storaged, opus_data, opus_length);

    _storaged += opus_length;
    // 更新_ogg_sync_state 中fill内容
    ret = ogg_sync_wrote(_ogg_sync_state, opus_length);
    if(ret < 0)
    {
        std::cout << "decoder update ogg_sync_wrote error !" << std::endl;
        return -1;
    }

    int pcm_decode_size=0;
    long skip_pcm_length = pcm_length + 960;

    opus_int16 tmp_pcm_data[skip_pcm_length];
    opus_int16 * tmp_pcm_data_ptr = tmp_pcm_data;
    long tmp_pcm_length = skip_pcm_length;

    int step_sample = 0;
    bool has_pre_skip = false;

    int opus_count=0;



    while (true)
    {
        ogg_page o_page;

        ret = ogg_sync_pageout(_ogg_sync_state, &o_page);

        if(ret ==0)
        {
            std::cout << "decoder page complete : " << ret << std::endl;
            break;
        }
        if(ret <0)
        {
            std::cout << "decoder page continue : " << ret << std::endl;
            continue;
        }
        
        ogg_stream_reset_serialno(_stream_state_struct, ogg_page_serialno(&o_page));

        // 获取当前页为止所有pcm的总数，idheader 和 comment header 为o
        int64_t granulepos = ogg_page_granulepos(&o_page);

        std::cout << "page: "<< ogg_page_pageno(&o_page) << " was decoded; granulepos: " << granulepos << "; page has packets: "<< ogg_page_packets(&o_page) << std::endl;
        // print_header(&o_page);

        ogg_stream_pagein(_stream_state_struct, &o_page);

        while(true)
        {
            ogg_packet o_packet;
            
            ret = ogg_stream_packetout(_stream_state_struct, &o_packet);

            if(ret<=0)
            {
                std::cout << "decoder packet complete " << std::endl;
                break;
                
            }

            if(o_packet.b_o_s)
            {
                OpusHead o_head;
                ret = opus_head_parse(&o_head, o_packet.packet, o_packet.bytes);

                if(ret<0)
                {
                    std::cout << "decoder header error " << std::endl;

                    return -1;
                }
                _pre_skip = o_head.pre_skip;
                has_pre_skip = true;
                std::cout << "has_pre_skip:  "<< _pre_skip  << " channel_count: "<< o_head.channel_count<< std::endl;

            }

            if(granulepos==0)
            {
                std::cout << "this is header page " << std::endl;
                break;
            }
            // 开始解码
            opus_count += o_packet.bytes;

            // 校验剩余的pcm的数据buffer长度是否够用
            int one_packet_samples = opus_decoder_get_nb_samples(_opus_decoder, o_packet.packet, o_packet.bytes);
            if(one_packet_samples>0 && tmp_pcm_length< one_packet_samples)
            {
                std::cout << "left pcm buffer is too small! left: "<< tmp_pcm_length << std::endl;
                return -1;
            }

            step_sample = opus_decode(_opus_decoder, o_packet.packet, o_packet.bytes, tmp_pcm_data_ptr, tmp_pcm_length, 0);
            // std::cout << "packet channels:" << opus_packet_get_nb_channels(o_packet.packet) << " tmp_pcm_length:" << tmp_pcm_length << " opus_count:" << opus_count<< " step_sample: " <<step_sample<< " samples: "<< opus_decoder_get_nb_samples(_opus_decoder, o_packet.packet, o_packet.bytes)<< std::endl;
            

            if(step_sample<0)
            {
                std::cout << "decode pcm error! error code: "<< step_sample << std::endl;
                return -1;
            }
            tmp_pcm_data_ptr += step_sample;
            total_pcm_length += step_sample;
            tmp_pcm_length -= step_sample;

            pcm_decode_size += step_sample;
        }

    }
    if(!_has_remove_skip && pcm_decode_size > _pre_skip)
    {
        pcm_decode_size -= _pre_skip;
        _has_remove_skip = true;
    }

    std::cout << "total_pcm_length: " << total_pcm_length << " pcm_decode_size:" << pcm_decode_size << std::endl;


    if(has_pre_skip)
    {
        for(int i=0;i<pcm_decode_size;i++)
        {
            pcm_data[i] = tmp_pcm_data[_pre_skip+i];
        }
    }else
    {
        for(int i=0;i<pcm_decode_size;i++)
        {
            pcm_data[i] = tmp_pcm_data[i];
        }  
    }

    return pcm_decode_size;
}


