#include <elf32.h>
#include <stdint.h>
#include <stdbool.h>

#include <kernel/list.h>
#include <kernel/panic.h>
#include <kernel/boot.h>
#include <kernel/mm.h>
#include <kernel/mmu.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/fat32.h>
#include <kernel/heap.h>
#include <kernel/errno.h>
#include <kernel/sleep.h>
#include <kernel/log.h>


/* Globals */
process_t process_table[MAX_PROCESSES]; // TODO dynamic processes

process_t* current_process;
process_t* next_process;

/* Statics */
static uint32_t total_processes;
static uint32_t curr_pid;      // both of these could be kept in the scheduler struct
static uint8_t asid_bitmap[MAX_ASID + 1] = {0};

process_t* spawn_elf_init_process(const char* file_path) {
    fat32_fs_t sd_card;
    fat32_file_t userspace_application;
    if (fat32_mount(&sd_card, &mmc_fat32_diskio) != 0) {
        printk("Failed to mount SD card\n");
        return NULL;
    }
    if (fat32_open(&sd_card, file_path, &userspace_application)) {
        printk("Failed to open file\n");
        return NULL;
    }

    uint8_t* buffer = kmalloc(userspace_application.file_size);
    if (!buffer) {
        printk("Failed to allocate buffer for ELF\n");
        return NULL;
    }

    if ((uint32_t)fat32_read(&userspace_application, buffer, userspace_application.file_size, 0) != userspace_application.file_size) {
        printk("Init process not read fully off disk\n");
        return NULL;
    };

    binary_t* init_bin = load_elf32(buffer, userspace_application.file_size);
    return create_process(init_bin, NULL);
}

static int32_t get_next_pid(void) {
    return curr_pid++;
}

int scheduler_init(void) {
    scheduler_driver.current_tick = 0;
    current_process = NULL;
    total_processes = 0;
    curr_pid = 0;
    memset(process_table, 0, sizeof(process_table));


    process_t* nullp = spawn_elf_init_process(NULL_PROCESS_FILE);
    if (!nullp) panic("Failed to start " NULL_PROCESS_FILE);

    process_t* initp = spawn_elf_init_process(INIT_PROCESS_FILE);
    if (!initp) panic("Failed to start " INIT_PROCESS_FILE);
    LOG(INFO, "Created init process\n");

    return 0;
}

uint8_t allocate_asid(void) {
    for (uint32_t i = 1; i <= MAX_ASID; i++) {  // Skip ASID 0 (reserved)
        if (!asid_bitmap[i]) {
            asid_bitmap[i] = 1;
            return i;
        }
    }
    // If all ASIDs are used, flush TLB and reuse
    mmu_driver.flush_tlb();
    memset(asid_bitmap, 0, sizeof(asid_bitmap));
    asid_bitmap[1] = 1;
    return 1;
}

// round robin scheduler
process_t* get_next_process(void) {
    uint32_t start;
    if (current_process == NULL) {
        start = 0;
    } else {
        start = (current_process->pid + 1) % MAX_PROCESSES;
    }
    uint32_t current = start;
    do {
        if (process_table[current].state == PROCESS_READY) {
            return &process_table[current];
        }
        current = (current + 1) % MAX_PROCESSES;
    } while (current != start);

    return current_process; // If no other process found, return current one
}
void __attribute__((noreturn, naked)) user_context_return(uint32_t stack_ptr);

