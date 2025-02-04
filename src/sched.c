

// never return
void scheduler_init() {
    while(1) {
        asm volatile("wfi");
    }
}
