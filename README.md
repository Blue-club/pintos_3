# Project2

# Project 2

---

## 할 일

1. system call이 호출되는 과정 이해
2. Passing argument 구현
3. system call 구현

---

## 목표 일정

### 6/1 ~ 6/3

1. Project2 구현에 있어 필요한 지식 공부
2. system call이 호출되는 과정에 대해 이해

### 6/4 ~ 6/5

1. Passing argument 구현

### 6/6 ~ 6/9

1. system call 구현

### 6/10 ~ 6/11

1. 배운 내용 정리
2. 구현 중 어려웠던 내용 정리하며 Readme 파일에 기재
3. 발표 시간 7분에 맞춰 준비

---

## 구현 내용

### 6/4 ~ 6/5 - argument passing 구현

![Untitled](Project2%20aab3fd3a18364188909879e084d32eff/Untitled.png)

### 6/6 ~ 6/9 - system call 구현

![화면 캡처 2023-06-12 013631.png](Project2%20aab3fd3a18364188909879e084d32eff/%25ED%2599%2594%25EB%25A9%25B4_%25EC%25BA%25A1%25EC%25B2%2598_2023-06-12_013631.png)

### 6/10 ~ 6/11 extra 도전

file descriptor를 linked list로 바꾸다가 실패

---

## 회고

### 김용현

![Untitled](Project2%20aab3fd3a18364188909879e084d32eff/Untitled.png)

hex_dump() 를 이용하여 스택에 쌓인 목록을 확인하였다.

리틀 에디안 방식을 사용했기 때문에 주소는 거꾸로 저장이 되었다.

하지만 문자열의 경우 원래 순서 그대로 쌓인 것을 볼 수 있다.

찾아보았더니 char 형의 경우 1byte이기 때문에 리틀 에디안 방식에 위반되지 않는다고 하였는데 막 와닿지는 않는다.

![Untitled](Project2%20aab3fd3a18364188909879e084d32eff/Untitled%201.png)

file descriptor를 linked list로 바꾸다가 실패하였다.

신기하게 나와 똑같은 구조로 구현한 사람이 있어서 구현할 시간이 있다면 참고하여 구현해보고 싶다.

### 최 원

이 두 문제가 User Programs를 진행하면서 고민을 했었던 구간이라고 생각합니다.

문제 1.

![화면 캡처 2023-06-12 035047.png](Project2%20aab3fd3a18364188909879e084d32eff/%25ED%2599%2594%25EB%25A9%25B4_%25EC%25BA%25A1%25EC%25B2%2598_2023-06-12_035047.png)

process_fork에서 __do_fork에 들어가기 전에 intr_frame의 값인 if_를 복사해주는 이유에 대해서 의문점이 생겼다.
__do_fork에 들어가기 전과 후에 printf를 찍어보니 두 intr_frame의 값이 다른 것을 확인할 수 있다.
검색해보니 fork를 수행하면서 context switch가 일어나서 부모 스레드의 intr_frame에는 커널에 작업하던 정보가 저장이 되있고
자식에게는 user영역에서 작업하던 정보를 넘겨줘야 한다고 한다.

문제 2.

그리고 process_fork에는 sema_down를 해주고 __do_fork에는 sema_up를 해준다
문제 : 코드 진행상 __do_fork가 먼저 실행되는데 sema_up를 해주면 코드 진행이 안돼지 않을까?

![화면 캡처 2023-06-12 035123.png](Project2%20aab3fd3a18364188909879e084d32eff/%25ED%2599%2594%25EB%25A9%25B4_%25EC%25BA%25A1%25EC%25B2%2598_2023-06-12_035123.png)

![화면 캡처 2023-06-12 035154.png](Project2%20aab3fd3a18364188909879e084d32eff/%25ED%2599%2594%25EB%25A9%25B4_%25EC%25BA%25A1%25EC%25B2%2598_2023-06-12_035154.png)

fork를 해서 부모 프로세스와 자식프로세스가 있으므로 2개의 프로세스가 함께 실행되면서 부모 프로세스는 sema_down을 해주고
자식프로세스를 기다린다(핀토스에서는 단일 CPU로 프로세스가 하나씩 실행되서 자식프로세스가 부모프로세스를 먼저 기다릴 가능성도 있다.)
복제가 끝이나면 sema_up를 해주어서 두 프로세스가 실행이 된다

