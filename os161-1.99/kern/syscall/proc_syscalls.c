#include <types.h>
#include <limits.h> // for NAME_MAX
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <kern/limits.h> // for NAME_MAX
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <synch.h> // For lock and cv
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h> // So that it knows trapframe
#include <vfs.h> //vfsopen.close
#include <kern/fcntl.h>	// global not in limit such as O_RDONLY
#include "opt-A2.h"

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  //(void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);


#if OPT_A2
	//p->p_exitcode = _MKWAIT_EXIT(exitcode);
	//p->p_exited = true;
	// Update the proc table
	updaterelation(0, p->p_pid, true, exitcode);

	// Delete itself from the proc table if it don't have parent waiting
	if (p->p_parent == NULL) {
		deleterelation(p->p_pid);
	}
	
	lock_acquire(p->p_waitlk);
	cv_broadcast(p->p_waitcv, p->p_waitlk);
	lock_release(p->p_waitlk);	
#else
#endif /* OPT_A2 */


  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */

  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
#if OPT_A2
	*retval = curproc->p_pid;
#else
  *retval = 1;
#endif /* OPT_A2 */
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }

#if OPT_A2
	struct proc *parent = curproc;
	// If child exit already
	if (childexited(parent->p_pid, pid, true)) {
		exitstatus = getexitcode(pid);
		exitstatus = _MKWAIT_EXIT(exitstatus);
	} else {
		// See if the provided pid is one of the child of the parent (it need to be)
		struct proc *child = NULL;
		for (unsigned int i = 0; i < array_num(parent->p_children); i++) {
			struct proc *temp = array_get(parent->p_children, i);
			if (temp->p_pid == pid) {
				child = temp;
				break;
			}
		}
		if (child == NULL) {
			panic("Provided pid is not the children of the current process");
		}
		// Check if the child exist already
		lock_acquire(child->p_waitlk);
		while (!childexited(parent->p_pid, pid, true)) {
			//exitstatus = getexitcode(pid);
			cv_wait(child->p_waitcv, child->p_waitlk);
		}
		exitstatus = getexitcode(pid);
		exitstatus = _MKWAIT_EXIT(exitstatus);
	}
#else

  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
#endif /* OPT_A2 */
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

