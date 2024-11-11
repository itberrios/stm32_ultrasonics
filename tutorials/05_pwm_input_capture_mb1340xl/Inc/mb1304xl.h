/*
 * mb1304xl.h
 *
 *  Created on: Oct 24, 2024
 *      Author: iberrios
 */

#ifndef MB1304XL_H_
#define MB1304XL_H_

#include "stm32f4xx.h"


// 'raw' timer configs (need to subtract one in timer config functions)
#define TRIGGER_TIMER_PSC    160 // slow clock to 0.1MHz --> 16 MHz/160
#define TRIGGER_TIMER_ARR    7000 // 70ms --> 0.1MHz * 70ms

#define ECHO_TIMER_PSC    16 // slow clock to 1MHz --> 16 MHz/16
#define ECHO_TIMER_ARR    70000 // 70ms --> 1MHz * 70ms

/* timer configs */
void config_PWM_TIM2_ch2_trigger(void);
void config_TIM5_ch1_echo(void);
void config_TIM5_ch1_echo_pwm_input(void);

/* pin configs */
void config_PA0_AF2(void); // trigger pin
void config_PA1_AF1(void); // echo pin



#endif /* MB1304XL_H_ */
