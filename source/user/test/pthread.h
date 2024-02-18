#ifndef SOS_PTHREAD_H
#define SOS_PTHREAD_H

typedef void pthread_func();
typedef long long pthread;

long long pthread_run(const char* name, pthread_func* func, pthread* thread);
long long pthread_join(pthread thread, long long* exit_code);
long long pthread_detach(pthread thread);

#endif // SOS_PTHREAD_H