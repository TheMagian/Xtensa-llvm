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
 
#ifndef LLVM_LIB_TARGET_XTENSA_XTENSA_H
#define LLVM_LIB_TARGET_XTENSA_XTENSA_H

#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "llvm/Support/CodeGen.h"

namespace llvm 
{
  class XtensaTargetMachine;
  class FunctionPass;

  namespace Xtensa 
  {
    // Condition-code mask values.
    const unsigned CCMASK_0 = 1 << 3;
    const unsigned CCMASK_1 = 1 << 2;
    const unsigned CCMASK_2 = 1 << 1;
    const unsigned CCMASK_3 = 1 << 0;
    const unsigned CCMASK_ANY = CCMASK_0 | CCMASK_1 | CCMASK_2 | CCMASK_3;

    // Condition-code mask assignments
    const unsigned CCMASK_CMP_EQ = CCMASK_0;
    const unsigned CCMASK_CMP_LT = CCMASK_1;
    const unsigned CCMASK_CMP_GT = CCMASK_2;
    const unsigned CCMASK_CMP_UO = CCMASK_3;
    const unsigned CCMASK_CMP_NE = CCMASK_CMP_LT | CCMASK_CMP_GT;
    const unsigned CCMASK_CMP_LE = CCMASK_CMP_EQ | CCMASK_CMP_LT;
    const unsigned CCMASK_CMP_GE = CCMASK_CMP_EQ | CCMASK_CMP_GT;
    const unsigned CCMASK_CMP_O  = CCMASK_ANY ^ CCMASK_CMP_UO;  
  } // end namespace Xtensa
    
  FunctionPass *createXtensaISelDag(XtensaTargetMachine &TM,
                                     CodeGenOpt::Level OptLevel);
  FunctionPass *createXtensaBranchSelectionPass();
} // end namespace llvm;
#endif /* LLVM_LIB_TARGET_XTENSA_XTENSA_H */
  
