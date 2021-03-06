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
 
#define DEBUG_TYPE "xtensa-lower"

#include "XtensaISelLowering.h"
#include "XtensaCallingConv.h"
#include "XtensaMachineFunctionInfo.h"
#include "XtensaSubtarget.h"
#include "XtensaTargetMachine.h"
#include "XtensaTargetObjectFile.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static const MCPhysReg XtensaIntRegs[16] = 
{
  Xtensa::a0, Xtensa::sp, Xtensa::a2, Xtensa::a3,
  Xtensa::a4, Xtensa::a5, Xtensa::a6, Xtensa::a7,
  Xtensa::a8, Xtensa::a9, Xtensa::a10, Xtensa::a11,
  Xtensa::a12, Xtensa::a13, Xtensa::a14, Xtensa::a15
};

/*
void XtensaTargetObjectFile::Initialize(MCContext &Ctx, const TargetMachine &TM) 
{
  TargetLoweringObjectFileELF::Initialize(Ctx, TM);
  InitializeELF(TM.Options.UseInitArray);
}
*/

XtensaTargetLowering::XtensaTargetLowering(const TargetMachine &tm, 
                                         const XtensaSubtarget &STI)
    :TargetLowering(tm), Subtarget(STI)
{
  MVT PtrVT = MVT::i32;
  // Set up the register classes.
  addRegisterClass(MVT::i32,  &Xtensa::ARRegClass);

  // Set up special registers.
  setStackPointerRegisterToSaveRestore(Xtensa::sp);

  setSchedulingPreference(Sched::RegPressure);

  //For i1 types all bits are zero except bit 0
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent); //vectors of i1s are the same

  // Used by legalize types to correctly generate the setcc result.
  AddPromotedToType(ISD::SETCC, MVT::i1, MVT::i32);

  setMinFunctionAlignment(1);


  setOperationAction(ISD::BR_CC, MVT::i32, Expand);
  setOperationAction(ISD::BR_CC, MVT::i64, Expand);
  
