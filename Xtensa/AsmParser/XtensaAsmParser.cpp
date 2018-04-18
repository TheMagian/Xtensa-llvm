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
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "xtensav-asm-parser"

// TODO add real stuff
class XtensaAsmParser : public MCTargetAsmParser 
{
#define GET_ASSEMBLER_HEADER
#include "XtensaGenAsmMatcher.inc"

private:
  const MCSubtargetInfo &STI;
  MCAsmParser &Parser;

public:
  XtensaAsmParser(const MCSubtargetInfo &sti, MCAsmParser &parser,
                 const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, sti), STI(sti), Parser(parser) 
  {
    MCAsmParserExtension::Initialize(Parser);

  }
      
  // Override MCTargetAsmParser.
  bool ParseDirective(AsmToken DirectiveID) override;
  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;   

 
};

bool XtensaAsmParser::ParseDirective(AsmToken DirectiveID) {
  return true;
}

bool XtensaAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                     SMLoc &EndLoc) 
{
  // TODO
  return Error(StartLoc, "invalid register");
}

bool XtensaAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                      StringRef Name, SMLoc NameLoc,
                                      OperandVector &Operands) 
{
  // TODO
  return Error(NameLoc, "unknown instruction");
}

bool XtensaAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) 
{
  // TODO
  return Error(IDLoc, "invalid instruction");
}

void XtensaAsmParser::convertToMapAndConstraints(unsigned Kind,
                                          const OperandVector &Operands)
{
  // TODO
}

// Force static initialization.
extern "C" void LLVMInitializeXtensaAsmParser() 
{
  RegisterMCAsmParser<XtensaAsmParser> X(TheXtensaTarget);
}

