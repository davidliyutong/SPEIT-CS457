#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex;

/* this function is run by the second thread */
void *inc_x(void *x_void_ptr)
{

	/* increment x */
	pthread_mutex_lock(&mutex);
	int *x_ptr = (int *)x_void_ptr;
	for (int i = 0; i < 100000000; ++i) {
		++(*x_ptr);
	}
	pthread_mutex_unlock(&mutex);
	/* the function must return something - NULL will do */
	return NULL;

}

int main()
{

	int x = 0;

	/* show the initial values of x and y */
	printf("x: %d\n", x);

	/* this variable is our reference to the second thread */
	pthread_t inc_x_thread;

	/* lock mutex */
	pthread_mutex_lock(&mutex);

	/* create a second thread which executes inc_x(&x) */
	if(pthread_create(&inc_x_thread, NULL, inc_x, &x)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	for (int i = 0; i < 100000000; ++i) {
		++x;
	}
	/* unlock mutex so that the second thread can proceed. */
	pthread_mutex_unlock(&mutex);

	/* wait for the second thread to finish */
	if(pthread_join(inc_x_thread, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return 2;
	}

	/* show the results - x is now 200 thanks to proper synchonization */
	printf("x: %d\n", x);

	return 0;
}
