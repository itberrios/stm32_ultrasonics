/*
 * hc_sr04.h
 *
 *  Created on: Oct 6, 2024
 *      Author: iberrios
 */

#ifndef HC_SR04_H_
#define HC_SR04_H_

#include "stm32f4xx.h"

/* timer configs */
void config_PWM_TIM2_ch1_trigger(void);
void config_TIM5_ch2_echo(void);

/* pin configs */
void config_PA0_AF1(void); // trigger pin
void config_PA1_AF2(void); // echo pin


#endif /* HC_SR04_H_ */