//  setOperationAction(ISD::SELECT, MVT::i32, Expand);
//  setOperationAction(ISD::SELECT, MVT::i64, Expand);  
  
  setOperationAction(ISD::SELECT_CC, MVT::i32, Expand);
  setOperationAction(ISD::SELECT_CC, MVT::i64, Custom);  
  
  setOperationAction(ISD::SETCC, MVT::i32, Legal);   //folds into brcond
  setOperationAction(ISD::SETCC, MVT::i64, Expand); 
  setOperationAction(ISD::Constant, MVT::i32, Custom);
  setOperationAction(ISD::Constant, MVT::i64, Custom);

  // Expand jump table branches as address arithmetic followed by an
  // indirect jump.
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  //Xtensa also does not have indirect branch so expand them
  //TODO: don't we have one via JALR?
  setOperationAction(ISD::BRIND, MVT::Other, Expand);

  //make BRCOND legal, its actually only legal for a subset of conds
  setOperationAction(ISD::BRCOND, MVT::Other, Legal);

  //Custom Lower Overflow operators

  // Handle integer types.
  for (unsigned I = MVT::FIRST_INTEGER_VALUETYPE;
       I <= MVT::LAST_INTEGER_VALUETYPE;
       ++I) 
  {
    MVT VT = MVT::SimpleValueType(I);
    if (isTypeLegal(VT)) 
    {
      //No support at all
      setOperationAction(ISD::SDIVREM, VT, Expand);
      setOperationAction(ISD::UDIVREM, VT, Expand);
      
      setOperationAction(ISD::ATOMIC_LOAD,  VT, Expand);
      setOperationAction(ISD::ATOMIC_STORE, VT, Expand);

    }
  }

  setOperationAction(ISD::MUL  , MVT::i32, Legal);
  setOperationAction(ISD::MUL  , MVT::i64, Expand);
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHS, MVT::i64, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i64, Expand);
  setOperationAction(ISD::SDIV , MVT::i32, Legal);
  setOperationAction(ISD::SDIV , MVT::i64, Expand);
  setOperationAction(ISD::UDIV , MVT::i32, Legal);
  setOperationAction(ISD::SDIV , MVT::i32, Legal);
  setOperationAction(ISD::SDIV , MVT::i64, Expand);
  setOperationAction(ISD::SREM , MVT::i32, Legal);
  setOperationAction(ISD::SREM , MVT::i64, Expand);
  setOperationAction(ISD::UREM , MVT::i32, Legal);
  setOperationAction(ISD::UREM , MVT::i64, Expand);  
  
  //Xtensa doesn't support  [ADD,SUB][E,C]
  setOperationAction(ISD::ADDC, MVT::i32, Expand);
  setOperationAction(ISD::ADDE, MVT::i32, Expand);
  setOperationAction(ISD::SUBC, MVT::i32, Expand);
  setOperationAction(ISD::SUBE, MVT::i32, Expand);

  setOperationAction(ISD::ADD, MVT::i64, Expand);
  setOperationAction(ISD::SUB, MVT::i64, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);
  
  //Xtensa doesn't support s[hl,rl,ra]_parts
  // TODO  
  setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);

  // Bit Manipulation
  setOperationAction(ISD::CTPOP, MVT::i32, Expand);
  //Xtensa doesn't support s[hl,rl,ra]_parts
  setOperationAction(ISD::ROTL , MVT::i32, Expand);
  setOperationAction(ISD::ROTR , MVT::i32, Expand);
  // No special instructions for these.
  setOperationAction(ISD::CTTZ_ZERO_UNDEF, MVT::i32, Expand);
  setOperationAction(ISD::CTLZ_ZERO_UNDEF, MVT::i32, Expand);

  setOperationAction(ISD::TRAP, MVT::Other, Legal);

  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i64, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i64, Expand);

  // No sign extend instructions for i1
  for (MVT VT : MVT::integer_valuetypes()) 
  {
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::EXTLOAD,  VT, MVT::i1, Promote);
  }
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i32, Expand);

  // Handle the various types of symbolic address.
  setOperationAction(ISD::ConstantPool,     PtrVT, Custom);
  setOperationAction(ISD::GlobalAddress,    PtrVT, Custom);
  setOperationAction(ISD::GlobalTLSAddress, PtrVT, Custom);
  setOperationAction(ISD::BlockAddress,     PtrVT, Custom);
  setOperationAction(ISD::JumpTable,        PtrVT, Custom);

  //Expand stack allocations
  setOperationAction(ISD::DYNAMIC_STACKALLOC, PtrVT, Expand);

  // Use custom expanders so that we can force the function to use
  // a frame pointer.
  // TODO: real comment
  setOperationAction(ISD::STACKSAVE,    MVT::Other, Custom);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Custom);
  setOperationAction(ISD::FRAMEADDR,    MVT::Other, Custom);

  // Handle floating-point types.
  // TODO
  for (unsigned I = MVT::FIRST_FP_VALUETYPE;
       I <= MVT::LAST_FP_VALUETYPE;
       ++I) 
  {
    MVT VT = MVT::SimpleValueType(I);
    if (isTypeLegal(VT)) 
    {
      // We can use FI for FRINT.
      //setOperationAction(ISD::FRINT, VT, Legal);
      if (VT.getSizeInBits() == 32  && Subtarget.hasF())
      {  
        setOperationAction(ISD::FADD, VT, Legal);
        setOperationAction(ISD::FSUB, VT, Legal);
        setOperationAction(ISD::FMUL, VT, Legal);
        setOperationAction(ISD::FDIV, VT, Legal);
      }
      else
      {
        setOperationAction(ISD::FADD, VT, Expand);
        setOperationAction(ISD::FSUB, VT, Expand);
        setOperationAction(ISD::FMUL, VT, Expand);
        setOperationAction(ISD::FDIV, VT, Expand);        
      }  
      
      //TODO: once implemented in InstrInfo uncomment
      setOperationAction(ISD::FSQRT, VT, Expand);

      // No special instructions for these.
      setOperationAction(ISD::FSIN, VT, Expand);
      setOperationAction(ISD::FCOS, VT, Expand);
      setOperationAction(ISD::FREM, VT, Expand);
      setOperationAction(ISD::FABS, VT, Expand);
    }
  }

  // Handle floating-point types.
  if (Subtarget.hasF())
  {
    setOperationAction(ISD::FMA, MVT::f32,  Legal);
    setOperationAction(ISD::BITCAST, MVT::i32, Legal);
    setOperationAction(ISD::BITCAST, MVT::f32, Legal);
    setOperationAction(ISD::UINT_TO_FP, MVT::i32, Legal);
    setOperationAction(ISD::SINT_TO_FP, MVT::i32, Legal);
    setOperationAction(ISD::FP_TO_UINT, MVT::i32, Legal);
    setOperationAction(ISD::FP_TO_SINT, MVT::i32, Legal);
    setOperationAction(ISD::FCOPYSIGN, MVT::f32, Legal);
  }
  else
  {
    setOperationAction(ISD::FMA, MVT::f32,  Expand);
    setOperationAction(ISD::SETCC, MVT::f32, Expand);
    setOperationAction(ISD::BITCAST, MVT::i32, Expand);
    setOperationAction(ISD::BITCAST, MVT::f32, Expand);
    setOperationAction(ISD::UINT_TO_FP, MVT::i32, Expand);
    setOperationAction(ISD::SINT_TO_FP, MVT::i32, Expand);
    setOperationAction(ISD::FP_TO_UINT, MVT::i32, Expand);
    setOperationAction(ISD::FP_TO_SINT, MVT::i32, Expand);
    setOperationAction(ISD::UINT_TO_FP, MVT::i64, Expand);
    setOperationAction(ISD::SINT_TO_FP, MVT::i64, Expand);
    setOperationAction(ISD::FP_TO_UINT, MVT::i64, Expand);
    setOperationAction(ISD::FP_TO_SINT, MVT::i64, Expand);
  }
  setOperationAction(ISD::FMA, MVT::f64,  Expand);
  setOperationAction(ISD::SETCC, MVT::f64, Expand);
  setOperationAction(ISD::BITCAST, MVT::i64, Expand);
  setOperationAction(ISD::BITCAST, MVT::f64, Expand);

  // Needed so that we don't try to implement f128 constant loads using
  // a load-and-extend of a f80 constant (in cases where the constant
  // would fit in an f80).
  for (MVT VT : MVT::fp_valuetypes())
    setLoadExtAction(ISD::EXTLOAD, VT, MVT::f80, Expand);

  // Floating-point truncation and stores need to be done separately.
  setTruncStoreAction(MVT::f64,  MVT::f32, Expand);

  // We have 64-bit FPR<->GPR moves, but need special handling for
  // 32-bit forms.

  // VASTART and VACOPY need to deal with the Xtensa-specific varargs
  // structure, but VAEND is a no-op.
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  //we always write var args with word boundary so we have to customize this
  setOperationAction(ISD::VAARG  , MVT::Other, Custom);
  setOperationAction(ISD::VACOPY , MVT::Other, Expand);
  setOperationAction(ISD::VAEND  , MVT::Other, Expand);

  //to have the best chance and doing something good with fences custom lower them
  setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Custom);  

  // Compute derived properties from the register classes
  computeRegisterProperties(STI.getRegisterInfo());
}

/// If a physical register, this returns the register that receives the
/// exception address on entry to an EH pad.
unsigned
XtensaTargetLowering::getExceptionPointerRegister(const Constant *PersonalityFn) const 
{
  // TODO
  return Xtensa::a14;
}

/// If a physical register, this returns the register that receives the
/// exception typeid on entry to a landing pad.
unsigned
XtensaTargetLowering::getExceptionSelectorRegister(const Constant *PersonalityFn) const 
{
  // TODO
  return Xtensa::a13;
}

bool XtensaTargetLowering::isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const 
{
  // The Xtensa target isn't yet aware of offsets.
  return false;
}

bool XtensaTargetLowering::isFPImmLegal(const APFloat &Imm, EVT VT) const 
{
  return false;
}

//===----------------------------------------------------------------------===//
// Inline asm support
//===----------------------------------------------------------------------===//

TargetLowering::ConstraintType
XtensaTargetLowering::getConstraintType(StringRef Constraint) const 
{
  if (Constraint.size() == 1) 
  {
    switch (Constraint[0]) 
    {
      case 'a': // Address register
      case 'd': // Data register (equivalent to 'r')
      case 'f': // Floating-point register
      case 'r': // General-purpose register
        return C_RegisterClass;

      case 'Q': // Memory with base and unsigned 12-bit displacement
      case 'R': // Likewise, plus an index
      case 'S': // Memory with base and signed 20-bit displacement
      case 'T': // Likewise, plus an index
      case 'm': // Equivalent to 'T'.
        return C_Memory;

      case 'I': // Unsigned 8-bit constant
      case 'J': // Unsigned 12-bit constant
      case 'K': // Signed 16-bit constant
      case 'L': // Signed 20-bit displacement (on all targets we support)
      case 'M': // 0x7fffffff
        return C_Other;

      default:
        break;
    }
  }
  return TargetLowering::getConstraintType(Constraint);
}

