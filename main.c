/*
 * SAMD21_OneWire.c
 *
 * Created: 15/12/2023 22:57:47
 * Author : Admin
 */ 


#include "sam.h"


int main(void)
{
    while (1) 
    {
		NVMCTRL->CTRLB.bit.RWS = 1; //set wait state for DFLL
		
		//Waiting for external on-board (hopefully) 32K oscillator to power up
		SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_STARTUP(0x7) | SYSCTRL_XOSC32K_EN32K | SYSCTRL_XOSC32K_XTALEN;
		
		SYSCTRL->XOSC32K.bit.ENABLE = 1;
		
		while(!SYSCTRL->PCLKSR.bit.XOSC32KRDY); //Wait for XOSC32K to be ready
		
		//Set GCLK1 division to 1
		GCLK->GENDIV.reg = GCLK_GENDIV_ID(1) | GCLK_GENDIV_DIV(1);
		
		//Set GCLK1 to use the XOSC32K
		GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(1) | GCLK_GENCTRL_SRC_XOSC32K | GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN;
		
		while(GCLK->STATUS.bit.SYNCBUSY); //Wait for write to complete
		
		//Feed GCLK1 to the DFLL
		GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_DFLL48 | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;
		
		//DFLL register must be set to this value, as explained in errata 1.2.1
		while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
		SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
		while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
		
		//Setting up the multiplier (32K to 48M)
		SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_MUL(1465) | SYSCTRL_DFLLMUL_FSTEP(511) | SYSCTRL_DFLLMUL_CSTEP(31);
		
		while(!SYSCTRL->PCLKSR.bit.DFLLRDY); //Wait for write to finish
		
		//This value helps the DFLL lock onto the freq quicker
		uint32_t DFLL_coarse_help = (*((uint32_t *)FUSES_DFLL48M_COARSE_CAL_ADDR) & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;
		SYSCTRL->DFLLVAL.bit.COARSE = DFLL_coarse_help;
		while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
		
		//Enabling the DFLL
		SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_WAITLOCK | SYSCTRL_DFLLCTRL_ENABLE;
		
		//Wait for the DFLL to lock on
		while(!SYSCTRL->PCLKSR.bit.DPLLLCKF || !SYSCTRL->PCLKSR.bit.DFLLLCKC);
		
		//The most important part: switch GCLK0 (CPU generator) to DFLL
		GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0) | GCLK_GENCTRL_SRC_DFLL48M | GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN;
		
		while(GCLK->STATUS.bit.SYNCBUSY); //Wait for the write to complete
    }
}
