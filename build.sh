clear

gcc sipclient.c audio_rtp_recv.c audio_rtp_send.c g711.c g711codec.c video_rtp_pack.c video_rtp_recv.c video_rtp_send.c -o sipclient  -losip2 -leXosip2 -losipparser2 -lpthread