void __attribute__ ((noreturn)) scheduler(void) {
    // wake up sleeping processes if necessary
    check_sleep_expiry();
    // how do we decied on an order for waking sleep or blocked?
    // wake up a blocked process if necessary

    next_process = get_next_process();
    if (next_process == NULL || next_process->state == PROCESS_NONE) {
        panic("No more processes to run, halting!\n"); // this should never happen, so we panic
    }

    // printk("Starting process pid %u\n", next_process->pid);
    // printk("Jumping to code at %p\n", *(uint32_t*)(next_process->stack_page_paddr + PAGE_SIZE - (3 * sizeof(uint32_t))));
    // printk("Stack top: %p\n", next_process->stack_top);

    // debug_l1_l2_entries((void*)0x00010000, next_process->ttbr0);
    // printk("Phys=0x446A9000: %p", *(uint32_t*)0x446A9000);
    mmu_driver.set_l1_with_asid(next_process->ttbr0, next_process->asid);
    // Add TLB invalidation for the new ASID
    __asm__ volatile (
        "dsb ish\n"
        "mcr p15, 0, %0, c8, c7, 2\n"  // Invalidate TLB by ASID
        "dsb ish\n"
        "isb\n"
        : : "r" (next_process->asid)
    );

    current_process = next_process;
    current_process->state = PROCESS_RUNNING;
    scheduler_driver.schedule_next = 0;

    // TODO: This is a hack to fix the r0 register being written over at some point. this shouldn't be needed.
    if (current_process->forked) {
        // if (current_process->stack_top[0] != 0) printk("WARNING: Stacked process wasn't set to return 0!\n");
        current_process->stack_top[0] = 0;
        current_process->forked = 0;
    }

    // TODO this is a hack, fix it. this WILL break.
    // fix the stack from the compiler function prologue
    __asm__ ("add sp, sp, #8\n");
    // TO fix, we should have the scheduler setup in another function and return then pass to asm
    // this way we won't have to deal with the compiler's prologue

    // Direct tail call with forced assembly branch
    register uint32_t stack_ptr __asm__("r0") = (uint32_t)next_process->stack_top;
    __asm__ volatile(
        "bx %0"
        :
        : "r" (user_context_return), "r" (stack_ptr)
        : "memory"
    );
    __builtin_unreachable();
}

process_t* get_available_process(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_NONE) {
            return &process_table[i];
        }
    }
    return NULL;
}

void get_kernel_regs(struct cpu_regs* regs) {
    __asm__ volatile("mrs %0, cpsr" : "=r"(regs->cpsr));
    __asm__ volatile("mov %0, r0" : "=r"(regs->r0));
    __asm__ volatile("mov %0, r1" : "=r"(regs->r1));
    __asm__ volatile("mov %0, r2" : "=r"(regs->r2));
    __asm__ volatile("mov %0, r3" : "=r"(regs->r3));
    __asm__ volatile("mov %0, r4" : "=r"(regs->r4));
    __asm__ volatile("mov %0, r5" : "=r"(regs->r5));
    __asm__ volatile("mov %0, r6" : "=r"(regs->r6));
    __asm__ volatile("mov %0, r7" : "=r"(regs->r7));
    __asm__ volatile("mov %0, r8" : "=r"(regs->r8));
    __asm__ volatile("mov %0, r9" : "=r"(regs->r9));
    __asm__ volatile("mov %0, r10" : "=r"(regs->r10));
    __asm__ volatile("mov %0, r11" : "=r"(regs->r11));
    __asm__ volatile("mov %0, r12" : "=r"(regs->r12));
    __asm__ volatile("mov %0, sp" : "=r"(regs->sp));
    __asm__ volatile("mov %0, lr" : "=r"(regs->lr));
    __asm__ volatile("mov %0, pc" : "=r"(regs->pc));
}

void tick(void) {
    scheduler_driver.current_tick++; // increment the tick count
    if (scheduler_driver.current_tick % SCHEDULER_PREEMPT_TICKS == 0) {
        scheduler_driver.schedule_next = 1;
        if (current_process) current_process->state = PROCESS_READY;
    }

    if (scheduler_driver.current_tick % LOG_CONSUME_TICKS) {
        log_consume();
    }
}

process_page_ref_t* create_page_ref(process_page_t* page) {
    process_page_ref_t* ref = kmalloc(sizeof(process_page_ref_t));
    if (!ref) return NULL;

    INIT_LIST_HEAD(&ref->list);
    ref->page = page;
    return ref;
}