#if OPT_A2
	// fork() system call
	int sys_fork(struct trapframe *tf, pid_t *retval) {

		// Create process structure for child process-------------------------
		char *childname = kmalloc(sizeof(char) * NAME_MAX);
    		strcpy(childname, curproc->p_name); // from lib.h
    		strcat(childname, "_Child");  // from lib.h
		struct proc *p_child = proc_create_runprogram(childname);
		// Check
		if (p_child == NULL) {
        		kfree(childname);
        		return ENOMEM;
    		}
		//--------------------------------------------------------------------
		
		// Create and copy address space-------------------------------------
		struct addrspace *as_child = kmalloc(sizeof(struct addrspace));
		if (as_child == NULL) {
			kfree(childname);
			proc_destroy(p_child);
			return ENOMEM;
		}
		int rv = as_copy(curproc->p_addrspace, &as_child);
		// Check
		if (rv) {
			kfree(childname);
			kfree(as_child);
			proc_destroy(p_child);
			return rv;
		}
		//----------------------------------------------------------------------

		// Attach the newly created address space to child
		spinlock_acquire(&(p_child->p_lock));
		p_child->p_addrspace = as_child;
		spinlock_release(&(p_child->p_lock));
		//----------------------------------------------------------------------------

		// Assign PID to child process and create parent/child relationship----------
		// PID is assigned when proc_create_runprogram is called
		spinlock_acquire(&(p_child->p_lock));
		p_child->p_parent = curproc;
		// update the proc table for the child
		updaterelation(curproc->p_pid, p_child->p_pid, false, 0);
		array_add(curproc->p_children, p_child, NULL);	
		spinlock_release(&(p_child->p_lock));
		//---------------------------------------------------------------------------

		// Create trapframe and thread for child process------------------------------
		struct trapframe *tf_child = kmalloc(sizeof(struct trapframe));                                                      
		if (tf_child == NULL) {
			kfree(childname);
			kfree(tf_child);
			as_destroy(as_child);
			proc_destroy(p_child);
			return ENOMEM;
		}
		memcpy(tf_child, tf, sizeof(struct trapframe)); // from lib.h

		rv = thread_fork(childname, p_child, &enter_forked_process, tf_child, 0);
		// Check
		if (rv) {
			kfree(childname);
			kfree(tf_child);
			as_destroy(as_child);
			proc_destroy(p_child);
			return ENOMEM;
		}

		*retval = p_child->p_pid;
		return (0);
		//-----------------------------------------------------------------------------			
	}

	// exec() system call
	int sys_execv(const_userptr_t path, userptr_t argv) {
		
		char *progname = (char *)path;
		char **args = (char **)argv;
		//(void)argv;

		if (progname == NULL) {
			return EFAULT;
		}

		// Count the number of argv and copy them into the kernel----------------------
		int argc = 0;
		int result;

		// count the argument
		while (args[argc] != 0) {
			argc++;
		}

		// copy them into the kernel
		char **k_args = kmalloc((argc+1) * sizeof(char));
		for (int i = 0; i < argc; i++) {
			k_args[i] = kmalloc((strlen(args[i])+1) * sizeof(char));
			result = copyinstr((userptr_t)args[i], k_args[i], strlen(args[i]) + 1, NULL);
			if (result) {
				return result;
			}
		}
		k_args[argc] = NULL;		
		//-----------------------------------------------------------------------------

		// Copy the program path into the kernel---------------------------------------
		size_t p_length = strlen(progname) + 1;
		char * k_prog = kmalloc(sizeof(char *) * p_length);
                result = copyinstr((userptr_t)progname, k_prog, p_length, NULL);

                if (k_prog == NULL) {
                        return ENOMEM;
                }

                if (result) {
                        return result;
                }
		//-----------------------------------------------------------------------------

		// Open the program file using vfs_open(progname, ....)------------------------
		char *temp;
		struct vnode *v;
		temp = kstrdup(progname);
		result = vfs_open(temp, O_RDONLY, 0, &v);	// Direct copy from runprogram.c
		if (result) {
			return result;
		} 
		//-----------------------------------------------------------------------------

		// Create new address space, set process to the new address space, and activate it
		/* We should be an existing process. */
                KASSERT(curproc_getas() != NULL);
		
                /* Create a new address space. */
                struct addrspace *as = as_create();
                if (as == NULL) {
                        vfs_close(v);
                        return ENOMEM;
                }
                /* Switch to it and activate it. */
                struct addrspace *as_old = curproc_setas(as);
                as_activate();
		//--------------------------------------------------------------------------------

		// Using the opened program file, load the program image using load_elf--------
		vaddr_t entrypoint;	//------------------------------------------------------------------------------------------------------
		/* Load the executable. */
                result = load_elf(v, &entrypoint);
                if (result) {
                        /* p_addrspace will go away when curproc is destroyed */
                        vfs_close(v);
			curproc_setas(as_old);
                        return result;
                }
                /* Done with the file now. */
                vfs_close(v);
		//-----------------------------------------------------------------------------

		// Copy the arguments into the new address space-------------------------------
		vaddr_t stackptr;
		/* Define the user stack in the address space */
                result = as_define_stack(as, &stackptr);
                if (result) {
                        /* p_addrspace will go away when curproc is destroyed */
			curproc_setas(as_old);
                        return result;
                }

		stackptr = ROUNDUP(stackptr, 8);

        	vaddr_t arg_p[argc+1];

	        for (int i = (argc - 1); i >= 0; i--) {
        	        char *s = k_args[i];
	                int l = strlen(s) + 1;
        	        stackptr = stackptr - l;
	                result = copyoutstr(s, (userptr_t)stackptr, l, NULL);
	                if (result) {
	                        return result;
	                }
	                arg_p[i] = stackptr;
	       	}

	        while ((stackptr % 4) != 0) {
	                stackptr--;
        	}

	        arg_p[argc] = 0;

        	for (int i = argc; i >= 0; i--) {
                	int padding = ROUNDUP(sizeof(vaddr_t), 4);
	                stackptr = stackptr - padding;
        	        result = copyout(&arg_p[i], (userptr_t)stackptr, sizeof(vaddr_t));
			if (result) {
				return result;
			}
	        }
		//-----------------------------------------------------------------------------

		// Delete the old address space -----------------------------------------------
		as_destroy(as_old);
		//-----------------------------------------------------------------------------

		
		// Cal enter new process with the specific argument----------------------------
		/* Warp to user mode. */
                enter_new_process(argc, (userptr_t)stackptr, stackptr, entrypoint);

		//enter_new_process(0, NULL, stackptr, entrypoint);

                /* enter_new_process does not return. */
                panic("enter_new_process returned\n");
                return EINVAL;
		//-----------------------------------------------------------------------------
	}
#else
#endif /* OPT_A2 */
