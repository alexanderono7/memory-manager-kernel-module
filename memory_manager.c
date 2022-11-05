// Alexander Ono
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/hrtimer.h>

static int pid = 0;
static int rss_pages = 0;
static int swap_pages = 0;

int ptep_test_and_clear_young (struct vm_area_struct *vma, unsigned long addr, pte_t *ptep);
/* Test and clear the accessed bit of a given pte entry. vma is the pointer
to the memory region, addr is the address of the page, and ptep is a pointer
to a pte. It returns 1 if the pte was accessed, or 0 if not accessed. */
/* The ptep_test_and_clear_young() is architecture dependent and is not
exported to be used in a kernel module. You will need to add its
implementation as follows to your kernel module. */
int ptep_test_and_clear_young(struct vm_area_struct *vma,
unsigned long addr, pte_t *ptep)
{
    int ret = 0;
    if (pte_young(*ptep)){
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
    }
    return ret;
}

// Receive arguments to the kernel module
module_param(pid, int, 0644);

// experimental function - access a page? idk.
pte_t* access_page(struct mm_struct* mm, unsigned long address){
    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pud_t *pud;
    pte_t *ptep, pte;
    pte_t *result = NULL;

    swap_pages++;

    pgd = pgd_offset(mm, address); // get pgd from mm and the page address
    if (pgd_none(*pgd) || pgd_bad(*pgd)) {
        // check if pgd is bad or does not exist
        //printk("pgd is bad or does not exist");
        return NULL;
    }else{
        //printk("pgd is good!");
    }

    p4d = p4d_offset(pgd, address); //get p4d from from pgd and the page address
    if (p4d_none(*p4d) || p4d_bad(*p4d)) {
        // check if p4d is bad or does not exist
        //printk("p4d is bad or does not exist");
        return NULL;
    }else{
        //printk("p4d is good!");
    }

    pud = pud_offset(p4d, address); // get pud from from p4d and the page address
    if (pud_none(*pud) || pud_bad(*pud)) {
        // check if pud is bad or does not exist
        //printk("pud is bad or does not exist");
        //return;
        return NULL;
    }else{
        //printk("pud is good!");
    }

    pmd = pmd_offset(pud, address); // get pmd from from pud and the page address
    if (pmd_none(*pmd) || pmd_bad(*pmd)) {
        // check if pmd is bad or does not exist
        //printk("pmd is bad or does not exist");
        return NULL;
    }else{
        //printk("pmd is good!");
    }

    ptep = pte_offset_map(pmd, address); // get pte from pmd and the page address
    if (!ptep){
        // check if pte does not exist
        //printk("pte is bad or does not exist");
        return NULL;
    }else{
        //printk("pte is good!");
    }

    pte = *ptep; //is this necessary??
    result = ptep;
    rss_pages++;
    swap_pages--;
    return result;
}

// Print out all tasks which have a valid mm and mmap (used for testing purposes)
void probe(void){
    struct task_struct* p;
    for_each_process(p){
        if(p->mm){
            if(p->mm->mmap){
                printk("\nvalid: process # %d", p->pid);
                if(p->mm->mmap->vm_start){
                    unsigned long start = p->mm->mmap->vm_start;
                    unsigned long end = p->mm->mmap->vm_end;
                    unsigned long i;
                    struct vm_area_struct* foo;

                    printk("starting addr: %p", (void*)(p->mm->mmap->vm_start));
                    printk("ending addr: %p", (void*)(p->mm->mmap->vm_end));
                    printk("diff: %lu", end-start);
                    printk("# pages?: %lu", (end-start)/PAGE_SIZE);
                    printk("remainder?: %lu", (end-start)%PAGE_SIZE);

                    if(p->mm->mmap->vm_next){
                        printk("has a next vma?: yes!");
                    }else{
                        printk("has a next vma?: no.");
                    }
                    //access_page(p->mm, p->mm->mmap->vm_start); // access_page on vm_start
                    foo = p->mm->mmap;
                    for(i = start; i <= end; i+=PAGE_SIZE){
                        access_page(p->mm, i);
                        //if(i==end) printk("completed.");
                    }
                    int res;
                    pte_t *ptep;
                    while(foo->vm_next){
                        start = foo->vm_start;
                        end = foo->vm_end;
                        for(i = start; i <= end; i+=PAGE_SIZE){
                            ptep = access_page(p->mm, i);
                            if(ptep){
                                res = ptep_test_and_clear_young(foo, i, ptep);
                                printk("ptep test value: %d", res);
                            }
                        }
                        foo = foo->vm_next;
                    }
                }
            }
        }
        return; // I only want to do one process for testing the page walk.
    }
    return;
}

// Find the process whose PID matches `pid` and return its task_struct ptr
struct task_struct* find_pid(void){
    struct task_struct* p;
    struct task_struct* result = NULL;
    
    for_each_process(p){
        if(p->pid == pid){
            printk("\nFOUND IT! : %d\n",p->pid);
            result = p;
        }
        //printk("don't care: %d",p->pid);
    }
    return result;
}

// Traverse Memory regions (VMAs)?
int traverse_vmas(struct task_struct* task){
    //struct vm_area_struct* vma;
    //printk("%d",PAGE_SIZE); // this works btw, printing the page size
    //vma = task->mm->mmap;

    if(task->mm){
        if(task->mm->mmap){
            if(task->mm->mmap->vm_start){
                unsigned long start = task->mm->mmap->vm_start;
                unsigned long end = task->mm->mmap->vm_end;
                unsigned long i;
                struct vm_area_struct* foo;
                //printk("starting addr: %p", (void*)(p->mm->mmap->vm_start));
                //printk("ending addr: %p", (void*)(p->mm->mmap->vm_end));
                //printk("diff: %lu", end-start);
                //printk("# pages?: %lu", (end-start)/PAGE_SIZE);
                //printk("remainder?: %lu", (end-start)%PAGE_SIZE);

                //access_page(p->mm, p->mm->mmap->vm_start); // access_page on vm_start
                foo = task->mm->mmap;
                while(foo){
                    start = foo->vm_start;
                    end = foo->vm_end;
                    for(i = start; i <= end; i+=PAGE_SIZE){
                        access_page(task->mm, i);
                        //if(i==end) printk("completed.");
                    }
                    foo = foo->vm_next;
                }
            }
        }
    }
    return 0;
}

// 10 second timer for measuring 1 instance of the WSS
unsigned long timer_interval_ns = 10e9; // 10-second timer
enum hrtimer_restart no_restart_callback(struct hrtimer *timer){
    ktime_t currtime, interval;
    currtime = ktime_get();
    interval = ktime_set(0, timer_interval_ns);
    hrtimer_forward(timer, currtime, interval);
    // Do the measurement
    printk("Timer...running?");
    return HRTIMER_NORESTART;
}

// Count # of page table entries being accessed for a given process right now
//int count_ptes(){ }


// Initialize kernel module
int memman_init(void){
    struct task_struct* proc;
    //printk("Memory manager launched!\n");
    probe();
    return 0;
    proc = find_pid();
    if(!proc){
        printk("Couldn't find process w/ PID %d", pid);
        return 0;
    }
    traverse_vmas(proc);
    printk("pages in rss: %d", rss_pages);
    printk("pages in swap: %d", swap_pages);

    return 0;
}

// Exit kernel module.
void memman_exit(void){
    printk("Farewell!!!\n");
    return;
}

// Run kernel module
module_init(memman_init);
module_exit(memman_exit);
MODULE_LICENSE("GPL");
