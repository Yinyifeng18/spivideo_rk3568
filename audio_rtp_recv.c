
#include <fcntl.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "audio_rtp_recv.h"
#include "g711codec.h"




void *audio_recv(void *arg)
{
	int rc;
	int ret = 0;
	struct audio_param_recv *audiorecvparam= (audio_param_recv *)arg;

	char recvbuffer[256];
    char outbuffer[320];
	int recv_len;
	int frames=160;//注意定义alsa中frames

	int stream_fd = -1;
	// 打开一个PCM文件以写入数据
    stream_fd = open("audio_recv.pcm", O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (stream_fd < 0) 
	{
       printf("audio recv open file failed\n");

	   pthread_exit(0);

	   return 0;
	}


	while (audiorecvparam->recv_quit==1) 
	{
		bzero(recvbuffer, sizeof(recvbuffer));
		usleep(100); //防止cpu过高
		recv_len = recv(audiorecvparam->audio_rtp_socket, recvbuffer, sizeof(recvbuffer), 0 );
		if(recv_len<0) 	continue;
		//printf("audio recv_len=%d,seq=%d\n",recv_len,recvbuffer[2] << 8|recvbuffer[3] << 0);
		rc=G711u2PCM(&recvbuffer[12], outbuffer, 160, 0);//应该返回值为320
		if(rc<0)  fprintf(stderr,RED "[%s]:" NONE "G711u2PCM error:rc=%d\n",__FILE__,rc);
		
		/* 把数据保存到文件里 */
		ret = write(stream_fd, outbuffer, frames);
		if (ret != frames) 
		{
			break;
		}
	}

	printf("audio_recv pthread exit\n");
	
	pthread_exit(0);

	return 0;
}