void mmu_set_l1_with_asid(uint32_t ttbr0, uint32_t asid) {
    mmu_driver.set_l1_with_asid((uint32_t*)ttbr0, asid);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

process_page_t* alloc_process_page(void) {
    process_page_t* page = kmalloc(sizeof(process_page_t));
    if (!page) return NULL;

    // printk("OK\n");
    page->paddr = alloc_page(&kpage_allocator);
    if (!page->paddr) {
        kfree(page);
        return NULL;
    }

    return page;
}

// initialize the process memory table and other memory related fields
static int initialize_process_memory(process_t* p) {
    p->ttbr0 = (uint32_t*) alloc_l1_table(&kpage_allocator);
    if (!p->ttbr0) return -ENOMEM;

    INIT_LIST_HEAD(&p->pages_head);
    p->asid = allocate_asid();
    // other generic memory initialization

    return 0;
}

// load an elf binary into memory, allocating all required pages
static int load_elf_binary(process_t* p, binary_t* bin) {
    for (uint32_t i = 0; i < bin->data.elf.program_header_count; i++) {
        elf_program_header_t* phdr = &bin->data.elf.program_headers[i];
        if (phdr->p_type == ELF_PROGRAM_HEADER_TYPE_LOAD) {
            uint32_t page_count = (phdr->p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
            uint32_t vaddr_aligned = phdr->p_vaddr & ~(PAGE_SIZE - 1);

            bool is_code = phdr->p_flags & ELF_PROGRAM_HEADER_FLAG_EXECUTABLE;
            bool is_writable = phdr->p_flags & ELF_PROGRAM_HEADER_FLAG_WRITABLE;

            for (uint32_t j = 0; j < page_count; j++) {
                process_page_t *page = alloc_process_page();
                if (!page) return -ENOMEM;

                page->vaddr = (void*) (vaddr_aligned + (j * PAGE_SIZE));
                page->ref_count = 1;

                // Copy segment data into page
                size_t copy_size = MIN(PAGE_SIZE, phdr->p_filesz - (j * PAGE_SIZE));
                if (copy_size > 0) {
                    memcpy(PHYS_TO_KERNEL_VIRT(page->paddr), bin->data.elf.raw + phdr->p_offset + (j * PAGE_SIZE), copy_size);
                }

                // Map page with appropriate permissions
                uint32_t prot = MMU_NORMAL_MEMORY | MMU_SHAREABLE | MMU_EXECUTE_NEVER;
                if (is_writable) prot |= PAGE_AP_FULL_ACCESS;
                else prot |= PAGE_AP_USER_RO;
                if (is_code) prot |= MMU_EXECUTE;

                page->flags = prot;

                page->page_type = is_code ? PROCESS_PAGE_CODE : PROCESS_PAGE_DATA;
                process_page_ref_t* ref = create_page_ref(page);
                list_add_tail(&ref->list, &p->pages_head);
                p->num_pages++;
            }
        }
    }
    return 0;
}

static int clone_parent_pages(process_t* p, process_t* parent) {
    process_page_ref_t* current_ref;
    list_for_each_entry(current_ref, process_page_ref_t, &parent->pages_head, list) {
        process_page_t* current_page = current_ref->page;
        if (current_page->page_type == PROCESS_PAGE_CODE) {
            process_page_ref_t *new_ref = create_page_ref(current_page);
            list_add_tail(&new_ref->list, &p->pages_head);
            current_page->ref_count++;
        } else if (current_page->page_type == PROCESS_PAGE_DATA) {
            process_page_t* data_page = alloc_process_page();
            if (!data_page) {
                printk("Failed to allocate page for cloned process\n");
                return -ENOMEM;
            }

            memcpy(PHYS_TO_KERNEL_VIRT(data_page), PHYS_TO_KERNEL_VIRT(current_page->paddr), PAGE_SIZE);
            process_page_ref_t *ref = create_page_ref(data_page);
            list_add_tail(&ref->list, &p->pages_head);
        }
    }
    return 0;
}

static void clone_parent_fds(process_t* p, process_t* parent) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (parent->fd_table[i]) {
            p->fd_table[i] = parent->fd_table[i];
            p->fd_table[i]->refcount++;
        }
    }
    p->num_fds = parent->num_fds;
}

