#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"


/* Project 2 */
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include <list.h>
#include <stdlib.h>
// #include "threads/malloc.h"
// #include "lib/kernel/list.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* Project 2 */
struct lock filesys_lock;

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

bool 
cmp_fd(const struct list_elem *a, const struct list_elem *b, void *aux)
{
	int* fd = aux;
	// printf("a fd : %d, aux %d\n",list_entry(a, struct file_descriptor, elem)->fd,*fd);
	// printf("a fd : %d, aux %d\n",list_entry(a, struct file_descriptor, elem)->fd,*fd);
	return list_entry(a, struct file_descriptor, elem)->fd == *fd;
}


static struct list_elem *
find_end_of_run (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	ASSERT (a != NULL);
	ASSERT (b != NULL);
	ASSERT (less != NULL);
	ASSERT (a != b);
	// printf("find start!!\n");
	// if(a==b) printf("a==b\n");
	// less (a, list_prev (a), aux);
	while (a != b && !less (a, list_prev (a), aux)){
		a = list_next (a);
	}

	// printf("1 a fd %d\n",list_entry(a, struct file_descriptor, elem)->fd);
	return a;
}

void check_address(void *addr)
{
	if(addr == NULL) exit(-1);
	if(!is_user_vaddr(addr)) exit(-1);
  if (pml4_get_page(thread_current()->pml4, addr) == NULL) exit(-1);
}



struct list_elem *
find_file_in_fdt(const struct thread * cur, const int fd)
{

	struct list_elem * elem;

	for(elem = list_begin(&cur->fdt_list); elem!=list_end(&cur->fdt_list);elem = list_next (elem))
		if(list_entry(elem,struct file_descriptor, elem)->fd == fd) break;
	

	if(elem == list_end(&cur->fdt_list)) return NULL;

	return elem;
}

// 0.
void halt(void){
	power_off();
}

// 1.
void exit(int status)
{
    struct thread *curr = thread_current();
    curr->exit_status = status;
    printf("%s: exit(%d)\n", curr->name, status);
    thread_exit();
}

// 2.
int fork(const char *thread_name, struct intr_frame *f)
{
    return process_fork(thread_name,f);
}

// 3.
int exec(const char *cmd_line)
{

	check_address(cmd_line);
	char * cmd_line_copy;
	cmd_line_copy = palloc_get_page(0);
	if (cmd_line_copy == NULL)
			exit(-1);
	strlcpy(cmd_line_copy, cmd_line, PGSIZE);

	// 스레드의 이름을 변경하지 않고 바로 실행한다.
	if (process_exec(cmd_line_copy) == -1)
			exit(-1); // 실패 시 status -1로 종료한다.
}

// 4.
int wait(int pid)
{
    return process_wait(pid);
}

// 5.
bool create(const char *file, unsigned initial_size)
{
    check_address(file);
    return filesys_create(file, initial_size);
}

// 6.
bool remove (const char *file) {
	check_address(file);
	return filesys_remove(file);
}

// 7.
int open (const char *file) {
	// printf("open start\n");
	check_address(file);

	int fd;
	struct thread *cur = thread_current();
	struct file *file_obj = filesys_open(file);
	if(cur->name==NULL || file_obj == NULL) return -1;
	
	// for(fd=2;fd<128,cur->fdt[fd]!=NULL;fd++) continue;

	// if(fd==128){
	// 	file_close(file_obj);
	// 	fd = -1;
	// }
	// else{
	// 	cur->fdt[fd] = file_obj;
	// 	cur->fd = fd;
	// }

	struct file_descriptor * temp = malloc(sizeof(struct file_descriptor));
	temp->fd = cur->fd++;
	temp->file = file_obj;
	list_push_back(&cur->fdt_list, &temp->elem);
	// printf("open end : %d\n",temp->fd);
	return temp->fd;
}

// 8.
int filesize(int fd) {

	if(	fd < 0 || fd >= 128) return -1;

	struct thread * cur = thread_current();
	if(list_empty(&cur->fdt_list)) return -1;
	//struct file *fileobj = cur->fdt[fd];

	struct list_elem * elem = find_file_in_fdt(cur, fd);
	if(elem == NULL) return -1;
	
	struct file *fileobj = list_entry(elem, struct file_descriptor, elem)->file;
	if (fileobj == NULL) return -1;

	return file_length(fileobj);
}

