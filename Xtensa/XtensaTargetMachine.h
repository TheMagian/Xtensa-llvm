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
 
#ifndef LLVM_LIB_TARGET_XTENSA_XTENSATARGETMACHINE_H
#define LLVM_LIB_TARGET_XTENSA_XTENSATARGETMACHINE_H

#include "llvm/Target/TargetMachine.h"
#include "XtensaSubtarget.h"

namespace llvm 
{

class TargetFrameLowering;

class XtensaTargetMachine : public LLVMTargetMachine 
{
 
public:
  XtensaTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Reloc::Model RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL);
  
  // Override TargetMachine.
  const XtensaSubtarget *getSubtargetImpl() const { return &Subtarget; }
  const XtensaSubtarget *getSubtargetImpl(const Function &F) const override;
  // Override LLVMTargetMachine
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  TargetLoweringObjectFile *getObjFileLowering() const override 
  {
    return TLOF.get();
  }
  
protected:
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  XtensaSubtarget Subtarget;

private:
//  mutable StringMap<std::unique_ptr<XtensaSubtarget>> SubtargetMap;
};
} // end namespace llvm

#endif /* LLVM_LIB_TARGET_XTENSA_XTENSATARGETMACHINE_H */

