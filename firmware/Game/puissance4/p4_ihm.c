#include "main.h"
#include "input.h"
#include "p4_ihm.h"

/* Définis dans main.c : DMA de l'ADC joystick et flag levé par l'EXTI0 (PA0) */
extern volatile uint16_t joy_adc[2];
extern volatile uint8_t button_pressed;

#define P4_DEBOUNCE_MS 50

/* Direction Y précédente, pour ne bouger que d'une colonne par inclinaison */
static int last_y;

/* Attend que le bouton soit relâché puis efface les rebonds */
static void consume_button(void)
{
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
    HAL_Delay(P4_DEBOUNCE_MS);
    button_pressed = 0;
}

void p4_ihm_init(void)
{
    consume_button();
    last_y = selection_joy(joy_adc[0], joy_adc[1]).y;
}

int p4_ihm_choose_line(void)
{
    while (1)
    {
        if (button_pressed)
        {
            consume_button();
            return P4_SELECT;
        }

        /* Comme dans le menu, l'axe Y du joystick donne la gauche / droite de l'écran */
        int y = selection_joy(joy_adc[0], joy_adc[1]).y;
        if (y != last_y)
        {
            last_y = y;
            if (y != 0)
                return y;
        }
    }
}

void p4_ihm_wait_button(void)
{
    while (!button_pressed);
    consume_button();
}