// Helper function to setup stack and heap for the new process.
static int setup_stack_and_heap(process_t* p) {
    process_page_t* heap_page = alloc_process_page();
    process_page_t* stack_page = alloc_process_page();
    if (!stack_page || !heap_page) {
        return -ENOMEM;
    }

    heap_page->vaddr = (void*)MEMORY_USER_HEAP_BASE;
    heap_page->ref_count = 1;
    heap_page->page_type = PROCESS_PAGE_HEAP;
    heap_page->flags = L2_USER_DATA_PAGE;
    p->num_pages++;
    process_page_ref_t* ref = create_page_ref(heap_page);
    list_add_tail(&ref->list, &p->pages_head);

    stack_page->vaddr = (void*)MEMORY_USER_STACK_BASE;
    stack_page->ref_count = 1;
    stack_page->page_type = PROCESS_PAGE_STACK;
    stack_page->flags = L2_USER_DATA_PAGE;
    p->num_pages++;
    ref = create_page_ref(stack_page);
    list_add_tail(&ref->list, &p->pages_head);

    return 0;
}

static void copy_parent_stack_and_heap(process_t* p, process_t* parent) {
    process_page_t* parent_stack_page = NULL;
    process_page_t* parent_heap_page = NULL;

    int count = 0;
    process_page_ref_t* current_ref;
    list_for_each_entry(current_ref, process_page_ref_t, &parent->pages_head, list) {
        if (current_ref->page->page_type == PROCESS_PAGE_HEAP) {
            parent_heap_page = current_ref->page;
            count++;
        } else if (current_ref->page->page_type == PROCESS_PAGE_STACK) {
            parent_stack_page = current_ref->page;
            count++;
        }
    }

    if (!parent_stack_page) {
        panic("Parent process is missing a stack page!");
    }

    if (!parent_heap_page) {
        panic("Parent process is missing a heap page!");
    }

    if (count > 2) {
        panic("Parent process does not handle more than 2 currently.\n");
    }

    list_for_each_entry(current_ref, process_page_ref_t, &p->pages_head, list) {
        if (current_ref->page->page_type == PROCESS_PAGE_HEAP) {
            memcpy(PHYS_TO_KERNEL_VIRT(current_ref->page->paddr), PHYS_TO_KERNEL_VIRT(parent_heap_page->paddr), PAGE_SIZE);
        } else if (current_ref->page->page_type == PROCESS_PAGE_STACK) {
            memcpy(PHYS_TO_KERNEL_VIRT(current_ref->page->paddr), PHYS_TO_KERNEL_VIRT(parent_stack_page->paddr), PAGE_SIZE);
        }
    }
}


// Main function to create a process. this should be made safer later.
process_t* create_process(binary_t* bin, process_t* parent) {
    process_page_ref_t* current_ref;
    if (!bin && !parent) {
        panic("No binary or parent process provided to create_process\n");
    }

    process_t* p = get_available_process(); // allocate a new process
    if (initialize_process_memory(p) != 0) return NULL;

    if (bin) {
        if (bin->type == BINARY_TYPE_ELF32) {
            if (load_elf_binary(p, bin) != 0) return NULL;
        } else {
            panic("Unsupported binary type\n");
        }
    } else if (parent) {
        if (clone_parent_pages(p, parent) != 0) return NULL;
        clone_parent_fds(p, parent);
        p->stack_top = parent->stack_top;
    } else {
        panic("No binary or parent process provided\n");
    }

    if (setup_stack_and_heap(p) != 0) return NULL;

    if (parent) {
        copy_parent_stack_and_heap(p, parent);
    }

    process_page_ref_t *stack_ref;
    process_page_t* stack_page = NULL;
    list_for_each_entry(stack_ref, process_page_ref_t, &p->pages_head, list) {
        if (stack_ref->page->page_type == PROCESS_PAGE_STACK) {
            stack_page = stack_ref->page;
            break;
        }
    }

    if (stack_page == NULL) {
        LOG(ERROR, "Process created with no stack page! aborted!");
        free_process_memory(p);
        // TODO other cleanup, make sure everything is sorted.
        return NULL;
    }

    // Set up the process entry point for the stack if binary exists
    if (bin) {
        p->stack_top = (uint32_t*)(MEMORY_USER_STACK_BASE + PAGE_SIZE - (16 * sizeof(uint32_t)));
        uint32_t* sp_phys = PHYS_TO_KERNEL_VIRT(stack_page->paddr + PAGE_SIZE - (16 * sizeof(uint32_t)));

        sp_phys[13] = 0xCAFEBABE; // lr -- TODO: set to exit handler backup when we have dynamic linking
        sp_phys[14] = 0x10; // cpsr
        sp_phys[15] = bin->entry; // pc
    }

    // map all the pages to the process page table
    list_for_each_entry(current_ref, process_page_ref_t, &p->pages_head, list) {
        mmu_driver.map_page(p->ttbr0, current_ref->page->vaddr, current_ref->page->paddr, current_ref->page->flags);
    }

    p->state = PROCESS_READY;
    p->pid = get_next_pid();
    p->ppid = parent ? parent->pid : 0;
    return p;
}


