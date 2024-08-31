#include <eXosip2/eXosip.h>
 
int main()
{
	int i=0;
	i=eXosip_init();
	if(i!=0)
          return -1;
	printf("=========  %s",eXosip_get_version());//打印当前sip库的版本号
	getchar();
	return 0;
}