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
 
#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAMCTARGETDESC_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"
#include "llvm/Support/TargetRegistry.h"

namespace llvm {

class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class StringRef;
class Target;
class raw_ostream;

extern Target TheXtensaTarget;
extern Target TheXtensa64Target;

MCCodeEmitter *createXtensaMCCodeEmitter(const MCInstrInfo &MCII,
                                          const MCRegisterInfo &MRI,
                                          MCContext &Ctx);

MCAsmBackend *createXtensaMCAsmBackend(const Target &T,
                                      const MCRegisterInfo &MRI, const Triple &TT,
                                      StringRef CPU);

MCObjectWriter *createXtensaObjectWriter(raw_pwrite_stream &OS, uint8_t OSABI);

namespace XtensaMC 
{
  // How many bytes are in the ABI-defined, caller-allocated part of
  // a stack frame.
  const int64_t CallFrameSize = 72;  // TODO

  // The offset of the DWARF CFA from the incoming stack pointer.
  const int64_t CFAOffsetFromInitialSP = CallFrameSize;
}
} // end namespace llvm

// Defines symbolic names for Xtensa registers.
// This defines a mapping from register name to register number.
#define GET_REGINFO_ENUM
#include "XtensaGenRegisterInfo.inc"

// Defines symbolic names for the Xtensa instructions.
#define GET_INSTRINFO_ENUM
#include "XtensaGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "XtensaGenSubtargetInfo.inc"

#endif /* LLVM_LIB_TARGET_XTENSA_XTENSAMCTARGETDESC_H */

