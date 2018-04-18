/*===-----------------------------------------------------------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details.
 *
 *===------------------------------------------------------------------------=*/
 /* --------------------------------------------------------------------- 
 Andrei Safronov, e-mail: andrei@kudeyar.com
 Alexei Shistko, e-mail: alexey@kudeyar.com
  --------------------------------------------------------------------- */
 
//#include "Xtensa.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

Target llvm::TheXtensaTarget;

extern "C" void LLVMInitializeXtensaTargetInfo() 
{
  RegisterTarget<Triple::xtensa> X(TheXtensaTarget, "xtensa", "XTENSA");
}
