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
 
#include "XtensaTargetMachine.h"
#include "XtensaTargetObjectFile.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

extern "C" void LLVMInitializeXtensaTarget() 
{
  // Register the target.
  RegisterTargetMachine<XtensaTargetMachine> A(TheXtensaTarget);
}

static std::string computeDataLayout(const Triple &TT) 
{
  std::string Ret = "e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32-i64:32"
    "-f64:32-a:0:32-n32";
  //       "e-m:e-p:32:32:32-i1:8:16-i8:8:16-i16:16-i32:32-"
  //       "f32:32-f64:64-f80:128-f128:128-n32";  

  return Ret;
}

XtensaTargetMachine::XtensaTargetMachine(const Target &T, const Triple &TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Reloc::Model RM, CodeModel::Model CM,
                                       CodeGenOpt::Level OL)
    : LLVMTargetMachine(T, computeDataLayout(TT), TT, CPU, FS, Options, RM, CM, OL),
    TLOF(make_unique<XtensaTargetObjectFile>()),
      Subtarget(TT, CPU, FS, *this) 
{
  initAsmInfo();
}

const XtensaSubtarget *
XtensaTargetMachine::getSubtargetImpl(const Function &F) const 
{
  /* TODO Multiple subtargets
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");
 
  std::string CPU = !CPUAttr.hasAttribute(Attribute::None)
                        ? CPUAttr.getValueAsString().str()
                        : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None)
                       ? FSAttr.getValueAsString().str()
                       : TargetFS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = llvm::make_unique<XtensaSubtarget>(TargetTriple, CPU, FS, *this);
  }
  return I.get();
  */
  return &Subtarget;
}

namespace 
{
/// Xtensa Code Generator Pass Configuration Options.
class XtensaPassConfig : public TargetPassConfig 
{
public:
  XtensaPassConfig(XtensaTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  XtensaTargetMachine &getXtensaTargetMachine() const 
  {
    return getTM<XtensaTargetMachine>();
  }

  bool addInstSelector() override;
  void addPreEmitPass() override;
};
} // end anonymous namespace

bool XtensaPassConfig::addInstSelector() 
{
  addPass(createXtensaISelDag(getXtensaTargetMachine(), getOptLevel()));
  return false;
}

void XtensaPassConfig::addPreEmitPass()
{
  addPass(createXtensaBranchSelectionPass());
}

TargetPassConfig *XtensaTargetMachine::createPassConfig(PassManagerBase &PM) 
{
  return new XtensaPassConfig(this, PM);
}