TargetLowering::ConstraintWeight XtensaTargetLowering::
getSingleConstraintMatchWeight(AsmOperandInfo &info,
                               const char *constraint) const 
{
  ConstraintWeight weight = CW_Invalid;
  Value *CallOperandVal = info.CallOperandVal;
  // If we don't have a value, we can't do a match,
  // but allow it at the lowest weight.
  if (CallOperandVal == NULL)
    return CW_Default;
  Type *type = CallOperandVal->getType();
  // Look at the constraint type.
  switch (*constraint) 
  {
    default:
      weight = TargetLowering::getSingleConstraintMatchWeight(info, constraint);
      break;

    case 'a': // Address register
    case 'd': // Data register (equivalent to 'r')
    case 'r': // General-purpose register
      if (CallOperandVal->getType()->isIntegerTy())
        weight = CW_Register;
      break;

    case 'f': // Floating-point register
      if (type->isFloatingPointTy())
        weight = CW_Register;
      break;

    case 'I': // Unsigned 8-bit constant
      if (ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
        if (isUInt<8>(C->getZExtValue()))
          weight = CW_Constant;
      break;

    case 'J': // Unsigned 12-bit constant
      if (ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
        if (isUInt<12>(C->getZExtValue()))
          weight = CW_Constant;
      break;

    case 'K': // Signed 16-bit constant
      if (ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
        if (isInt<16>(C->getSExtValue()))
          weight = CW_Constant;
      break;

    case 'L': // Signed 20-bit displacement (on all targets we support)
      if (ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
        if (isInt<20>(C->getSExtValue()))
          weight = CW_Constant;
      break;

    case 'M': // 0x7fffffff
      if (ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
        if (C->getZExtValue() == 0x7fffffff)
          weight = CW_Constant;
      break;
  }
  return weight;
}

std::pair<unsigned, const TargetRegisterClass *> XtensaTargetLowering::
getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI, StringRef Constraint, 
        MVT VT) const 
{
  /* TODO
  if (Constraint.size() == 1) 
  {
    // GCC Constraint Letters
    switch (Constraint[0]) 
    {
      default: break;
      case 'd': // Data register (equivalent to 'r')
      case 'r': // General-purpose register
        return std::make_pair(0U, &Xtensa::ARBitRegClass);

      case 'f': // Floating-point register
        if(Subtarget.hasF())
          return std::make_pair(0U, &Xtensa::FP32BitRegClass);
        return std::make_pair(0U, &Xtensa::ARBitRegClass);
    }
  }
   */ 
  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}

void XtensaTargetLowering::
LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                             std::vector<SDValue> &Ops,
                             SelectionDAG &DAG) const 
{
  // Only support length 1 constraints for now.
  /* TODO
  if (Constraint.length() == 1) 
  {
    switch (Constraint[0]) 
    {
      case 'I': // Unsigned 8-bit constant
        if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op))
          if (isUInt<8>(C->getZExtValue()))
            Ops.push_back(DAG.getTargetConstant(C->getZExtValue(), SDLoc(Op),
                                                Op.getValueType()));
        return;

      case 'J': // Unsigned 12-bit constant
        if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op))
          if (isUInt<12>(C->getZExtValue()))
            Ops.push_back(DAG.getTargetConstant(C->getZExtValue(), SDLoc(Op),
                                                Op.getValueType()));
        return;

      case 'K': // Signed 16-bit constant
        if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op))
          if (isInt<16>(C->getSExtValue()))
            Ops.push_back(DAG.getTargetConstant(C->getSExtValue(), SDLoc(Op),
                                                Op.getValueType()));
        return;

      case 'L': // Signed 20-bit displacement (on all targets we support)
        if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op))
          if (isInt<20>(C->getSExtValue()))
            Ops.push_back(DAG.getTargetConstant(C->getSExtValue(), SDLoc(Op),
                                                Op.getValueType()));
        return;

      case 'M': // 0x7fffffff
        if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op))
          if (C->getZExtValue() == 0x7fffffff)
            Ops.push_back(DAG.getTargetConstant(C->getZExtValue(), SDLoc(Op),
                                                Op.getValueType()));
        return;
    }
  }
  TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops, DAG);
   */ 
}

//===----------------------------------------------------------------------===//
//  Lower helper functions
//===----------------------------------------------------------------------===//

// addLiveIn - This helper function adds the specified physical register to the
// MachineFunction as a live in value.  It also creates a corresponding
// virtual register for it.
static unsigned
addLiveIn(MachineFunction &MF, unsigned PReg, const TargetRegisterClass *RC)
{
  unsigned VReg = MF.getRegInfo().createVirtualRegister(RC);
  MF.getRegInfo().addLiveIn(PReg, VReg);
  return VReg;
}

//===----------------------------------------------------------------------===//
// Calling conventions
//===----------------------------------------------------------------------===//

#include "XtensaGenCallingConv.inc"

// Value is a value that has been passed to us in the location described by VA
// (and so has type VA.getLocVT()).  Convert Value to VA.getValVT(), chaining
// any loads onto Chain.
static SDValue convertLocVTToValVT(SelectionDAG &DAG, SDLoc DL, CCValAssign &VA,
                                   SDValue Chain, SDValue Value) 
{
  // If the argument has been promoted from a smaller type, insert an
  // assertion to capture this.
  if (VA.getLocInfo() == CCValAssign::SExt)
    Value = DAG.getNode(ISD::AssertSext, DL, VA.getLocVT(), Value,
                        DAG.getValueType(VA.getValVT()));
  else if (VA.getLocInfo() == CCValAssign::ZExt)
    Value = DAG.getNode(ISD::AssertZext, DL, VA.getLocVT(), Value,
                        DAG.getValueType(VA.getValVT()));

  if (VA.isExtInLoc())
    Value = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Value);
  else if (VA.getLocInfo() == CCValAssign::Indirect)
    Value = DAG.getLoad(VA.getValVT(), DL, Chain, Value,
                        MachinePointerInfo(), false, false, false, 0);
  else
    assert(VA.getLocInfo() == CCValAssign::Full && "Unsupported getLocInfo");
  return Value;
}

// Value is a value of type VA.getValVT() that we need to copy into
// the location described by VA.  Return a copy of Value converted to
// VA.getValVT().  The caller is responsible for handling indirect values.
static SDValue convertValVTToLocVT(SelectionDAG &DAG, SDLoc DL, CCValAssign &VA,
                                   SDValue Value) 
{
  switch (VA.getLocInfo()) {
  case CCValAssign::SExt:
    return DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Value);
  case CCValAssign::ZExt:
    return DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Value);
  case CCValAssign::AExt:
    return DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Value);
  case CCValAssign::BCvt:
    return DAG.getNode(ISD::BITCAST, DL, VA.getLocVT(), Value);
  case CCValAssign::Full:
    return Value;
  default:
    llvm_unreachable("Unhandled getLocInfo()");
  }
}

