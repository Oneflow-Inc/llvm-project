// TODO: add header

#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporterVisitors.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExplodedGraph.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/MemRegion.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/Store.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SymExpr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <stack>

#include "Utils.h"

using namespace clang;
using namespace ento;

namespace maybe {

struct MaybeState {
  enum State {
    Invalid = 0,
    Inited,
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

struct ComparableSVal : public SVal {
  ComparableSVal(SVal V) : SVal(V) {}

  bool operator<(ComparableSVal V) const {
    if (Kind == V.Kind) {
      return Data < V.Data;
    }

    return Kind < V.Kind;
  }
};

} // namespace maybe

REGISTER_MAP_WITH_PROGRAMSTATE(MaybeStateMap, MemRegion const *,
                               maybe::MaybeState);

namespace maybe {

static llvm::StringRef MaybeTypeName = "oneflow::Maybe";
static llvm::StringRef EitherPtrTypeName = "oneflow::EitherPtr";
static llvm::StringRef SharedOrScalarTypeName = "oneflow::SharedOrScalar";
static llvm::StringRef MaybeCheckMethod = "IsOk";
static llvm::StringRef MaybeGetData =
    "Data_YouAreNotAllowedToCallThisFuncOutsideThisFile";
static llvm::StringRef MaybeGetErr = "error";
static llvm::StringRef GlogFatal = "google::LogMessageFatal::LogMessageFatal";

class MaybeUsageChecker
    : public Checker<check::PostCall, check::PreCall, check::Bind,
                     check::BeginFunction, check::PreStmt<ReturnStmt>> {

public:
  static const MemRegion *region(SVal v, bool W = false) {
    if (auto LCV = v.getAs<nonloc::LazyCompoundVal>()) {
      return LCV->getRegion();
    }

    const auto *R = v.getAsRegion();
    if (!R && W) {
      DebugOuts() << "[WARING] " << v
                  << ": failed to retrieve region, keep zero\n";
    }
    return R;
  }

private:
  BuiltinBug BugType{this, "Wrong usage of Maybe"};

  struct PrintSVal : SVal {
    PrintSVal(SVal V) : SVal(V) {}

    friend auto &operator<<(llvm::raw_ostream &OS, const PrintSVal &V) {
      return OS << (const SVal &)V << "|"
                << (const void *)region((const SVal &)V);
    }
  };

#define FULLSTACKLOG

  llvm::raw_ostream &log(CheckerContext &C,
                         llvm::raw_ostream &os = DebugOuts()) const {
    const auto *LC = C.getLocationContext();
#ifdef FULLSTACKLOG
    std::stack<const LocationContext *> LS;
#endif
    while (LC->getParent()) {
#ifdef FULLSTACKLOG
      LS.push(LC);
#endif
      LC = LC->getParent();
    }

#ifdef FULLSTACKLOG
    LS.push(LC);
#endif

    os << "[";
#ifdef FULLSTACKLOG
    bool F = true;
    while (!LS.empty()) {
      if (F) {
        F = false;
      } else {
        os << "=>";
      }
      os << LS.top()
                ->getAnalysisDeclContext()
                ->getDecl()
                ->getAsFunction()
                ->getQualifiedNameAsString();
      LS.pop();
    }
#else
    if (LC != C.getLocationContext()) {
      os << LC->getAnalysisDeclContext()
                ->getDecl()
                ->getAsFunction()
                ->getQualifiedNameAsString()
         << "..";
    }

    os << C.getCurrentAnalysisDeclContext()
              ->getDecl()
              ->getAsFunction()
              ->getQualifiedNameAsString();
#endif
    return os << "] ";
  }

  llvm::raw_ostream &log(const CallEvent &B, CheckerContext &C,
                         llvm::raw_ostream &os = DebugOuts()) const {
    return log(C, os)
           << B.getDecl()->getAsFunction()->getQualifiedNameAsString() << ": ";
  }

  llvm::raw_ostream &log(SVal S, const CallEvent &B, CheckerContext &C,
                         llvm::raw_ostream &os = DebugOuts()) const {
    return log(B, C, os) << PrintSVal(S) << " ";
  }

  static auto getPointeeTypeOrSelf(const QualType &T) {
    auto PT = T->getPointeeType();
    if (PT.isNull()) {
      return T;
    }

    return PT;
  }

  static QualType getResultType(const Decl *D) {
    return getPointeeTypeOrSelf(CallEvent::getDeclaredResultType(D));
  }

  static auto getReturnValue(const CallEvent *B) {
#ifdef GET_RET_VAL_UNDER_CONSTRUCT
    if (auto O = B->getReturnValueUnderConstruction()) {
      return *O;
    }
#endif

    return B->getReturnValue();
  }

  static auto get(ProgramStateRef State, SVal V, bool W = false) {
    return State->get<MaybeStateMap>(region(V, W));
  }

  static auto set(ProgramStateRef State, SVal V, const MaybeState &S) {
    return State->set<MaybeStateMap>(region(V, true), S);
  }

  auto emitReport(CheckerContext &C, StringRef Desc,
                  const ExplodedNode *EN) const {
    return C.emitReport(
        std::make_unique<PathSensitiveBugReport>(BugType, Desc, EN));
  }

  static auto IsConstCharPtr(QualType T) {
    if (!T->isPointerType())
      return false;

    auto P = T->getPointeeType();
    return P->isCharType() && P.isConstQualified();
  };

public:
  void checkBeginFunction(CheckerContext &C) const {
    if (C.inTopFrame()) {
      log(C) << "start\n";
    }
  }

  void checkPreStmt(const ReturnStmt *R, CheckerContext &C) const {
    auto RV = C.getSVal(R);
    auto RT = RV.getType(C.getASTContext());

    if (RT.isNull()) {
      return;
    }

    if (MatchNameOfClassTemplate(getPointeeTypeOrSelf(RT), MaybeTypeName)) {
      log(C) << "return " << PrintSVal(RV) << "\n";
    }
  }

  void checkPreCall(const CallEvent &B, CheckerContext &C) const {
    for (size_t i = 0; i < B.getNumArgs(); ++i) {
      auto ArgVal = B.getArgSVal(i);
      auto ArgType = ArgVal.getType(C.getASTContext());

      if (!ArgType.isNull() &&
          MatchNameOfClassTemplate(getPointeeTypeOrSelf(ArgType),
                                   MaybeTypeName)) {
        log(B, C) << "call with arg " << PrintSVal(ArgVal) << "\n";
      }
    }
  }

  void checkPostCall(const CallEvent &B, CheckerContext &C) const {
    const auto *D = B.getDecl();
    if (!D)
      return;

    auto State = C.getState();
    auto RetVal = getReturnValue(&B);
    const auto &ResultType = getResultType(D);
    const auto *CurrFunc =
        C.getCurrentAnalysisDeclContext()->getDecl()->getAsFunction();

    if (maybe::MatchNameOfClassTemplate(ResultType, MaybeTypeName)) {

      if (!get(State, RetVal, true)) {
        State = set(State, RetVal, MaybeState::Inited);
        log(RetVal, B, C) << "inited from call\n";
      }
    }

    if (const auto *CtorCall = dyn_cast<CXXConstructorCall>(&B)) {
      auto RT = RetVal.getType(C.getASTContext());

      if (MatchNameOfClassTemplate(RT, MaybeTypeName)) {
        bool IsCopyCtor = false;

        if (CtorCall->getNumArgs() == 1) {
          auto ArgVal = CtorCall->getArgSVal(0);
          auto ArgType =
              getPointeeTypeOrSelf(ArgVal.getType(C.getASTContext()));

          if (MatchNameOfClassTemplate(ArgType, MaybeTypeName)) {
            IsCopyCtor = true;

            log(RetVal, B, C)
                << "copy constructed from " << PrintSVal(ArgVal) << "\n";
            if (const auto *MS = get(State, ArgVal)) {
              State = set(State, RetVal, *MS);
            }
          }
        }
        if (!IsCopyCtor) {
          log(RetVal, B, C) << "constructed\n";
        }
      }
    }

    if (D->getAsFunction()->getQualifiedNameAsString() == GlogFatal) {
      bool IsInMaybeContext = false;

      for (const auto *LC = C.getLocationContext(); LC; LC = LC->getParent()) {
        const auto *Func =
            LC->getAnalysisDeclContext()->getDecl()->getAsFunction();
        auto RetType = Func->getReturnType();

        if (MatchNameOfClassTemplate(getPointeeTypeOrSelf(RetType),
                                     MaybeTypeName)) {
          if (const auto *MD = dyn_cast<CXXMethodDecl>(Func)) {
            if (MD->getOverloadedOperator() == OO_Call &&
                !MD->getParent()->getIdentifier() && MD->getNumParams() == 1 &&
                IsConstCharPtr(MD->getParamDecl(0)->getType())) {
              // ignore CHECK_JUST
              continue;
            }
          }

          IsInMaybeContext = true;
          break;
        }
      }

      if (IsInMaybeContext) {
        bool Filter = false;

        if (auto O = GetMethodNameInClassTemplate(CurrFunc)) {
          if (O->first == MaybeTypeName || O->first == SharedOrScalarTypeName ||
              O->first == EitherPtrTypeName) {
            Filter = true;
          }
        }

        if (!Filter) {
          emitReport(C, "abort in maybe context", C.generateErrorNode());
          log(B, C) << "abort\n";
        }
      }
    }

    if (const auto *MethodCall = llvm::dyn_cast<CXXInstanceCall>(&B)) {
      auto This = MethodCall->getCXXThisVal();
      if (MatchMethodNameInClassTemplate(MethodCall->getDecl(), MaybeTypeName,
                                         MaybeCheckMethod)) {
        if (get(State, This)) {
          State = set(State, This, getReturnValue(MethodCall));

          log(This, B, C) << "checked\n";
        }
      }

      else if (MatchMethodNameInClassTemplate(MethodCall->getDecl(),
                                              MaybeTypeName, MaybeGetData)) {
        if (const auto *MS = get(State, This)) {
          log(This, B, C) << "get data\n";

          if (MS->state == MaybeState::Inited) {
            emitReport(C, "maybe not check", C.generateErrorNode());
          } else {
            if (auto DVal = MS->checkVal.getAs<DefinedSVal>()) {
              if (auto PS = State->assume(*DVal, false)) {
                emitReport(C, "get data from a NOT OK branch",
                           C.generateErrorNode(PS));

                log(This, B, C) << "assume NOT OK\n";
              }

              if (State->assume(*DVal, true)) {
                log(This, B, C) << "assume OK\n";
              }
            }
          }
        }
      }

      else if (MatchMethodNameInClassTemplate(MethodCall->getDecl(),
                                              MaybeTypeName, MaybeGetErr)) {
        if (const auto *MS = get(State, This)) {
          log(This, B, C) << "get error\n";

          if (MS->state == MaybeState::Inited) {
            emitReport(C, "maybe not check", C.generateErrorNode());
          } else {
            if (auto DVal = MS->checkVal.getAs<DefinedSVal>()) {
              if (auto PS = State->assume(*DVal, true)) {
                emitReport(C, "get error from a OK branch",
                           C.generateErrorNode(PS));

                log(This, B, C) << "assume OK\n";
              }

              if (State->assume(*DVal, false)) {
                log(This, B, C) << "assume NOT OK\n";
              }
            }
          }
        }
      }

      else if (llvm::isa<CXXDestructorCall>(&B)) {
        auto ThisType = This.getType(C.getASTContext())->getPointeeType();

        if (maybe::MatchNameOfClassTemplate(ThisType, MaybeTypeName)) {
          if (const auto *MS = get(State, This)) {
            if (MS->state == MaybeState::Inited) {
              emitReport(C, "maybe not check", C.generateErrorNode());
            }
            log(This, B, C) << "destructed\n";
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
    auto LType = getPointeeTypeOrSelf(L.getType(C.getASTContext()));

    if (!MatchNameOfClassTemplate(LType, MaybeTypeName)) {
      return;
    }

    if (const auto *MS = get(State, V)) {
      State = set(State, L, *MS);
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
