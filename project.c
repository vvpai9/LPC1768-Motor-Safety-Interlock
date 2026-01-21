// Full working cod with all three sensors control motor with UARt printing
#include <RTL.h>
#include "LPC17xx.h"
#include <stdio.h>

/* ================= PIN & THRESHOLD DEFINITIONS ================= */
#define TRIG            (1 << 26)     // P0.26
#define ECHO            (1 << 25)     // P0.25

#define M1A             (1 << 0)      // P2.0
#define M1B             (1 << 1)      // P2.1
#define ENA             (1 << 2)      // P2.2

// ------------------ Thresholds ------------------
#define STOP_DISTANCE_CM    20        // Stop if closer than this
#define TEMP_THRESHOLD      38        // Stop if >= this °C
#define GAS_THRESHOLD_ADC   1600      // Stop if >= this ADC value

/* ================= GLOBALS ================= */
volatile U32 distance_cm     = 0;
volatile U32 temperature_c   = 0;
volatile U32 gas_adc_value   = 0;

/* ================= SHORT DELAY (~10us) ================= */
static void short_delay(void)
{
    volatile int i;
    for (i = 0; i < 500; i++);     // ~10 µs @100 MHz
}

/* ================= UART ================= */
void UART0_Init(void)
{
    LPC_SC->PCONP |= (1 << 3);
    LPC_PINCON->PINSEL0 &= ~((3 << 4) | (3 << 6));
    LPC_PINCON->PINSEL0 |= (1 << 4) | (1 << 6);
    LPC_SC->PCLKSEL0 &= ~(3 << 6);          // PCLK = CCLK/4
    LPC_UART0->LCR = 0x83;                  // 8N1 + DLAB
    LPC_UART0->DLL = 162;                   // 9600 baud @25MHz PCLK
    LPC_UART0->DLM = 0;
    LPC_UART0->LCR = 0x03;                  // Disable DLAB
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

void UART_Print_All(void)
{
    // Distance
    UART0_SendString("Distance: ");
    UART0_SendChar((distance_cm / 100) + '0');
    UART0_SendChar(((distance_cm / 10) % 10) + '0');
    UART0_SendChar((distance_cm % 10) + '0');
    UART0_SendString(" cm  |  ");

    // Temperature
    UART0_SendString("Temperature: ");
    UART0_SendChar((temperature_c / 10) + '0');
    UART0_SendChar((temperature_c % 10) + '0');
    UART0_SendString(" C  |  ");

    // Gas
    UART0_SendString("MQ5 ADC: ");
    UART0_SendChar((gas_adc_value / 1000) + '0');
    UART0_SendChar(((gas_adc_value / 100) % 10) + '0');
    UART0_SendChar(((gas_adc_value / 10) % 10) + '0');
    UART0_SendChar((gas_adc_value % 10) + '0');

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

/* ================= ULTRASONIC ================= */
void Ultrasonic_Init(void)
{
    LPC_GPIO0->FIODIR |= TRIG;
    LPC_GPIO0->FIODIR &= ~ECHO;

    LPC_SC->PCONP |= (1 << 1);              // Power Timer0
    LPC_SC->PCLKSEL0 &= ~(3 << 2);          // PCLK_timer0 = CCLK/4 = 25MHz
    LPC_TIM0->TCR = 0x02;                   // Reset
    LPC_TIM0->PR = 24;                      // 1µs tick @25MHz
    LPC_TIM0->TCR = 0x00;                   // Stop
}

U32 Measure_Distance_cm(void)
{
    U32 elapsed;
    #define ECHO_TIMEOUT_US 30000

    // Trigger 10us pulse
    LPC_GPIO0->FIOCLR = TRIG;
    short_delay();
    LPC_GPIO0->FIOSET = TRIG;
    short_delay();
    LPC_GPIO0->FIOCLR = TRIG;

    LPC_TIM0->TC = 0;
    LPC_TIM0->TCR = 1;                      // Start timer

    // Wait for rising edge
    while (!(LPC_GPIO0->FIOPIN & ECHO))
    {
        if (LPC_TIM0->TC > ECHO_TIMEOUT_US) {
            LPC_TIM0->TCR = 0;
            return 0;
        }
    }

    LPC_TIM0->TC = 0;                       // Reset at rising edge

    // Wait for falling edge
    while (LPC_GPIO0->FIOPIN & ECHO)
    {
        if (LPC_TIM0->TC > ECHO_TIMEOUT_US) {
            LPC_TIM0->TCR = 0;
            return 0;
        }
    }

    elapsed = LPC_TIM0->TC;
    LPC_TIM0->TCR = 0;

    return (elapsed * 343) / 20000;         // speed of sound 343 m/s
}

/* ================= ADC - LM35 & MQ-5 ================= */
void ADC_Init(void)
{
    LPC_SC->PCONP |= (1 << 12);             // ADC power ON

    // P0.23 ? AD0.0 ? LM35
    LPC_PINCON->PINSEL1 &= ~(3 << 14);
    LPC_PINCON->PINSEL1 |= (1 << 14);

    // P0.24 ? AD0.1 ? MQ-5
    LPC_PINCON->PINSEL1 &= ~(3 << 16);
    LPC_PINCON->PINSEL1 |= (1 << 16);

    LPC_ADC->ADCR = (1 << 21) |             // ADC enable
    (4 << 8);               // CLKDIV = 4 ? ~3.125 MHz
}

/* Read selected channel (0 or 1) */
U32 ADC_Read_Channel(uint8_t channel)
{
    LPC_ADC->ADCR &= ~(0xFF << 0);          // Clear channel bits
    LPC_ADC->ADCR |= (1 << channel);        // Select channel

    LPC_ADC->ADCR |= (1 << 24);             // Start conversion
    while (!(LPC_ADC->ADGDR & (1U << 31))); // Wait done

    return (LPC_ADC->ADGDR >> 4) & 0xFFF;
}

U32 LM35_Read_Temp(void)
{
    U32 adc = ADC_Read_Channel(0);          // AD0.0
    float voltage = (adc * 3.3f) / 4095.0f;
    return (U32)(voltage * 100.0f);         // °C * 1 (no decimal)
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
            os_dly_wait(3);
        }
        distance_cm = sum / 5;
        os_dly_wait(25);
    }
}

