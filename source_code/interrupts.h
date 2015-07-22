/*
 * interrupts.h
 *
 * Created: 30/04/2015 20:20:39
 *  Author: limpkin
 */ 


#ifndef INTERRUPTS_H_
#define INTERRUPTS_H_

inline void enable_interrupts(void)
{    
    PMIC.CTRL = PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
    sei();
}

#endif /* INTERRUPTS_H_ */