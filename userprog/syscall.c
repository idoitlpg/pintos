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

/* User Address Limit */
#define USER_START 0x8048000

void check_address (void *addr)
{
  /* check boundary of address */
  if (addr > (void*)PHYS_BASE || addr < (void*)USER_START)
    exit (-1);
}

/* copy arguments from stack to kernel space with count */
void get_argument (void *esp, int *arg, int count)
{
  int i = 0;
  char *pointer = NULL;

  for (i=0; i < count; i++)
  {
    pointer = (char*)esp + i + 1;
    check_address (pointer);
    arg[i] = *pointer;
  }
}

/* system power off */
void halt(void)
{
  printf("HALT\n");
  shutdown_power_off();
}

/* process exit */
void exit(int status)
{
  /* get pointer of current thread */
  struct thread *current = thread_current();
  current->thread_exit_status = status;
  /* print the name and status of current thread */
  printf("%s: EXIT (%d)\n", current->name, status);
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
  if (!child->thread_loaded)
  {
    sema_down(&child->sema_load);
  }

  /* Return error for exception */
  if (child->thread_loaded == -1)
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
}

/* Handler for syscall */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");

  /* syscall number */
  int syscall_num;
  /* argument for stack */
  int arg[3] = {0,};
  /* pointer for stack */
  char *esp = f->esp;

  /* boundary check for the user address */
  check_address (esp);

  syscall_num = *esp;

  switch (syscall_num) 
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      f->eax = arg[0];
      break;
    case SYS_EXEC:
      get_argument (esp, arg, 1);
      check_address ((void*)arg[0]);
      f->eax = exec ((const char*)arg[0]);
      break;
    case SYS_WAIT:
      get_argument (esp, arg, 1);
      check_address ((void*)arg[0]);
      f->eax = wait ((int)arg[0]);
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
    default:
      printf("Syscall Not supported [%d]\n", syscall_num);
  }

  thread_exit ();
}
