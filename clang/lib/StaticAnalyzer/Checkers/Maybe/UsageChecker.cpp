// TODO: add header

#include "clang/Basic/Diagnostic.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporterVisitors.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/MemRegion.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SymExpr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

#include "Utils.h"

using namespace clang;
using namespace ento;

namespace maybe {

struct MaybeState {
  enum State {
    Invalid = 0,
    InitFromCall,
    Checked,
  } state;

  SVal checkVal;

  MaybeState() : state(Invalid) {}
  MaybeState(State s) : state(s) {}
  MaybeState(SVal c) : state(Checked), checkVal(c) {}

  operator State() const { return state; }

  bool operator==(const MaybeState &other) const {
    return state == other.state;
  }

  void Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(state);
    checkVal.Profile(ID);
  }
};

} // namespace maybe

REGISTER_MAP_WITH_PROGRAMSTATE(MaybeStateMap, SymbolRef, maybe::MaybeState);

namespace maybe {

static llvm::StringRef MaybeTypeName = "oneflow::Maybe";
static llvm::StringRef MaybeCheckMethod = "IsOk";
static llvm::StringRef MaybeGetData =
    "Data_YouAreNotAllowedToCallThisFuncOutsideThisFile";
static llvm::StringRef MaybeGetErr = "error";

class MaybeUsageChecker
    : public Checker<check::PostCall, check::Bind, check::BeginFunction> {
private:
  BuiltinBug BugType{this, "Wrong usage of Maybe"};
  std::unique_ptr<const FunctionDecl *> TopFunc =
      std::make_unique<const FunctionDecl *>(nullptr);

  llvm::raw_ostream &log(CheckerContext &C,
                         llvm::raw_ostream &os = DebugOuts()) const {
    return os << "[" << (*TopFunc)->getQualifiedNameAsString() << ", "
              << C.getCurrentAnalysisDeclContext()
                     ->getDecl()
                     ->getAsFunction()
                     ->getQualifiedNameAsString()
              << "] ";
  }

  llvm::raw_ostream &log(const CallEvent &B, CheckerContext &C,
                         llvm::raw_ostream &os = DebugOuts()) const {
    return log(C, os)
           << B.getDecl()->getAsFunction()->getQualifiedNameAsString() << ": ";
  }

public:
  void checkBeginFunction(CheckerContext &C) const {
    if (C.inTopFrame()) {
      *TopFunc = C.getLocationContext()->getDecl()->getAsFunction();
    }
  }

  void checkPostCall(const CallEvent &B, CheckerContext &C) const {
    const auto *D = B.getDecl();
    if (!D)
      return;

    auto State = C.getState();
    const auto &ResultType =
        CallEvent::getDeclaredResultType(D).getNonReferenceType();

    if (maybe::MatchNameOfClassTemplate(ResultType, MaybeTypeName)) {
      auto RetVal = B.getReturnValue();

      const auto *Val = RetVal.getAsSymbol();
      if (!State->get<MaybeStateMap>(Val)) {
        State = State->set<MaybeStateMap>(Val, MaybeState::InitFromCall);
        log(B, C) << "inited\n";
      }
    }

    if (const auto *MethodCall = llvm::dyn_cast<CXXInstanceCall>(&B)) {
      auto This = MethodCall->getCXXThisVal();
      if (MatchMethodNameInClassTemplate(MethodCall->getDecl(), MaybeTypeName,
                                         MaybeCheckMethod)) {
        if (const auto *MS = State->get<MaybeStateMap>(This.getAsSymbol())) {
          if (MS->state == MaybeState::InitFromCall) {
            State = State->set<MaybeStateMap>(This.getAsSymbol(),
                                              MethodCall->getReturnValue());
            log(B, C) << "checked\n";
          }
        }
      }

      else if (MatchMethodNameInClassTemplate(MethodCall->getDecl(),
                                              MaybeTypeName, MaybeGetData)) {
        log(B, C) << "get data\n";
        if (const auto *MS = State->get<MaybeStateMap>(This.getAsSymbol())) {
          if (MS->state == MaybeState::InitFromCall) {
            C.emitReport(std::make_unique<PathSensitiveBugReport>(
                BugType, "maybe not check", C.generateErrorNode()));
          } else {
            Optional<DefinedOrUnknownSVal> DVal =
                MS->checkVal.getAs<DefinedOrUnknownSVal>();
            if (DVal) {
              if (auto PS = State->assume(*DVal, false)) {
                C.emitReport(std::make_unique<PathSensitiveBugReport>(
                    BugType, "get data from a NOT OK branch",
                    C.generateErrorNode(PS)));
              }
            }
          }
        }
      }

      else if (MatchMethodNameInClassTemplate(MethodCall->getDecl(),
                                              MaybeTypeName, MaybeGetErr)) {
        log(B, C) << "get error\n";
        if (const auto *MS = State->get<MaybeStateMap>(This.getAsSymbol())) {
          if (MS->state == MaybeState::InitFromCall) {
            C.emitReport(std::make_unique<PathSensitiveBugReport>(
                BugType, "maybe not check", C.generateErrorNode()));
          } else {
            Optional<DefinedOrUnknownSVal> DVal =
                MS->checkVal.getAs<DefinedOrUnknownSVal>();
            if (DVal) {
              if (auto PS = State->assume(*DVal, true)) {
                C.emitReport(std::make_unique<PathSensitiveBugReport>(
                    BugType, "get error from a OK branch",
                    C.generateErrorNode(PS)));
              }
            }
          }
        }
      }

      else if (const auto *DtorCall = llvm::dyn_cast<CXXDestructorCall>(&B)) {
        auto ThisType = This.getType(C.getASTContext())->getPointeeType();
        if (maybe::MatchNameOfClassTemplate(ThisType, MaybeTypeName)) {
          if (const auto *MS = State->get<MaybeStateMap>(This.getAsSymbol())) {
            if (MS->state == MaybeState::InitFromCall) {
              C.emitReport(std::make_unique<PathSensitiveBugReport>(
                  BugType, "maybe not check", C.generateErrorNode()));
            }
            log(B, C) << "destructed\n";
          }
        }
      }
    }

    if (State != C.getState()) {
      C.addTransition(State);
    }
  }

  void checkBind(SVal L, SVal V, const Stmt *S, CheckerContext &C) const {
    auto State = C.getState();

    if (const auto *MS = State->get<MaybeStateMap>(V.getAsSymbol())) {
      State = State->set<MaybeStateMap>(L.getAsSymbol(), *MS);
    }

    if (State != C.getState()) {
      C.addTransition(State);
    }
  }
};

} // namespace maybe

void ento::registerUsageChecker(CheckerManager &mgr) {
  mgr.registerChecker<maybe::MaybeUsageChecker>();
}

bool ento::shouldRegisterUsageChecker(const CheckerManager &mgr) {
  return true;
}
