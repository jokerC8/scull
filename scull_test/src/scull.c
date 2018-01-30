#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>

#define DEVPATH "/dev/scull"
#define BUFSIZE 256
#define COUNTS 0

pthread_mutex_t mutex;

static void *read_routine(void *arg)
{
#if  1
	int rfd,i,counts;
	char rbuf[BUFSIZE] = {0};
	if ((rfd = open(DEVPATH,O_RDWR)) < 0) {
		printf("open %s failed (%s)\n",DEVPATH,strerror(errno));
		goto wait;
	}
	i = 0;
	counts = COUNTS;
	do {
		if (read(rfd, rbuf, sizeof(rbuf)) <= 0) {
			printf("read failed (%s)\n",strerror(errno));
			close(rfd);
			if ((rfd = open(DEVPATH, O_RDWR)) < 0) {
				printf("open %s failed (%s)\n",DEVPATH,strerror(errno));
				pthread_exit(NULL);
			}
			goto wait;
		}else {
				pthread_mutex_lock(&mutex);
				printf("\n");
				printf("[read:%d]: %s\n",i++,rbuf);	
				printf("\n");
				pthread_mutex_unlock(&mutex);
		}
		memset(rbuf, '\0', sizeof(rbuf));
	wait:
		usleep(1000000);
	} while(counts--);
#endif
}

static void *write_routine(void *arg)
{
#if 1
	int wfd,i,counts;
	char wbuf[BUFSIZE] = {0};

	for (i = 0; i < sizeof(wbuf); i++) {
		wbuf[i] = 'a' + i % 26;
	}
	if ((wfd = open(DEVPATH, O_RDWR)) < 0) {
		printf("open %s failed (%s)\n",DEVPATH,strerror(errno));
		pthread_exit(NULL);
	}
	i = 0;
	counts = COUNTS;
	do {
		if (write(wfd, wbuf, sizeof(wbuf)) != sizeof(wbuf)) {
			printf("write failed (%s)\n",strerror(errno));
			goto wait;
		}
		pthread_mutex_lock(&mutex);
		printf("\n");
		printf("[write:%d]: %s\n",i++,wbuf);
		printf("\n");
		pthread_mutex_unlock(&mutex);
	wait:
		usleep(1000000);
	
	} while(counts--);
#endif
}

int main(void)
{
	int i;
	pthread_t pid[2];
	if (access(DEVPATH,F_OK)) {
		printf("%s not exist\n",DEVPATH);
		return -1;
	}
	pthread_mutex_init(&mutex,NULL);
	for (i = 0; i < sizeof(pid)/sizeof(pthread_t); i++) {
		if (pthread_create(&pid[i], NULL, i == 0 ? read_routine:write_routine,NULL)) {
			printf("create thread failed\n");
			return -1;
		}
	}
	pthread_join(pid[0], NULL);
	pthread_join(pid[1], NULL);

	return 0;
}
