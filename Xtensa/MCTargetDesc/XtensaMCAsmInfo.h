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
 
#ifndef LLVM_LIB_TARGET_XTENSA_XTENSATARGETASMINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSATARGETASMINFO_H

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/Support/Compiler.h"

namespace llvm 
{
class StringRef;

class XtensaMCAsmInfo : public MCAsmInfo 
{
public:
  explicit XtensaMCAsmInfo();

};

} // namespace llvm

#endif
