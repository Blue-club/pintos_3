#ifndef FILESYS_OFF_T_H
#define FILESYS_OFF_T_H

#include <stdint.h>

/* An offset within a file.
 This is a separate header because multiple headers want this
 definition but not any others. 
 파일 내 오프셋
 이것은 여러 개의 헤더에서 이 정의를 사용하지만 
 다른 정의는 필요하지 않기 때문에 별도의 헤더로 분리되었습니다. */
typedef int32_t off_t;

/* Format specifier for printf(), e.g. 
printf()를 위한 형식 지정자, 예:
printf ("offset=%"PROTd"\n", offset); */
#define PROTd PRId32

#endif /* filesys/off_t.h */
