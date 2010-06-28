/******************************************************************************
 * include files
 *****************************************************************************/

#include "x86-decoder.h"



/******************************************************************************
 * global variables 
 *****************************************************************************/
xed_control_t x86_decoder_settings;



/******************************************************************************
 * local variables 
 *****************************************************************************/

static xed_state_t xed_machine_state_x86_64 = { XED_MACHINE_MODE_LONG_64, 
						XED_ADDRESS_WIDTH_64b,
						XED_ADDRESS_WIDTH_64b };



/******************************************************************************
 * interface operations 
 *****************************************************************************/

void x86_family_decoder_init()
{
  x86_decoder_settings.xed_settings = xed_machine_state_x86_64;
  x86_decoder_settings.sp_reg = XED_REG_RSP;
  x86_decoder_settings.bp_reg = XED_REG_RBP;
  x86_decoder_settings.ip_reg = XED_REG_RIP;

  xed_tables_init();
}