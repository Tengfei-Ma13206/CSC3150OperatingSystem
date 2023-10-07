#include <linux/module.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/jiffies.h>
#include <linux/kmod.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

struct wait_opts {
	enum pid_type wo_type;
	int wo_flags;
	struct pid *wo_pid;
	struct waitid_info *wo_info;
	int wo_stat;
	struct rusage *wo_rusage;
	wait_queue_entry_t child_wait;
	int notask_error;
};

extern pid_t kernel_clone(struct kernel_clone_args *kargs);
extern pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags);
extern int do_execve(struct filename *filename, const char __user *const __user *__argv, const char __user *const __user *__envp);
extern struct filename *getname_kernel(const char *filename);
extern long do_wait(struct wait_opts *wo);

int WEXITSTATUS(int status)
{
	return (((status)&0xff00) >> 8);
}

int WSTOPSIG(int status)
{
	return (((status)&0xff00) >> 8);
}

int WTERMSIG(int status)
{
	return ((status)&0x7f);
} 

bool WIFEXITED(int status) 
{
	return (WTERMSIG(status) == 0);
}

bool WIFSTOPPED(int status)
{
	return (((status)&0xff) == 0x7f);
} 

bool WIFSIGNALED(int status) //?
{
	return (((signed char)(((status)&0x7f) + 1) >> 1) > 0);
}

int iexec(void *unused)
{
	/* execute a test program in child process */
	int return_value;
	const char *path = "/tmp/test";

	printk("[program2] : child process\n");

	return_value = do_execve(getname_kernel(path), NULL, NULL);

	if (!return_value) { return 0; }
	do_exit(return_value);
}

//implement fork function
int my_fork(void *argc){
	char *sig[] = { NULL, "SIGHUP", "SIGINT", "SIGQUIT",
		    "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS",
		    "SIGFPE", "SIGKILL", NULL, "SIGSEGV",
		    NULL, "SIGPIPE", "SIGALRM", "SIGTERM" };
	pid_t pid;	
	int status;
	struct wait_opts wo;
	struct pid *wo_pid = NULL;
	//set default sigaction for current process
	int i;
	struct k_sigaction *k_action = &current->sighand->action[0];
	for(i=0;i<_NSIG;i++){
		k_action->sa.sa_handler = SIG_DFL;
		k_action->sa.sa_flags = 0;
		k_action->sa.sa_restorer = NULL;
		sigemptyset(&k_action->sa.sa_mask);
		k_action++;
	}
	
	/* fork a process using kernel_clone or kernel_thread */
	pid = kernel_thread(&iexec, NULL, SIGCHLD);

	printk("[program2] : The child process has pid = %d\n", pid);
	printk("[program2] : This is the parent process, pid = %d\n", (int)current->pid);
	
	/* wait until child process terminates */
	wo_pid = find_get_pid(pid);

	wo.wo_type = PIDTYPE_PID;
	wo.wo_pid = wo_pid;
	wo.wo_flags = WEXITED | WUNTRACED;
	wo.wo_info = NULL;
	wo.wo_stat = 0;
	wo.wo_rusage = NULL;

	do_wait(&wo);

	status = wo.wo_stat;

	put_pid(wo_pid);


	if (WIFEXITED(status))//
	{
		printk("[program2] : Normal termination with EXIT STATUS = %d\n", WEXITSTATUS(status));
	} 
	else if (WIFSIGNALED(status)) 
	{
		printk("[program2] : get %s signal\n", sig[WTERMSIG(status)]);
		printk("[program2] : child process terminated");
		printk("[program2] : The return signal is %d\n", WTERMSIG(status));
	} 
	else if (WIFSTOPPED(status)) //
	{
		printk("[program2] : get SIGSTOP signal\n");
		printk("[program2] : children process stopped\n");
		printk("[program2] : The return signal is %d\n", WSTOPSIG(status));
	}
	return 0;
}

static int __init program2_init(void){
	struct task_struct *tsk;
	printk("[program2] : Module_init\n");
	printk("[program2] : module_init create kthread start\n");	
	/* create a kernel thread to run my_fork */
	tsk = kthread_create(&my_fork, NULL,"MyThread");

	if (!IS_ERR(tsk)) 
	{
		printk("[program2] : module_init kthread start\n");
		wake_up_process(tsk);
	}
	
	return 0;
}

static void __exit program2_exit(void){
	printk("[program2] : Module_exit\n");
}

module_init(program2_init);
module_exit(program2_exit);
