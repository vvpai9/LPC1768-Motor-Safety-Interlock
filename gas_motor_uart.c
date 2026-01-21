---------------proper Gas sensor working with uart readings with motor control

#include <RTL.h>
#include "LPC17xx.h"
#include <stdio.h>

/* ================= DEFINES ================= */
#define GAS_THRESHOLD_ADC   1600    // Tune this value

#define M1A     (1 << 0)    // P2.0
#define M1B     (1 << 1)    // P2.1
#define ENA     (1 << 2)    // P2.2

/* ================= GLOBAL ================= */
volatile U32 gas_adc_value = 0;

/* ================= UART ================= */
void UART0_Init(void)
{
    LPC_SC->PCONP |= (1 << 3);

    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |=  (1 << 4) | (1 << 6);

    LPC_SC->PCLKSEL0 &= ~(3 << 6);   // PCLK = CCLK / 4

    LPC_UART0->LCR = 0x83;
    LPC_UART0->DLL = 162;            // 9600 baud @25 MHz
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
    UART0_SendString("MQ5 ADC Value: ");

    UART0_SendChar((val / 1000) + '0');
    UART0_SendChar(((val / 100) % 10) + '0');
    UART0_SendChar(((val / 10) % 10) + '0');
    UART0_SendChar((val % 10) + '0');

    UART0_SendString("\r\n");
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

/* ================= ADC (MQ-5) ================= */
void ADC_Init(void)
{
    LPC_SC->PCONP |= (1 << 12);     // ADC power ON

    // P0.24 ? AD0.1
    LPC_PINCON->PINSEL1 &= ~(3 << 16);
    LPC_PINCON->PINSEL1 |=  (1 << 16);

    LPC_ADC->ADCR = (1 << 1) |      // Select AD0.1
    (4 << 8) |     // ADC clock
    (1 << 21);     // Enable ADC
}

U32 ADC_Read(void)
{
    LPC_ADC->ADCR |= (1 << 24);     // Start conversion
    while (!(LPC_ADC->ADGDR & (1U << 31)));
    return (LPC_ADC->ADGDR >> 4) & 0xFFF;
}

/* ================= RTX TASKS ================= */
__task void task_gas_sensor(void)
{
    while (1)
    {
        gas_adc_value = ADC_Read();
        os_dly_wait(10);
    }
}

__task void task_motor(void)
{
    while (1)
    {
        if (gas_adc_value < GAS_THRESHOLD_ADC)
            Motor_Run();
        else
            Motor_Stop();

        os_dly_wait(1);
    }
}

__task void task_uart(void)
{
    while (1)
    {
        UART_PrintGas(gas_adc_value);
        os_dly_wait(50);
    }
}

/* ================= INIT TASK ================= */
__task void task_init(void)
{
    UART0_Init();
    Motor_Init();
    ADC_Init();

    UART0_SendString("MQ-5 Gas Sensor System Started\r\n");
    UART0_SendString("Preheating MQ-5...\r\n");

    os_dly_wait(50);   // ~30 seconds preheat (RTX ticks)

    UART0_SendString("MQ-5 Ready\r\n");

    os_tsk_create(task_gas_sensor, 1);
    os_tsk_create(task_motor, 2);
    os_tsk_create(task_uart, 0);
    os_tsk_delete_self();
}

int main(void)
{
    SystemInit();
    os_sys_init(task_init);
    while (1);
}
