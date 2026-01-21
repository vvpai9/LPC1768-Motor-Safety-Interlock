-----------------------Proper ultrasonic working with motor control- and proper ultrasonic readings on UART-------------------------------------------------------
#include <RTL.h>
#include "LPC17xx.h"
#include <stdio.h>

/* ================= PIN DEFINITIONS ================= */
#define TRIG    (1 << 26)   // P0.26
#define ECHO    (1 << 25)   // P0.25

#define M1A     (1 << 0)    // P2.0
#define M1B     (1 << 1)    // P2.1
#define ENA     (1 << 2)    // P2.2

#define STOP_DISTANCE_CM   15
#define ECHO_TIMEOUT_US    30000   // ~5 meters

/* ================= GLOBAL ================= */
volatile U32 distance_cm = 0;

/* ================= SHORT DELAY (~10us) ================= */
static void short_delay(void)
{
    volatile int i;
    for (i = 0; i < 500; i++); // ~10 µs @100 MHz
}

/* ================= UART ================= */
void UART0_Init(void)
{
    LPC_SC->PCONP |= (1 << 3);

    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |=  (1 << 4) | (1 << 6);

    LPC_SC->PCLKSEL0 &= ~(3 << 6);   // PCLK = CCLK/4

    LPC_UART0->LCR = 0x83;           // 8N1 + DLAB
    LPC_UART0->DLL = 162;            // 9600 baud @25MHz
    LPC_UART0->DLM = 0;
    LPC_UART0->LCR = 0x03;           // Disable DLAB
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

void UART_PrintDistance(U32 d)
{
    UART0_SendString("Distance: ");
    UART0_SendChar((d / 100) + '0');
    UART0_SendChar(((d / 10) % 10) + '0');
    UART0_SendChar((d % 10) + '0');
    UART0_SendString(" cm\r\n");
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

/* ================= ULTRASONIC ================= */
void Ultrasonic_Init(void)
{
    LPC_GPIO0->FIODIR |= TRIG;
    LPC_GPIO0->FIODIR &= ~ECHO;

    LPC_SC->PCONP |= (1 << 1);      // Timer0 power
    LPC_SC->PCLKSEL0 &= ~(3 << 2);  // PCLK = CCLK/4 = 25 MHz

    LPC_TIM0->TCR = 0x02;           // Reset timer
    LPC_TIM0->PR  = 24;             // 1 µs per tick
    LPC_TIM0->TCR = 0x00;           // Stop
}

U32 Measure_Distance_cm(void)
{
    U32 elapsed;

    // Trigger 10us pulse
    LPC_GPIO0->FIOCLR = TRIG;
    short_delay();
    LPC_GPIO0->FIOSET = TRIG;
    short_delay();
    LPC_GPIO0->FIOCLR = TRIG;

    LPC_TIM0->TC = 0;
    LPC_TIM0->TCR = 1;   // Start timer

    // Wait for echo HIGH
    while (!(LPC_GPIO0->FIOPIN & ECHO))
    {
        if (LPC_TIM0->TC > ECHO_TIMEOUT_US)
        {
            LPC_TIM0->TCR = 0;
            return 0;
        }
    }

    LPC_TIM0->TC = 0;    // Reset at rising edge

    // Wait for echo LOW
    while (LPC_GPIO0->FIOPIN & ECHO)
    {
        if (LPC_TIM0->TC > ECHO_TIMEOUT_US)
        {
            LPC_TIM0->TCR = 0;
            return 0;
        }
    }

    elapsed = LPC_TIM0->TC;
    LPC_TIM0->TCR = 0;   // Stop timer

    return (elapsed * 343) / 20000;
}

/* ================= RTX TASKS ================= */
__task void task_ultrasonic(void)
{
    U32 sum;
    int i;

    while (1)
    {
        sum = 0;
        for (i = 0; i < 5; i++)
        {
            sum += Measure_Distance_cm();
            os_dly_wait(2);
        }
        distance_cm = sum / 5;
        os_dly_wait(20);
    }
}

__task void task_motor(void)
{
    while (1)
    {
        if (distance_cm && distance_cm < STOP_DISTANCE_CM)
            Motor_Stop();
        else
            Motor_Run();

        os_dly_wait(10);
    }
}

__task void task_uart(void)
{
    while (1)
    {
        UART_PrintDistance(distance_cm);
        os_dly_wait(50);
    }
}

__task void task_init(void)
{
    UART0_Init();
    Motor_Init();
    Ultrasonic_Init();

    UART0_SendString("RTX Started\r\n");

    os_tsk_create(task_ultrasonic, 1);
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
