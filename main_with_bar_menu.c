/* ============================================================================
 *  STM32F401 Black Pill - Real-Time Audio Spectrum Analyzer
 *  SINGLE FILE VERSION for Keil uVision 5 (CMSIS, register level, no HAL)
 *
 *  Keil: device STM32F401CCUx (or CEUx), Run-Time Environment: tick
 *  CMSIS->CORE and Device->Startup only. Options: Xtal 25 MHz,
 *  Floating Point Hardware = Single Precision. Add only this file.
 *
 *  Wiring (all GND together, board powered from USB-C):
 *   MAX4466 : VCC->3V3  GND->GND  OUT->PA1
 *   ILI9341 : VCC->3V3  GND->GND  CS->PA4  RESET->PB1  DC->PB0
 *             SDI(MOSI)->PA7  SCK->PA5  LED->3V3  SDO(MISO)->PA6 (optional)
 *   HC-05   : VCC->5V  GND->GND  TXD->PA10  RXD->PA9  STATE->PB15 (optional)
 *   Buttons : MODE PB12->GND   PAUSE PB13->GND   GRID PB14->GND
 *   LEDs    : PB7->220R->red LED->GND (PEAK)   PB8->220R->green (HEARTBEAT)
 *             PB9->220R->blue (GRID)   on-board PC13 = paused
 *   ST-LINK : SWDIO->DIO(PA13)  SWCLK->SCK(PA14)  GND->GND
 * ==========================================================================*/

/* ======================= board.h ======================= */
/* ============================================================================
 * board.h  -  STM32F401 Black Pill : pin map and system constants
 * Project : Real-Time Audio Spectrum Analyzer
 *
 * Every pin used by the project is listed here, in one place.
 * ==========================================================================*/
#ifndef BOARD_H
#define BOARD_H

/* Keil's STM32F4xx_DFP normally defines the device macro for you.
 * This guard only applies if it is missing, so it is always safe.        */
#if !defined(STM32F401xC) && !defined(STM32F401xE)
#define STM32F401xC
#endif

#include "stm32f4xx.h"          /* CMSIS device header (registers, IRQn)   */
#include <stdint.h>

/* ---------------- clocks ------------------------------------------------ */
#define HSE_CRYSTAL_HZ   25000000UL  /* Black Pill v2/v3 crystal = 25 MHz  */
#define SYSCLK_HZ        84000000UL  /* PLL output (max for STM32F401)     */
#define APB1_HZ          42000000UL  /* APB1 = SYSCLK/2 (max 42 MHz)       */
#define APB2_HZ          84000000UL  /* APB2 = SYSCLK/1                    */
#define TIM2_CLK_HZ      84000000UL  /* APB1 timers run at 2 x APB1        */

/* ---------------- MAX4466 microphone (sensor, ADC) ---------------------- */
/*   MAX4466 OUT  -> PA1 (ADC1 channel 1)                                   */
#define MIC_PIN          1u
#define MIC_ADC_CHANNEL  1u

/* ---------------- ILI9341 TFT (SPI1) ------------------------------------ */
/*   SCK  -> PA5 (SPI1_SCK, AF5)      SDI/MOSI -> PA7 (SPI1_MOSI, AF5)       */
/*   SDO/MISO -> PA6 (optional, not used)                                   */
/*   CS   -> PA4      DC -> PB0      RESET -> PB1      LED -> 3V3           */
#define TFT_CS_PIN       4u    /* GPIOA */
#define TFT_DC_PIN       0u    /* GPIOB */
#define TFT_RST_PIN      1u    /* GPIOB */

/* ---------------- HC-05 Bluetooth (USART1) ------------------------------ */
/*   HC-05 TXD -> PA10 (USART1_RX)    HC-05 RXD -> PA9 (USART1_TX)          */
/*   HC-05 STATE -> PB15 (optional: high when a phone is connected)         */
#define BT_TX_PIN        9u    /* GPIOA */
#define BT_RX_PIN        10u   /* GPIOA */
#define BT_STATE_PIN     15u   /* GPIOB */
#define BT_BAUD          9600UL  /* HC-05 factory default data-mode baud    */

/* ---------------- push buttons (to GND, internal pull-ups, EXTI) -------- */
#define BTN_MODE_PIN     12u   /* GPIOB : bars <-> waveform                */
#define BTN_PAUSE_PIN    13u   /* GPIOB : freeze / resume                  */
#define BTN_GRID_PIN     14u   /* GPIOB : grid lines on / off              */

/* ---------------- LEDs (pin -> 220R -> LED -> GND) ---------------------- */
#define LED_PEAK_PIN     7u    /* GPIOB : red,   loud sound / clipping     */
#define LED_HEART_PIN    8u    /* GPIOB : green, blinks = firmware alive   */
#define LED_GRID_PIN     9u    /* GPIOB : blue,  grid is on                */
#define LED_BOARD_PIN    13u   /* GPIOC : on-board LED (active low) = paused */

/* ---------------- NVIC priorities (0 = highest) ------------------------- */
#define PRIO_ADC         1u    /* audio sampling must never be late        */
#define PRIO_SYSTICK     2u
#define PRIO_USART1      3u
#define PRIO_EXTI        4u

#endif /* BOARD_H */

/* ======================= bsp.h ======================= */
/* ============================================================================
 * bsp.h  -  board support: clock, SysTick, GPIO, LEDs, buttons (EXTI/NVIC),
 *           ADC sampling of the microphone (TIM2 -> ADC1 -> NVIC interrupt)
 * ==========================================================================*/
#ifndef BSP_H
#define BSP_H

/* GPIO helper ------------------------------------------------------------ */
#define PIN_MODE_IN    0u
#define PIN_MODE_OUT   1u
#define PIN_MODE_AF    2u
#define PIN_MODE_AN    3u
#define PIN_PULL_NONE  0u
#define PIN_PULL_UP    1u
#define PIN_PULL_DOWN  2u

void     gpio_config(GPIO_TypeDef *port, uint32_t pin, uint32_t mode,
                     uint32_t pull, uint32_t af);

/* clock / time ------------------------------------------------------------ */
void     bsp_clock_init(void);      /* 84 MHz from HSE (falls back to HSI)  */
uint8_t  bsp_clock_is_hse(void);
void     bsp_systick_init(void);    /* 1 ms tick                            */
uint32_t millis(void);
void     delay_ms(uint32_t ms);

/* LEDs + HC-05 STATE input -------------------------------------------------*/
#define LED_PEAK   0u
#define LED_HEART  1u
#define LED_GRID   2u
#define LED_BOARD  3u
void     bsp_io_init(void);
void     led_set(uint32_t led, uint32_t on);
uint8_t  bsp_bt_linked(void);

/* buttons: EXTI interrupt + confirmed in main loop (debounced) ------------ */
#define EV_MODE    0x01u
#define EV_PAUSE   0x02u
#define EV_GRID    0x04u
#define EV_BARS    0x08u      /* Bluetooth 'B': cycle 8 -> 16 -> 32 bars */
void     bsp_buttons_init(void);
uint8_t  bsp_buttons_poll(uint32_t now_ms);   /* returns EV_* bits          */

/* audio capture: TIM2 TRGO triggers ADC1, ADC interrupt stores samples ---- */
void            bsp_sampling_init(void);
void            bsp_sampling_start(void);     /* capture one block          */
uint8_t         bsp_capture_ready(void);
const uint16_t *bsp_capture_buffer(void);

#endif /* BSP_H */

/* ======================= uart_bt.h ======================= */
/* ============================================================================
 * uart_bt.h  -  HC-05 Bluetooth over USART1, interrupt driven (NVIC)
 * ==========================================================================*/
#ifndef UART_BT_H
#define UART_BT_H

#include <stdint.h>

void bt_init(uint32_t baud);
void bt_putc(char c);                 /* non-blocking, drops if buffer full */
void bt_puts(const char *s);
void bt_put_uint(uint32_t v, uint32_t width);   /* zero-padded to width   */
int  bt_getc(void);                   /* -1 if nothing received             */

#endif /* UART_BT_H */

/* ======================= lcd.h ======================= */
/* ============================================================================
 * lcd.h  -  minimal display interface (implemented by ili9341.c)
 * ==========================================================================*/
#ifndef LCD_H_INCLUDED
#define LCD_H_INCLUDED

#include <stdint.h>

#define LCD_WIDTH   320      /* landscape */
#define LCD_HEIGHT  240

/* RGB565 colours */
#define C_BLACK   0x0000u
#define C_WHITE   0xFFFFu
#define C_RED     0xF800u
#define C_GREEN   0x07E0u
#define C_YELLOW  0xFFE0u
#define C_CYAN    0x07FFu
#define C_GREY    0x8410u
#define C_GRID    0x2965u    /* dim blue-grey */

void lcd_init(void);
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_push_color(uint16_t color, uint32_t count);

#endif /* LCD_H_INCLUDED */

/* ======================= font5x7.h ======================= */
/* font5x7.h - classic 5x7 font, ASCII 32..95 (space to '_', upper case) */
#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT_FIRST  32
#define FONT_LAST   95

extern const uint8_t font5x7[FONT_LAST - FONT_FIRST + 1][5];

#endif

/* ======================= gfx.h ======================= */
/* gfx.h - rectangles and text on top of lcd_set_window / lcd_push_color */
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define GFX_CHAR_W  6     /* 5 pixels + 1 space */
#define GFX_CHAR_H  8

