INCLUDE=-I./lib/libosip/include -I./lib/libexosip/include
LIBS= -L./lib/libosip/lib/  -L./lib/libexosip/lib/  -lpthread -lm -lrt -ldl -leXosip2 -losip2  -losipparser2 -lresolv
all:sipclient
sipclient:
	 /opt/atk-dlrk356x-toolchain/usr/bin/aarch64-buildroot-linux-gnu-gcc -o sipclient -O2 -w  sipclient.c audio_rtp_recv.c audio_rtp_send.c g711.c g711codec.c video_rtp_pack.c video_rtp_recv.c video_rtp_send.c   $(INCLUDE) $(LIBS)
clean:
	rm -rfv sipclient
