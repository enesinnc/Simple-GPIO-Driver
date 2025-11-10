#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x77358855, "iomem_resource" },
	{ 0x1035c7c2, "__release_region" },
	{ 0x37a0cba, "kfree" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x2cf56265, "__dynamic_pr_debug" },
	{ 0x4c03a563, "random_kmalloc_seed" },
	{ 0x8da0819, "kmalloc_caches" },
	{ 0xd0c3484c, "kmalloc_trace" },
	{ 0xde80cd09, "ioremap" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x858c69be, "cdev_init" },
	{ 0x6b732375, "cdev_add" },
	{ 0x6ca9b86a, "class_create" },
	{ 0x3b69de06, "device_create" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xa78af5f3, "ioread32" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x4a453f53, "iowrite32" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x479b90bb, "param_ops_int" },
	{ 0x122c3a7e, "_printk" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x5b40b481, "device_destroy" },
	{ 0x75646747, "class_destroy" },
	{ 0xc892ac3e, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xedc03953, "iounmap" },
	{ 0xe2fd41e5, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "3FED6FDEF1A1359C712F569");