void gfx_fill_rect(int x, int y, int w, int h, uint16_t color);
int  gfx_char(int x, int y, char ch, uint16_t fg, uint16_t bg, int scale);
int  gfx_text(int x, int y, const char *s, uint16_t fg, uint16_t bg, int scale);
int  gfx_text_width(const char *s, int scale);

#endif

/* ======================= spectrum.h ======================= */
/* ============================================================================
 * spectrum.h  -  signal processing: DC removal, Hann window, 512-point FFT,
 *                32 log-spaced bars, peak frequency, sound level.
 *                Pure C (no hardware access) so it can be unit tested on a PC.
 * ==========================================================================*/
#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>

#define SPEC_FFT_N     512u      /* samples per block (power of two)     */
#define SPEC_FS_HZ     16000u    /* sampling rate -> bandwidth 0..8 kHz  */
#define SPEC_NUM_BARS  32u      /* MAXIMUM bars; 8, 16 or 32 chosen at start-up */

typedef struct {
    float   bar_db[SPEC_NUM_BARS];  /* strongest bin in each bar, dBFS     */
    float   peak_hz;                /* dominant frequency                  */
    float   peak_db;                /* its level, dBFS                     */
    float   rms_dbfs;               /* overall loudness (0 dB = full-scale sine) */
    uint8_t level_pct;              /* rms mapped to 0..100 %              */
    uint8_t clipped;                /* ADC hit 0 or 4095                   */
    int16_t wave[SPEC_FFT_N];       /* DC-removed samples for scope view   */
} spectrum_result_t;

void     spectrum_init(void);
void     spectrum_load(const uint16_t *raw);     /* copy 12-bit ADC block */
void     spectrum_compute(spectrum_result_t *out);
void     spectrum_fft(float *re, float *im);      /* in-place, N points    */
uint16_t spectrum_bar_first_bin(uint32_t bar);
uint16_t spectrum_bar_end_bin(uint32_t bar);      /* exclusive             */
void     spectrum_set_bars(uint32_t n);           /* 8, 16 or 32 bars      */
uint32_t spectrum_num_bars(void);

#endif /* SPECTRUM_H */

/* ======================= ui.h ======================= */
/* ============================================================================
 * ui.h  -  screen layout and drawing for the spectrum analyzer
 *          (only redraws pixels that changed, so the TFT updates quickly)
 * ==========================================================================*/
#ifndef UI_H
#define UI_H

#include <stdint.h>

#define UI_MODE_BARS  0u
#define UI_MODE_WAVE  1u

void    ui_splash(uint8_t hse_ok);
void    ui_init(void);
void    ui_set_mode(uint8_t mode);
uint8_t ui_get_mode(void);
void    ui_set_grid(uint8_t on);
void    ui_set_paused(uint8_t paused);
void    ui_set_bt_linked(uint8_t linked);
void    ui_set_gain_db(int gain_db);
int     ui_get_gain_db(void);

void    ui_update(const spectrum_result_t *r);   /* draw one new frame       */
void    ui_refresh(void);                         /* redraw text if changed  */
uint8_t ui_peak_alert(const spectrum_result_t *r);
uint8_t ui_bar_digit(const spectrum_result_t *r, uint32_t bar); /* 0..9, Bluetooth */

void    ui_redraw_plot(void);                     /* full redraw from state  */

uint8_t ui_bars_menu(void);                       /* start-up menu: returns 8, 16 or 32 */
void    ui_set_bars(uint8_t n);                   /* 8, 16 or 32 bars        */
uint8_t ui_get_bars(void);

#endif /* UI_H */

/* ======================= bsp.c ======================= */
/* ============================================================================
 * bsp.c  -  register-level board support (CMSIS only, no HAL)
 * ==========================================================================*/



/* ------------------------------------------------------------------------ */
/* GPIO                                                                      */
/* ------------------------------------------------------------------------ */
void gpio_config(GPIO_TypeDef *port, uint32_t pin, uint32_t mode,
                 uint32_t pull, uint32_t af)
{
    uint32_t s2 = pin * 2u;

    if (mode == PIN_MODE_AF) {                   /* set AF before MODER     */
        uint32_t idx = pin >> 3;
        uint32_t sh  = (pin & 7u) * 4u;
        port->AFR[idx] = (port->AFR[idx] & ~(0xFu << sh)) | ((af & 0xFu) << sh);
    }
    port->OTYPER  &= ~(1u << pin);                               /* push-pull */
    port->OSPEEDR  = (port->OSPEEDR & ~(3u << s2)) | (2u << s2); /* fast      */
    port->PUPDR    = (port->PUPDR   & ~(3u << s2)) | ((pull & 3u) << s2);
    port->MODER    = (port->MODER   & ~(3u << s2)) | ((mode & 3u) << s2);
}

/* ------------------------------------------------------------------------ */
/* Clock: 84 MHz = (25 MHz / 25) * 336 / 4 ; USB clock 336/7 = 48 MHz        */
/* If the 25 MHz crystal does not start, the same PLL is fed from HSI 16 MHz */
/* ------------------------------------------------------------------------ */
static uint8_t s_hse_ok;

void bsp_clock_init(void)
{
    uint32_t t, pllm, cfg;

    /* 1. run from HSI while the PLL is (re)configured */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0u) { }
    RCC->CFGR &= ~RCC_CFGR_SW;                                   /* SW = HSI */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) { }
    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0u) { }

    /* 2. regulator scale 2 (required up to 84 MHz on F401) */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR = (PWR->CR & ~PWR_CR_VOS) | (2u << PWR_CR_VOS_Pos);

    /* 3. try the external 25 MHz crystal */
    RCC->CR |= RCC_CR_HSEON;
    for (t = 0u; t < 400000u; t++) {
        if ((RCC->CR & RCC_CR_HSERDY) != 0u) { break; }
    }
    s_hse_ok = ((RCC->CR & RCC_CR_HSERDY) != 0u) ? 1u : 0u;
    if (!s_hse_ok) { RCC->CR &= ~RCC_CR_HSEON; }
    pllm = s_hse_ok ? 25u : 16u;               /* PLL input = 1 MHz either way */

    /* 4. PLL: M, N=336, P=4, Q=7 (keep reserved bits untouched) */
    cfg  = RCC->PLLCFGR;
    cfg &= ~(RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP |
             RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLQ);
    cfg |= (pllm  << RCC_PLLCFGR_PLLM_Pos) |
           (336u  << RCC_PLLCFGR_PLLN_Pos) |
           (1u    << RCC_PLLCFGR_PLLP_Pos) |           /* 01 -> divide by 4 */
           (7u    << RCC_PLLCFGR_PLLQ_Pos);
    if (s_hse_ok) { cfg |= RCC_PLLCFGR_PLLSRC_HSE; }
    RCC->PLLCFGR = cfg;
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0u) { }
    for (t = 0u; t < 100000u; t++) {              /* regulator ready      */
        if ((PWR->CSR & PWR_CSR_VOSRDY) != 0u) { break; }
    }

    /* 5. flash: 2 wait states for 84 MHz @ 3.3 V, caches + prefetch on */
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN |
                 FLASH_ACR_LATENCY_2WS;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_2WS) { }

    /* 6. bus prescalers: AHB /1, APB1 /2 (42 MHz), APB2 /1 (84 MHz) */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2))
              | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

    /* 7. switch to PLL */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClock = SYSCLK_HZ;              /* CMSIS global variable       */
}

uint8_t bsp_clock_is_hse(void) { return s_hse_ok; }

/* ------------------------------------------------------------------------ */
/* SysTick 1 ms (CMSIS SysTick_Config)                                      */
/* ------------------------------------------------------------------------ */
static volatile uint32_t s_ms;

void bsp_systick_init(void)
{
    (void)SysTick_Config(SYSCLK_HZ / 1000u);
    NVIC_SetPriority(SysTick_IRQn, PRIO_SYSTICK);
}

void SysTick_Handler(void) { s_ms++; }

uint32_t millis(void) { return s_ms; }

void delay_ms(uint32_t ms)
{
    uint32_t start = s_ms;
    while ((uint32_t)(s_ms - start) < ms) { }
}

/* ------------------------------------------------------------------------ */
/* LEDs and HC-05 STATE input                                               */
/* ------------------------------------------------------------------------ */
void bsp_io_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    gpio_config(GPIOB, LED_PEAK_PIN,  PIN_MODE_OUT, PIN_PULL_NONE, 0u);
    gpio_config(GPIOB, LED_HEART_PIN, PIN_MODE_OUT, PIN_PULL_NONE, 0u);
    gpio_config(GPIOB, LED_GRID_PIN,  PIN_MODE_OUT, PIN_PULL_NONE, 0u);
    GPIOC->BSRR = 1u << LED_BOARD_PIN;                  /* off (active low) */
    gpio_config(GPIOC, LED_BOARD_PIN, PIN_MODE_OUT, PIN_PULL_NONE, 0u);
    GPIOB->BSRR = (1u << (LED_PEAK_PIN + 16u)) | (1u << (LED_HEART_PIN + 16u)) |
                  (1u << (LED_GRID_PIN + 16u));

    /* HC-05 STATE: pulled down, so "not connected" if the wire is absent */
    gpio_config(GPIOB, BT_STATE_PIN, PIN_MODE_IN, PIN_PULL_DOWN, 0u);
}

