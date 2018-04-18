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
 
#include "XtensaMCInstLower.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/IR/Mangler.h"

using namespace llvm;

XtensaMCInstLower::XtensaMCInstLower(MCContext &ctx, XtensaAsmPrinter &asmPrinter)
  : Ctx(ctx),  Printer(asmPrinter) 
{
}

MCSymbol *XtensaMCInstLower::
GetExternalSymbolSymbol(const MachineOperand &MO) const 
{
  /*
  switch (MO.getTargetFlags()) 
  {
    default: llvm_unreachable("Unknown target flag on GV operand");
    case 0: break;
  }
  */
  return Printer.GetExternalSymbolSymbol(MO.getSymbolName());
}

MCSymbol *XtensaMCInstLower::
GetJumpTableSymbol(const MachineOperand &MO) const 
{
  /*
  SmallString<256> Name;
  raw_svector_ostream(Name) // << Printer.MAI->getPrivateGlobalPrefix()  
                            << "JTI"
                            << Printer.getFunctionNumber() << '_'
                            << MO.getIndex();

  switch (MO.getTargetFlags()) 
  {
  default: llvm_unreachable("Unknown target flag on GV operand");
  case 0: break;
  }

  // Create a symbol for the name.
  return Printer.MF->getContext().GetOrCreateSymbol(Name.str());
  */
  return Printer.GetJTISymbol(MO.getIndex());
}

MCSymbol *XtensaMCInstLower::
GetConstantPoolIndexSymbol(const MachineOperand &MO) const 
{
  /*
  SmallString<256> Name;
  raw_svector_ostream(Name) // <<  Printer.MAI->getPrivateGlobalPrefix() 
                            <<  "CPI"
                            << Printer.getFunctionNumber() << '_'
                            << MO.getIndex();

  switch (MO.getTargetFlags()) {
  default: llvm_unreachable("Unknown target flag on GV operand");
  case 0: break;
  }
  */
  // Create a symbol for the name.
  return Printer.GetCPISymbol(MO.getIndex());
}

MCSymbol *XtensaMCInstLower::
GetBlockAddressSymbol(const MachineOperand &MO) const 
{
  /*
  switch (MO.getTargetFlags()) 
  {
  default: llvm_unreachable("Unknown target flag on GV operand");
  case 0: break;
  }
   */ 

  return Printer.GetBlockAddressSymbol(MO.getBlockAddress());
}

MCOperand XtensaMCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                               MachineOperand::MachineOperandType MOTy,
                                               unsigned Offset) const 
{
  MCSymbolRefExpr::VariantKind Kind = MCSymbolRefExpr::VK_None;
  const MCSymbol *Symbol;

  switch (MOTy) 
  {
    case MachineOperand::MO_MachineBasicBlock:
      Symbol = MO.getMBB()->getSymbol();
      break;
    case MachineOperand::MO_GlobalAddress:
      Symbol = Printer.getSymbol(MO.getGlobal());
      Offset += MO.getOffset();
      break;
    case MachineOperand::MO_BlockAddress:
      Symbol = Printer.GetBlockAddressSymbol(MO.getBlockAddress());
      Offset += MO.getOffset();
      break;
    case MachineOperand::MO_ExternalSymbol:
      Symbol = GetExternalSymbolSymbol(MO);
      Offset += MO.getOffset();
      break;
    case MachineOperand::MO_JumpTableIndex:
      Symbol = GetJumpTableSymbol(MO);
      break;
    case MachineOperand::MO_ConstantPoolIndex:
      Symbol = GetConstantPoolIndexSymbol(MO);
      Offset += MO.getOffset();
      break;
    default:
      llvm_unreachable("<unknown operand type>");
  }

  const MCSymbolRefExpr *MCSym = MCSymbolRefExpr::create(Symbol, Kind, Ctx);

  if (!Offset)
    return MCOperand::createExpr(MCSym);

  // Assume offset is never negative.
  assert(Offset > 0);

  const MCConstantExpr *OffsetExpr =  MCConstantExpr::create(Offset, Ctx);
  const MCBinaryExpr *Add = MCBinaryExpr::createAdd(MCSym, OffsetExpr, Ctx);
  return MCOperand::createExpr(Add);
}

MCOperand XtensaMCInstLower::lowerOperand(const MachineOperand &MO,
                                         unsigned Offset) const 
{
  MachineOperand::MachineOperandType MOTy = MO.getType();

  switch (MOTy) 
  {
    case MachineOperand::MO_Register:
      // Ignore all implicit register operands.
      if (MO.isImplicit()) break;
      return MCOperand::createReg(MO.getReg());
    case MachineOperand::MO_Immediate:
      return MCOperand::createImm(MO.getImm() + Offset);
    case MachineOperand::MO_MachineBasicBlock:
    case MachineOperand::MO_GlobalAddress:
    case MachineOperand::MO_ExternalSymbol:
    case MachineOperand::MO_JumpTableIndex:
    case MachineOperand::MO_ConstantPoolIndex:
    case MachineOperand::MO_BlockAddress:
      return LowerSymbolOperand(MO, MOTy, Offset);
    case MachineOperand::MO_RegisterMask:
      break;
    default: llvm_unreachable("unknown operand type");  
  }

  return MCOperand();
}

void XtensaMCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) const 
{
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) 
  {
    const MachineOperand &MO = MI->getOperand(i);
    MCOperand MCOp = lowerOperand(MO);

    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}