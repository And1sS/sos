#ifndef SOS_FILE_H
#define SOS_FILE_H

long long int open(const char* path, unsigned long long flags);
long long int close(unsigned long long fd);
long long int read(unsigned long long fd, void* buffer,
                   unsigned long long size);
long long int write(unsigned long long fd, void* buffer,
                    unsigned long long size);
long long int chroot(const char* path);
long long int chdir(const char* path);
long long mkdir(const char* path, unsigned long long mode);
long long mkdirat(unsigned long long dirfd, const char* path,
                  unsigned long long mode);

#endif // SOS_FILE_H
