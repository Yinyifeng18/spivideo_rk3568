/*

This example reads from the default PCM device
and G711 code and rtp send.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <pthread.h>

#include <math.h>
#include "g711codec.h"

#include "audio_rtp_send.h"

#define BUFFERSIZE 4096
#define PERIOD_SIZE 1024
#define PERIODS 2
#define SAMPLE_RATE 8000
#define CHANNELS 1
#define FSIZE 2*CHANNELS

void *audio_send(void *arg)
{
	struct audio_param_send *audiosendparam=(audio_param_send *)arg;

    int m_mark=1;
	int rc;	//return code.
    unsigned char buffer[320] = {0};
    int bytesRead;
    unsigned char sendbuf[1500];
	unsigned char sendbuf1[1500];
	uint16_t m_sequenceNumber;
    uint32_t m_timestamp;
      

	unsigned int timestamp_increse = 0,ts_current = 0;
	timestamp_increse = 320;

    // 打开音频文件
	FILE *file_g711 = NULL;  
	file_g711 = fopen("output.pcm","rb");  
    if(file_g711 == NULL)  
    { 
        printf("fopen error!\n");  
    }  

    printf("g711 open successfully!\n");  

	printf("audio_send thread\n");
	int cnt = 0;
	int i = 0;

    // https://www.cnblogs.com/frkang/p/3385604.html
	while (audiosendparam->send_quit==1) 
	{
		//1.读取文件数据
		memset(buffer, 0, 320);
		memset(sendbuf, 0, 1500);
		memset(sendbuf1, 0, 1500);

		 bytesRead = fread(&buffer[0], sizeof(char), sizeof(buffer), file_g711);
        if (feof(file_g711)) // 读完，关闭文件重新在读
        {
            printf("read pcm file eof\n");
            fclose(file_g711);
            file_g711 = fopen("output.pcm", "rb");
            if (file_g711 == NULL) {
                printf("Failed to reopen file\n");
				pthread_exit(0);
                return NULL;
            }
            continue;
        }

        //printf("bytesRead: %d\n", bytesRead);
		for(i = 0; i < bytesRead; i++)
		{
			buffer[i] = (unsigned char)buffer[i];
		}

		//2.编码
		//rc = PCM2G711u( (char *)buffer, (char *)&sendbuf1[0], bytesRead, 0 );//pcm转g711a
		//if(rc<0)  fprintf(stderr,RED "[%s@%s,%d]：" NONE "PCM2G711u error:rc=%d\n",__func__, __FILE__, __LINE__,rc);
		rc = bytesRead;
		
		//3.打包
		uint8_t payloadType = 0;
		uint8_t version = 2;
		uint8_t padding = 0;
		uint8_t extension = 0;
		uint8_t csrcCount = 0;
		uint8_t m_mark = m_mark ? 1 : 0;
		uint8_t byte1 = (version << 6) | (padding << 5) | (extension << 4) | csrcCount;
    	uint8_t byte2 = (m_mark << 7) | payloadType;

		// 将头部字节写入缓冲区
		sendbuf[0] = byte1;
		sendbuf[1] = byte2;

		// 序列号和时间戳以网络字节序存储
		sendbuf[2] = m_sequenceNumber >> 8;
		sendbuf[3] = m_sequenceNumber & 0xFF;
		sendbuf[4] = m_timestamp >> 24;
		sendbuf[5] = (m_timestamp >> 16) & 0xFF;
		sendbuf[6] = (m_timestamp >> 8) & 0xFF;
		sendbuf[7] = m_timestamp & 0xFF;

		// SSRC (同步源标识符) 字段为 0
		if(payloadType != 0){
			sendbuf[8] = 1;
			sendbuf[9] = 2;
			sendbuf[10] = 3;
			sendbuf[11] = 4;
		}else{
			sendbuf[8] = 3;
			sendbuf[9] = 4;
			sendbuf[10] = 5;
			sendbuf[11] = 6;
		}


		for(i = 0; i < bytesRead; i++)
		{
			sendbuf[12+i] = (unsigned char)buffer[i];
		}

		
		
		m_sequenceNumber++;
		if(rc == 320){
            m_timestamp += 320;
		}
		else if(rc == 160){
			m_timestamp += 160;
		}
		if(m_mark){
			m_mark = 0;
		}
		printf("payloadType: %d, timestamp: %d\n", payloadType, m_timestamp);
		printf("send data len: %d\n", rc+12);
		for(i = 0; i < 100; i++)
		{
			printf("%02x ", sendbuf[i]);
			if(i%16 == 0)
			  printf("\n");
		}
		printf("\n");

		//4.发送
		rc = send( audiosendparam->audio_rtp_socket, sendbuf, rc+12, 0 );//开始发送rtp包，+12是rtp的包头+g711荷载
		//printf("audio send ok cnt: %d\n", cnt++);
		if(rc<0) {
			//对方呼叫结束产生错误
			//fprintf(stderr , RED "[%s@%s,%d]:" NONE "net send error=%d\n", __func__, __FILE__, __LINE__,rc);
			break;
		}
		
        memset(sendbuf,0,1500);//清空sendbuf；此时会将上次的时间戳清空，因此需要ts_current来保存上次的时间戳值
        
		usleep(40 * 1000); // 休眠40毫秒
	}

    fclose(file_g711);
	
	close(audiosendparam->audio_rtp_socket);

	pthread_exit(0);
	
	return NULL;
}


void *audio_send_call(void *arg)
{
	struct audio_param_send *audiosendparam=(audio_param_send *)arg;

    int m_mark=1;
	int rc;	//return code.
    unsigned char buffer[320] = {0};
    int bytesRead;
    unsigned char sendbuf[1500];
	unsigned char sendbuf1[1500];
	uint16_t m_sequenceNumber;
    uint32_t m_timestamp;
      

	unsigned int timestamp_increse = 0,ts_current = 0;
	timestamp_increse = 320;

    // 打开音频文件
	FILE *file_g711 = NULL;  
	file_g711 = fopen("output.pcm","rb");  
    if(file_g711 == NULL)  
    { 
        printf("fopen error!\n");  
    }  

    printf("g711 open successfully!\n");  

	printf("audio_send thread\n");
	int cnt = 0;
	int i = 0;

    // https://www.cnblogs.com/frkang/p/3385604.html
	while (audiosendparam->send_quit==1) 
	{
		//1.读取文件数据
		memset(buffer, 0, 320);
		memset(sendbuf, 0, 1500);
		memset(sendbuf1, 0, 1500);

		 bytesRead = fread(&buffer[0], sizeof(char), sizeof(buffer), file_g711);
        if (feof(file_g711)) // 读完，关闭文件重新在读
        {
            printf("read pcm file eof\n");
            fclose(file_g711);
            file_g711 = fopen("output.pcm", "rb");
            if (file_g711 == NULL) {
                printf("Failed to reopen file\n");
				pthread_exit(0);
                return NULL;
            }
            continue;
        }

        //printf("bytesRead: %d\n", bytesRead);
		for(i = 0; i < bytesRead; i++)
		{
			buffer[i] = (unsigned char)buffer[i];
		}

		//2.编码
		rc = PCM2G711a( (char *)buffer, (char *)&sendbuf1[0], bytesRead, 0 );//pcm转g711a
		if(rc<0)  fprintf(stderr,RED "[%s@%s,%d]：" NONE "PCM2G711u error:rc=%d\n",__func__, __FILE__, __LINE__,rc);
		//rc = bytesRead;
		
		//3.打包
		uint8_t payloadType = 8;
		uint8_t version = 2;
		uint8_t padding = 0;
		uint8_t extension = 0;
		uint8_t csrcCount = 0;
		uint8_t m_mark = m_mark ? 1 : 0;
		uint8_t byte1 = (version << 6) | (padding << 5) | (extension << 4) | csrcCount;
    	uint8_t byte2 = (m_mark << 7) | payloadType;

		// 将头部字节写入缓冲区
		sendbuf[0] = byte1;
		sendbuf[1] = byte2;

		// 序列号和时间戳以网络字节序存储
		sendbuf[2] = m_sequenceNumber >> 8;
		sendbuf[3] = m_sequenceNumber & 0xFF;
		sendbuf[4] = m_timestamp >> 24;
		sendbuf[5] = (m_timestamp >> 16) & 0xFF;
		sendbuf[6] = (m_timestamp >> 8) & 0xFF;
		sendbuf[7] = m_timestamp & 0xFF;

		// SSRC (同步源标识符) 字段为 0
		if(payloadType != 0){
			sendbuf[8] = 1;
			sendbuf[9] = 2;
			sendbuf[10] = 3;
			sendbuf[11] = 4;
		}else{
			sendbuf[8] = 3;
			sendbuf[9] = 4;
			sendbuf[10] = 5;
			sendbuf[11] = 6;
		}


		for(i = 0; i < bytesRead; i++)
		{
			sendbuf[12+i] = (unsigned char)buffer[i];
		}

		//printf("send data len: %d\n", rc+12);
		//for(i = 0; i < 100; i++)
		//{
		//	printf("%02x ", sendbuf[i]);
		//	if(i%8 == 0)
		//	  printf("\n");
		//}
		//printf("\n");
		
		m_sequenceNumber++;
		if(rc == 320){
            m_timestamp += 320;
		}
		else if(rc == 160){
			m_timestamp += 160;
		}
		if(m_mark){
			m_mark = 0;
		}

		//4.发送
		rc = send( audiosendparam->audio_rtp_socket, sendbuf, rc+12, 0 );//开始发送rtp包，+12是rtp的包头+g711荷载
		//printf("audio send ok cnt: %d\n", cnt++);
		if(rc<0) {
			//对方呼叫结束产生错误
			//fprintf(stderr , RED "[%s@%s,%d]:" NONE "net send error=%d\n", __func__, __FILE__, __LINE__,rc);
			break;
		}
		
        memset(sendbuf,0,1500);//清空sendbuf；此时会将上次的时间戳清空，因此需要ts_current来保存上次的时间戳值
        
		usleep(40*1000);
	}

    fclose(file_g711);
	
	close(audiosendparam->audio_rtp_socket);

	pthread_exit(0);
	
	return NULL;
}

