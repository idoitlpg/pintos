#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/* Header files for syscall implementation */
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
/* Header files for Project.4 */
#include "devices/input.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
/* functions for syscall implementation */
void check_address (void *addr);
void get_argument (void *esp, int *arg, int count);
void halt(void);
void exit(int status);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
tid_t exec (const char *cmd_line);
int wait (tid_t tid);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

/* User Address Limit */
#define USER_START 0x08048000

void check_address (void *addr)
{
  /* check boundary of address */
  if (addr >= (void*)PHYS_BASE || addr < (void*)USER_START)
  {
    exit (-1);
  }
}

/* copy arguments from stack to kernel space with count */
void get_argument (void *esp, int *arg, int count)
{
  int i = 0;
  int *pointer = NULL;

  for (i=0; i < count; i++)
  {
    pointer = (int*)esp + i + 1;
    check_address (pointer);
    arg[i] = *pointer;
  }
}

/* system power off */
void halt(void)
{
  printf("system halt\n");
  shutdown_power_off();
}

/* process exit */
void exit(int status)
{
  /* get pointer of current thread */
  struct thread *current = thread_current();
  current->thread_exit_status = status;
  /* print the name and status of current thread */
  printf("%s: exit(%d)\n", current->name, status);
  /* exit the thread */
  thread_exit();
}

/* Create File */
bool create(const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

/* Remove File */
bool remove(const char *file)
{
  return filesys_remove(file);
}

int open (const char *file)
{
  struct file *f = filesys_open (file);
  if (!f) {
    return -1;
  }

  return process_add_file(f);
}

int filesize (int fd)
{
  struct file *f = process_get_file (fd);
  if (!f)
    return -1;

  return file_length(f);
}

int read (int fd, void *buffer, unsigned size)
{
  unsigned i = 0;

  if (fd == STDOUT_FILENO)
    return -1;

  if (fd == STDIN_FILENO)
  {
    uint8_t *local_buffer = (uint8_t*) buffer;

    for (i = 0; i < size; i++)
    {
      local_buffer[i] = input_getc();
    }
  return size;
  }

  lock_acquire (&filesys_lock);

  struct file *f = process_get_file (fd);
  if (!f)
  {
    lock_release (&filesys_lock);
    return -1;
  }

  int bytes = file_read (f, buffer, size);

  lock_release (&filesys_lock);

  return bytes;
}

int write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDIN_FILENO)
    return -1;

  if (fd == STDOUT_FILENO)
  {
    putbuf ((const char*)buffer, size);
    return size;
  }

  lock_acquire (&filesys_lock);

  struct file *f = process_get_file (fd);
  if (!f)
  {
    lock_release (&filesys_lock);
    return -1;
  }

  int bytes = file_write (f, buffer, size);

  lock_release (&filesys_lock);

  return bytes;
}

void seek (int fd, unsigned position)
{
  struct file *f = process_get_file (fd);
  if (!f)
    return ;

  file_seek(f, position);
}

unsigned tell (int fd)
{
  struct file *f = process_get_file (fd);
  if (!f)
    return -1;

  return file_tell(f);
}

void close (int fd)
{
  process_close_file (fd);
}


/* Execute Process */
tid_t exec (const char *cmd_line)
{
  struct thread *child = NULL;
  tid_t child_pid = 0;

  /* Execute process with cmd_line */
  child_pid = process_execute (cmd_line);
  /* get Process context with pid */
  child = get_child_process (child_pid);
  if (!child)
    return -1;

  /* Wait for loading child process */
  if (child->thread_loaded == NO_LOAD)
  {
    sema_down(&child->sema_load);
  }

  /* Return error for exception */
  if (child->thread_loaded == FAIL_LOAD)
    return -1;

  /* Return pid of child process */
  return child_pid;
}

/* Wait for child process */
int wait (tid_t tid)
{
  return process_wait(tid);
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  lock_init (&filesys_lock);
}

/* Handler for syscall */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{

  /* syscall number */
  int syscall_num;
  /* argument for stack */
  int arg[3] = {0,};
  /* pointer for stack */
  int *esp = f->esp;

  /* boundary check for the user address */
  check_address (esp);

  syscall_num = *esp;

  //printf("syscall %d\n", syscall_num);

  switch (syscall_num) 
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      get_argument (esp, arg, 1);
      exit(arg[0]);
      f->eax = arg[0];
      break;
    case SYS_EXEC:
      get_argument (esp, arg, 1);
      check_address ((void*)arg[0]);
      f->eax = exec ((const char*)arg[0]);
      break;
    case SYS_WAIT:
      get_argument (esp, arg, 1);
      f->eax = wait (arg[0]);
      break;
    case SYS_CREATE:
      get_argument (esp, arg, 2);
      check_address ((void*)arg[0]);
      f->eax = create ((const char*)arg[0], (unsigned)arg[1]);
      break;
    case SYS_REMOVE:
      get_argument (esp, arg, 1);
      check_address ((void*)arg[0]);
      f->eax = remove ((const char*)arg[0]);
      break;
    case SYS_OPEN:                   /* Open a file. */
      get_argument (esp, arg, 1);
      check_address ((void*)arg[0]);
      f->eax = open ((const char*)arg[0]);
      break;
    case SYS_FILESIZE:               /* Obtain a file's size. */
      get_argument (esp, arg, 1);
      f->eax = filesize (arg[0]);
      break; 
    case SYS_READ:                   /* Read from a file. */
      get_argument (esp, arg, 3);
      check_address ((void*)arg[1]);
      f->eax = read ((int)arg[0], (void*)arg[1], (unsigned)arg[2]);
      break;
    case SYS_WRITE:                  /* Write to a file. */
      get_argument (esp, arg, 3);
      check_address ((void*)arg[1]);
      f->eax = write ((int)arg[0], (const void*)arg[1], (unsigned)arg[2]);
      break;
    case SYS_SEEK:                   /* Change position in a file. */
      get_argument (esp, arg, 2);
      seek (arg[0], (unsigned) arg[1]);
      break;
    case SYS_TELL:                   /* Report current position in a file. */
      get_argument (esp, arg, 1);
      f->eax = tell (arg[0]);
      break;
    case SYS_CLOSE:                  /* Close a file. */
      get_argument (esp, arg, 1);
      close (arg[0]);
      break;

    default:
      printf("Syscall Not supported [%d]\n", syscall_num);
      thread_exit ();
  }
}
