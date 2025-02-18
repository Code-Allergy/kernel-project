#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/printk.h>
#include <kernel/boot.h>

unsigned long __stack_chk_guard = STACK_CANARY_VALUE;

void __stack_chk_fail(void) {
    uint32_t kernel_phys_l1 = (uint32_t)l1_page_table - KERNEL_START + DRAM_BASE;
    mmu_driver.set_l1_table((void*)kernel_phys_l1);
    printk("STACK CORRUPTION DETECTED (__stack_chk_fail)\nHALT\n");
    while(1);  // Infinite loop or trigger a fault handler
}


#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) printk("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define DEBUG_TRACE() printk("[DEBUG] %s:%d in %s\n", __FILE__, __LINE__, __func__)
#define DEBUG_ASSERT(cond) if(!(cond)) { DEBUG_LOG("Assertion failed: %s", #cond); abort(); }
#define DEBUG_BREAK() __builtin_trap()
#define DEBUG_PRINT_VAR(var) DEBUG_LOG(#var " = %d", var)
#define DEBUG_PRINT_PTR(ptr) DEBUG_LOG(#ptr " = %p", (void*)ptr)
#define DEBUG_PRINT_STR(str) DEBUG_LOG(#str " = %s", str)
#define DEBUG_FUNCTION_ENTER() DEBUG_LOG("Entering function %s", __func__)
#define DEBUG_FUNCTION_EXIT() DEBUG_LOG("Exiting function %s", __func__)
#define DEBUG_FENCE() __sync_synchronize()
#else
#define DEBUG_LOG(fmt, ...)
#define DEBUG_TRACE()
#define DEBUG_ASSERT(cond)
#define DEBUG_BREAK()
#define DEBUG_PRINT_VAR(var)
#define DEBUG_PRINT_PTR(ptr)
#define DEBUG_PRINT_STR(str)
#define DEBUG_FUNCTION_ENTER()
#define DEBUG_FUNCTION_EXIT()
#define DEBUG_FENCE()
#endif
