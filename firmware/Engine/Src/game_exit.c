#include <setjmp.h>
#include <stdbool.h>
#include "main.h"
#include "game_exit.h"

static jmp_buf menu_return;
static volatile bool in_game = false;
static volatile bool exit_requested = false;

void game_exit_init(void)
{
    GPIO_InitTypeDef gpio = {
        .Pin  = GPIO_PIN_4,
        .Mode = GPIO_MODE_IT_RISING,
        .Pull = GPIO_PULLDOWN,   /* doublon du 10 kΩ du PCB ; évite une entrée flottante sur la DISC1 */
    };
    __HAL_RCC_GPIOE_CLK_ENABLE();
    HAL_GPIO_Init(GPIOE, &gpio);

    HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

/* Absent de stm32f4xx_it.c tant que PE4 n'est pas dans le .ioc : si on l'ajoute
 * un jour dans CubeMX, supprimer ce handler (sinon double définition). */
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void game_exit_request(void)
{
    /* Hors d'un jeu (menu), START ne fait rien : les rebonds ne doivent pas
     * laisser une demande qui couperait le prochain jeu dès son lancement */
    if (in_game)
        exit_requested = true;
}

void game_exit_poll(void)
{
    if (in_game && exit_requested)
        longjmp(menu_return, 1);
}

void game_run(void (*entry)(void))
{
    exit_requested = false;
    if (setjmp(menu_return) == 0)
    {
        in_game = true;
        entry();
    }
    in_game = false;
    exit_requested = false;
}

/* Remplace la version weak de la HAL (stm32f4xx_hal.c), identique à part le
 * test de START : toute attente d'un jeu devient un point de sortie */
void HAL_Delay(uint32_t Delay)
{
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;

    if (wait < HAL_MAX_DELAY)
        wait += (uint32_t)uwTickFreq;

    while ((HAL_GetTick() - tickstart) < wait)
        game_exit_poll();
    game_exit_poll();
}