void led_set(uint32_t led, uint32_t on)
{
    switch (led) {
    case LED_PEAK:  GPIOB->BSRR = on ? (1u << LED_PEAK_PIN)  : (1u << (LED_PEAK_PIN + 16u));  break;
    case LED_HEART: GPIOB->BSRR = on ? (1u << LED_HEART_PIN) : (1u << (LED_HEART_PIN + 16u)); break;
    case LED_GRID:  GPIOB->BSRR = on ? (1u << LED_GRID_PIN)  : (1u << (LED_GRID_PIN + 16u));  break;
    case LED_BOARD: GPIOC->BSRR = on ? (1u << (LED_BOARD_PIN + 16u)) : (1u << LED_BOARD_PIN); break;
    default: break;
    }
}

uint8_t bsp_bt_linked(void)
{
    return ((GPIOB->IDR & (1u << BT_STATE_PIN)) != 0u) ? 1u : 0u;
}

/* ------------------------------------------------------------------------ */
/* Buttons: falling edge on PB12/13/14 -> EXTI15_10 interrupt (NVIC).       */
/* The ISR only records the event; the main loop confirms the pin is still  */
/* low 25 ms later, which rejects contact bounce and release bounce.        */
/* ------------------------------------------------------------------------ */
#define BTN_COUNT       3u
#define BTN_CONFIRM_MS  25u
#define BTN_LOCKOUT_MS  150u

static const uint8_t      k_btn_pin[BTN_COUNT] = { BTN_MODE_PIN, BTN_PAUSE_PIN, BTN_GRID_PIN };
static volatile uint8_t   s_btn_pend[BTN_COUNT];
static volatile uint32_t  s_btn_ts[BTN_COUNT];
static volatile uint32_t  s_btn_last[BTN_COUNT];

void bsp_buttons_init(void)
{
    uint32_t i, mask = 0u;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    for (i = 0u; i < BTN_COUNT; i++) {
        gpio_config(GPIOB, k_btn_pin[i], PIN_MODE_IN, PIN_PULL_UP, 0u);
        mask |= 1u << k_btn_pin[i];
    }

    /* EXTI12, EXTI13, EXTI14 -> port B (code 1) : SYSCFG_EXTICR4 */
    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~0x0FFFu) |
                        (1u << 0) | (1u << 4) | (1u << 8);

    EXTI->RTSR &= ~mask;
    EXTI->FTSR |=  mask;            /* button pulls the pin to GND           */
    EXTI->PR    =  mask;            /* clear anything pending                */
    EXTI->IMR  |=  mask;

    NVIC_SetPriority(EXTI15_10_IRQn, PRIO_EXTI);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler(void)
{
    uint32_t i, pr = EXTI->PR, now = s_ms;

    for (i = 0u; i < BTN_COUNT; i++) {
        uint32_t bit = 1u << k_btn_pin[i];
        if ((pr & bit) != 0u) {
            EXTI->PR = bit;                                  /* clear (w1c)  */
            if (!s_btn_pend[i] && (uint32_t)(now - s_btn_last[i]) >= BTN_LOCKOUT_MS) {
                s_btn_ts[i]   = now;
                s_btn_pend[i] = 1u;
            }
        }
    }
}

uint8_t bsp_buttons_poll(uint32_t now_ms)
{
    uint8_t  ev = 0u;
    uint32_t i;

    for (i = 0u; i < BTN_COUNT; i++) {
        if (s_btn_pend[i] && (uint32_t)(now_ms - s_btn_ts[i]) >= BTN_CONFIRM_MS) {
            if ((GPIOB->IDR & (1u << k_btn_pin[i])) == 0u) {   /* still pressed */
                s_btn_last[i] = now_ms;
                ev |= (uint8_t)(1u << i);
            }
            s_btn_pend[i] = 0u;
        }
    }
    return ev;
}

/* ------------------------------------------------------------------------ */
/* Audio capture                                                            */
/*   TIM2 update (16 kHz) -> TRGO -> starts one ADC1 conversion on PA1      */
/*   ADC end-of-conversion interrupt stores the sample. After SPEC_FFT_N    */
/*   samples the timer is stopped and the block is flagged as ready.        */
/* ------------------------------------------------------------------------ */
static uint16_t          s_cap[SPEC_FFT_N];
static volatile uint16_t s_cap_idx;
static volatile uint8_t  s_cap_ready;

void bsp_sampling_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    gpio_config(GPIOA, MIC_PIN, PIN_MODE_AN, PIN_PULL_NONE, 0u);

    /* TIM2: 84 MHz / 5250 = 16 kHz, update event -> TRGO (timer stopped) */
    TIM2->CR1 = 0u;
    TIM2->PSC = 0u;
    TIM2->ARR = (TIM2_CLK_HZ / SPEC_FS_HZ) - 1u;
    TIM2->CNT = 0u;
    TIM2->CR2 = TIM_CR2_MMS_1;                              /* MMS = 010     */

    /* ADC1: 21 MHz ADC clock, 12-bit, single channel, 84-cycle sampling */
    ADC1_COMMON->CCR = (ADC1_COMMON->CCR & ~ADC_CCR_ADCPRE) | ADC_CCR_ADCPRE_0; /* /4 */
    ADC1->CR1   = ADC_CR1_EOCIE;
    ADC1->SMPR2 = (ADC1->SMPR2 & ~ADC_SMPR2_SMP1) | (4u << ADC_SMPR2_SMP1_Pos);
    ADC1->SQR1  = 0u;                                       /* 1 conversion  */
    ADC1->SQR3  = MIC_ADC_CHANNEL;                          /* SQ1 = ch 1    */
    ADC1->SR    = 0u;
    ADC1->CR2   = ADC_CR2_ADON |
                  (6u << ADC_CR2_EXTSEL_Pos) |              /* TIM2_TRGO     */
                  ADC_CR2_EXTEN_0;                          /* rising edge   */
    delay_ms(1);                                            /* ADC power-up  */

    s_cap_idx   = 0u;
    s_cap_ready = 0u;
    NVIC_SetPriority(ADC_IRQn, PRIO_ADC);
    NVIC_EnableIRQ(ADC_IRQn);
}

void bsp_sampling_start(void)
{
    s_cap_idx   = 0u;
    s_cap_ready = 0u;
    TIM2->CNT   = 0u;
    TIM2->CR1  |= TIM_CR1_CEN;
}

void ADC_IRQHandler(void)
{
    uint32_t sr = ADC1->SR;

    if ((sr & ADC_SR_OVR) != 0u) {
        ADC1->SR = ~ADC_SR_OVR;                     /* rc_w0: write 0 clears */
    }
    if ((sr & ADC_SR_EOC) != 0u) {
        uint16_t v = (uint16_t)ADC1->DR;            /* reading DR clears EOC */
        if (s_cap_idx < SPEC_FFT_N) {
            s_cap[s_cap_idx] = v;
            s_cap_idx++;
            if (s_cap_idx >= SPEC_FFT_N) {
                TIM2->CR1 &= ~TIM_CR1_CEN;          /* block complete        */
                s_cap_ready = 1u;
            }
        }
    }
}

uint8_t bsp_capture_ready(void) { return s_cap_ready; }

const uint16_t *bsp_capture_buffer(void) { return s_cap; }

/* ------------------------------------------------------------------------ */
/* HardFault: fast-blink the on-board LED so a crash is visible             */
/* ------------------------------------------------------------------------ */
void HardFault_Handler(void)
{
    volatile uint32_t d;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER = (GPIOC->MODER & ~(3u << (LED_BOARD_PIN * 2u))) |
                   (1u << (LED_BOARD_PIN * 2u));
    for (;;) {
        GPIOC->ODR ^= (1u << LED_BOARD_PIN);
        for (d = 0u; d < 300000u; d++) { }
    }
}

/* ======================= uart_bt.c ======================= */
/* ============================================================================
 * uart_bt.c  -  USART1 <-> HC-05, TX and RX ring buffers serviced by the
 *               USART1 interrupt, so the main loop never waits on Bluetooth.
 * ==========================================================================*/


#define TX_SIZE 512u                  /* must be a power of two             */
#define RX_SIZE 64u

static char              s_tx[TX_SIZE];
static volatile uint16_t s_tx_head, s_tx_tail;
static char              s_rx[RX_SIZE];
static volatile uint16_t s_rx_head, s_rx_tail;

void bt_init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;

    gpio_config(GPIOA, BT_TX_PIN, PIN_MODE_AF, PIN_PULL_NONE, 7u);   /* AF7 */
    gpio_config(GPIOA, BT_RX_PIN, PIN_MODE_AF, PIN_PULL_UP,   7u);

    USART1->CR1 = 0u;
    USART1->CR2 = 0u;                                  /* 1 stop bit        */
    USART1->CR3 = 0u;
    USART1->BRR = (APB2_HZ + baud / 2u) / baud;        /* 9600 -> 0x222E    */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    NVIC_SetPriority(USART1_IRQn, PRIO_USART1);
    NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;

    /* RX (also clears ORE/FE/NE: read SR then DR) */
    if ((sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) != 0u) {
        char d = (char)(USART1->DR & 0xFFu);
        if ((sr & USART_SR_RXNE) != 0u) {
            uint16_t next = (uint16_t)((s_rx_head + 1u) & (RX_SIZE - 1u));
            if (next != s_rx_tail) {
                s_rx[s_rx_head] = d;
                s_rx_head = next;
            }
        }
    }

    /* TX */
    if (((USART1->CR1 & USART_CR1_TXEIE) != 0u) && ((sr & USART_SR_TXE) != 0u)) {
        if (s_tx_tail != s_tx_head) {
            USART1->DR = (uint8_t)s_tx[s_tx_tail];
            s_tx_tail = (uint16_t)((s_tx_tail + 1u) & (TX_SIZE - 1u));
        } else {
            USART1->CR1 &= ~USART_CR1_TXEIE;          /* nothing left       */
        }
    }
}

