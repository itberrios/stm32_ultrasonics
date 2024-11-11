/*
 * hc_sr04.h
 *
 *  Created on: Oct 6, 2024
 *      Author: iberrios
 */

#ifndef HC_SR04_H_
#define HC_SR04_H_

#include <stdint.h>
#include "stm32f4xx.h"


// 'raw' timer configs (need to subtract one in timer config functions)
#define TRIGGER_TIMER_PSC    160 // slow clock to 0.1MHz --> 16 MHz/160
#define TRIGGER_TIMER_ARR    7000 // 70ms --> 0.1MHz * 70ms

#define ECHO_TIMER_PSC    16     // slow clock to 1MHz --> 16 MHz/16
#define ECHO_TIMER_ARR    70000 // 70ms --> 1MHz * 70ms

// arrays to store captured data
extern uint32_t tim5_channel1_ccr[1]; // clocks at rising edge
extern uint32_t tim5_channel2_ccr[1]; // clocks at falling edge


/* timer configs */
void config_PWM_TIM2_ch2_trigger(void);
void config_TIM5_ch1_echo_pwm_input(void);

/* pin configs */
void config_PA0_AF2(void); // echo pin
void config_PA1_AF1(void); // trigger pin


#endif /* HC_SR04_H_ */