SDValue XtensaTargetLowering::
LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                     const SmallVectorImpl<ISD::InputArg> &Ins,
                     SDLoc DL, SelectionDAG &DAG,
                     SmallVectorImpl<SDValue> &InVals) const 
{
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  XtensaFunctionInfo *XtensaFI = MF.getInfo<XtensaFunctionInfo>();

  XtensaFI->setVarArgsFrameIndex(0);

  // Used with vargs to acumulate store chains.
  std::vector<SDValue> OutChains;

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
		 *DAG.getContext());

  CCInfo.AnalyzeFormalArguments(Ins,
    IsVarArg ? CC_Xtensa_VAR : CC_Xtensa);
  
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) 
  {
    CCValAssign &VA = ArgLocs[i];
    // Arguments stored on registers
    if (VA.isRegLoc()) 
    {
      EVT RegVT = VA.getLocVT();
      const TargetRegisterClass *RC;

      if (RegVT == MVT::i32) 
      {
        RC = &Xtensa::ARRegClass;
      } 
      /* TODO
      else if (RegVT == MVT::i64)
      {
        RC = &Xtensa::PairAR64BitRegClass;
      } 
      else if (RegVT == MVT::f32) 
      {
        if (Subtarget.hasF())
          RC = &Xtensa::FP32BitRegClass;
        else 
            RC = &Xtensa::ARBitRegClass;
      } 
      else if (RegVT == MVT::f64) 
      {
        if(Subtarget.hasF())
          RC = &Xtensa::PairFP64BitRegClass;
        else
          RC = &Xtensa::PairAR64BitRegClass;
      }
       */ 
      else  llvm_unreachable("RegVT not supported by FormalArguments Lowering");

      // Transform the arguments stored on
      // physical registers into virtual ones
      unsigned Reg = MF.addLiveIn(VA.getLocReg(), RC);
      SDValue ArgValue = DAG.getCopyFromReg(Chain, DL, Reg, RegVT);

      // If this is an 8 or 16-bit value, it has been passed promoted
      // to 32 bits.  Insert an assert[sz]ext to capture this, then
      // truncate to the right size.
      if (VA.getLocInfo() != CCValAssign::Full) 
      {
        unsigned Opcode = 0;
        if (VA.getLocInfo() == CCValAssign::SExt)
          Opcode = ISD::AssertSext;
        else if (VA.getLocInfo() == CCValAssign::ZExt)
          Opcode = ISD::AssertZext;
        if (Opcode)
          ArgValue = DAG.getNode(Opcode, DL, RegVT, ArgValue,
                                 DAG.getValueType(VA.getValVT()));
        ArgValue = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), ArgValue);
      }

      InVals.push_back(ArgValue);
    } 
    else 
    { // !VA.isRegLoc()
      // sanity check
      assert(VA.isMemLoc());

      EVT ValVT = VA.getValVT();

      // The stack pointer offset is relative to the caller stack frame.
      int FI = MFI->CreateFixedObject(ValVT.getSizeInBits()/8,
                                      VA.getLocMemOffset(), true);

      // Create load nodes to retrieve arguments from the stack
      SDValue FIN = DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
      InVals.push_back(DAG.getLoad(ValVT, DL, Chain, FIN,
                                   MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI),
                                   false, false, false, 0));
    }
  }

  if (IsVarArg)
  {
    auto ArgRegs = XtensaIntRegs;
    unsigned NumRegs = llvm::Xtensa::NumArgGPRs;
    unsigned Idx = CCInfo.getFirstUnallocated(ArrayRef<MCPhysReg>(ArgRegs, 8));
    unsigned RegSize = 4;
    MVT RegTy = MVT::getIntegerVT(RegSize * 8);
    const TargetRegisterClass *RC = getRegClassFor(RegTy);

    // Offset of the first variable argument from stack pointer.
    int VaArgOffset;

    if (NumRegs == Idx)
      VaArgOffset = RoundUpToAlignment(CCInfo.getNextStackOffset(), RegSize);
    else
      VaArgOffset = -(int)(RegSize * (NumRegs - Idx));

    // Record the frame index of the first variable argument
    // which is a value necessary to VASTART.
    int FI = MFI->CreateFixedObject(RegSize, VaArgOffset, true);
    XtensaFI->setVarArgsFrameIndex(FI);

    // Copy the integer registers that have not been used for argument passing
    // to the argument register save area. 
    for (unsigned I = Idx; I < NumRegs; ++I, VaArgOffset += RegSize) 
    {
      unsigned Reg = addLiveIn(MF, ArgRegs[I], RC);
      SDValue ArgValue = DAG.getCopyFromReg(Chain, DL, Reg, RegTy);
      FI = MFI->CreateFixedObject(RegSize, VaArgOffset, true);
      SDValue PtrOff = DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
      SDValue Store = DAG.getStore(Chain, DL, ArgValue, PtrOff,
                                   MachinePointerInfo(), false, false, 0);
      cast<StoreSDNode>(Store.getNode())
          ->getMemOperand()
          ->setValue((Value *)nullptr);
      OutChains.push_back(Store);
    }
  }

  // All stores are grouped in one node to allow the matching between
  // the size of Ins and InVals. This only happens when on varg functions
  if (!OutChains.empty()) 
  {
    OutChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, OutChains);
  }

  return Chain;
}

SDValue XtensaTargetLowering::getTargetNode(SDValue Op, SelectionDAG &DAG, 
        unsigned Flag) const 
{
  EVT Ty = getPointerTy(DAG.getDataLayout());

  if (GlobalAddressSDNode *N = dyn_cast<GlobalAddressSDNode>(Op))
    return DAG.getTargetGlobalAddress(N->getGlobal(), SDLoc(Op), Ty, 0, Flag);
  if (ExternalSymbolSDNode *N = dyn_cast<ExternalSymbolSDNode>(Op))
    return DAG.getTargetExternalSymbol(N->getSymbol(), Ty, Flag);
  if (BlockAddressSDNode *N = dyn_cast<BlockAddressSDNode>(Op))
    return DAG.getTargetBlockAddress(N->getBlockAddress(), Ty, 0, Flag);
  if (JumpTableSDNode *N = dyn_cast<JumpTableSDNode>(Op))
    return DAG.getTargetJumpTable(N->getIndex(), Ty, Flag);
  if (ConstantPoolSDNode *N = dyn_cast<ConstantPoolSDNode>(Op))
    return DAG.getTargetConstantPool(N->getConstVal(), Ty, N->getAlignment(),
                                     N->getOffset(), Flag);

  llvm_unreachable("Unexpected node type.");
  return SDValue();
}


