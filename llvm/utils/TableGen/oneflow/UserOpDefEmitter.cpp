//===- UserOpDefEmitter.cpp - Oneflow TableGen backend          -*- C++ -*-===//
//
// This Tablegen backend emits user op definition code for Oneflow-Inc/oneflow
//
//===----------------------------------------------------------------------===//

#include "inja/inja.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#define DEBUG_TYPE "user-op-def-emitter"

using namespace llvm;
using namespace nlohmann;
using namespace inja;

namespace oneflow {

cl::opt<std::string> SourceIncludeFilename{
  "of-include", cl::desc("header filename to include in source file (used in gen-of-user-op-def)"), 
  cl::value_desc("include filename"), cl::init("")
};

cl::opt<std::string> DumpJson{
  "of-dump-json", cl::desc("dump tablegen code to json in provided file (used in gen-of-user-op-def)"), 
  cl::value_desc("filename"), cl::init("")
};

const std::string& outputFilename() {
  auto *Option = static_cast<cl::opt<std::string>*>(
    cl::getRegisteredOptions().lookup("o"));

  return Option->getValue();
}

enum class UserOpDefTarget {
  Header = 1,
  Source,
};

template <UserOpDefTarget Target>
class UserOpDefEmitter {
private:
  RecordKeeper &Records;
  Environment Env;
  Template Temp;

  static const nonstd::string_view Code;

public:
  UserOpDefEmitter(RecordKeeper &RK) : Records(RK) {
    Env.add_callback("quoted", 1, [](Arguments& Args){
      auto Str = Args.at(0)->get<std::string>();
      std::ostringstream OS;
      OS << std::quoted(Str);
      return OS.str();
    });

    Env.add_callback("to_header", 1, [](Arguments& Args) {
      auto Str = Args.at(0)->get<std::string>();
      auto DotPos = Str.find_last_of('.');
      if (DotPos != std::string::npos) {
        Str.replace(DotPos, Str.size() - DotPos, ".h");
      }

      // assume that the source and header file is in the same directory
      auto SlashPos = Str.find_last_of('/');
      if (SlashPos != std::string::npos) {
        Str.replace(0, SlashPos + 1, "");
      }
      return Str;
    });

    Temp = Env.parse(Code);
  }

  void run(raw_ostream &OS) {
  emitSourceFileHeader("Oneflow User Op Definitions", OS);

  json Ops;

  for (const auto& R : Records.getAllDerivedDefinitions("Op")) {
    auto Name = R->getValueAsString("OpName");
    if (Name == "") {
      PrintFatalError(R, "Name of `Op` definitions cannot be omitted");
    }

    json Op {
      {"name", Name},
      {"desc", R->getValueAsString("Description")},
    };

    for (const auto* P : *R->getValueAsListInit("Params")) {
      const auto* D = dyn_cast<DagInit>(P);
      if (!D) {
        PrintFatalError(R, "`Params` in `Op` should be typed as `list<dag>`");
      }

      auto Type = D->getOperatorAsDef(R->getLoc())->getValueAsString("ParamNode");
      for (size_t I = 0; I < D->getNumArgs(); ++I) {
        const auto* A = dyn_cast<DefInit>(D->getArg(I));
        StringRef AS;
        if (!A) {
          if (Type == "in" || Type == "out") {
            AS = Records.getDef("Tensor")->getValueAsString("TensorType");
          } else {
            PrintFatalError(R, "Arguments in `Params` should be type definitions");
          }
        } else {
          if (Type == "in" || Type == "out") {
            AS = A->getDef()->getValueAsString("TensorType");
          } else {
            AS = A->getDef()->getValueAsString("AttributeType");
          }
        }
        
        auto NS = D->getArgName(I)->getAsUnquotedString();
        if (NS == "") {
          PrintFatalError(R, "Name in `Params` cannot be omitted");
        }
        if (Type == "in" || Type == "out" || Type == "attr") {
          Op[Type.data()].push_back({{"name", NS}, {"type", AS}});
        } else {
          PrintFatalError(R, "Operator in `Params` must be one of `In`, `Out` or `Attr`");
        }
      }
    }

    for (auto E : R->getValues()) {
      if (E.getPrintType() == "FunctionMeta") {
        const auto *F = dyn_cast<DefInit>(E.getValue());
        if(!F) {
          PrintFatalError(R, "FunctionMeta value should be definition");
        }

        const auto* FD = F->getDef();
        auto Type = FD->getValueAsString("FunctionMeta");
        json Func {{"type", Type}, {"name", E.getNameInitAsString()}};

        if (Type == "extern") {
          Func["value"] = FD->getValueAsString("FunctionName");
        } else if (Type == "inlined") {
          Func["value"] = FD->getValueAsString("FunctionCode");
        }

        Op["func"].push_back(Func);
      }
    }

    Ops[R->getName().data()] = Op;
  }

  if (Ops.empty()) {
    PrintWarning("No `Op` in this file");
  }

  auto Filename = outputFilename();
  Filename = Filename != "-"? Filename : "";

  json Data {
    {"filename", Filename},
    {"ops", Ops},
  };

  if (Target == UserOpDefTarget::Source) {
    Data["include"] = SourceIncludeFilename;
  }

  if (!DumpJson.empty()) {
    std::ofstream File(DumpJson);
    File << Data.dump();
  }

  OS << Env.render(Temp, Data);
}
};

template <>
const nonstd::string_view UserOpDefEmitter<UserOpDefTarget::Header>::Code {
  #include "UserOpDefTemplateHeader.inc"
};

template <>
const nonstd::string_view UserOpDefEmitter<UserOpDefTarget::Source>::Code {
  #include "UserOpDefTemplateSource.inc"
};

} // namespace oneflow

namespace llvm {

void EmitOneflowUserOpDefHeader(RecordKeeper &RK, raw_ostream &OS) {
  // Instantiate the emitter class and invoke run().
  oneflow::UserOpDefEmitter<oneflow::UserOpDefTarget::Header>(RK).run(OS);
}

void EmitOneflowUserOpDefSource(RecordKeeper &RK, raw_ostream &OS) {
  // Instantiate the emitter class and invoke run().
  oneflow::UserOpDefEmitter<oneflow::UserOpDefTarget::Source>(RK).run(OS);
}

} // namespace llvm
