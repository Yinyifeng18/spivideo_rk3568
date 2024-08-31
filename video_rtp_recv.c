

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <malloc.h>

#include "video_rtp_recv.h" 
#include "video_rtp_send.h"

#include <pthread.h>

	

#define  RTP_HEADLEN 12
int  UnpackRTPH264(unsigned char *bufIn,int len,unsigned char *bufout,video_frame *videoframe)
{
int outlen=0;
     if  (len  <  RTP_HEADLEN){
         return  -1 ;
    }

    unsigned  char *src  =  (unsigned  char * )bufIn  +  RTP_HEADLEN;
    unsigned  char  head1  =   * src; // 获取第一个字节
    unsigned  char  nal  =  head1  &   0x1f ; // 获取FU indicator的类型域，
    unsigned  char  head2  =   * (src + 1 ); // 获取第二个字节
    unsigned  char  flag  =  head2  &   0xe0 ; // 获取FU header的前三位，判断当前是分包的开始、中间或结束
    unsigned  char  nal_fua  =  (head1  &   0xe0 )  |  (head2  &   0x1f ); // FU_A nal
	//这里可以获取CSRC/sequence number/timestamp/SSRC/CSRC等rtp头部信息.
	//是否可以定义rtp头部struct,memcopy缓存20个字节到rtp头部即可获取．
    videoframe->seq_no=bufIn[2] << 8|bufIn[3] << 0;//序号，用于判断rtp乱序
    videoframe->timestamp=bufIn[4] << 24|bufIn[5] << 16|bufIn[6] << 8|bufIn[7] << 0;//时间戳，送到解码中
    videoframe->flag= src[1]&0xe0;//判断包是开始　结束
    videoframe->nal= src[0]&0x1f;//是否是fu-a,还是单个包
    videoframe->frametype= src[1]&0x1f;//判断帧类型Ｉ,还有错误


	if  (nal == 0x1c ){//fu-a＝28
		if  (flag == 0x80 ) // 开始=128
		{
			//printf("s ");
			bufout[0]=0x0;
			bufout[1]=0x0;
			bufout[2]=0x0;
			bufout[3]=0x1;
			bufout[4]=nal_fua;
			outlen  = len - RTP_HEADLEN -2+5;//-2跳过前2个字节，+5前面前导码和类型码，+5
			memcpy(bufout+5,src+2,outlen);
		}
		else   if (flag == 0x40 ) // 结束=64
		{
			//printf("e ");
			outlen  = len - RTP_HEADLEN -2 ;
			memcpy(bufout,src+2,len-RTP_HEADLEN-2);
		}
		else // 中间
		{
			//printf("c ");
			outlen  = len - RTP_HEADLEN -2 ;
			memcpy(bufout,src+2,len-RTP_HEADLEN-2);
		}
	}
	else {//当个包，1,7,8
		//printf("*[%d]* ",nal);
		bufout[0]=0x0;
		bufout[1]=0x0;
		bufout[2]=0x0;
		bufout[3]=0x1;
		memcpy(bufout+4,src,len-RTP_HEADLEN);
		outlen=len-RTP_HEADLEN+4;
	}
	return  outlen;
}




void *video_recv(void *arg)	
{
	unsigned char buffer[2048];
	video_frame videoframe;
	unsigned char outbuffer[2048];
	int outbuffer_len;
	
	struct video_param_recv *video_recv_param=(video_param_recv *)arg;

	while((video_recv_param->recv_quit==1))
	{
		usleep(100);
		int recv_len;
		

		bzero(buffer, sizeof(buffer));
		recv_len = recv(video_recv_param->video_rtp_socket, buffer, sizeof(buffer),0);
		if(recv_len<0) 	continue; 
		outbuffer_len=UnpackRTPH264(buffer,recv_len,outbuffer,&videoframe);
		//fprintf(stderr,BLUE"[%s]:" NONE"seq_no[%d],flage[%d],video_recv_len[%d],outbuffer_len[%d]\n",__FILE__,videoframe.seq_no,videoframe.flag,recv_len,outbuffer_len);
		//printf("frame:seq_no[%d],nal[%d],type[%d],flage[%d]\n",videoframe.seq_no,videoframe.nal,videoframe.frametype,videoframe.flag);
		//in_buffer=(unsigned char *)malloc(outbuffer_len);//分配内存，保存到队列中去
		//memcpy(in_buffer,outbuffer,outbuffer_len);

		printf("video_recv_len: %d\n", recv_len);
	}

	printf("video_recv exit\n");
	
	pthread_exit(0);

	return NULL;
}

