#include "main.h"
#include "buzzer.h"
#include "power.h"
#include "storage.h"

/* TIM12 compte à 1 MHz : ARR = 1e6 / f - 1 tient sur 16 bits dès 16 Hz */
#define TICK_HZ 1000000UL

static TIM_HandleTypeDef htim12;
static volatile bool muted = false;

/* Mélodie en cours, partagée avec l'interruption SysTick */
static const note_t *volatile melody = 0;
static volatile uint16_t remaining = 0;   /* ms restantes de la note en cours */

/* ---- Sons ------------------------------------------------------------------ */

const note_t SND_BOOT[]   = {{1319, 90}, {1568, 90}, {2093, 90}, {2637, 220}, {0, 0}};
const note_t SND_MOVE[]   = {{2700, 12}, {0, 0}};
const note_t SND_SELECT[] = {{2093, 50}, {2794, 80}, {0, 0}};
const note_t SND_BACK[]   = {{2794, 50}, {2093, 80}, {0, 0}};
const note_t SND_HIT[]    = {{1760, 20}, {0, 0}};
const note_t SND_POINT[]  = {{2349, 50}, {3136, 90}, {0, 0}};
const note_t SND_LOSE[]   = {{784, 150}, {659, 150}, {523, 350}, {0, 0}};
const note_t SND_WIN[]    = {{1568, 100}, {2093, 100}, {2637, 100}, {3136, 300}, {0, 0}};

/* ---- Matériel -------------------------------------------------------------- */

static void output(uint16_t freq)
{
    if (freq == 0 || muted)
    {
        __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_1, 0);
        return;
    }
    uint32_t period = TICK_HZ / freq;
    __HAL_TIM_SET_AUTORELOAD(&htim12, period - 1);
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_1, period / 2);   /* 50 % au maximum */
    /* ARR et CCR sont préchargés : l'UG les applique tout de suite et remet le
     * compteur à 0, sinon il pourrait dépasser le nouvel ARR et tourner 65 ms */
    htim12.Instance->EGR = TIM_EGR_UG;
}

void buzzer_init(void)
{
    GPIO_InitTypeDef gpio = {
        .Pin       = GPIO_PIN_14,
        .Mode      = GPIO_MODE_AF_PP,
        .Pull      = GPIO_NOPULL,     /* R38 tire déjà la grille de Q2 à GND */
        .Speed     = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF9_TIM12,
    };
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM12_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &gpio);

    /* Horloge des timers APB1 : PCLK1, doublée si le prédiviseur APB1 n'est pas 1 */
    uint32_t clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_HCLK_DIV1)
        clk *= 2;

    htim12.Instance               = TIM12;
    htim12.Init.Prescaler         = clk / TICK_HZ - 1;
    htim12.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim12.Init.Period            = TICK_HZ / 2700 - 1;
    htim12.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim12);

    TIM_OC_InitTypeDef oc = {
        .OCMode     = TIM_OCMODE_PWM1,
        .Pulse      = 0,                 /* muet au démarrage */
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
    };
    HAL_TIM_PWM_ConfigChannel(&htim12, &oc, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);

    muted = (storage_get(STORAGE_MUTE) == 1);   /* jamais écrit = son actif */
}

/* ---- Mode muet ------------------------------------------------------------- */

void buzzer_set_mute(bool mute)
{
    muted = mute;
    if (mute)
        buzzer_stop();
}

bool buzzer_muted(void)
{
    return muted;
}

void buzzer_save(void)
{
    storage_set(STORAGE_MUTE, muted ? 1 : 0);
}

/* ---- Lecture --------------------------------------------------------------- */

void buzzer_play(const note_t *m)
{
    __disable_irq();
    melody = m;
    remaining = m->ms;
    output(m->ms ? m->freq : 0);
    if (m->ms == 0)
        melody = 0;
    __enable_irq();
}

void buzzer_tone(uint16_t freq, uint16_t ms)
{
    /* Mélodie d'une note : elle doit survivre à l'appel, d'où le static */
    static note_t tone[2];
    __disable_irq();
    tone[0] = (note_t){freq, ms};
    tone[1] = (note_t){0, 0};
    __enable_irq();
    buzzer_play(tone);
}

void buzzer_stop(void)
{
    __disable_irq();
    melody = 0;
    remaining = 0;
    output(0);
    __enable_irq();
}

bool buzzer_playing(void)
{
    return melody != 0;
}

void buzzer_tick(void)
{
    const note_t *m = melody;
    if (m == 0 || --remaining > 0)
        return;

    m++;
    if (m->ms == 0)
    {
        melody = 0;
        output(0);
        return;
    }
    melody = m;
    remaining = m->ms;
    output(m->freq);
}

/* Remplace la version weak de la HAL (stm32f4xx_hal.c) : le SysTick fait aussi
 * avancer les mélodies et surveille l'inactivité (power.c), sans toucher aux fichiers générés par CubeMX */
void HAL_IncTick(void)
{
    uwTick += uwTickFreq;
    buzzer_tick();
    power_tick();   /* repère les actions du joueur pour la mise en veille */
}
