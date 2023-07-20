/*
 *  linux/init/main.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__

#include <unistd.h>
#include <time.h>

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
static inline _syscall0(int, fork);

static inline _syscall0(int, pause);

static inline _syscall1(int, setup, void *, BIOS);

static inline _syscall0(int, sync);

static inline _syscall3(int, read, int, fd, char *, buf, int, count);

static inline _syscall1(int, chdir, const char *, filename);

#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <asm/system.h>
#include <asm/io.h>

#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <linux/fs.h>

#include <errno.h>

#define NAME_LEN 256
#define CMD_LEN 256

static char printbuf[1024];

extern int vsprintf();

extern void init(void);

extern void blk_dev_init(void);

extern void chr_dev_init(void);

extern void hd_init(void);

extern void floppy_init(void);

extern void mem_init(long start, long end);

extern long rd_init(long mem_start, int length);

extern long kernel_mktime(struct tm *tm);

extern long startup_time;

extern int read_line(char *buf, int maxlength);

extern void myshell(void);

void mycd(char *target_path);
int myls(void);
int mycat(char *filename);

void exec_cp(char *command, int len);
void exec_vi(char *command, int len);
void exec_ls(char *command, int len);
char cwd[NAME_LEN + 1];

/*
 * This is set up by the setup-routine at boot-time
 */
#define EXT_MEM_K (*(unsigned short *)0x90002)
#define DRIVE_INFO (*(struct drive_info *)0x90080)
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */

#define CMOS_READ(addr) ({     \
	outb_p(0x80 | addr, 0x70); \
	inb_p(0x71);               \
})

#define BCD_TO_BIN(val) ((val) = ((val)&15) + ((val) >> 4) * 10)