SDValue XtensaTargetLowering::getAddrPIC(SDValue Op, SelectionDAG &DAG) const 
{
  SDLoc DL(Op);
  EVT Ty = Op.getValueType();
  return DAG.getNode(XtensaISD::PCREL_WRAPPER, DL, Ty, Op);
}

SDValue
XtensaTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                 SmallVectorImpl<SDValue> &InVals) const 
{
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &DL = CLI.DL;
  SmallVector<ISD::OutputArg, 32> &Outs = CLI.Outs;
  SmallVector<SDValue, 32> &OutVals = CLI.OutVals;
  SmallVector<ISD::InputArg, 32> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  bool &isTailCall = CLI.IsTailCall;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;
  MachineFunction &MF = DAG.getMachineFunction();
  EVT PtrVT = getPointerTy(DAG.getDataLayout());

  // Xtensa target does not yet support tail call optimization.
  isTailCall = false;

  // Analyze the operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());

  CCAssignFn *CC = IsVarArg ? CC_Xtensa_VAR : CC_Xtensa;
  CCInfo.AnalyzeCallOperands(Outs, CC);
  //
  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = CCInfo.getNextStackOffset();

  // Mark the start of the call.
  Chain = DAG.getCALLSEQ_START(Chain, DAG.getConstant(NumBytes, DL, PtrVT, true),
                               DL);

  // Copy argument values to their designated locations.
  std::deque< std::pair<unsigned, SDValue> > RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;
  SDValue StackPtr;
  for (unsigned I = 0, E = ArgLocs.size(); I != E; ++I) 
  {
    CCValAssign &VA = ArgLocs[I];
    SDValue ArgValue = OutVals[I];
    ISD::ArgFlagsTy Flags = Outs[I].Flags;

    ArgValue = convertValVTToLocVT(DAG, DL, VA, ArgValue);

    if (VA.isRegLoc())
      // Queue up the argument copies and emit them at the end.
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), ArgValue));
    else if (Flags.isByVal()) 
    {
      assert(VA.isMemLoc());
      assert(Flags.getByValSize() &&
             "ByVal args of size 0 should have been ignored by front-end.");
      assert(!isTailCall &&
             "Do not tail-call optimize if there is a byval argument.");

      // True if this byval aggregate will be split between registers
      // and memory.
      unsigned ByValArgsCount = CCInfo.getInRegsParamsCount();
      unsigned CurByValIdx = CCInfo.getInRegsParamsProcessed();
      if (CurByValIdx < ByValArgsCount) {
        unsigned RegBegin, RegEnd;
        CCInfo.getInRegsParamInfo(CurByValIdx, RegBegin, RegEnd);

        EVT PtrVT = DAG.getTargetLoweringInfo().getPointerTy(DAG.getDataLayout());
        unsigned int i, j;
        for (i = 0, j = RegBegin; j < RegEnd; i++, j++) {
          SDValue Const = DAG.getConstant(4*i, DL, MVT::i32);//TODO:should this i32 be ptrTy
          SDValue AddArg = DAG.getNode(ISD::ADD, DL, PtrVT, ArgValue, Const);
          SDValue Load = DAG.getLoad(PtrVT, DL, Chain, AddArg,
                                     MachinePointerInfo(),
                                     false, false, false,
                                     DAG.InferPtrAlignment(AddArg));
          MemOpChains.push_back(Load.getValue(1));
          RegsToPass.push_back(std::make_pair(j, Load));
        }

        CCInfo.nextInRegsParam();
      }

      // TODO: Handle byvals partially or entirely not in registers

    }
    else 
    {
      assert(VA.isMemLoc() && "Argument not register or memory");

      // Work out the address of the stack slot.  Unpromoted ints and
      // floats are passed as right-justified 8-byte values.
      if (!StackPtr.getNode())
        StackPtr = DAG.getCopyFromReg(Chain, DL, Xtensa::sp, PtrVT);
      unsigned Offset = VA.getLocMemOffset();
      SDValue Address = DAG.getNode(ISD::ADD, DL, PtrVT, StackPtr,
                                    DAG.getIntPtrConstant(Offset, DL));

      // Emit the store.
      MemOpChains.push_back(DAG.getStore(Chain, DL, ArgValue, Address,
                                         MachinePointerInfo(),
                                         false, false, 0));
    }
  }

  // Join the stores, which are independent of one another.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);

  // Build a sequence of copy-to-reg nodes, chained and glued together.
  SDValue Glue;
  for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I) 
  {
    Chain = DAG.getCopyToReg(Chain, DL, RegsToPass[I].first,
                             RegsToPass[I].second, Glue);
    Glue = Chain.getValue(1);
  }

  // Accept direct calls by converting symbolic call addresses to the
  // associated Target* opcodes.
  if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee)) 
  {
    if (DAG.getTarget().getRelocationModel() == Reloc::PIC_) 
      Callee = getAddrPIC(DAG.getTargetExternalSymbol(E->getSymbol(), PtrVT), DAG);
    else
      Callee = DAG.getTargetExternalSymbol(E->getSymbol(), PtrVT);
  }

  // The first call operand is the chain and the second is the target address.
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  // Add argument registers to the end of the list so that they are
  // known live into the call.
  for (unsigned I = 0, E = RegsToPass.size(); I != E; ++I)
    Ops.push_back(DAG.getRegister(RegsToPass[I].first,
                                  RegsToPass[I].second.getValueType()));

  // Glue the call to the argument copies, if any.
  if (Glue.getNode())
    Ops.push_back(Glue);

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(XtensaISD::CALL, DL, NodeTys, Ops);
  Glue = Chain.getValue(1);

  // Mark the end of the call, which is glued to the call itself.
  Chain = DAG.getCALLSEQ_END(Chain,
                             DAG.getConstant(NumBytes, DL, PtrVT, true),
                             DAG.getConstant(0, DL, PtrVT, true),
                             Glue, DL);
  Glue = Chain.getValue(1);

  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RetLocs;
  CCState RetCCInfo(CallConv, IsVarArg, MF, RetLocs, *DAG.getContext());
  RetCCInfo.AnalyzeCallResult(Ins, RetCC_Xtensa);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned I = 0, E = RetLocs.size(); I != E; ++I) {
    CCValAssign &VA = RetLocs[I];

    // Copy the value out, gluing the copy to the end of the call sequence.
    SDValue RetValue = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(),
                                          VA.getLocVT(), Glue);
    Chain = RetValue.getValue(1);
    Glue = RetValue.getValue(2);

    // Convert the value of the return register into the value that's
    // being returned.
    InVals.push_back(convertLocVTToValVT(DAG, DL, VA, Chain, RetValue));
  }
  return Chain;
}

