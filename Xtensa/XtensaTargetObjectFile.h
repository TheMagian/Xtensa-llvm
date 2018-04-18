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
 
#ifndef LLVM_TARGET_XTENSA_XTENSATARGETOBJECTFILE_H
#define LLVM_TARGET_XTENSA_XTENSATARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm 
{
class XtensaTargetMachine;
  class XtensaTargetObjectFile : public TargetLoweringObjectFileELF 
  {
    MCSection *SmallDataSection;
    MCSection *SmallBSSSection;
    const XtensaTargetMachine *TM;
  public:

    void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

    /// Return true if this global address should be placed into small data/bss
    /// section.
    bool IsGlobalInSmallSection(const GlobalValue *GV, const TargetMachine &TM,
                                SectionKind Kind) const;
    bool IsGlobalInSmallSection(const GlobalValue *GV,
                                const TargetMachine &TM) const;
    bool IsGlobalInSmallSectionImpl(const GlobalValue *GV,
                                    const TargetMachine &TM) const;

    MCSection *SelectSectionForGlobal(const GlobalValue *GV, SectionKind Kind,
                                      Mangler &Mang,
                                      const TargetMachine &TM) const override;

    /// Return true if this constant should be placed into small data section.
    bool IsConstantInSmallSection(const DataLayout &DL, const Constant *CN,
                                  const TargetMachine &TM) const;

    MCSection *getSectionForConstant(const DataLayout &DL, SectionKind Kind,
                                     const Constant *C) const override;
  };
} // end namespace llvm

#endif /* LLVM_TARGET_XTENSA_XTENSATARGETOBJECTFILE_H */
