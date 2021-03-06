#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tonny LEUNG");
MODULE_DESCRIPTION("A kernel module to read system information");
MODULE_VERSION("0.2");

static int __init MyRD_init(void){
    printk(KERN_INFO "MyRD: Started.\n");
    u32 coreid;
    __asm__ __volatile__ (
        "mrs %0, mpidr_el1"
        : "=r" (coreid)
    );
    printk("Current core id: 0x%8x\n", coreid);
    return 0;
}

static void __exit MyRD_exit(void){
    printk(KERN_INFO "MyRD: Finished.\n");
}

module_init(MyRD_init);
module_exit(MyRD_exit);