/// This hook should be implemented to check whether the return values
/// described by the Outs array can fit into the return registers.  If false
/// is returned, an sret-demotion is performed.
bool
XtensaTargetLowering::CanLowerReturn(CallingConv::ID CallConv,
                                   MachineFunction &MF, bool IsVarArg,
                                   const SmallVectorImpl<ISD::OutputArg> &Outs,
                                   LLVMContext &Context) const 
{
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  return CCInfo.CheckReturn(Outs, RetCC_Xtensa);
}

SDValue
XtensaTargetLowering::LowerReturn(SDValue Chain,
                                   CallingConv::ID CallConv, bool IsVarArg,
                                   const SmallVectorImpl<ISD::OutputArg> &Outs,
                                   const SmallVectorImpl<SDValue> &OutVals,
                                   SDLoc DL, SelectionDAG &DAG) const 
{
  MachineFunction &MF = DAG.getMachineFunction();

  // Assign locations to each returned value.
  SmallVector<CCValAssign, 16> RetLocs;
  CCState RetCCInfo(CallConv, IsVarArg, MF, RetLocs, *DAG.getContext());
  RetCCInfo.AnalyzeReturn(Outs, RetCC_Xtensa);

  SDValue Glue;
  // Quick exit for void returns
  if (RetLocs.empty())
    return DAG.getNode(XtensaISD::RET_FLAG, DL, MVT::Other, Chain);


  // Copy the result values into the output registers.
  SmallVector<SDValue, 4> RetOps;
  RetOps.push_back(Chain);
  for (unsigned I = 0, E = RetLocs.size(); I != E; ++I) 
  {
    CCValAssign &VA = RetLocs[I];
    SDValue RetValue = OutVals[I];

    // Make the return register live on exit.
    assert(VA.isRegLoc() && "Can only return in registers!");

    // Promote the value as required.
    RetValue = convertValVTToLocVT(DAG, DL, VA, RetValue);

    // Chain and glue the copies together.
    unsigned Reg = VA.getLocReg();
    Chain = DAG.getCopyToReg(Chain, DL, Reg, RetValue, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(Reg, VA.getLocVT()));
  }

  // Update chain and glue.
  RetOps[0] = Chain;
  if (Glue.getNode())
    RetOps.push_back(Glue);

  return DAG.getNode(XtensaISD::RET_FLAG, DL, MVT::Other, RetOps);
}

SDValue XtensaTargetLowering::
lowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const
{
  SDLoc DL(Op);
  EVT Ty = Op.getOperand(0).getValueType();
  SDValue Cond = DAG.getNode(ISD::SETCC, DL,
                             getSetCCResultType(DAG.getDataLayout(),*DAG.getContext(), Ty),
                             Op.getOperand(0), Op.getOperand(1),
                             Op.getOperand(4));

  return DAG.getNode(ISD::SELECT, DL, Op.getValueType(), Cond, Op.getOperand(2),
                     Op.getOperand(3));
}

SDValue XtensaTargetLowering::lowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const {
  // check the depth
  //TODO: riscv-gcc can handle this, by navigating through the stack, we should be able to do this too
  assert((cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue() == 0) &&
    "Return address can be determined only for current frame.");
      
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MVT VT = Op.getSimpleValueType();
  unsigned RA = Xtensa::a0;
  MFI->setReturnAddressIsTaken(true);

  // Return RA, which contains the return address. Mark it an implicit live-in.
  unsigned Reg = MF.addLiveIn(RA, getRegClassFor(VT));
  return DAG.getCopyFromReg(DAG.getEntryNode(), SDLoc(Op), Reg, VT);
}

 SDValue XtensaTargetLowering::lowerImmediate(SDValue Op, SelectionDAG &DAG) const
 {
   const ConstantSDNode *CN = cast<ConstantSDNode>(Op);
   SDLoc DL(CN);
   APInt apval = CN->getAPIntValue();
   int64_t value = apval.getSExtValue();
   if (Op.getValueType() == MVT::i32)
   {
     if (value > -2048 && value <= 2047)
       return Op;
     Type *Ty = Type::getInt32Ty(*DAG.getContext());
     Constant *CV = ConstantInt::get(Ty, value);
     SDValue CP = DAG.getConstantPool(CV, MVT::i32);
     return DAG.getLoad(getPointerTy(DAG.getDataLayout()), DL,
       DAG.getEntryNode(), CP, MachinePointerInfo(), false,
       false, false, 0);
   }
   else if (Op.getValueType() == MVT::i64)
   {
     // TODO long constants

   }
    return Op;
 }

SDValue XtensaTargetLowering::lowerGlobalAddress(SDValue Op,
                                                  SelectionDAG &DAG) const 
{
//  Reloc::Model RM = DAG.getTarget().getRelocationModel();

  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Op)) 
  {
    Op = DAG.getTargetGlobalAddress(G->getGlobal(), SDLoc(Op), getPointerTy(DAG.getDataLayout()));
    return getAddrPIC(Op, DAG);
  }
  llvm_unreachable("invalid global addresses to lower");
}

