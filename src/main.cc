#include <iostream>
#include <fstream>
#include <string>
#include <opus.h>
#include "stream_opus_decoder.h"

void usage()
{
    std::cout << "usage: ./main in.opus out.pcm"<< std::endl;
}


int main(int argc, char** argv)
{
    if(argc<3)
    {
        usage();
        return -1;
    }
    usage();
    std::string input = argv[1];
    std::string output = argv[2];
    

    std::fstream opus_file(input, std::ios::in|std::ios::binary|std::ios::ate);
    int opus_size = opus_file.tellg();
    std::cout << "opus file size: " << opus_size << std::endl;
    opus_file.seekg(0,std::ios::beg);


    unsigned char opus_buffer[opus_size];
    opus_file.read((char *)opus_buffer, opus_size);
    
    opus_file.close();

    unsigned char * opus_buffer_ptr = opus_buffer;
    int tmp_opus_size = opus_size;
    
    long pcm_length = opus_size*20;

    opus_int16 pcm_data[pcm_length];
    opus_int16 * pcm_data_ptr = pcm_data;
    long tmp_pcm_length = pcm_length;

    int step = 200;
    int count =0;

    StreamOggOpusDecoder s_decoder;
    while(tmp_opus_size>0)
    {
        int left_step = tmp_opus_size > step ? step : tmp_opus_size;
        int tmp = s_decoder.decode(opus_buffer_ptr, left_step, pcm_data_ptr, tmp_pcm_length);
        // std::cout << "----tmp:" << tmp << " " ;
        count += tmp;
        opus_buffer_ptr += left_step;
        tmp_opus_size -= left_step;
        pcm_data_ptr += tmp;
        tmp_pcm_length -= tmp;
        // break;

    }
    std::cout << std::endl ;

    std::cout << "count: " << count  << std::endl;

    // 将int16转为小端char
    char out[count*2];
    for(int i=0;i<count; i++)
    {
        out[2*i+1] = pcm_data[i]>>8&0xFF;
        out[2*i+0] = pcm_data[i]&0xFF;

    }

    std::fstream out_file(output, std::ios::out|std::ios::binary);

    // 写wav头

    out_file.write(out, count*2);
    out_file.close();
    
    return 0;
}