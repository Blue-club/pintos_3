#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.

Previously system call services was handled by the interrupt handler
(e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
efficient path for requesting the system call, the `syscall` instruction.

The syscall instruction works by reading the values from the the Model
Specific Register (MSR). For the details, see the manual. 
시스템 호출.

이전에는 시스템 호출 서비스가 인터럽트 핸들러에 의해 처리되었습니다.
(예: 리눅스의 int 0x80). 그러나 x86-64에서는 제조업체가 효율적인 경로를
제공하여 시스템 호출을 요청할 수 있게 되었는데, 그것은 `syscall` 명령어입니다.

syscall 명령은 Model Specific Register (MSR)에서 값을 읽어옴으로써 작동합니다.
자세한 내용은 메뉴얼을 참조하십시오.
 */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	  until the syscall_entry swaps the userland stack to the kernel
	  mode stack. Therefore, we masked the FLAG_FL. 
	  시스템 호출_entry가 사용자 모드 스택을 커널 모드 스택으로 교체하기 전까지
	  인터럽트 서비스 루틴은 어떤 인터럽트도 처리해서는 안됩니다.
	  따라서 FLAG_FL이 마스킹되어 있습니다. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface  주요 시스템 호출 인터페이스*/
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here. 여기에 구현을 작성하세요
	printf ("system call!\n");
	thread_exit ();
}