SDValue XtensaTargetLowering::lowerGlobalTLSAddress(GlobalAddressSDNode *GA,
						     SelectionDAG &DAG) const 
{
  // TODO
  
  // If the relocation model is PIC, use the General Dynamic TLS Model or
  // Local Dynamic TLS model, otherwise use the Initial Exec or
  // Local Exec TLS Model.

  SDLoc DL(GA);
  const GlobalValue *GV = GA->getGlobal();
  EVT PtrVT = getPointerTy(DAG.getDataLayout());

  TLSModel::Model model = getTargetMachine().getTLSModel(GV);

  SDValue Offset;
  if (model == TLSModel::LocalExec) 
  {
    // Local Exec TLS Model
    assert(model == TLSModel::LocalExec);
    SDValue TGAHi = DAG.getTargetGlobalAddress(GV, DL, PtrVT, 0,
                                               XtensaII::MO_TPREL_HI);
    SDValue TGALo = DAG.getTargetGlobalAddress(GV, DL, PtrVT, 0,
                                               XtensaII::MO_TPREL_LO);
    SDValue Hi = DAG.getNode(XtensaISD::Hi, DL, PtrVT, TGAHi);
    SDValue Lo = DAG.getNode(XtensaISD::Lo, DL, PtrVT, TGALo);
    Offset = DAG.getNode(ISD::ADD, DL, PtrVT, Hi, Lo); 
  } 
  else llvm_unreachable("only local-exec TLS mode supported");

  //SDValue ThreadPointer = DAG.getNode(MipsISD::ThreadPointer, DL, PtrVT);
  SDValue ThreadPointer = DAG.getRegister(Xtensa::a11 /* TODO Xtensa::tp */, PtrVT);
  
  return DAG.getNode(ISD::ADD, DL, PtrVT, ThreadPointer, Offset);


}

SDValue XtensaTargetLowering::lowerBlockAddress(BlockAddressSDNode *Node,
                                                 SelectionDAG &DAG) const 
{
  const BlockAddress *BA = Node->getBlockAddress();
  int64_t Offset = Node->getOffset();
  EVT PtrVT = getPointerTy(DAG.getDataLayout());

  SDValue Result = DAG.getTargetBlockAddress(BA, PtrVT, Offset);
  return Result;
}

SDValue XtensaTargetLowering::lowerJumpTable(JumpTableSDNode *JT,
                                              SelectionDAG &DAG) const 
{
  SDLoc DL(JT);
  EVT PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetJumpTable(JT->getIndex(), PtrVT);

  // Use LARL to load the address of the table.
  return getAddrPIC(Result, DAG);
}

SDValue XtensaTargetLowering::lowerConstantPool(ConstantPoolSDNode *CP,
                                                 SelectionDAG &DAG) const 
{
  EVT PtrVT = getPointerTy(DAG.getDataLayout());

  SDValue Result;
  if (CP->isMachineConstantPoolEntry())
    Result = DAG.getTargetConstantPool(CP->getMachineCPVal(), PtrVT,
				       CP->getAlignment());
  else
    Result = DAG.getTargetConstantPool(CP->getConstVal(), PtrVT,
				       CP->getAlignment(), CP->getOffset());

//  Reloc::Model RM = DAG.getTarget().getRelocationModel();

  return getAddrPIC(Result, DAG);
}

SDValue XtensaTargetLowering::lowerVASTART(SDValue Op,
                                            SelectionDAG &DAG) const 
{
  MachineFunction &MF = DAG.getMachineFunction();
  XtensaFunctionInfo *FuncInfo =
    MF.getInfo<XtensaFunctionInfo>();
  EVT PtrVT = getPointerTy(DAG.getDataLayout());

  SDValue Chain   = Op.getOperand(0);
  SDValue Addr    = Op.getOperand(1);
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  SDLoc DL(Op);
  SDValue FI      = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(),
                                 PtrVT);

  // vastart just stores the address of the VarArgsFrameIndex slot into the
  // memory location argument.
  return DAG.getStore(Chain, DL, FI, Addr,
                      MachinePointerInfo(SV), false, false, 0);
}

SDValue XtensaTargetLowering::lowerVAARG(SDValue Op, SelectionDAG &DAG) const
{
  SDNode *Node = Op.getNode();
  EVT VT = Node->getValueType(0);
  SDValue InChain = Node->getOperand(0);
  SDValue VAListPtr = Node->getOperand(1);
  EVT PtrVT = VAListPtr.getValueType();
  const Value *SV = cast<SrcValueSDNode>(Node->getOperand(2))->getValue();
  SDLoc DL(Node);
  SDValue VAList = DAG.getLoad(PtrVT, DL, InChain, VAListPtr,
                               MachinePointerInfo(SV), false, false, false, 0);
  // Increment the pointer, VAList, to the next vaarg.
  SDValue NextPtr = DAG.getNode(ISD::ADD, DL, PtrVT, VAList,
                                DAG.getIntPtrConstant(VT.getSizeInBits()/8, DL));
  // Store the incremented VAList to the legalized pointer.
  InChain = DAG.getStore(VAList.getValue(1), DL, NextPtr,
                         VAListPtr, MachinePointerInfo(SV), false, false, 0);
  // Load the actual argument out of the pointer VAList.
  // We can't count on greater alignment than the word size.
  return DAG.getLoad(VT, DL, InChain, VAList, MachinePointerInfo(),
                     false, false, false,
                     std::min(PtrVT.getSizeInBits(), VT.getSizeInBits())/8);
}

SDValue XtensaTargetLowering::lowerATOMIC_FENCE(SDValue Op, SelectionDAG &DAG) 
  const 
{
  // TODO 
  // dummy return
  return lowerSTACKSAVE(Op, DAG);
}

SDValue XtensaTargetLowering::lowerSTACKSAVE(SDValue Op,
                                              SelectionDAG &DAG) const 
{
  MachineFunction &MF = DAG.getMachineFunction();
  MF.getInfo<XtensaFunctionInfo>()->setManipulatesSP(true);
  unsigned sp = Xtensa::sp;
  return DAG.getCopyFromReg(Op.getOperand(0), SDLoc(Op), sp, Op.getValueType());
}

SDValue XtensaTargetLowering::lowerSTACKRESTORE(SDValue Op,
                                                 SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MF.getInfo<XtensaFunctionInfo>()->setManipulatesSP(true);
  unsigned sp = Xtensa::sp;
  return DAG.getCopyToReg(Op.getOperand(0), SDLoc(Op), sp, Op.getOperand(1));
}

SDValue XtensaTargetLowering::
lowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const 
{
  // check the depth
  assert((cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue() == 0) &&
         "Frame address can only be determined for current frame.");

  MachineFrameInfo *MFI = DAG.getMachineFunction().getFrameInfo();
  MFI->setFrameAddressIsTaken(true);
  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  SDValue FrameAddr = DAG.getCopyFromReg(DAG.getEntryNode(), DL, Xtensa::a15, VT);
  return FrameAddr;
}

