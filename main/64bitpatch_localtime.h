#ifndef patch_64BIT_LOCALTIME_H_
#define patch_64BIT_LOCALTIME_H_

#include <time.h>
#include <sys/param.h>

struct tm * localtime_patch(const time_t *__restrict tim_p, struct tm *__restrict res);
void tzset_patch(void);

#endif
