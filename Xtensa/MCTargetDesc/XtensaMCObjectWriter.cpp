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
 
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "MCTargetDesc/XtensaMCFixups.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCValue.h"

// Add to ELF.h
namespace llvm
{
  namespace ELF
  {
#define ELF_RELOC(name, value) name = value,
    enum
    {
#include "Xtensa.def"  
    };
  }
}

using namespace llvm;

namespace 
{
class XtensaObjectWriter : public MCELFObjectTargetWriter 
{
public:
  XtensaObjectWriter(uint8_t OSABI);

  virtual ~XtensaObjectWriter();

protected:
  // Override MCELFObjectTargetWriter.
  unsigned GetRelocType(const MCValue &Target, const MCFixup &Fixup,
                        bool IsPCRel) const override;
};
} // end anonymouse namespace

XtensaObjectWriter::XtensaObjectWriter(uint8_t OSABI)
  : MCELFObjectTargetWriter(/*Is64Bit=*/true, OSABI, ELF::EM_Xtensa,
                            /*HasRelocationAddend=*/ true) 
{
}

XtensaObjectWriter::~XtensaObjectWriter() 
{
}

// Return the relocation type for an absolute value of MCFixupKind Kind.
static unsigned getAbsoluteReloc(unsigned Kind) 
{
  switch (Kind) 
  {
    case FK_Data_4: return ELF::R_Xtensa_32;
  }
  llvm_unreachable("Unsupported absolute address");
}

//TODO: fix relocation types
// Return the relocation type for a PC-relative value of MCFixupKind Kind.
static unsigned getPCRelReloc(unsigned Kind) 
{
  switch (Kind) 
  {
    case FK_Data_4:                return ELF::R_Xtensa_CALL;
    case Xtensa::fixup_xtensa_brlo:  return ELF::R_Xtensa_BRANCH;
    case Xtensa::fixup_xtensa_brhi:  return ELF::R_Xtensa_BRANCH;
    case Xtensa::fixup_xtensa_jal:   return ELF::R_Xtensa_JAL;
    case Xtensa::fixup_xtensa_call:  return ELF::R_Xtensa_CALL;
  }
  llvm_unreachable("Unsupported PC-relative address");
}

// Return the R_Xtensa_TLS* relocation type for MCFixupKind Kind.
static unsigned getTLSLEReloc(unsigned Kind) 
{
  switch (Kind) 
  {
    case FK_Data_4: return ELF::R_Xtensa_TLS_TPREL32;
  }
  llvm_unreachable("Unsupported absolute address");
}

// Return the PLT relocation counterpart of MCFixupKind Kind.
static unsigned getPLTReloc(unsigned Kind) 
{
  switch (Kind) 
  {
    case Xtensa::fixup_xtensa_call_plt: return ELF::R_Xtensa_CALL_PLT;
  }
  llvm_unreachable("Unsupported absolute address");
}

unsigned XtensaObjectWriter::GetRelocType(const MCValue &Target,
                                         const MCFixup &Fixup,
                                         bool IsPCRel) const 
{
  MCSymbolRefExpr::VariantKind Modifier = (Target.isAbsolute() ?
                                           MCSymbolRefExpr::VK_None :
                                           Target.getSymA()->getKind());
  unsigned Kind = Fixup.getKind();
  switch (Modifier) 
  {
    case MCSymbolRefExpr::VK_None:
      if (IsPCRel)
        return getPCRelReloc(Kind);
      return getAbsoluteReloc(Kind);

    case MCSymbolRefExpr::VK_NTPOFF:
      assert(!IsPCRel && "NTPOFF shouldn't be PC-relative");
      return getTLSLEReloc(Kind);

    case MCSymbolRefExpr::VK_GOT:
      llvm_unreachable("GOT accesses are not supported yet");

    case MCSymbolRefExpr::VK_PLT:
      assert(IsPCRel && "@PLT shouldt be PC-relative");
      return getPLTReloc(Kind);

    default:
      llvm_unreachable("Modifier not supported");
  }
}

MCObjectWriter *llvm::createXtensaObjectWriter(raw_pwrite_stream &OS,
                                                uint8_t OSABI) 
{
  MCELFObjectTargetWriter *MOTW = new XtensaObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, /*IsLittleEndian=*/false);
}