void free_process_page(process_page_t* process_page) {
    free_page(&kpage_allocator, process_page->paddr);
    kfree(process_page);
}

// free specifically only memory from the process
void free_process_memory(process_t* p) {
    process_page_ref_t* ref, *next;
    list_for_each_entry_safe(ref, process_page_ref_t, next, &p->pages_head, list) {
        if (ref->page->ref_count == 0) panic("ref_count is 0");
        else if (ref->page->ref_count == 1) {
            free_process_page(ref->page);
        } else {
            ref->page->ref_count--; // // TODO free pages in l1 table of process:
            // remove l2 entry and free page
        }

        list_del(&ref->list);
        kfree(ref);
    }
}


// TODO clean this up
__attribute__((naked, noreturn)) void userspace_return(void) {
    __asm__ volatile(
        "ldr r3, =scheduler_driver\n\t"
        "ldr r3, [r3]\n\t"
        "cmp r3, #0\n\t"
        "beq 1f\n\t"
        "b scheduler\n"
        "1:\n\t"
        "ldr r3, =current_process\n\t"
        "ldr r3, [r3]\n\t"
        "ldr r0, [r3, %[ttbr0_off]]\n\t"
        "ldr r1, [r3, %[asid_off]]\n\t"
        "bl mmu_set_l1_with_asid\n\t"
        "ldr r3, =current_process\n\t"
        "ldr r3, [r3]\n\t"
        "ldr r0, [r3, %[stack_off]]\n\t"
        "b user_context_return\n\t"
        :
        : [ttbr0_off] "i" (offsetof(process_t, ttbr0)),
          [asid_off] "i" (offsetof(process_t, asid)),
          [stack_off] "i" (offsetof(process_t, stack_top))
    );
}

// for exec* syscalls
int swap_process(binary_t* bin, process_t* p) {
    process_page_ref_t* current_ref;
    if (!bin || !p) {
        return -1;
    }

    /* free the old process memory */
    free_process_memory(p);

    if (bin->type != BINARY_TYPE_ELF32) {
        panic("Flat binaries aren't supported at this time!");
    }

    if (initialize_process_memory(p) != 0) return -1;

    if (load_elf_binary(p, bin) != 0) return -1;

    if (setup_stack_and_heap(p) != 0) return -1;

    process_page_ref_t *stack_ref;
    process_page_t* stack_page = NULL;
    list_for_each_entry(stack_ref, process_page_ref_t, &p->pages_head, list) {
        if (stack_ref->page->page_type == PROCESS_PAGE_STACK) {
            stack_page = stack_ref->page;
            break;
        }
    }

    if (stack_page == NULL) {
        LOG(ERROR, "Process created with no stack page! aborted!");
        free_process_memory(p);
        // TODO other cleanup, make sure everything is sorted.
        return -1;
    }


    p->stack_top = (uint32_t*)(MEMORY_USER_STACK_BASE + PAGE_SIZE - (16 * sizeof(uint32_t)));
    uint32_t* sp_phys = PHYS_TO_KERNEL_VIRT(stack_page->paddr + PAGE_SIZE - (16 * sizeof(uint32_t)));

    sp_phys[13] = 0xCAFEBABE; // lr -- TODO: set to exit handler backup when we have dynamic linking
    sp_phys[14] = 0x10; // cpsr
    sp_phys[15] = bin->entry; // pc

    // map all the pages to the process page table
    list_for_each_entry(current_ref, process_page_ref_t, &p->pages_head, list) {
        mmu_driver.map_page(p->ttbr0, current_ref->page->vaddr, current_ref->page->paddr, current_ref->page->flags);
    }

    return 0;
}

scheduler_t scheduler_driver = {
    .schedule_next = 0,
    .tick = tick,
};
