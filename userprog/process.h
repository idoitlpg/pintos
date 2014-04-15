#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

#define NO_LOAD 0
#define SUCCESS_LOAD 1
#define FAIL_LOAD 2

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
/* function for store arguments into the stack frame */ 
void argument_stack(char **, int, void**);
/* Functions for Proj3 */
struct thread *get_child_process (int pid);
void remove_child_process (struct thread *cp);
/* Functions for Project.4 */
int process_add_file (struct file *f);
struct file *process_get_file (int fd);
void process_close_file (int fd);

#endif /* userprog/process.h */
