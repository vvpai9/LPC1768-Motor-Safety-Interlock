// finalize script for gas sensor

#include <RTL.h>
#include "LPC17xx.h"
#include <stdio.h>

/* ================= DEFINES ================= */
#define M1A     (1 << 0)   // P2.0
#define M1B     (1 << 1)   // P2.1
#define ENA     (1 << 2)   // P2.2

#define GAS_THRESHOLD 2000   // Adjust after calibration

/* ================= GLOBAL ================= */
U32 gas_value;

/* ================= UART ================= */
void UART0_Init(void)
{
    LPC_SC->PCONP |= (1 << 3);

    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |=  (1 << 4) | (1 << 6);

    LPC_UART0->LCR = 0x83;
    LPC_UART0->DLL = 163;
    LPC_UART0->DLM = 0;
    LPC_UART0->LCR = 0x03;
}

void UART0_SendChar(char ch)
{
    while (!(LPC_UART0->LSR & (1 << 5)));
    LPC_UART0->THR = ch;
}

void UART0_SendString(const char *s)
{
    while (*s) UART0_SendChar(*s++);
}

void UART_PrintGas(U32 val)
{
    char buf[20];
    sprintf(buf, "Gas: %lu\r\n", val);
    UART0_SendString(buf);
}

/* ================= MOTOR ================= */
void Motor_Init(void)
{
    LPC_GPIO2->FIODIR |= M1A | M1B | ENA;
}

void Motor_Run(void)
{
    LPC_GPIO2->FIOSET = M1A | ENA;
    LPC_GPIO2->FIOCLR = M1B;
}

void Motor_Stop(void)
{
    LPC_GPIO2->FIOCLR = M1A | M1B | ENA;
}

/* ================= ADC (MQ-5 on P0.24) ================= */
void ADC_Init(void)
{
    LPC_SC->PCONP |= (1 << 12);

    LPC_PINCON->PINSEL1 &= ~(3 << 16);
    LPC_PINCON->PINSEL1 |=  (1 << 16);   // P0.24 ? AD0.1

    LPC_ADC->ADCR = (1 << 1) |       // Select AD0.1
    (4 << 8) |       // CLKDIV
    (1 << 21);       // Enable ADC
}

U32 ADC_Read(void)
{
    LPC_ADC->ADCR |= (1 << 24);          // Start conversion
    while (!(LPC_ADC->ADGDR & (1U << 31)));
    return (LPC_ADC->ADGDR >> 4) & 0xFFF; // 12-bit result
}

/* ================= RTX TASKS ================= */

/* Task 1: Gas Sensor */
__task void task_gas(void)
{
    while (1)
    {
        gas_value = ADC_Read();
        os_dly_wait(10);
    }
}

/* Task 2: Motor Control */
__task void task_motor(void)
{
    while (1)
    {
        if (gas_value > GAS_THRESHOLD)
            Motor_Stop();
        else
            Motor_Run();

        os_dly_wait(5);
    }
}

/* Task 3: UART */
__task void task_uart(void)
{
    while (1)
    {
        UART_PrintGas(gas_value);
        os_dly_wait(50);
    }
}

/* ================= INIT TASK ================= */
__task void task_init(void)
{
    UART0_Init();
    Motor_Init();
    ADC_Init();

    UART0_SendString("RTX MQ-5 System Started\r\n");

    os_tsk_create(task_gas,   1);
    os_tsk_create(task_motor,2);
    os_tsk_create(task_uart, 0);

    os_tsk_delete_self();
}

/* ================= MAIN ================= */
int main(void)
{
    SystemInit();
    os_sys_init(task_init);
    while (1);
}
