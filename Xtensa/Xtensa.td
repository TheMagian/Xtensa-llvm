//===-----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===-----------------------------------------------------------------------===//
// --------------------------------------------------------------------- //
// Andrei Safronov, e-mail: andrei@kudeyar.com
// Alexei Shistko, e-mail: alexey@kudeyar.com
// --------------------------------------------------------------------- //
 
//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Subtarget Features. 
//===----------------------------------------------------------------------===//
def FeatureF
 : SubtargetFeature<"f", "HasF", "true",
                    "Enable Xtensa FP instructions">;

//===----------------------------------------------------------------------===//
// Xtensa supported processors.
//===----------------------------------------------------------------------===//
class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic",         []>;

//===----------------------------------------------------------------------===//
// Register File Description
//===----------------------------------------------------------------------===//

include "XtensaRegisterInfo.td"
 
//===----------------------------------------------------------------------===//
// Calling Convention Description
//===----------------------------------------------------------------------===//

include "XtensaCallingConv.td"

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

include "XtensaInstrInfo.td"

def XtensaInstrInfo : InstrInfo;

def XtensaInstPrinter : AsmWriter 
{
  string AsmWriterClassName  = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Target Declaration
//===----------------------------------------------------------------------===//

def Xtensa : Target 
{
  let InstructionSet = XtensaInstrInfo;
  let AssemblyWriters = [XtensaInstPrinter];
}

