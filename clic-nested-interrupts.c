/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <metal/cpu.h>
#include <metal/csr.h>
#include <metal/drivers/sifive_clic0.h>
#include <metal/led.h>
#include <metal/riscv.h>
#include <stdio.h>

#define RTC_FREQ	32768

#ifndef metal_led_ld0red
#define metal_led_ld0red metal_led_none
#endif

#ifndef metal_led_ld0green
#define metal_led_ld0green metal_led_none
#endif

#define led0_red metal_led_ld0red
#define led0_green metal_led_ld0green

struct metal_interrupt clic = (struct metal_interrupt){0};

void display_instruction (void) {
    printf("\n");
    printf("SIFIVE, INC.\n!!\n");
    printf("\n");
    printf("Coreplex IP Eval Kit 'clic-nested-interrupts' example.\n");
    printf("A 5s timer is used to trigger MSIP and CSIP interrupts.\n");
    printf("Interrupts are prioritized MSIP > CSIP > MTIP to demonstrate preemption.\n");
    printf("\n");
}

/* This special interrupt attribute automatically saves the interrupt context in the
 * mepc and mcause CSRs and re-enables interrupts during the ISR.
 * This requires the -fomit-frame-pointer compiler flag.
 */
__attribute__((interrupt("SiFive-CLIC-preemptible")))
void metal_riscv_cpu_intc_mtip_handler(void) {
    printf("**** Got MTIP interrupt, triggering MSIP and CSIP ****\n");

    /* Trigger the MSIP and CSIP interrupts */
    struct metal_cpu cpu = metal_cpu_get(metal_cpu_get_current_hartid());
    metal_cpu_set_ipi(cpu);
    sifive_clic0_set(clic, SIFIVE_CLIC_SOFTWARE_INTERRUPT_ID);

    printf("**** Re-arming timer for 5 seconds ****\n");
    metal_cpu_set_mtimecmp(cpu, metal_cpu_get_mtime(cpu) + 5*RTC_FREQ);

    printf("**** Exiting Timer handler ****\n");
}

void metal_sifive_clic0_csip_handler(void) {
    printf("**** Got CSIP interrupt ****\n");
    fflush(stdout);

    metal_led_toggle(led0_red);
    sifive_clic0_clear(clic, SIFIVE_CLIC_SOFTWARE_INTERRUPT_ID);
}

void metal_riscv_cpu_intc_msip_handler(void) {
    printf("**** Got MSIP interrupt ****\n");
    fflush(stdout);

    metal_led_toggle(led0_green);
    struct metal_cpu cpu = metal_cpu_get(metal_cpu_get_current_hartid());
    metal_cpu_clear_ipi(cpu);
}

int main (void)
{
    metal_led_enable(led0_red);
    metal_led_enable(led0_green);
    metal_led_off(led0_red);
    metal_led_off(led0_green);
 
    struct metal_cpu cpu = metal_cpu_get(metal_cpu_get_current_hartid());
    metal_cpu_set_mtimecmp(cpu, metal_cpu_get_mtime(cpu) + 5*RTC_FREQ);

    metal_cpu_enable_timer_interrupt();
    metal_cpu_enable_ipi();
    sifive_clic0_enable(clic, SIFIVE_CLIC_SOFTWARE_INTERRUPT_ID);
    metal_cpu_enable_interrupts();

    display_instruction();

    // Wait For Interrupt
    while (1) {
        __asm__ volatile ("wfi");
    }

    return 0;
}
