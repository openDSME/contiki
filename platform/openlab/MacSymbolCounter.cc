/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "MacSymbolCounter.h"
//#include "cmsis_device.h"

extern "C" {
#include "timer_.h"
#include "nvic_.h"
#include "timer_registers.h"

int printf(const char *format, ...);
}

TIMER_INIT(_tim3, TIM3_BASE_ADDRESS, (rcc_apb_bus_t)RCC_APB_BUS_TIM3, RCC_APB_BIT_TIM3, NVIC_IRQ_LINE_TIM3, 4);
auto _timer = &_tim3;

static MacSymbolCounter msc;

MacSymbolCounter& MacSymbolCounter::getInstance() {
    return msc;
}

void MacSymbolCounter::init(void (*compareMatch)()) {
    this->compareMatch = compareMatch;
    lastCapture = 0;

	//Disable interrupt during setup
    //TIM3->DIER &= ~TIM_DIER_UIE;
    *timer_get_DIER_bitband(_timer, TIMER_DIER__UIE_bit) = 0;

	//RCC_ClocksTypeDef clocks;
	//RCC_GetClocksFreq(&clocks);

    // TODO calculate correct prescaler
    //uint32_t freq = 65536;
    uint32_t freq = 62500;
	uint32_t prescaler = (timer_get_frequency(_timer)) / (uint32_t)freq;

	// Init interrupt line
    /*
	NVIC_InitTypeDef nvicInit;
	nvicInit.NVIC_IRQChannel = TIM3_IRQn;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority = 0xF;
	nvicInit.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_Init(&nvicInit);
    */
    nvic_enable_interrupt_line(_timer->irq_line);

	//Activate clock for timer module
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    rcc_apb_enable(_timer->apb_bus, _timer->apb_bit);

	//Set Registers
	//TIM3->PSC = prescaler;
	*timer_get_PSC(_timer) = prescaler;
	//TIM3->EGR = TIM_PSCReloadMode_Immediate;
	*timer_get_EGR(_timer) = TIMER_EGR__UG;

	//TIM3->ARR = 0xFFFF;
	*timer_get_ARR(_timer) = 0xFFFF;
	//TIM3->CNT = 0;
	*timer_get_CNT(_timer) = 0;

	//TIM3->CR1 &= ~TIM_CR1_DIR; // upcounting
	*timer_get_CR1(_timer) &= ~TIMER_CR1__DIR; // upcounting

    ///////////////////////////////////////
    // Enable capture
    //TIM3->CCMR2 &= ~TIM_CCMR2_CC3S;
    //TIM3->CCMR2 |= TIM_CCMR2_CC3S_0; // enable input
    //TIM3->CCMR2 &= ~TIM_CCMR2_IC3PSC; // disable input prescaler
    //TIM3->CCMR2 &= ~TIM_CCMR2_IC3F; // disable input filter
 
    // Get CCMR register
	uint8_t channel = TIMER_CHANNEL_3;
    volatile uint16_t *ccmr;
    ccmr = timer_get_CCMRx(_timer, (channel < TIMER_CHANNEL_3) ? 1 : 2);

    // Get bit offset
    uint8_t ccmr_offset;
    ccmr_offset = (channel & 1) ? 8 : 0;   
    
	*ccmr &= ~(TIMER_CCMRx__CCxS__MASK << ccmr_offset);
    *ccmr |= (TIMER_CCMRx__CCxS__INPUT1 << ccmr_offset); // enable input
    *ccmr &= ~(TIMER_CCMRx__ICxPSC__MASK << ccmr_offset); // disable input prescaler
    *ccmr &= ~(TIMER_CCMRx__ICxF__MASK << ccmr_offset); // disable input filter

    printf("CCMR 0x%x\n",*ccmr);
    
    //TIM3->CCER &= ~TIM_CCER_CC3P; // rising edge
    //TIM3->CCER |= TIM_CCER_CC3E; // enable capture

    *timer_get_CCER(_timer) &= ~(TIMER_CCER__CC1P << (2*4)); // rising edge
    *timer_get_CCER(_timer) |= TIMER_CCER__CC1E << (2*4); // enable capture

    printf("CCER 0x%x\n",*timer_get_CCER(_timer));

    ///////////////////////////////////////
    // Enable Interrupts
    //TIM3->SR &= ~TIM_SR_UIF; // clear overflow interrupt
    //TIM3->DIER |= TIM_DIER_UIE; // enable overflow interrupt
    //TIM3->SR &= ~TIM_SR_CC1IF; // clear compare match interrupt
    //TIM3->DIER |= TIM_DIER_CC1IE; // enable compare match interrupt
    //TIM3->SR &= ~TIM_SR_CC3IF; // clear capture interrupt
    //TIM3->DIER |= TIM_DIER_CC3IE; // enable capture interrupt

    *timer_get_SR(_timer) = ~TIMER_SR__UIF; // clear overflow interrupt
    *timer_get_DIER_bitband(_timer, TIMER_DIER__UIE_bit) = 1; // enable overflow interrupt
    *timer_get_SR(_timer) = ~(TIMER_SR__CC1IF << TIMER_CHANNEL_1); // clear compare match interrupt
    *timer_get_DIER_bitband(_timer, TIMER_DIER__CC1IE_bit + TIMER_CHANNEL_1) = 1; // enable compare match interrupt
    *timer_get_SR(_timer) = ~(TIMER_SR__CC1IF << TIMER_CHANNEL_3); // clear capture interrupt
    *timer_get_DIER_bitband(_timer, TIMER_DIER__CC1IE_bit + TIMER_CHANNEL_3) = 1; // enable capture interrupt

    printf("SR 0x%x\n",*timer_get_SR(_timer));

    printf("DIER 0x%x\n",*timer_get_DIER(_timer));

	//Enable Timer
	//TIM3->CR1 |= TIM_CR1_CEN;
	*timer_get_CR1(_timer) |= TIMER_CR1__CEN;
}

