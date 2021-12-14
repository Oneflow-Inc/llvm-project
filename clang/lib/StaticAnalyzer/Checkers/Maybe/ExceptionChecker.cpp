// TODO: add header

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

using namespace clang;
using namespace ento;

namespace {
class MaybeExceptionChecker : public Checker<check::PreStmt<BinaryOperator>> {
public:
  void checkPreStmt(const BinaryOperator *B, CheckerContext &C) const {}
};
} // end anonymous namespace


void ento::registerExceptionChecker(CheckerManager &mgr) {
  mgr.registerChecker<MaybeExceptionChecker>();
}

bool ento::shouldRegisterExceptionChecker(const CheckerManager &mgr) {
  return false;
}
