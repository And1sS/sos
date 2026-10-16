#ifndef UTIL_H
#define UTIL_H

#define UNUSED(x) (void) (x)

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MEMBER_SIZE(type, member) (sizeof(((type*) 0)->member))

#endif // UTIL_H