### 김동윤

스택 형식으로 저장되는 과정이 이해되지 않아 argument passing에서 많은 시간이 소요되었다.

```c
tid_t process_create_initd(const char *file_name)
{
	char *fn_copy;			//filename을 복사하기 위한 임시 버퍼
	tid_t tid;

...

	strlcpy(fn_copy, file_name, PGSIZE);		//문자열을 복사
	char *save_ptr;								
	strtok_r(file_name, " ", &save_ptr);		
//문자열을 " " 기준으로 분리
	tid = thread_create(file_name, PRI_DEFAULT, initd, fn_copy);

...
}
```

 name 만들어진 스레드 이름  `여기서는 파싱된 file_name의 첫번째 입력만 사용됨`
 priority 기본 우선순위
 function 실행할 함수 이름
 aux 실행할 함수에 들어갈 매개변수 `파싱되지 않은 fn_copy가 매개변수로 들어감`

```c
static void
initd(void *f_name) ////f_name에 fn_copy가 들어감
{
...

	if (process_exec(f_name) < 0)               
		PANIC("Fail to launch initd\n");
	NOT_REACHED();
}
```

```c
int
process_exec (void *f_name) {                 //f_name = fn_copy(파싱되지 않은 초기 입력)
	char *file_name = f_name;

...

	success = load (file_name, &_if);           //file_name = fn_copy

...
}
```

```c
load (const char *file_name, struct intr_frame *if_) {

// 파싱되지 않았던 fn_copy가 여기서 파싱, argv 배열에 저장함
	for(token = strtok_r(temp," ",&save_ptr); token!=NULL; 
token=strtok_r(NULL," ",&save_ptr),argc++) argv[argc] = token;

	process_activate (thread_current ());

	/* Open executable file. */
	file = filesys_open (argv[0]);       //파싱된 argv[0](파일이름)을 통해 파일을 열기
...
	}
```

argument passing 함수를 통해 사용자 스택에 저장

```c
void argument_passing(char ** argv, int argc, struct intr_frame *if_){
	char * temp[64];
	int len;
//인자를 스택에 복사
	for(int i = argc - 1; i>=0; i--){
		len = strlen(argv[i]);
		if_->rsp = if_->rsp - (len + 1);      
		memcpy(if_->rsp, argv[i], len+1);             //인자의 길이만큼 감소하며 저장
		temp[i] = if_->rsp;
	}
//스택 포인터가 8의 배수가 되도록
	while(if_->rsp % 8 !=0){  
		if_->rsp--;
		*(uint8_t *) if_->rsp = 0;
	}
	
//인자 주소 저장하며 감소
	for(int i = argc; i>=0; i--){
		if_->rsp-=8;
		if(i == argc) memset(if_->rsp, 0, sizeof(char**));
		else memcpy(if_->rsp, &temp[i],sizeof(char**));
	}
	if_->rsp = if_->rsp - 8;
	memset(if_->rsp, 0, 8);                 //argc에 인자의 개수 복사

	if_->R.rdi = argc;
  if_->R.rsi = if_->rsp + 8;
}
```

의 과정을 통해 gitbook 자료의 모습이 되는 것을 확인하였다.

| Address | Name | Data | Type |
| --- | --- | --- | --- |
| 0x4747fffc | argv[3][...] | 'bar\0' | char[4] |
| 0x4747fff8 | argv[2][...] | 'foo\0' | char[4] |
| 0x4747fff5 | argv[1][...] | '-l\0' | char[3] |
| 0x4747ffed | argv[0][...] | '/bin/ls\0' | char[8] |
| 0x4747ffe8 | word-align | 0 | uint8_t[] |
| 0x4747ffe0 | argv[4] | 0 | char * |
| 0x4747ffd8 | argv[3] | 0x4747fffc | char * |
| 0x4747ffd0 | argv[2] | 0x4747fff8 | char * |
| 0x4747ffc8 | argv[1] | 0x4747fff5 | char * |
| 0x4747ffc0 | argv[0] | 0x4747ffed | char * |
| 0x4747ffb8 | return address | 0 | void (*) () |