// 9.
int read(int fd, void *buffer, unsigned size) {
	// 유효한 주소인지부터 체크
	unsigned char *buf = buffer;
	int read_count;
	check_address(buffer);

	if(	fd < 0 || fd>=128) return -1;
	if (fd == STDOUT_FILENO) return -1;

	/* STDIN일 때: */
	if (fd == STDIN_FILENO) {
		char key;
		for (read_count = 0; read_count < size; read_count++) {
			key  = input_getc();
			*buf++ = key;
			if (key == '\0') break;
		}
		return read_count;
	}
	
	struct thread *cur = thread_current();
	if(list_empty(&cur->fdt_list)) return -1;
	
	//struct file *fileobj = cur->fdt[fd];
	struct list_elem * elem = find_file_in_fdt(cur, fd);
	if(elem == NULL) return -1;
	
	struct file *fileobj = list_entry(elem, struct file_descriptor, elem)->file;
	if (fileobj == NULL) return -1;
	
	lock_acquire(&filesys_lock);
	read_count = file_read(fileobj, buffer, size);
	lock_release(&filesys_lock);

	
	return read_count;
}



// 10.
int write (int fd, const void *buffer, unsigned size) {
	check_address(buffer);

	// if(	fd < 0 || fd >= 128) return -1;
	if (fd == STDIN_FILENO) return -1;
	if (fd == STDOUT_FILENO) {
		putbuf(buffer, size);
		return size;
	}

	struct thread *cur = thread_current();


	int write_count;

	//struct file *fileobj = cur->fdt[fd];
	if(list_empty(&cur->fdt_list)) return -1;
	
	struct list_elem * elem = find_file_in_fdt(cur, fd);
	if(elem == NULL) return -1;
	struct file *fileobj = list_entry(elem, struct file_descriptor, elem)->file;
	if (fileobj == NULL) return -1;

	
	
	lock_acquire(&filesys_lock);
	write_count = file_write(fileobj, buffer, size);
	lock_release(&filesys_lock);

	return write_count;
}
// 11.
void seek(int fd, unsigned position)
{
	if(	fd < 2 || fd >= 128) return;
	struct thread *cur = thread_current();
	//struct file *fileobj = cur->fdt[fd];

	if(list_empty(&cur->fdt_list)) return;
	struct list_elem * elem = find_file_in_fdt(cur, fd);
	if(elem == NULL) return;
	
	struct file *fileobj = list_entry(elem, struct file_descriptor, elem)->file;
	if (fileobj == NULL) return;

	file_seek(fileobj , position);
}
// 12.
unsigned tell(int fd)
{

	if(	fd < 2 || fd >= 128) return;
	struct thread *cur = thread_current();

	if(list_empty(&cur->fdt_list)) return;
	
	//struct file *fileobj = cur->fdt[fd];
	struct list_elem * elem = find_file_in_fdt(cur, fd);
	if(elem == NULL) return;
	struct file *fileobj = list_entry(elem, struct file_descriptor, elem)->file;
	if (fileobj == NULL) return;


	return file_tell(fileobj);
}

// 13.
void close(int fd)
{
	// printf("close start %d\n",fd);
	if(	fd < 2 || fd >= 128) return;
	struct thread * cur = thread_current();
	if(list_empty(&cur->fdt_list)) return;
	// //struct file *fileobj = cur->fdt[fd];
	struct list_elem * elem = find_file_in_fdt(cur, fd);
	if(elem == NULL) return;
	// printf("find elem !! %d\n",list_entry(elem, struct file_descriptor, elem)->fd);
	list_remove(elem);
	// printf("list remove complete\n");

	struct file *fileobj = list_entry(elem, struct file_descriptor, elem)->file;
	if (fileobj == NULL) return;
	file_close(fileobj);
	// printf("close complete\n");

	struct file_descriptor * temp = list_entry(elem, struct file_descriptor, elem);
	free(temp);
	// printf("free complete\n");
	// cur->fdt[fd] = NULL;
}

// 22.
int dup2(int oldfd, int newfd){

	struct thread * cur = thread_current();
	struct file * file_old = cur->fdt[oldfd];
	struct file * file_new = cur->fdt[newfd];
	if(file_old == NULL) return -1;
	if(file_old == file_new) return newfd;
	if(file_new != NULL) close(newfd);
	cur->fdt[newfd] = file_duplicate(file_old);

	return newfd;
}

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
	lock_init(&filesys_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	int sys_number = f->R.rax; // rax: 시스템 콜 넘버
	// printf("sys_number : %d\n",sys_number);
    /* 
	인자 들어오는 순서:
	1번째 인자: %rdi
	2번째 인자: %rsi
	3번째 인자: %rdx
	4번째 인자: %r10
	5번째 인자: %r8
	6번째 인자: %r9 
	*/
	// TODO: Your implementation goes here.
	switch(sys_number) {
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			f->R.rax = fork(f->R.rdi, f);
			break;		
		case SYS_EXEC:
			f->R.rax = exec(f->R.rdi);
			break;
		case SYS_WAIT:
			f->R.rax = wait(f->R.rdi);
			break;
		case SYS_CREATE:
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = remove(f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize(f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;	
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell(f->R.rdi);
			break;
		case SYS_CLOSE:
			close(f->R.rdi);

		case SYS_DUP2:
			f->R.rax = dup2(f->R.rdi, f->R.rsi);
		default:
			break;
	}
	// printf ("system call!\n");
}
