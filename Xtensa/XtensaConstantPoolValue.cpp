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
 
#include "XtensaConstantPoolValue.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
using namespace llvm;

XtensaConstantPoolValue::XtensaConstantPoolValue(Type *Ty, unsigned id,
                                           XtensaCP::XtensaCPKind kind,
                                           bool addCurrentAddress)
  : MachineConstantPoolValue(Ty), LabelId(id), Kind(kind),
    AddCurrentAddress(addCurrentAddress) {}

XtensaConstantPoolValue::XtensaConstantPoolValue(LLVMContext &C, unsigned id,
                                           XtensaCP::XtensaCPKind kind,
                                           unsigned char PCAdj,
                                           bool addCurrentAddress)
  : MachineConstantPoolValue((Type*)Type::getInt32Ty(C)),
    LabelId(id), Kind(kind),
    AddCurrentAddress(addCurrentAddress) 
{
}

XtensaConstantPoolValue::~XtensaConstantPoolValue() {}

int XtensaConstantPoolValue::getExistingMachineCPValue(MachineConstantPool *CP,
                                                    unsigned Alignment) 
{
  llvm_unreachable("Shouldn't be calling this directly!");
}

void
XtensaConstantPoolValue::addSelectionDAGCSEId(FoldingSetNodeID &ID) 
{
  ID.AddInteger(LabelId);
}

bool
XtensaConstantPoolValue::hasSameValue(XtensaConstantPoolValue *ACPV) 
{
  if (ACPV->Kind == Kind) 
  {
    if (ACPV->LabelId == LabelId)
      return true;
    // Two PC relative constpool entries containing the same GV address or
    // external symbols. FIXME: What about blockaddress?
    if (Kind == XtensaCP::CPValue || Kind == XtensaCP::CPExtSymbol)
      return true;
  }
  return false;
}

void XtensaConstantPoolValue::dump() const 
{
  errs() << "  " << *this;
}

void XtensaConstantPoolValue::print(raw_ostream &O) const 
{
}

//===----------------------------------------------------------------------===//
// XtensaConstantPoolConstant
//===----------------------------------------------------------------------===//

XtensaConstantPoolConstant::XtensaConstantPoolConstant(Type *Ty,
                                                 const Constant *C,
                                                 unsigned ID,
                                                 XtensaCP::XtensaCPKind Kind,
                                                 bool AddCurrentAddress)
  : XtensaConstantPoolValue(Ty, ID, Kind, PCAdj, Modifier, AddCurrentAddress),
    CVal(C) 
{
}

XtensaConstantPoolConstant::XtensaConstantPoolConstant(const Constant *C,
                                                 unsigned ID,
                                                 XtensaCP::XtensaCPKind Kind,
                                                 bool AddCurrentAddress)
  : XtensaConstantPoolValue((Type*)C->getType(), ID, Kind, 
                         AddCurrentAddress),
    CVal(C) 
{
}

XtensaConstantPoolConstant *
XtensaConstantPoolConstant::Create(const Constant *C, unsigned ID) 
{
  return new XtensaConstantPoolConstant(C, ID, XtensaCP::CPValue, 0, false);
}

XtensaConstantPoolConstant *
XtensaConstantPoolConstant::Create(const GlobalValue *GV) 
{
  return new XtensaConstantPoolConstant((Type*)Type::getInt32Ty(GV->getContext()),
                                     GV, 0, XtensaCP::CPValue, 0,  false);
}

XtensaConstantPoolConstant *
XtensaConstantPoolConstant::Create(const Constant *C, unsigned ID,
                                XtensaCP::XtensaCPKind Kind, unsigned char PCAdj) {
  return new XtensaConstantPoolConstant(C, ID, Kind, false);
}

XtensaConstantPoolConstant *
XtensaConstantPoolConstant::Create(const Constant *C, unsigned ID,
                                XtensaCP::XtensaCPKind Kind, bool AddCurrentAddress) 
{
  return new XtensaConstantPoolConstant(C, ID, Kind, AddCurrentAddress);
}

const GlobalValue *XtensaConstantPoolConstant::getGV() const 
{
  return dyn_cast_or_null<GlobalValue>(CVal);
}

const BlockAddress *XtensaConstantPoolConstant::getBlockAddress() const 
{
  return dyn_cast_or_null<BlockAddress>(CVal);
}