__task void task_temperature(void)
{
    while (1)
    {
        temperature_c = LM35_Read_Temp();
        os_dly_wait(40);
    }
}

__task void task_gas_sensor(void)
{
    while (1)
    {
        gas_adc_value = ADC_Read_Channel(1);   // AD0.1
        os_dly_wait(20);
    }
}

__task void task_motor_control(void)
{
    while (1)
    {
        // YOUR REQUIRED LOGIC
        // Motor runs ONLY if ALL three conditions are satisfied:
        //   distance > threshold    AND    temp < threshold    AND    gas < threshold
        if ((distance_cm > STOP_DISTANCE_CM) &&
            (temperature_c < TEMP_THRESHOLD) &&
            (gas_adc_value < GAS_THRESHOLD_ADC))
        {
            Motor_Run();
        }
        else
        {
            Motor_Stop();
        }

        os_dly_wait(15);
    }
}

__task void task_uart(void)
{
    while (1)
    {
        UART_Print_All();
        os_dly_wait(80);           // ~ every 0.8–1.2 sec (adjust if needed)
    }
}

__task void task_init(void)
{
    SystemInit();   // Just to be safe (some people call it twice)

    UART0_Init();
    Motor_Init();
    Ultrasonic_Init();
    ADC_Init();

    UART0_SendString("\r\n=== Multi-Sensor System Started ===\r\n");
    UART0_SendString("Preheating MQ-5 (please wait) ...\r\n");

    os_dly_wait(300);   // ~30 seconds preheat for MQ-5 (rough)

    UART0_SendString("MQ-5 Ready. Starting normal operation...\r\n\n");

    os_tsk_create(task_ultrasonic,    1);
    os_tsk_create(task_temperature,   1);
    os_tsk_create(task_gas_sensor,    1);
    os_tsk_create(task_motor_control, 2);
    os_tsk_create(task_uart,          0);

    os_tsk_delete_self();
}

int main(void)
{
    os_sys_init(task_init);
    while (1);
}
