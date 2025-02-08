#ifndef KERNEL_CCM_H
#define KERNEL_CCM_H

typedef struct {
    // initialize the driver
    void (*init)(void);

    // set the clock speed for the CPU
    void (*set_cpu_speed)(int speed);
} ccm_t;

extern ccm_t ccm_driver;

#endif // KERNEL_CCM_H