extern "C" {
    void foo();
}

void MacSymbolCounter::interrupt() {
    bool overflow = false;

	//if(TIM3->SR & TIM_SR_UIF){
	if(*timer_get_SR(_timer) & TIMER_SR__UIF){
		//TIM3->SR = ~TIM_SR_UIF;
        *timer_get_SR(_timer) = ~TIMER_SR__UIF;
        overflow = true;
        msw++;
	}

    //if(TIM3->SR & TIM_SR_CC1IF) {
    if(*timer_get_SR(_timer) & (TIMER_SR__CC1IF << TIMER_CHANNEL_1)) {
		//TIM3->SR = ~TIM_SR_CC1IF;
        *timer_get_SR(_timer) = ~(TIMER_SR__CC1IF << TIMER_CHANNEL_1);

        uint16_t compareMSW = msw;

        //if(overflow && TIM3->CCR1 >= 0x8000) {
        if(overflow && *timer_get_CCRx(_timer,TIMER_CHANNEL_1) >= 0x8000) {
            // The compare match interrupt was probably
            // before the overflow interrupt
            compareMSW--;
        }

        if(compareMSW == compareValueMSW) {
            compareMatch();
        }
    }
    
    //if(TIM3->SR & TIM_SR_CC3OF) {
    if(*timer_get_SR(_timer) & (TIMER_SR__CC1OF << TIMER_CHANNEL_3)) {
        // overcapture
        //TIM3->SR = ~TIM_SR_CC3OF;
        *timer_get_SR(_timer) = ~(TIMER_SR__CC1OF << TIMER_CHANNEL_3);
        //TIM3->SR = ~TIM_SR_CC3IF;
        *timer_get_SR(_timer) = ~(TIMER_SR__CC1IF << TIMER_CHANNEL_3);

        lastCapture = INVALID_CAPTURE;
    }
    //else if(TIM3->SR & TIM_SR_CC3IF) {
    else if(*timer_get_SR(_timer) & (TIMER_SR__CC1IF << TIMER_CHANNEL_3)) {
		//TIM3->SR = ~TIM_SR_CC3IF;
        *timer_get_SR(_timer) = ~(TIMER_SR__CC1IF << TIMER_CHANNEL_3);

        //uint16_t captureLSW = TIM3->CCR3;
        uint16_t captureLSW = *timer_get_CCRx(_timer,TIMER_CHANNEL_3);
        uint16_t captureMSW = msw;

        foo();

        if(overflow && captureLSW >= 0x8000) {
            // The capture interrupt was probably
            // before the overflow interrupt
            captureMSW--;
        }

        lastCapture = (captureMSW << (uint32_t)16) | captureLSW;
    }

    //ASSERT((TIM3->SR & TIM_SR_CC1OF) == 0); // no overcapture for compare match
}

uint32_t MacSymbolCounter::getValue() {
    static uint32_t lastValue;
	platform_enter_critical();
    //uint32_t result = (((uint32_t)msw) << 16) | TIM3->CNT;
    uint32_t result = (((uint32_t)msw) << 16) | *timer_get_CNT(_timer);
    //if(TIM3->SR & TIM_SR_UIF) {
	if(*timer_get_SR(_timer) & TIMER_SR__UIF){
        // Recent and unhandled overflow
        //result = (((uint32_t)msw) << 16) | TIM3->CNT; // request value again since UIF might have flipped after the read
        result = (((uint32_t)msw) << 16) | *timer_get_CNT(_timer); // request value again since UIF might have flipped after the read
        result += (1 << (uint32_t)16); // increment msw since it is unhandled
    }

    //ASSERT((TIM3->SR & TIM_SR_CC1OF) == 0); // no overcapture for compare match
    //ASSERT(lastValue <= result); // TODO remove to allow for overflows of the 32 bit counter

    lastValue = result;
	platform_exit_critical();
    return result;
}

uint32_t MacSymbolCounter::getCapture() {
	platform_enter_critical();
    uint32_t result = lastCapture;
	platform_exit_critical();
    return result;
}

void MacSymbolCounter::setCompareMatch(uint32_t compareValue) {
	platform_enter_critical();
    compareValueMSW = compareValue >> 16;
    //TIM3->CCR1 = compareValue & 0xFFFF;
    *timer_get_CCRx(_timer,TIMER_CHANNEL_1) = compareValue & 0xFFFF;
	platform_exit_critical();
}

extern "C" {

//void TIM3_IRQHandler(void)
void tim3_isr()
{
    msc.interrupt();
}

}