int XtensaConstantPoolConstant::getExistingMachineCPValue(MachineConstantPool *CP,
                                                       unsigned Alignment) 
{
  return getExistingMachineCPValueImpl<XtensaConstantPoolConstant>(CP, Alignment);
}

bool XtensaConstantPoolConstant::hasSameValue(XtensaConstantPoolValue *ACPV) 
{
  const XtensaConstantPoolConstant *ACPC = dyn_cast<XtensaConstantPoolConstant>(ACPV);
  return ACPC && ACPC->CVal == CVal && XtensaConstantPoolValue::hasSameValue(ACPV);
}

void XtensaConstantPoolConstant::addSelectionDAGCSEId(FoldingSetNodeID &ID) 
{
  ID.AddPointer(CVal);
  XtensaConstantPoolValue::addSelectionDAGCSEId(ID);
}

void XtensaConstantPoolConstant::print(raw_ostream &O) const 
{
  O << CVal->getName();
  XtensaConstantPoolValue::print(O);
}

XtensaConstantPoolSymbol::XtensaConstantPoolSymbol(LLVMContext &C, const char *s,
                                             unsigned id,
                                             bool AddCurrentAddress)
  : XtensaConstantPoolValue(C, id, XtensaCP::CPExtSymbol,
                         AddCurrentAddress), S(s) 
{
}

XtensaConstantPoolSymbol *
XtensaConstantPoolSymbol::Create(LLVMContext &C, const char *s,
                              unsigned ID) 
{
  return new XtensaConstantPoolSymbol(C, s, ID, false);
}

int XtensaConstantPoolSymbol::getExistingMachineCPValue(MachineConstantPool *CP,
                                                     unsigned Alignment) 
{
  return getExistingMachineCPValueImpl<XtensaConstantPoolSymbol>(CP, Alignment);
}

bool XtensaConstantPoolSymbol::hasSameValue(XtensaConstantPoolValue *ACPV) 
{
  const XtensaConstantPoolSymbol *ACPS = dyn_cast<XtensaConstantPoolSymbol>(ACPV);
  return ACPS && ACPS->S == S && XtensaConstantPoolValue::hasSameValue(ACPV);
}

void XtensaConstantPoolSymbol::addSelectionDAGCSEId(FoldingSetNodeID &ID) 
{
  ID.AddString(S);
  XtensaConstantPoolValue::addSelectionDAGCSEId(ID);
}

void XtensaConstantPoolSymbol::print(raw_ostream &O) const 
{
  O << S;
  XtensaConstantPoolValue::print(O);
}

XtensaConstantPoolMBB::XtensaConstantPoolMBB(LLVMContext &C,
                                       const MachineBasicBlock *mbb,
                                       unsigned id, bool AddCurrentAddress)
  : XtensaConstantPoolValue(C, id, XtensaCP::CPMachineBasicBlock, 
        AddCurrentAddress),
    MBB(mbb) 
{
}

XtensaConstantPoolMBB *XtensaConstantPoolMBB::Create(LLVMContext &C,
                                               const MachineBasicBlock *mbb,
                                               unsigned ID,
                                               unsigned char PCAdj) 
{
  return new XtensaConstantPoolMBB(C, mbb, ID, false);
}

int XtensaConstantPoolMBB::getExistingMachineCPValue(MachineConstantPool *CP,
                                                  unsigned Alignment) 
{
  return getExistingMachineCPValueImpl<XtensaConstantPoolMBB>(CP, Alignment);
}

bool XtensaConstantPoolMBB::hasSameValue(XtensaConstantPoolValue *ACPV) 
{
  const XtensaConstantPoolMBB *ACPMBB = dyn_cast<XtensaConstantPoolMBB>(ACPV);
  return ACPMBB && ACPMBB->MBB == MBB &&
    XtensaConstantPoolValue::hasSameValue(ACPV);
}

void XtensaConstantPoolMBB::addSelectionDAGCSEId(FoldingSetNodeID &ID) 
{
  ID.AddPointer(MBB);
  XtensaConstantPoolValue::addSelectionDAGCSEId(ID);
}

void XtensaConstantPoolMBB::print(raw_ostream &O) const 
{
  O << "BB#" << MBB->getNumber();
  XtensaConstantPoolValue::print(O);
}
