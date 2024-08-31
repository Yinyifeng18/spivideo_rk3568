

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "video_rtp_send.h"
#include "video_rtp_pack.h"


typedef struct {
  uint8_t* data;
  uint32_t length;
} h264_buffer_t;

struct sockaddr_in s_peer;
static int udpsock;

static int sock_create(const char* peer_addr, int port,int localport)
{
	int s;

	memset(&s_peer, 0, sizeof(s_peer));
	s_peer.sin_family = AF_INET;
	s_peer.sin_addr.s_addr = inet_addr(peer_addr);
	s_peer.sin_port = htons(port);
	if (s_peer.sin_addr.s_addr == 0 || s_peer.sin_addr.s_addr == 0xffffffff) {
		fprintf(stderr, "Invalid address(%s)\n", peer_addr);
		return -1;
	}
	if (port <= 0 || port > 65535) {
		fprintf(stderr, "Invalid port(%d)\n", port);
		return -1;
	}
	s = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
	if (s < 0) { perror("socket"); return -1; }
	
	//端口复用
	int flag=1;
	if( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1)  {  
	fprintf(stderr, RED "[%s@%s,%d]："NONE"socket setsockopt error\n", __func__, __FILE__, __LINE__);
	return -1;
	}  
	//绑定本地端口
	struct sockaddr_in local;  
	local.sin_family=AF_INET;  
	local.sin_port=htons(localport);            ///监听端口  
	local.sin_addr.s_addr=INADDR_ANY;       ///本机  
	if(bind(s,(struct sockaddr*)&local,sizeof(local))==-1) {
	fprintf(stderr, RED "[%s@%s,%d]："NONE"udp port bind error\n",__func__, __FILE__, __LINE__);
	return -1;
	}
	return s;
}


static inline int startCode3(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

static inline int startCode4(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

static char* findNextStartCode(char* buf, int len)
{
    int i;

    if(len < 3)
        return NULL;

    for(i = 0; i < len-3; ++i)
    {
        if(startCode3(buf) || startCode4(buf))
            return buf;
        
        ++buf;
    }

    if(startCode3(buf))
        return buf;

    return NULL;
}

static int getFrameFromH264File(int fd, char* frame, int size)
{
    int rSize, frameSize;
    char* nextStartCode;

    if(fd < 0)
        return fd;

    rSize = read(fd, frame, size);
    if(!startCode3(frame) && !startCode4(frame))
        return -1;
    
    nextStartCode = findNextStartCode(frame+3, rSize-3);
    if(!nextStartCode)
    {
        lseek(fd, 0, SEEK_SET);
        frameSize = rSize;
    }
    else
    {
        frameSize = (nextStartCode-frame);
        lseek(fd, frameSize-rSize, SEEK_CUR);
    }

    return frameSize;
}


static int rtpSendH264Frame(int socket, char* ip, int16_t port,
                            struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
    uint8_t naluType; // nalu第一个字节
    int sendBytes = 0;
    int ret;

    naluType = frame[0];

    if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包场：单一NALU单元模式
    {
        /*
         *   0 1 2 3 4 5 6 7 8 9
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |F|NRI|  Type   | a single NAL unit ... |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacket(socket, ip, port, rtpPacket, frameSize);
        if(ret < 0)
            return -1;

        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) // 如果是SPS、PPS就不需要加时间戳
            goto out;
    }
    else // nalu长度小于最大包场：分片模式
    {
        /*
         *  0                   1                   2
         *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * | FU indicator  |   FU header   |   FU payload   ...  |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */

        /*
         *     FU Indicator
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |F|NRI|  Type   |
         *   +---------------+
         */

        /*
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|R|  Type   |
         *   +---------------+
         */

        int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 1;

        /* 发送完整的包 */
        for (i = 0; i < pktNum; i++)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            
            if (i == 0) //第一包数据
                rtpPacket->payload[1] |= 0x80; // start
            else if (remainPktSize == 0 && i == pktNum - 1) //最后一包数据
                rtpPacket->payload[1] |= 0x40; // end

            memcpy(rtpPacket->payload+2, frame+pos, RTP_MAX_PKT_SIZE);
            ret = rtpSendPacket(socket, ip, port, rtpPacket, RTP_MAX_PKT_SIZE+2);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
        }

        /* 发送剩余的数据 */
        if (remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            rtpPacket->payload[1] |= 0x40; //end

            memcpy(rtpPacket->payload+2, frame+pos, remainPktSize+2);
            ret = rtpSendPacket(socket, ip, port, rtpPacket, remainPktSize+2);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
        }
    }

out:

    return sendBytes;
}



void *video_send(void *arg)
{
	int fd;
	unsigned char *frame;
    int frameSize;
	int fps = 30;
    int startCode;
    int i = 0;
    int cnt = 0;

	video_param_send *video_send_param= (video_param_send *)arg;
	fprintf(stderr,BLUE"[%s]:" NONE"video_send_param:%s:%d\n", __FILE__,video_send_param->dest_ip, video_send_param->dest_port);
	
	//网络初始化
	udpsock = sock_create(video_send_param->dest_ip, video_send_param->dest_port,video_send_param->local_port);

	/* 打开文件 */
	fd = open("video.h264", O_RDONLY);
    if(fd < 0)
    {
        printf("failed to open %s\n", "video.h264");
		
        pthread_exit(0);
		return NULL;
    }

	struct RtpPacket* rtpPacket;

	rtpPacket = (struct RtpPacket*)malloc(500000);
    frame = (uint8_t*)malloc(500000);

	rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0,
                 0, 0, 0x88923423);

    fprintf(stderr, BLUE"[%s]:" NONE"video_send_param:%s:%d\n", __FILE__,video_send_param->dest_ip, video_send_param->dest_port);

	while (video_send_param->send_quit==1)
	{
		/* 读数据 */
		frameSize = getFrameFromH264File(fd, frame, 500000);
		if(frameSize < 0)
        {
            printf("read err\n");
            continue;
        }

		if(startCode3(frame))
            startCode = 3;
        else
            startCode = 4;

        frameSize -= startCode;

        //printf("rtpSendH264Frame h264 len: %d\n", frameSize);
        //for(i = 0; i < 200; i++)
        //{
        //    printf("%02x ", frame[i]);
        //    if(i % 16 == 0)
        //        printf("\r\n");
        //}
        //printf("\r\n");

        rtpSendH264Frame(udpsock, video_send_param->dest_ip, video_send_param->dest_port,
                            rtpPacket, frame+startCode, frameSize);
        rtpPacket->rtpHeader.timestamp += 90000/fps;  
        
		usleep(1000*1000/fps);
	}


	printf("video send thread exit\n");

	free(rtpPacket);
    free(frame);

	pthread_exit(0);
	
	return NULL;
}	


