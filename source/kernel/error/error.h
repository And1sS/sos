#ifndef SOS_ERROR_H
#define SOS_ERROR_H

#define IS_ERROR(errno) (((i64) (errno) > -0xFF) && ((i64) (errno) < 0))
#define ERROR_PTR(errno) ((void*) ((i64) (errno)))
#define PTR_ERROR(errno) ((i64) (errno))

#endif // SOS_ERROR_H