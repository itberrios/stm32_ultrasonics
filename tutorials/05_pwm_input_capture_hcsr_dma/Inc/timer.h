/*
 * timer.h
 *
 *  Created on: Oct 2, 2024
 *      Author: iberrios
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "stm32f4xx.h"


/** Exposed Functions **/
void config_LED_TIM2(void);
void config_PA8_ch1_TIM1(void);
void delay(uint16_t ms);
void update_counter(uint16_t ms);
void init_PWM_LED_control(void);
void config_PWM_TIM1_ch1(uint32_t pwm_freq, uint16_t duty);

/** Exposed function used in timer.c and externally **/
void config_PA5_AF1(void);
void config_PA8_AF1(void);


#endif /* TIMER_H_ */