void bt_putc(char c)
{
    uint16_t next = (uint16_t)((s_tx_head + 1u) & (TX_SIZE - 1u));
    if (next == s_tx_tail) {
        return;                                       /* full: drop         */
    }
    s_tx[s_tx_head] = c;
    s_tx_head = next;
    USART1->CR1 |= USART_CR1_TXEIE;
}

void bt_puts(const char *s)
{
    while (*s != '\0') {
        bt_putc(*s);
        s++;
    }
}

void bt_put_uint(uint32_t v, uint32_t width)
{
    char     buf[10];
    uint32_t n = 0u;

    do {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v != 0u && n < sizeof(buf));
    while (n < width && n < sizeof(buf)) {
        buf[n++] = '0';
    }
    while (n > 0u) {
        bt_putc(buf[--n]);
    }
}

int bt_getc(void)
{
    char c;
    if (s_rx_tail == s_rx_head) {
        return -1;
    }
    c = s_rx[s_rx_tail];
    s_rx_tail = (uint16_t)((s_rx_tail + 1u) & (RX_SIZE - 1u));
    return (int)(unsigned char)c;
}

/* ======================= ili9341.c ======================= */
/* ============================================================================
 * ili9341.c  -  ILI9341 240x320 TFT driver over SPI1 (register level)
 *               SPI clock = 84 MHz / 8 = 10.5 MHz (safe on a breadboard)
 * ==========================================================================*/


/* SPI baud prescaler: SPI_CR1_BR_1 = /8 (10.5 MHz).
 * Short wires? You can try SPI_CR1_BR_0 (/4 = 21 MHz) for a faster screen. */
#define LCD_SPI_BR     SPI_CR1_BR_1

/* Orientation (MADCTL): 0x28 = landscape.  Image upside down? use 0xE8.
 * Red and blue swapped on your module? use 0x20 (or 0xE0).               */
#define LCD_MADCTL     0x28u

#define DC_LOW()   (GPIOB->BSRR = 1u << (TFT_DC_PIN + 16u))
#define DC_HIGH()  (GPIOB->BSRR = 1u << TFT_DC_PIN)
#define CS_LOW()   (GPIOA->BSRR = 1u << (TFT_CS_PIN + 16u))
#define CS_HIGH()  (GPIOA->BSRR = 1u << TFT_CS_PIN)
#define RST_LOW()  (GPIOB->BSRR = 1u << (TFT_RST_PIN + 16u))
#define RST_HIGH() (GPIOB->BSRR = 1u << TFT_RST_PIN)

static void spi_wait_idle(void)
{
    while ((SPI1->SR & SPI_SR_TXE) == 0u) { }
    while ((SPI1->SR & SPI_SR_BSY) != 0u) { }
}

static void spi_tx(uint8_t b)
{
    while ((SPI1->SR & SPI_SR_TXE) == 0u) { }
    SPI1->DR = b;
}

static void lcd_cmd(uint8_t c)
{
    spi_wait_idle();
    DC_LOW();
    spi_tx(c);
    spi_wait_idle();
    DC_HIGH();
}

static void lcd_data(uint8_t d)
{
    spi_tx(d);
}

/* Standard ILI9341 power/gamma setup: {cmd, number of data bytes, data...} */
static const uint8_t k_init[] = {
    0xEF, 3, 0x03, 0x80, 0x02,
    0xCF, 3, 0x00, 0xC1, 0x30,
    0xED, 4, 0x64, 0x03, 0x12, 0x81,
    0xE8, 3, 0x85, 0x00, 0x78,
    0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
    0xF7, 1, 0x20,
    0xEA, 2, 0x00, 0x00,
    0xC0, 1, 0x23,                         /* power control 1          */
    0xC1, 1, 0x10,                         /* power control 2          */
    0xC5, 2, 0x3E, 0x28,                   /* VCOM control 1           */
    0xC7, 1, 0x86,                         /* VCOM control 2           */
    0x37, 1, 0x00,                         /* vertical scroll = 0      */
    0x3A, 1, 0x55,                         /* 16 bits per pixel        */
    0xB1, 2, 0x00, 0x18,                   /* frame rate               */
    0xB6, 3, 0x08, 0x82, 0x27,             /* display function control */
    0xF2, 1, 0x00,                         /* 3-gamma off              */
    0x26, 1, 0x01,                         /* gamma curve 1            */
    0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
              0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
              0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    0x00                                   /* end of table             */
};

void lcd_init(void)
{
    uint32_t i = 0u;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    CS_HIGH(); DC_HIGH(); RST_HIGH();
    gpio_config(GPIOA, TFT_CS_PIN,  PIN_MODE_OUT, PIN_PULL_NONE, 0u);
    gpio_config(GPIOB, TFT_DC_PIN,  PIN_MODE_OUT, PIN_PULL_NONE, 0u);
    gpio_config(GPIOB, TFT_RST_PIN, PIN_MODE_OUT, PIN_PULL_NONE, 0u);
    gpio_config(GPIOA, 5u, PIN_MODE_AF, PIN_PULL_NONE, 5u);     /* SCK  */
    gpio_config(GPIOA, 6u, PIN_MODE_AF, PIN_PULL_UP,   5u);     /* MISO */
    gpio_config(GPIOA, 7u, PIN_MODE_AF, PIN_PULL_NONE, 5u);     /* MOSI */

    /* SPI1: master, mode 0, 8-bit, MSB first, software NSS */
    SPI1->CR1 = 0u;
    SPI1->CR2 = 0u;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | LCD_SPI_BR;
    SPI1->CR1 |= SPI_CR1_SPE;

    /* hardware reset */
    delay_ms(5);
    RST_LOW();
    delay_ms(20);
    RST_HIGH();
    delay_ms(150);

    CS_LOW();                      /* only device on the bus: keep selected */
    lcd_cmd(0x01);                 /* software reset                        */
    delay_ms(150);

    while (k_init[i] != 0x00u) {
        uint8_t cmd = k_init[i++];
        uint8_t n   = k_init[i++];
        lcd_cmd(cmd);
        while (n-- > 0u) {
            lcd_data(k_init[i++]);
        }
    }

    lcd_cmd(0x36);                 /* memory access control (orientation)   */
    lcd_data(LCD_MADCTL);
    lcd_cmd(0x11);                 /* sleep out                             */
    delay_ms(120);
    lcd_cmd(0x29);                 /* display on                            */
    delay_ms(20);
}

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_cmd(0x2A);                 /* column address set */
    lcd_data((uint8_t)(x0 >> 8)); lcd_data((uint8_t)x0);
    lcd_data((uint8_t)(x1 >> 8)); lcd_data((uint8_t)x1);
    lcd_cmd(0x2B);                 /* page address set   */
    lcd_data((uint8_t)(y0 >> 8)); lcd_data((uint8_t)y0);
    lcd_data((uint8_t)(y1 >> 8)); lcd_data((uint8_t)y1);
    lcd_cmd(0x2C);                 /* memory write       */
}

void lcd_push_color(uint16_t color, uint32_t count)
{
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;

    while (count-- > 0u) {
        spi_tx(hi);
        spi_tx(lo);
    }
}

/* ======================= font5x7.c ======================= */
/* font5x7.c - 5 columns per glyph, bit 0 = top row */

