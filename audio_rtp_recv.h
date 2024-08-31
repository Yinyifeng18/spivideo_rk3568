#ifndef	__AUDIO_RTP_RECV_H__
#define	__AUDIO_RTP_RECV_H__

#define NONE                      "\033[m"  
#define RED                         "\033[1;31m"
#define GREEN                   "\033[1;32m" 
#define BLUE                       "\033[1;34m"

typedef struct audio_param_recv
{
int     audio_rtp_socket;
char    *dest_ip ;
int      dest_port;
int      local_port;
int      recv_quit;
} audio_param_recv;

void *audio_recv(void *arg);

#endif