static void time_init(void)
{
	struct tm time;

	do
	{
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
	time.tm_mon--;
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;

struct drive_info
{
	char dummy[32];
} drive_info;

void main(void) /* This really IS void, no error here. */
{				/* The startup routine assumes (well, ...) this */
	/*
	 * Interrupts are still disabled. Do necessary setups, then
	 * enable them
	 */
	ROOT_DEV = ORIG_ROOT_DEV;
	drive_info = DRIVE_INFO;
	memory_end = (1 << 20) + (EXT_MEM_K << 10);
	memory_end &= 0xfffff000;
	if (memory_end > 16 * 1024 * 1024)
		memory_end = 16 * 1024 * 1024;
	if (memory_end > 12 * 1024 * 1024)
		buffer_memory_end = 4 * 1024 * 1024;
	else if (memory_end > 6 * 1024 * 1024)
		buffer_memory_end = 2 * 1024 * 1024;
	else
		buffer_memory_end = 1 * 1024 * 1024;
	main_memory_start = buffer_memory_end;
#ifdef RAMDISK
	main_memory_start += rd_init(main_memory_start, RAMDISK * 1024);
#endif
	mem_init(main_memory_start, memory_end);
	trap_init();
	blk_dev_init();
	chr_dev_init();
	tty_init();
	time_init();
	sched_init();
	buffer_init(buffer_memory_end);
	hd_init();
	floppy_init();
	sti();
	move_to_user_mode();
	if (!fork())
	{ /* we count on this going ok */
		myshell();
		// init();
		// test();
	}
	/*
	 *   NOTE!!   For any other task 'pause()' would mean we have to get a
	 * signal to awaken, but task0 is the sole exception (see 'schedule()')
	 * as task 0 gets activated at every idle moment (when no other tasks
	 * can run). For task0 'pause()' just means we go check if some other
	 * task can run, and if not we return here.
	 */
	for (;;)
		pause();
}

static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1, printbuf, i = vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

static char *argv_rc[] = {"/bin/sh", NULL};
static char *envp_rc[] = {"HOME=/", NULL};

static char *argv[] = {"-/bin/sh", NULL};
static char *envp[] = {"HOME=/usr/root", NULL};

void init(void)
{
	int pid, i;

	setup((void *)&drive_info);
	(void)open("/dev/tty0", O_RDWR, 0);
	(void)dup(0);
	(void)dup(0);
	printf("%d buffers = %d bytes buffer space\n\r", NR_BUFFERS,
		   NR_BUFFERS * BLOCK_SIZE);
	printf("Free mem: %d bytes\n\r", memory_end - main_memory_start);
	if (!(pid = fork()))
	{
		close(0);
		if (open("/etc/rc", O_RDONLY, 0))
			_exit(1);
		execve("/bin/sh", argv_rc, envp_rc);
		_exit(2);
	}
	if (pid > 0)
		while (pid != wait(&i))
			/* nothing */;
	while (1)
	{
		if ((pid = fork()) < 0)
		{
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid)
		{
			close(0);
			close(1);
			close(2);
			setsid();
			(void)open("/dev/tty0", O_RDWR, 0);
			(void)dup(0);
			(void)dup(0);
			_exit(execve("/bin/sh", argv, envp));
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r", pid, i);
		sync();
	}
	_exit(0); /* NOTE! _exit, not exit() */
}

void print_hello(void)
{
	printf("  _              _   _                               _ \n");
	printf(" | |__     ___  | | | |   ___           ___    ___  | |\n");
	printf(" | '_ \\   / _ \\ | | | |  / _ \\         / _ \\  / __| | |\n");
	printf(" | | | | |  __/ | | | | | (_) |  _    | (_) | \\__ \\ |_|\n");
	printf(" |_| |_|  \\___| |_| |_|  \\___/  ( )    \\___/  |___/ (_)\n");
	printf("                                |/                      \n");
}

void myshell(void)
{
	char buf[CMD_LEN + 1];
	cwd[0] = '/';
	cwd[1] = '\0';
	setup((void *)&drive_info);
	(void)open("/dev/tty0", O_RDWR, 0);
	(void)dup(0);
	(void)dup(0);
	print_hello();
	while (1)
	{
		printf("myshell: %s $ ", cwd);
		read_line(buf, 256);
		run_command(buf);
	}
}

int read_line(char *buf, int maxlength)
{
	int i = 0;
	char c;
	while (i < maxlength - 1)
	{
		if (read(0, &c, 1) != 1)
		{
			printf("read error\n");
			return -1;
		}
		if (c == '\n')
		{
			buf[i] = '\0';
			break;
		}
		else
		{
			buf[i] = c;
			i++;
		}
	}
	return i;
}

int run_command(char *command)
{
	int res;
	int pid;
	int len = strlen(command);
	if (len == 0)
	{
		return 1;
	}
	else if (!strncmp(command, "cd", 2))
	{
		mycd(command + 3);
	}
	else if (!strcmp(command, "sync"))
	{
		sync();
	}
	else if (!strcmp(command, "pwd"))
	{
		printf("%s\n", cwd);
	}
	else if (!strcmp(command, "exit"))
	{
		_exit(0);
	}
	else if (!strncmp(command, "cat", 3))
	{
		mycat(command + 4);
	}
	else // outside command
	{
		if (!(pid = fork()))
		{
			if (!strncmp(command, "cp", 2))
			{
				exec_cp(command, len);
			}
			else if (!strncmp(command, "vi", 2))
			{
				exec_vi(command, len);
			}
			else if (!strcmp(command, "ls"))
			{
				exec_ls(command, len);
			}
			else
			{
				printf("command not found\n");
				return -1;
			}
			_exit(2);
		}
		if (pid > 0)
			while (pid != wait(NULL))
				;
	}

	return 1;
}

void mycd(char *target_path)
{
	int res = chdir(target_path);
	int abs = 0;
	printf("chdir res: %d\n", res);
	if (res == -1)
	{
		printf("chdir error\n");
		return;
	}
	else
	{
		char res_path[NAME_LEN + 1];
		if (target_path[0] == '/')
		{
			strcpy(res_path, target_path);
			abs = 1;
		}

		char *relative_path;
		relative_path = strtok(target_path, "/");
		while (relative_path != NULL)
		{

			printf("relative_path: %s\n", relative_path);
			if (!strcmp(relative_path, "."))
			{
				printf("cwd: %s\n", cwd);
			}
			else if (!strcmp(relative_path, ".."))
			{
				if (strcmp(cwd, "/"))
				{ // not top directory
					int i = strlen(cwd) - 1;
					while (cwd[i] != '/')
					{
						i--;
					}
					if (i == 0)
					{
						cwd[i + 1] = '\0';
					}
					else
					{
						cwd[i] = '\0';
					}
				}
				printf("cwd: %s\n", cwd);
			}
			else
			{
				if (abs == 1)
				{
					cwd[0] = '/';
					strcpy(cwd + 1, relative_path);
					abs = 0;
				}
				else
				{
					if (strcmp(cwd, "/"))
					{
						strcat(cwd, "/");
					}

					strcat(cwd, relative_path);
				}

				printf("cwd: %s\n", cwd);
			}
			relative_path = strtok(NULL, "/");
		}
	}
}

int myls(void)
{
	printf("cwd: %s\n", cwd);
	struct dir_entry *de;
	struct buffer_head *bh;
	struct m_inode *dir;
	int i, num_entries;

	char name[NAME_LEN + 1];

	int res = chdir(cwd);
	printf("res: %d\n", res);

	namei(cwd);
	printf("2\n");
	if (!(dir = namei(cwd))) // You may change "/" to any other directory you want to list
		return -ENOENT;		 // No such file or directory
	printf("3\n");
	if (!S_ISDIR(dir->i_mode))
	{
		iput(dir);
		return -ENOTDIR; // Not a directory
	}

	num_entries = dir->i_size / sizeof(struct dir_entry);
	printf("num_entries: %d\n", num_entries);
	for (i = 0; i < num_entries; i++)
	{
		if (!(bh = bread(dir->i_dev, bmap(dir, i))))
		{
			iput(dir);
			return -EIO; // I/O error
		}
		de = (struct dir_entry *)bh->b_data + i % (BLOCK_SIZE / sizeof(struct dir_entry));
		memcpy(name, de->name, NAME_LEN);
		name[NAME_LEN] = '\0';
		printk("%s\n", name);
		brelse(bh);
	}

	iput(dir);
	return 0;
}

int mycat(char *filename)
{
	char file_path[NAME_LEN + 1];
	char file_buf[1024];
	strcpy(file_path, cwd);
	strcat(file_path, filename);
	printf("file_path: %s\n", file_path);
	int fd = open(file_path, O_RDONLY, 0);
	if (fd == -1)
	{
		printf("open error\n");
		return -1;
	}
	else
	{
		int len = 0;
		while ((len = read(fd, file_buf, 1024)) > 0)
		{
			write(1, file_buf, len);
		}
	}
	close(fd);
	return 0;
}
void exec_cp(char *command, int len)
{
	int i = 0;
	for (i = 3; i < len; i++)
	{
		if (command[i] == ' ')
		{
			command[i] = '\0';
			break;
		}
	}
	printf("f1: %s\n", command + 3);
	printf("f2: %s\n", command + i + 1);
	char *cp_argv[] = {"/usr/bin/cp", command + 3, command + i + 1, NULL};
	char *cp_envp[] = {"PATH=/usr/root", NULL};
	execve("/usr/bin/cp", cp_argv, cp_envp);
}

void exec_vi(char *command, int len)
{
	int i = 0;
	for (i = 3; i < len; i++)
	{
		if (command[i] == ' ')
		{
			command[i] = '\0';
			break;
		}
	}
	printf("filename: %s\n", command + 3);
	char *vi_argv[] = {"/bin/vi", command + 3, NULL};
	char *vi_envp[] = {"HOME=/usr/root", "TERM=console", NULL};
	execve("/bin/vi", vi_argv, vi_envp);
}

void exec_ls(char *command, int len)
{
	char *ls_argv[] = {"/usr/bin/ls", NULL};
	char *ls_envp[] = {"PATH=/usr/root", NULL};
	execve("/usr/bin/ls", ls_argv, ls_envp);
}