SDValue XtensaTargetLowering::LowerOperation(SDValue Op,
                                              SelectionDAG &DAG) const 
{
  switch (Op.getOpcode()) 
  {
    case ISD::Constant:
      return lowerImmediate(Op, DAG);
    case ISD::RETURNADDR:
      return lowerRETURNADDR(Op, DAG);
    case ISD::SELECT_CC:
      return lowerSELECT_CC(Op, DAG);
    case ISD::GlobalAddress:
      return lowerGlobalAddress(Op, DAG);
    case ISD::GlobalTLSAddress:
      return lowerGlobalTLSAddress(cast<GlobalAddressSDNode>(Op), DAG);
    case ISD::BlockAddress:
      return lowerBlockAddress(cast<BlockAddressSDNode>(Op), DAG);
    case ISD::JumpTable:
      return lowerJumpTable(cast<JumpTableSDNode>(Op), DAG);
    case ISD::ConstantPool:
      return lowerConstantPool(cast<ConstantPoolSDNode>(Op), DAG);
    case ISD::VASTART:
      return lowerVASTART(Op, DAG);
    case ISD::VAARG:
      return lowerVAARG(Op, DAG);
    case ISD::ATOMIC_FENCE:
      return lowerATOMIC_FENCE(Op, DAG);
    case ISD::STACKSAVE:
      return lowerSTACKSAVE(Op, DAG);
    case ISD::STACKRESTORE:
      return lowerSTACKRESTORE(Op, DAG);
    case ISD::FRAMEADDR:
      return lowerFRAMEADDR(Op, DAG);
    default:
      //printf("--- Node %s\n", Op.getNode()->getOperationName().c_str());
      llvm_unreachable("Unexpected node to lower");
  }
}

const char *XtensaTargetLowering::getTargetNodeName(unsigned Opcode) const 
{
#define OPCODE(NAME) case XtensaISD::NAME: return "XtensaISD::" #NAME
  switch (Opcode) 
  {
    OPCODE(RET_FLAG);
    OPCODE(CALL);
    OPCODE(PCREL_WRAPPER);
    OPCODE(FENCE);
    OPCODE(SELECT);
    OPCODE(SELECT_CC);
  }
  return NULL;
#undef OPCODE
}

//===----------------------------------------------------------------------===//
// Custom insertion
//===----------------------------------------------------------------------===//


// Call pseduo ops for ABI compliant calls (output is always ra)
MachineBasicBlock *XtensaTargetLowering::
emitCALL(MachineInstr *MI, MachineBasicBlock *BB) const 
{
  const TargetInstrInfo *TII = BB->getParent()->getSubtarget().getInstrInfo();
  DebugLoc DL = MI->getDebugLoc();
/* TODO
  unsigned jump;
  unsigned RA;
  switch(MI->getOpcode()) 
  {
    
    case Xtensa::CALL0:
      jump = Xtensa::CALL0; RA = Xtensa::ra; break;
    case Xtensa::CALLX0:
      jump = Xtensa::CALLX0; RA = Xtensa::ra; break;
    default:
      llvm_unreachable("Unexpected call instr type to insert");
  }
  
  MachineInstrBuilder jumpMI = BuildMI(*BB, MI, DL, TII->get(jump), RA);

  //copy over other operands
  for(unsigned i = 0; i < MI->getNumOperands(); i++){
    jumpMI.addOperand(MI->getOperand(i));
  }
  MI->eraseFromParent();
 */ 
  return BB;
}

MachineBasicBlock *XtensaTargetLowering::
emitSelectCC(MachineInstr *MI, MachineBasicBlock *BB) const 
{
  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
  DebugLoc DL = MI->getDebugLoc();  
  
  // To "insert" a SELECT_CC instruction, we actually have to insert the diamond
  // control-flow pattern.  The incoming instruction knows the destination vreg
  // to set, the condition code register to branch on, the true/false values to
  // select between, and a branch opcode to use.
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator It = ++BB->getIterator();

  //  thisMBB:
  //  ...
  //   TrueVal = ...
  //   cmpTY ccX, r1, r2
  //   bCC copy1MBB
  //   fallthrough --> copy0MBB
  MachineBasicBlock *thisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *sinkMBB = F->CreateMachineBasicBlock(LLVM_BB);
  F->insert(It, copy0MBB);
  F->insert(It, sinkMBB);

  // Transfer the remainder of BB and its successor edges to sinkMBB.
  sinkMBB->splice(sinkMBB->begin(), BB,
                  std::next(MachineBasicBlock::iterator(MI)), BB->end());
  sinkMBB->transferSuccessorsAndUpdatePHIs(BB);

  // Next, add the true and fallthrough blocks as its successors.
  BB->addSuccessor(copy0MBB);
  BB->addSuccessor(sinkMBB);

  BuildMI(BB, DL, TII.get(Xtensa::BNE))
    .addReg(MI->getOperand(1).getReg()).addMBB(sinkMBB);

  //  copy0MBB:
  //   %FalseValue = ...
  //   # fallthrough to sinkMBB
  BB = copy0MBB;

  // Update machine-CFG edges
  BB->addSuccessor(sinkMBB);

  //  sinkMBB:
  //   %Result = phi [ %FalseValue, copy0MBB ], [ %TrueValue, thisMBB ]
  //  ...
  BB = sinkMBB;
  BuildMI(*BB, BB->begin(), DL,
          TII.get(Xtensa::PHI), MI->getOperand(0).getReg())
    .addReg(MI->getOperand(3).getReg()).addMBB(copy0MBB)
    .addReg(MI->getOperand(2).getReg()).addMBB(thisMBB);

  MI->eraseFromParent();   // The pseudo instruction is gone now.
  return BB;
}

MachineBasicBlock *XtensaTargetLowering::
EmitInstrWithCustomInserter(MachineInstr *MI, MachineBasicBlock *MBB) const 
{
  printf("--- Custom insert %d\n", MI->getOpcode());
  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
  DebugLoc DL = MI->getDebugLoc();    

  switch (MI->getOpcode()) 
  {
  case Xtensa::LA:
     {
       MachineOperand &dst = MI->getOperand(0);
       MachineOperand &addr = MI->getOperand(1);
       MI->dump();
       MachineConstantPool* CP = MBB->getParent();

//        unsigned Idx = CP->getConstantPoolIndex(C, 4);

    //     return DAG.getLoad(getPointerTy(DAG.getDataLayout()), DL,
    //       DAG.getEntryNode(), CP, MachinePointerInfo(), false,
    //       false, false, 0);      

        return MBB;
      }
    case Xtensa::SELECT_CC:
//    case Xtensa::FSELECT_CC_F:
//    case Xtensa::FSELECT_CC_D:
        return emitSelectCC(MI, MBB);
    /*    
    case Xtensa::CALL:
    case Xtensa::CALLREG:
        return emitCALL(MI, MBB);
     */ 
    default:
      llvm_unreachable("Unexpected instr type to insert");
  }
}
