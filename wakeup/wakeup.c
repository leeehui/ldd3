#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bgb");
MODULE_DESCRIPTION("A simple module for waking up idle cpus");

static unsigned int cpu_mask = 0xFF; // default for all cores
static unsigned int cpu_mask_valid; 
static struct cpumask mask;
static int data[] = {0, 1, 2, 3, 4, 5, 6, 7};


static int wakeup_task(void *data) {
    int *idx = (int *)data;
    int smp_id = smp_processor_id();
    if (*idx == smp_processor_id()) {
        printk(KERN_WARNING "cpu %d is woken up\n", smp_id);
    } else {
        printk(KERN_WARNING "cpu id not match! param:%d, smp_id:%d\n", *idx, smp_id);
    }
    return 0;
}

static int __init wakeup_init(void)
{
    int i;
    unsigned int cpu_mask_original = cpu_mask;
    printk(KERN_INFO "NR_CPUS: %d, cpu_mask: 0x%x\n", NR_CPUS, cpu_mask);
    for (i = 0; i < NR_CPUS && cpu_mask != 0 ; i++, cpu_mask >>= 1) {
        if (cpumask_test_cpu(i, cpu_online_mask)) {
            if (!test_bit(i, (const unsigned long *)&cpu_mask_original)) 
                continue;
            cpumask_set_cpu(i, &mask);
            cpu_mask_valid |= BIT(i);
        } else {
            printk(KERN_WARNING "cpu num %d not in cpu_online_mask, ignore!\n", i);
        }
    }
    printk(KERN_INFO "waking up cpus(mask): 0x%x!\n", cpu_mask_valid);
    for_each_cpu(i, &mask) {
        struct task_struct *task;
        printk(KERN_INFO "cpu: %d!\n", i);
        task = kthread_create(wakeup_task, &data[i], "wakeup_task_%d", i);
        kthread_bind(task, i);
        wake_up_process(task);
    }
    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit wakeup_cleanup(void)
{
    printk(KERN_INFO "wakeup_cleanup!\n");
}

module_param(cpu_mask, uint, S_IRUGO);

module_init(wakeup_init);
module_exit(wakeup_cleanup);
