---------------------- Proper Temperature sensor working with motor control and proper readings of ADC on UART---------------------------------------
#include <RTL.h>
#include "LPC17xx.h"
#include <stdio.h>

/* ================= DEFINES ================= */
#define TEMP_THRESHOLD      40      // 40 °C

#define M1A     (1 << 0)    // P2.0
#define M1B     (1 << 1)    // P2.1
#define ENA     (1 << 2)    // P2.2

/* ================= GLOBAL ================= */
volatile U32 temperature_c = 0;

/* ================= UART ================= */
void UART0_Init(void)
{
    LPC_SC->PCONP |= (1 << 3);

    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |=  (1 << 4) | (1 << 6);

    LPC_SC->PCLKSEL0 &= ~(3 << 6);   // PCLK = CCLK/4

    LPC_UART0->LCR = 0x83;
    LPC_UART0->DLL = 162;            // 9600 baud @25MHz
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

void UART_PrintTemp(U32 t)
{
    UART0_SendString("Temperature: ");
    UART0_SendChar((t / 10) + '0');
    UART0_SendChar((t % 10) + '0');
    UART0_SendString(" C\r\n");
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

/* ================= ADC (LM35) ================= */
void ADC_Init(void)
{
    LPC_SC->PCONP |= (1 << 12);     // ADC power

    LPC_PINCON->PINSEL1 &= ~(3 << 14);
    LPC_PINCON->PINSEL1 |=  (1 << 14); // P0.23 ? AD0.0

    LPC_ADC->ADCR = (1 << 0) |      // AD0.0
    (4 << 8) |     // ADC clock
    (1 << 21);     // Enable ADC
}

U32 ADC_Read(void)
{
    LPC_ADC->ADCR |= (1 << 24);     // Start conversion
    while (!(LPC_ADC->ADGDR & (1U << 31)));
    return (LPC_ADC->ADGDR >> 4) & 0xFFF;
}

U32 LM35_Read_Temp(void)
{
    U32 adc = ADC_Read();
    float voltage = (adc * 3.3f) / 4095.0f;
    return (U32)(voltage * 100.0f); // °C
}

/* ================= RTX TASKS ================= */
__task void task_temperature(void)
{
    while (1)
    {
        temperature_c = LM35_Read_Temp();
        os_dly_wait(20);
    }
}

__task void task_motor(void)
{
    while (1)
    {
        if (temperature_c < TEMP_THRESHOLD)
            Motor_Run();
        else
            Motor_Stop();

        os_dly_wait(10);
    }
}

__task void task_uart(void)
{
    while (1)
    {
        UART_PrintTemp(temperature_c);
        os_dly_wait(50);
    }
}

__task void task_init(void)
{
    UART0_Init();
    Motor_Init();
    ADC_Init();

    UART0_SendString("RTX LM35 System Started\r\n");

    os_tsk_create(task_temperature, 1);
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
