#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
/* function for store arguments into the stack frame */ 
void argument_stack(char **, int, void**);
/* Functions for Proj3 */
struct thread *get_child_process (int pid);
void remove_child_process (struct thread *cp);

#endif /* userprog/process.h */