const uint8_t font5x7[FONT_LAST - FONT_FIRST + 1][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /*   */  {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */  {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */  {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */  {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */  {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14}, /* * */  {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */  {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */  {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */  {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */  {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */  {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */  {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */  {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */  {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x08,0x14,0x22,0x41,0x00}, /* < */  {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x00,0x41,0x22,0x14,0x08}, /* > */  {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */  {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */  {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */  {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */  {0x3E,0x41,0x49,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */  {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */  {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */  {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */  {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */  {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */  {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */  {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */  {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */  {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */  {0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */  {0x00,0x41,0x41,0x7F,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */  {0x40,0x40,0x40,0x40,0x40}  /* _ */
};

/* ======================= gfx.c ======================= */
/* gfx.c - clipped rectangle fill and 5x7 text (opaque background) */



void gfx_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    int x0 = x, y0 = y, x1 = x + w, y1 = y + h;

    if (w <= 0 || h <= 0) {
        return;
    }
    if (x0 < 0) { x0 = 0; }
    if (y0 < 0) { y0 = 0; }
    if (x1 > LCD_WIDTH)  { x1 = LCD_WIDTH; }
    if (y1 > LCD_HEIGHT) { y1 = LCD_HEIGHT; }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }
    lcd_set_window((uint16_t)x0, (uint16_t)y0, (uint16_t)(x1 - 1), (uint16_t)(y1 - 1));
    lcd_push_color(color, (uint32_t)(x1 - x0) * (uint32_t)(y1 - y0));
}

int gfx_char(int x, int y, char ch, uint16_t fg, uint16_t bg, int scale)
{
    const uint8_t *g;
    int row, sr, col, cw, chh;

    if (scale < 1) {
        scale = 1;
    }
    cw  = GFX_CHAR_W * scale;
    chh = GFX_CHAR_H * scale;
    if (x < 0 || y < 0 || x + cw > LCD_WIDTH || y + chh > LCD_HEIGHT) {
        return x + cw;                          /* off screen: skip */
    }
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }
    if (ch < FONT_FIRST || ch > FONT_LAST) {
        ch = '?';
    }
    g = font5x7[ch - FONT_FIRST];

    lcd_set_window((uint16_t)x, (uint16_t)y, (uint16_t)(x + cw - 1), (uint16_t)(y + chh - 1));
    for (row = 0; row < GFX_CHAR_H; row++) {
        for (sr = 0; sr < scale; sr++) {
            for (col = 0; col < GFX_CHAR_W; col++) {
                int on = (col < 5) && (((g[col] >> row) & 1u) != 0u);
                lcd_push_color(on ? fg : bg, (uint32_t)scale);
            }
        }
    }
    return x + cw;
}

int gfx_text(int x, int y, const char *s, uint16_t fg, uint16_t bg, int scale)
{
    while (*s != '\0') {
        x = gfx_char(x, y, *s, fg, bg, scale);
        s++;
    }
    return x;
}

int gfx_text_width(const char *s, int scale)
{
    int n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n * GFX_CHAR_W * scale;
}

/* ======================= spectrum.c ======================= */
/* ============================================================================
 * spectrum.c  -  FFT based audio analysis (single precision, uses the M4 FPU)
 * ==========================================================================*/

#include <math.h>

#define N      SPEC_FFT_N
#define HALF   (SPEC_FFT_N / 2u)
static uint32_t s_nbars = SPEC_NUM_BARS;   /* active bar count: 8, 16 or 32 */
#define NB     s_nbars
#define PI_F   3.14159265358979f

static float    s_x[N];                /* DC-removed samples          */
static float    s_re[N], s_im[N];      /* FFT work buffers            */
static float    s_win[N];              /* Hann window                 */
static float    s_twr[HALF], s_twi[HALF];
static uint16_t s_rev[N];              /* bit-reversal permutation    */
static uint16_t s_edge[SPEC_NUM_BARS + 1u]; /* bar b = bins [edge[b], edge[b+1]) */
static float    s_rms;
static uint8_t  s_sp_clip;

/* Full-scale sine (amplitude 2048 counts) on an exact bin, Hann window:
 * |X[k]| = 2048 * N / 4.  Its square is the 0 dBFS reference.          */
static const float k_ref2 = (2048.0f * (float)N / 4.0f) * (2048.0f * (float)N / 4.0f);

void spectrum_init(void)
{
    uint32_t i, bits = 0u;

    while ((1u << bits) < N) {
        bits++;
    }
    for (i = 0u; i < N; i++) {
        uint32_t r = 0u, v = i, k;
        for (k = 0u; k < bits; k++) {
            r = (r << 1) | (v & 1u);
            v >>= 1;
        }
        s_rev[i] = (uint16_t)r;
        s_win[i] = 0.5f - 0.5f * cosf(2.0f * PI_F * (float)i / (float)N);
    }
    for (i = 0u; i < HALF; i++) {
        s_twr[i] =  cosf(2.0f * PI_F * (float)i / (float)N);
        s_twi[i] = -sinf(2.0f * PI_F * (float)i / (float)N);
    }

    spectrum_set_bars(s_nbars);
}

/* logarithmic bar edges from bin 1 (31 Hz) to bin 256 (8 kHz),
 * at least one bin per bar.  n = 8, 16 or 32 (anything else -> 32). */
void spectrum_set_bars(uint32_t n)
{
    uint32_t b, prev = 0u;

    if (n != 8u && n != 16u) {
        n = 32u;
    }
    s_nbars = n;
    for (b = 0u; b <= NB; b++) {
        uint32_t e = (uint32_t)(powf((float)HALF, (float)b / (float)NB) + 0.5f);
        if (b > 0u && e <= prev) {
            e = prev + 1u;
        }
        if (e > HALF) {
            e = HALF;
        }
        s_edge[b] = (uint16_t)e;
        prev = e;
    }
    s_edge[NB] = (uint16_t)HALF;
}

uint32_t spectrum_num_bars(void) { return s_nbars; }

uint16_t spectrum_bar_first_bin(uint32_t bar) { return s_edge[bar]; }
uint16_t spectrum_bar_end_bin(uint32_t bar)   { return s_edge[bar + 1u]; }

void spectrum_fft(float *re, float *im)
{
    uint32_t i, j, len;

    for (i = 0u; i < N; i++) {
        j = s_rev[i];
        if (i < j) {
            float t;
            t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (len = 2u; len <= N; len <<= 1) {
        uint32_t half = len >> 1;
        uint32_t step = N / len;
        for (i = 0u; i < N; i += len) {
            for (j = 0u; j < half; j++) {
                float    wr = s_twr[j * step];
                float    wi = s_twi[j * step];
                uint32_t u  = i + j;
                uint32_t v  = u + half;
                float    tr = re[v] * wr - im[v] * wi;
                float    ti = re[v] * wi + im[v] * wr;
                re[v] = re[u] - tr;
                im[v] = im[u] - ti;
                re[u] += tr;
                im[u] += ti;
            }
        }
    }
}

void spectrum_load(const uint16_t *raw)
{
    uint32_t i, sum = 0u;
    float    mean, ss = 0.0f;

    s_sp_clip = 0u;
    for (i = 0u; i < N; i++) {
        sum += raw[i];
        if (raw[i] <= 4u || raw[i] >= 4091u) {
            s_sp_clip = 1u;
        }
    }
    mean = (float)sum / (float)N;
    for (i = 0u; i < N; i++) {
        float v = (float)raw[i] - mean;
        s_x[i] = v;
        ss += v * v;
    }
    s_rms = sqrtf(ss / (float)N);
}

static float to_db(float mag2)
{
    return 10.0f * log10f(mag2 / k_ref2 + 1e-12f);
}

void spectrum_compute(spectrum_result_t *out)
{
    uint32_t i, b, k, best;
    float    a, c, d, den, pk, lvl;

    for (i = 0u; i < N; i++) {
        float v = s_x[i];
        int32_t w = (int32_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
        if (w > 32767)  { w = 32767; }
        if (w < -32768) { w = -32768; }
        out->wave[i] = (int16_t)w;
        s_re[i] = v * s_win[i];
        s_im[i] = 0.0f;
    }

    spectrum_fft(s_re, s_im);

    for (k = 0u; k < HALF; k++) {               /* power spectrum */
        s_re[k] = s_re[k] * s_re[k] + s_im[k] * s_im[k];
    }

    for (b = 0u; b < NB; b++) {
        float m = 0.0f;
        for (k = s_edge[b]; k < s_edge[b + 1u]; k++) {
            if (s_re[k] > m) {
                m = s_re[k];
            }
        }
        out->bar_db[b] = to_db(m);
    }

    /* dominant frequency, refined with parabolic interpolation in dB */
    best = 2u;
    for (k = 3u; k < HALF - 1u; k++) {
        if (s_re[k] > s_re[best]) {
            best = k;
        }
    }
    a   = to_db(s_re[best - 1u]);
    pk  = to_db(s_re[best]);
    c   = to_db(s_re[best + 1u]);
    den = a - 2.0f * pk + c;
    d   = (den < -1e-6f) ? 0.5f * (a - c) / den : 0.0f;
    if (d > 0.5f)  { d = 0.5f; }
    if (d < -0.5f) { d = -0.5f; }
    out->peak_hz = ((float)best + d) * (float)SPEC_FS_HZ / (float)N;
    out->peak_db = pk;

    /* loudness: 0 dBFS = full-scale sine (rms 2048/sqrt2) */
    out->rms_dbfs = 20.0f * log10f(s_rms / 1448.155f + 1e-9f);
    lvl = (out->rms_dbfs + 60.0f) * (100.0f / 60.0f);
    if (lvl < 0.0f)   { lvl = 0.0f; }
    if (lvl > 100.0f) { lvl = 100.0f; }
    out->level_pct = (uint8_t)(lvl + 0.5f);
    out->clipped   = s_sp_clip;
}

#undef N
#undef HALF
#undef NB
#undef PI_F

/* ======================= ui.c ======================= */
/* ============================================================================
 * ui.c  -  320x240 layout
 *
 *   y   0..10   title + status (BARS/WAVE, RUN/PAUSE, BT link)
 *   y  14..21   peak frequency, level, gain, CLIP warning
 *   y  30..209  plot area (180 px): 32 spectrum bars or oscilloscope trace
 *   y 214..221  axis labels (Hz or ms)
 * ==========================================================================*/



#define HDR_Y        2
#define INFO_Y       14
#define SEP_Y        25
#define PLOT_Y0      30
#define PLOT_H       180
#define PLOT_YB      (PLOT_Y0 + PLOT_H - 1)      /* bottom row = 209 */
#define AXIS_Y       214

static int s_nb   = (int)SPEC_NUM_BARS;   /* bars on screen: 8, 16 or 32      */
static int s_slot = 10;                   /* pixels per bar slot (320 / bars)  */
static int s_bw   = 8;                    /* bar width (slot - 2)              */
static uint8_t s_ui_ready = 0u;
#define NB           (s_nb)
#define BAR_SLOT     (s_slot)
#define BAR_W        (s_bw)
#define BAR_X0       1
#define MARK_H       2
#define H_MAX        (PLOT_H - MARK_H)           /* tallest bar = 178 px */
#define GREEN_TOP    108                          /* 60 % : green -> yellow */
#define YELLOW_TOP   153                          /* 85 % : yellow -> red   */
#define FALL_PX      6                            /* bar fall speed / frame */
#define PEAK_HOLD    18                           /* frames peak dot holds  */
#define PEAK_FALL_PX 2

#define DB_FLOOR     (-75.0f)                     /* bottom of the plot     */
#define DB_CEIL      (-15.0f)                     /* top of the plot        */

#define STATUS_X     (LCD_WIDTH - 18 * GFX_CHAR_W)
#define CLIP_X       (LCD_WIDTH - 5 * GFX_CHAR_W)

static const int k_grid_h[3] = { 45, 90, 135 };   /* every 15 dB            */

static uint8_t s_mode   = UI_MODE_BARS;
static uint8_t s_grid   = 1u;
static uint8_t s_paused = 0u;
static uint8_t s_bt     = 0u;
static int     s_gain   = 0;
static uint8_t s_status_dirty = 1u;
static uint8_t s_info_dirty   = 1u;
static uint32_t s_frame;

/* current on-screen state */
static uint8_t s_h[SPEC_NUM_BARS];      /* bar heights (px)                 */
static uint8_t s_pk[SPEC_NUM_BARS];     /* peak-dot heights (px)            */
static uint8_t s_hold[SPEC_NUM_BARS];
static uint8_t s_wlo[LCD_WIDTH];        /* scope column, heights [lo, hi)   */
static uint8_t s_whi[LCD_WIDTH];

/* last values for the info line */
static float   s_peak_hz;
static float   s_peak_db = -120.0f;
static uint8_t s_level;
static uint8_t s_clip;

/* ---------------------------------------------------------------------- */
/* small text helpers (no printf needed)                                   */
/* ---------------------------------------------------------------------- */
static char *put_str(char *p, const char *s)
{
    while (*s != '\0') { *p++ = *s++; }
    return p;
}

static char *put_uint(char *p, uint32_t v, int width)   /* right aligned */
{
    char tmp[10];
    int  n = 0;
    do {
        tmp[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v != 0u && n < 10);
    while (width > n) { *p++ = ' '; width--; }
    while (n > 0)     { *p++ = tmp[--n]; }
    return p;
}

/* ---------------------------------------------------------------------- */
/* plot primitives. "height" h counts up from the bottom row of the plot. */
/* ---------------------------------------------------------------------- */
static void rect_h(int x, int w, int a, int b, uint16_t c)   /* heights [a,b) */
{
    if (b > a) {
        gfx_fill_rect(x, PLOT_Y0 + PLOT_H - b, w, b - a, c);
    }
}

static void restore_bg(int x, int w, int a, int b)
{
    int i;
    if (b <= a) {
        return;
    }
    rect_h(x, w, a, b, C_BLACK);
    if (s_grid) {
        for (i = 0; i < 3; i++) {
            if (k_grid_h[i] >= a && k_grid_h[i] < b) {
                gfx_fill_rect(x, PLOT_YB - k_grid_h[i], w, 1, C_GRID);
            }
        }
    }
}

static void paint_bar(int x, int a, int b)       /* green / yellow / red  */
{
    int lo, hi;
    lo = a;               hi = (b < GREEN_TOP) ? b : GREEN_TOP;
    rect_h(x, BAR_W, lo, hi, C_GREEN);
    lo = (a > GREEN_TOP) ? a : GREEN_TOP;   hi = (b < YELLOW_TOP) ? b : YELLOW_TOP;
    rect_h(x, BAR_W, lo, hi, C_YELLOW);
    lo = (a > YELLOW_TOP) ? a : YELLOW_TOP; hi = b;
    rect_h(x, BAR_W, lo, hi, C_RED);
}

static int bar_x(int b) { return BAR_X0 + b * BAR_SLOT; }

static int db_to_h(float db)
{
    float h = (db + (float)s_gain - DB_FLOOR) * ((float)PLOT_H / (DB_CEIL - DB_FLOOR));
    if (h < 0.0f)          { return 0; }
    if (h > (float)H_MAX)  { return H_MAX; }
    return (int)h;
}

/* ---------------------------------------------------------------------- */
/* axis labels                                                            */
/* ---------------------------------------------------------------------- */
static void draw_axis(void)
{
    static const uint16_t freqs[7]  = { 100u, 200u, 500u, 1000u, 2000u, 4000u, 8000u };
    static const char    *flabel[7] = { "100", "200", "500", "1K", "2K", "4K", "8K" };
    static const char    *tlabel[5] = { "0", "5MS", "10MS", "15MS", "20MS" };
    int i, last_end = -100;

    gfx_fill_rect(0, AXIS_Y, LCD_WIDTH, GFX_CHAR_H, C_BLACK);

    if (s_mode == UI_MODE_BARS) {
        gfx_text(0, AXIS_Y, "HZ", C_GREY, C_BLACK, 1);
        last_end = 2 * GFX_CHAR_W;
        for (i = 0; i < 7; i++) {
            uint32_t bin = ((uint32_t)freqs[i] * SPEC_FFT_N) / SPEC_FS_HZ;
            int b, found = -1;
            for (b = 0; b < NB; b++) {
                if (bin >= spectrum_bar_first_bin((uint32_t)b) &&
                    bin <  spectrum_bar_end_bin((uint32_t)b)) {
                    found = b;
                    break;
                }
            }
            if (found < 0) {
                found = NB - 1;              /* 8 kHz = top of the last bar */
            }
            {
                int w = gfx_text_width(flabel[i], 1);
                int x = bar_x(found) + BAR_W / 2 - w / 2;
                if (x < 0)              { x = 0; }
                if (x > LCD_WIDTH - w)  { x = LCD_WIDTH - w; }
                if (x >= last_end + 4) {
                    gfx_text(x, AXIS_Y, flabel[i], C_GREY, C_BLACK, 1);
                    last_end = x + w;
                }
            }
        }
    } else {
        for (i = 0; i < 5; i++) {
            int w = gfx_text_width(tlabel[i], 1);
            int x = (i * (LCD_WIDTH - 1)) / 4 - w / 2;
            if (x < 0)             { x = 0; }
            if (x > LCD_WIDTH - w) { x = LCD_WIDTH - w; }
            gfx_text(x, AXIS_Y, tlabel[i], C_GREY, C_BLACK, 1);
        }
    }
}

/* ---------------------------------------------------------------------- */
/* full plot redraw from the stored state                                  */
/* ---------------------------------------------------------------------- */
void ui_redraw_plot(void)
{
    int i, b, x;

    gfx_fill_rect(0, PLOT_Y0, LCD_WIDTH, PLOT_H, C_BLACK);
    if (s_grid) {
        for (i = 0; i < 3; i++) {
            gfx_fill_rect(0, PLOT_YB - k_grid_h[i], LCD_WIDTH, 1, C_GRID);
        }
    }
    if (s_mode == UI_MODE_BARS) {
        for (b = 0; b < NB; b++) {
            paint_bar(bar_x(b), 0, s_h[b]);
            rect_h(bar_x(b), BAR_W, s_pk[b], s_pk[b] + MARK_H, C_WHITE);
        }
    } else {
        for (x = 0; x < LCD_WIDTH; x++) {
            rect_h(x, 1, s_wlo[x], s_whi[x], C_CYAN);
        }
    }
    draw_axis();
}

static void reset_state(void)
{
    int i;
    for (i = 0; i < NB; i++) {
        s_h[i] = 0u; s_pk[i] = 0u; s_hold[i] = 0u;
    }
    for (i = 0; i < LCD_WIDTH; i++) {
        s_wlo[i] = 0u; s_whi[i] = 0u;
    }
}

/* ---------------------------------------------------------------------- */
/* status line and info line                                               */
/* ---------------------------------------------------------------------- */
static void draw_status(void)
{
    char buf[20], *p = buf;
    p = put_str(p, (s_mode == UI_MODE_BARS) ? "BARS " : "WAVE ");
    p = put_str(p, s_paused ? "PAUSE " : "RUN   ");
    p = put_str(p, s_bt ? "BT:LINK" : "BT:----");
    *p = '\0';
    gfx_text(STATUS_X, HDR_Y, buf, s_paused ? C_YELLOW : C_WHITE, C_BLACK, 1);
    s_status_dirty = 0u;
}

static void draw_info(void)
{
    char buf[48], *p = buf;
    int  g = s_gain;

    p = put_str(p, "PEAK ");
    if (s_peak_db + (float)s_gain > DB_FLOOR + 10.0f) {
        p = put_uint(p, (uint32_t)(s_peak_hz + 0.5f), 4);
    } else {
        p = put_str(p, " ---");
    }
    p = put_str(p, " HZ  LEVEL ");
    p = put_uint(p, s_level, 3);
    p = put_str(p, "%  GAIN ");
    *p++ = (g < 0) ? '-' : '+';
    if (g < 0) { g = -g; }
    p = put_uint(p, (uint32_t)g, 2);
    p = put_str(p, " DB");
    *p = '\0';
    gfx_text(0, INFO_Y, buf, C_WHITE, C_BLACK, 1);
    gfx_text(CLIP_X, INFO_Y, s_clip ? "CLIP" : "    ", C_RED, C_BLACK, 1);
    s_info_dirty = 0u;
}

/* ---------------------------------------------------------------------- */
/* public API                                                              */
/* ---------------------------------------------------------------------- */
static void text_center(int y, const char *s, uint16_t c, int scale)
{
    gfx_text((LCD_WIDTH - gfx_text_width(s, scale)) / 2, y, s, c, C_BLACK, scale);
}

void ui_splash(uint8_t hse_ok)
{
    gfx_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, C_BLACK);
    text_center(56,  "AUDIO SPECTRUM", C_CYAN, 3);
    text_center(88,  "ANALYZER", C_CYAN, 3);
    text_center(136, "STM32F401 + MAX4466 + ILI9341", C_WHITE, 1);
    text_center(152, "HC-05 BLUETOOTH  9600 BAUD", C_WHITE, 1);
    text_center(176, hse_ok ? "CLOCK 84 MHZ (25 MHZ CRYSTAL)" : "CLOCK 84 MHZ (HSI, NO CRYSTAL)",
                hse_ok ? C_GREEN : C_YELLOW, 1);
}

void ui_init(void)
{
    gfx_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, C_BLACK);
    gfx_text(0, HDR_Y, "STM32F401 AUDIO SPECTRUM", C_CYAN, C_BLACK, 1);
    gfx_fill_rect(0, SEP_Y, LCD_WIDTH, 1, C_GRID);
    reset_state();
    s_frame = 0u;
    s_ui_ready = 1u;
    ui_redraw_plot();
    draw_status();
    draw_info();
}

void ui_set_mode(uint8_t mode)
{
    mode = (mode == UI_MODE_WAVE) ? UI_MODE_WAVE : UI_MODE_BARS;
    if (mode != s_mode) {
        s_mode = mode;
        reset_state();
        ui_redraw_plot();
        s_status_dirty = 1u;
    }
}

uint8_t ui_get_mode(void) { return s_mode; }

void ui_set_bars(uint8_t n)
{
    if (n != 8u && n != 16u) {
        n = 32u;
    }
    s_nb   = (int)n;
    s_slot = LCD_WIDTH / (int)n;         /* 40 / 20 / 10 px */
    s_bw   = s_slot - 2;                 /* 38 / 18 /  8 px */
    spectrum_set_bars(n);
    if (s_ui_ready) {
        reset_state();
        ui_redraw_plot();
    }
}

uint8_t ui_get_bars(void) { return (uint8_t)s_nb; }

void ui_set_grid(uint8_t on)
{
    on = on ? 1u : 0u;
    if (on != s_grid) {
        s_grid = on;
        ui_redraw_plot();            /* keeps the current bars / trace */
    }
}

void ui_set_paused(uint8_t paused)
{
    paused = paused ? 1u : 0u;
    if (paused != s_paused) { s_paused = paused; s_status_dirty = 1u; }
}

void ui_set_bt_linked(uint8_t linked)
{
    linked = linked ? 1u : 0u;
    if (linked != s_bt) { s_bt = linked; s_status_dirty = 1u; }
}

void ui_set_gain_db(int gain_db)
{
    if (gain_db != s_gain) { s_gain = gain_db; s_info_dirty = 1u; }
}

int ui_get_gain_db(void) { return s_gain; }

static void update_bars(const spectrum_result_t *r)
{
    int b;
    for (b = 0; b < NB; b++) {
        int x   = bar_x(b);
        int old = s_h[b];
        int opk = s_pk[b];
        int t   = db_to_h(r->bar_db[b]);
        int nh, npk;

        /* bars jump up instantly and fall smoothly */
        nh = (t >= old) ? t : ((old - FALL_PX > t) ? old - FALL_PX : t);

        /* peak dot: holds, then falls, never below the bar */
        if (nh >= opk) {
            npk = nh;
            s_hold[b] = PEAK_HOLD;
        } else if (s_hold[b] > 0u) {
            npk = opk;
            s_hold[b]--;
        } else {
            npk = opk - PEAK_FALL_PX;
            if (npk < nh) { npk = nh; }
        }

        if (npk != opk) {
            restore_bg(x, BAR_W, opk, opk + MARK_H);       /* remove old dot */
        }
        if (nh > old) {
            paint_bar(x, old, nh);                          /* grow          */
        } else if (nh < old) {
            restore_bg(x, BAR_W, nh, old);                  /* shrink        */
        }
        if (npk != opk) {
            rect_h(x, BAR_W, npk, npk + MARK_H, C_WHITE);   /* new dot       */
        }
        s_h[b]  = (uint8_t)nh;
        s_pk[b] = (uint8_t)npk;
    }
}

static void update_wave(const spectrum_result_t *r)
{
    const int span = LCD_WIDTH;                 /* 320 samples = 20 ms   */
    const int mid  = PLOT_H / 2;
    int   i, x, t = 0, prev = mid, peak = 0;
    float scale;

    /* trigger on a rising zero crossing so the trace stands still */
    for (i = 1; i <= (int)SPEC_FFT_N - span; i++) {
        if (r->wave[i - 1] < 0 && r->wave[i] >= 0) {
            t = i;
            break;
        }
    }
    for (i = 0; i < span; i++) {
        int v = r->wave[t + i];
        if (v < 0) { v = -v; }
        if (v > peak) { peak = v; }
    }
    if (peak < 64) { peak = 64; }               /* do not magnify silence */
    scale = (float)(mid - 3) / (float)peak;

    for (x = 0; x < span; x++) {
        int hy = mid + (int)((float)r->wave[t + x] * scale);
        int lo, hi, nlo, nhi, olo, ohi;

        if (hy < 0)          { hy = 0; }
        if (hy > PLOT_H - 1) { hy = PLOT_H - 1; }
        lo = hy; hi = hy;
        if (x > 0) {                            /* join to previous point */
            if (prev < lo) { lo = prev; }
            if (prev > hi) { hi = prev; }
        }
        nlo = lo;
        nhi = hi + 1;
        olo = s_wlo[x];
        ohi = s_whi[x];
        if (nlo != olo || nhi != ohi) {
            if (ohi > olo) {                    /* erase only what is not reused */
                restore_bg(x, 1, olo, (ohi < nlo) ? ohi : nlo);
                restore_bg(x, 1, (olo > nhi) ? olo : nhi, ohi);
            }
            rect_h(x, 1, nlo, nhi, C_CYAN);
            s_wlo[x] = (uint8_t)nlo;
            s_whi[x] = (uint8_t)nhi;
        }
        prev = hy;
    }
}

void ui_update(const spectrum_result_t *r)
{
    s_frame++;
    if (s_mode == UI_MODE_BARS) {
        update_bars(r);
    } else {
        update_wave(r);
    }

    s_peak_hz = r->peak_hz;
    s_peak_db = r->peak_db;
    s_level   = r->level_pct;
    s_clip    = r->clipped;
    if ((s_frame & 3u) == 0u) {                 /* text 7-8 times a second */
        s_info_dirty = 1u;
    }
    ui_refresh();
}

void ui_refresh(void)
{
    if (s_status_dirty) { draw_status(); }
    if (s_info_dirty)   { draw_info(); }
}

uint8_t ui_peak_alert(const spectrum_result_t *r)
{
    uint32_t b;
    if (r->clipped) {
        return 1u;
    }
    for (b = 0u; b < (uint32_t)NB; b++) {
        if (r->bar_db[b] + (float)s_gain >= DB_CEIL - 3.0f) {
            return 1u;
        }
    }
    return 0u;
}

uint8_t ui_bar_digit(const spectrum_result_t *r, uint32_t bar)
{
    if (bar >= (uint32_t)NB) {
        return 0u;
    }
    return (uint8_t)((db_to_h(r->bar_db[bar]) * 10) / (H_MAX + 1));
}


/* ---------------------------------------------------------------------- */
/* start-up menu: choose 8, 16 or 32 bars with the three push buttons     */
/*   MODE (PB12) = 8 bars   PAUSE (PB13) = 16 bars   GRID (PB14) = 32 bars */
/* ---------------------------------------------------------------------- */
static void menu_frame(int x, int y, int w, int h, uint16_t c)
{
    gfx_fill_rect(x, y,         w, 2, c);
    gfx_fill_rect(x, y + h - 2, w, 2, c);
    gfx_fill_rect(x, y,         2, h, c);
    gfx_fill_rect(x + w - 2, y, 2, h, c);
}

static void menu_card(int idx, uint8_t n, uint16_t col, uint8_t selected)
{
    const int cw = 96, ch = 140, cy = 46, gap = 8;
    int  x = 8 + idx * (cw + gap);
    int  slot = 80 / (int)n;
    int  bw   = (slot > 1) ? slot - 1 : 1;
    int  bx   = x + (cw - slot * (int)n) / 2;
    int  base = cy + ch - 10;
    char num[4], *p = num;
    int  i;

    gfx_fill_rect(x, cy, cw, ch, selected ? 0x0320u : C_BLACK);   /* dark green when chosen */
    menu_frame(x, cy, cw, ch, selected ? C_GREEN : C_GREY);

    p = put_uint(p, n, 0);
    *p = '\0';
    gfx_text(x + (cw - gfx_text_width(num, 4)) / 2, cy + 10, num, col, selected ? 0x0320u : C_BLACK, 4);
    gfx_text(x + (cw - gfx_text_width("BARS", 2)) / 2, cy + 48, "BARS", C_WHITE, selected ? 0x0320u : C_BLACK, 2);

    for (i = 0; i < (int)n; i++) {                 /* little preview of the bars */
        int h = 40 - (i * 26) / (int)n - ((i * 7) % 5) * 3;
        if (h < 4) { h = 4; }
        gfx_fill_rect(bx + i * slot, base - h, bw, h, (h > 32) ? C_YELLOW : C_GREEN);
    }
}

uint8_t ui_bars_menu(void)
{
    static const uint8_t  opt[3] = { 8u, 16u, 32u };
    static const char    *lbl[3] = { "PRESS MODE", "PRESS PAUSE", "PRESS GRID" };
    static const char    *pin[3] = { "BUTTON PB12", "BUTTON PB13", "BUTTON PB14" };
    static const uint16_t col[3] = { C_CYAN, C_YELLOW, C_GREEN };
    int     i, sel = -1;
    char    msg[24], *p = msg;

    gfx_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, C_BLACK);
    text_center(10, "SELECT BARS", C_CYAN, 3);
    for (i = 0; i < 3; i++) {
        int x = 8 + i * (96 + 8);
        menu_card(i, opt[i], col[i], 0u);
        gfx_text(x + (96 - gfx_text_width(lbl[i], 1)) / 2, 192, lbl[i], col[i], C_BLACK, 1);
        gfx_text(x + (96 - gfx_text_width(pin[i], 1)) / 2, 204, pin[i], C_GREY, C_BLACK, 1);
    }
    text_center(224, "PRESS A BUTTON TO CHOOSE", C_WHITE, 1);

    while (sel < 0) {
        uint8_t ev = bsp_buttons_poll(millis());
        if      (ev & EV_MODE)  { sel = 0; }
        else if (ev & EV_PAUSE) { sel = 1; }
        else if (ev & EV_GRID)  { sel = 2; }
    }

    menu_card(sel, opt[sel], col[sel], 1u);                       /* highlight */
    gfx_fill_rect(0, 220, LCD_WIDTH, 16, C_BLACK);
    p = put_uint(p, opt[sel], 0);
    p = put_str(p, " BARS SELECTED");
    *p = '\0';
    text_center(224, msg, C_GREEN, 1);
    delay_ms(800);
    return opt[sel];
}

#undef NB

/* ======================= main.c ======================= */
/* ============================================================================
 *  STM32F401 Real-Time Audio Spectrum Analyzer
 *  Keil uVision 5 + CMSIS (register level, no HAL)
 *
 *  Sensor   : MAX4466 electret microphone amplifier  -> ADC1 (PA1)
 *  Display  : ILI9341 2.4"/2.8" 320x240 TFT          -> SPI1
 *  Wireless : HC-05 Bluetooth                        -> USART1 (9600 baud)
 *  Inputs   : 3 push buttons on EXTI interrupts (NVIC)
 *  Outputs  : 3 LEDs (peak, heartbeat, grid) + on-board LED (paused)
 *
 *  Signal chain: TIM2 (16 kHz) triggers ADC1 -> ADC interrupt fills a
 *  512-sample block -> Hann window -> 512-point FFT -> 32 log-spaced bars
 *  (31 Hz .. 8 kHz) -> TFT + Bluetooth.
 *
 *  Bluetooth commands (any serial terminal app, 9600 baud):
 *    M = bars/wave   P = pause/resume   G = grid   B = 8/16/32 bars
 *    + / - = gain   ? = help
 *  At power-up the TFT asks for 8, 16 or 32 bars (MODE / PAUSE / GRID button).
 * ==========================================================================*/






#define BT_REPORT_MS   250u        /* spectrum report to the phone, 4 / s   */
#define GAIN_STEP_DB   6
#define GAIN_MIN_DB    (-12)
#define GAIN_MAX_DB    24

static spectrum_result_t s_res;    /* large: kept off the stack             */

static void bt_help(void)
{
    bt_puts("\r\nCommands: M=bars/wave  P=pause  G=grid  B=8/16/32 bars  +/-=gain  ?=help\r\n");
}

static void bt_report(const spectrum_result_t *r)
{
    uint32_t b;
    bt_puts("F=");
    bt_put_uint((uint32_t)(r->peak_hz + 0.5f), 4);
    bt_puts("Hz L=");
    bt_put_uint(r->level_pct, 3);
    bt_puts("% B=");
    for (b = 0u; b < spectrum_num_bars(); b++) {
        bt_putc((char)('0' + ui_bar_digit(r, b)));
    }
    if (r->clipped) {
        bt_puts(" CLIP");
    }
    bt_puts("\r\n");
}

static void bt_gain_msg(int g)
{
    bt_puts("GAIN ");
    bt_putc((g < 0) ? '-' : '+');
    bt_put_uint((uint32_t)((g < 0) ? -g : g), 1);
    bt_puts(" dB\r\n");
}

/* Bluetooth command -> same events as the buttons */
static uint8_t bt_command(int c, int *gain)
{
    switch (c) {
    case 'M': case 'm': return EV_MODE;
    case 'P': case 'p': return EV_PAUSE;
    case 'G': case 'g': return EV_GRID;
    case 'B': case 'b': return EV_BARS;
    case '+':
        if (*gain < GAIN_MAX_DB) { *gain += GAIN_STEP_DB; }
        bt_gain_msg(*gain);
        return 0u;
    case '-':
        if (*gain > GAIN_MIN_DB) { *gain -= GAIN_STEP_DB; }
        bt_gain_msg(*gain);
        return 0u;
    case '?': case 'H': case 'h':
        bt_help();
        return 0u;
    default:
        return 0u;                                   /* ignore CR/LF etc. */
    }
}

int main(void)
{
    uint32_t last_hb = 0u, last_tx = 0u, peak_until = 0u;
    uint8_t  hb = 0u, paused = 0u, grid = 1u;
    int      gain = 0;

    /* FPU on (also done by SystemInit when "Use FPU" is selected) */
#if (__FPU_PRESENT == 1)
    SCB->CPACR |= (3UL << 20) | (3UL << 22);
    __DSB();
    __ISB();
#endif

    bsp_clock_init();                /* 84 MHz                              */
    bsp_systick_init();              /* 1 ms tick                           */
    bsp_io_init();                   /* LEDs, HC-05 STATE                   */
    bt_init(BT_BAUD);                /* USART1 + interrupt                  */
    lcd_init();                      /* SPI1 + ILI9341                      */
    spectrum_init();                 /* FFT tables                          */

    ui_splash(bsp_clock_is_hse());
    bt_puts("\r\nSTM32F401 Audio Spectrum Analyzer ready\r\n");
    bt_puts(bsp_clock_is_hse() ? "Clock: 84 MHz from 25 MHz crystal\r\n"
                               : "Clock: 84 MHz from HSI (crystal not found)\r\n");
    bt_help();
    delay_ms(1500);

    bsp_buttons_init();              /* EXTI + NVIC (the menu needs them)   */
    ui_set_bars(ui_bars_menu());     /* ask: 8, 16 or 32 bars               */

    ui_init();
    ui_set_grid(grid);
    led_set(LED_GRID, grid);
    bsp_sampling_init();             /* TIM2 + ADC1 + NVIC                  */
    bsp_sampling_start();

    for (;;) {
        uint32_t now = millis();
        uint8_t  ev  = bsp_buttons_poll(now);
        int      c;
        int      old_gain = gain;

        while ((c = bt_getc()) >= 0) {
            ev |= bt_command(c, &gain);
        }
        if (gain != old_gain) {
            ui_set_gain_db(gain);
        }

        if (ev & EV_MODE) {
            ui_set_mode((ui_get_mode() == UI_MODE_BARS) ? UI_MODE_WAVE : UI_MODE_BARS);
            bt_puts((ui_get_mode() == UI_MODE_BARS) ? "MODE: BARS\r\n" : "MODE: WAVE\r\n");
        }
        if (ev & EV_PAUSE) {
            paused = paused ? 0u : 1u;
            ui_set_paused(paused);
            led_set(LED_BOARD, paused);
            bt_puts(paused ? "PAUSED\r\n" : "RUNNING\r\n");
        }
        if (ev & EV_BARS) {
            uint8_t n = ui_get_bars();
            n = (n == 8u) ? 16u : ((n == 16u) ? 32u : 8u);
            ui_set_bars(n);
            bt_puts("BARS: ");
            bt_put_uint(n, 1);
            bt_puts("\r\n");
        }
        if (ev & EV_GRID) {
            grid = grid ? 0u : 1u;
            ui_set_grid(grid);
            led_set(LED_GRID, grid);
            bt_puts(grid ? "GRID ON\r\n" : "GRID OFF\r\n");
        }
        ui_set_bt_linked(bsp_bt_linked());

        if (bsp_capture_ready()) {
            spectrum_load(bsp_capture_buffer());   /* copy the block ...      */
            bsp_sampling_start();                  /* ... and record the next */
            spectrum_compute(&s_res);

            if (!paused) {
                ui_update(&s_res);
                if (ui_peak_alert(&s_res)) {
                    peak_until = now + 150u;
                }
                if ((uint32_t)(now - last_tx) >= BT_REPORT_MS) {
                    last_tx = now;
                    bt_report(&s_res);
                }
            }
        }
        ui_refresh();

        led_set(LED_PEAK, ((int32_t)(peak_until - now) > 0) ? 1u : 0u);

        if ((uint32_t)(now - last_hb) >= 500u) {      /* heartbeat 1 Hz      */
            last_hb = now;
            hb = hb ? 0u : 1u;
            led_set(LED_HEART, hb);
        }
    }
}
