//===--- GlogFatalCheck.cpp - clang-tidy ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "GlogFatalCheck.h"
#include "CommonMatcher.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/SmallVector.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace maybe {

void GlogFatalCheck::registerMatchers(MatchFinder *Finder) {

  auto MatchedFatal =
      cxxTemporaryObjectExpr(
          hasTypeLoc(loc(asString("google::LogMessageFatal"))),
          forCallable(functionDecl(returns(matchesNameForType("^Maybe<.*>$")))))
          .bind("glog_fatal");
  // CHECK_JUST places its LOG(FATAL) in a lambda function, the `forCallable`
  // points to that lambda function and makes the matching failed. So write a
  // complicated matcher for these cases.
  auto MatchedFatalInCheckJust =
      callExpr(
          callee(
              functionDecl(
                  hasDescendant(
                      cxxTemporaryObjectExpr(
                          hasTypeLoc(loc(asString("google::LogMessageFatal"))),
                          forCallable(functionDecl().bind("nearest_callable")))
                          .bind("glog_fatal")))
                  .bind("called_callable")),
          forCallable(functionDecl(returns(matchesNameForType("^Maybe<.*>$")))))
          .bind("call_expr");
  Finder->addMatcher(expr(anyOf(MatchedFatal, MatchedFatalInCheckJust)), this);
}

void GlogFatalCheck::check(const MatchFinder::MatchResult &Result) {
  bool MatchCallExpr = true;
  const Expr *Matched = Result.Nodes.getNodeAs<CallExpr>("call_expr");
  if (Matched) {
    const auto *NearestCallable =
        Result.Nodes.getNodeAs<FunctionDecl>("nearest_callable");
    const auto *CalledCallable =
        Result.Nodes.getNodeAs<FunctionDecl>("called_callable");
    // skip LOG(FATAL) in inner functions, for example, this case
    // should be skipped:
    // Maybe<void> f1() {
    //   auto f2 = []() -> void {
    //     CHECK(true);
    //   }
    // }
    // 
    if (CalledCallable->getID() != NearestCallable->getID()) {
      return;
    }
  } else {
    MatchCallExpr = false;
    Matched = Result.Nodes.getNodeAs<CXXTemporaryObjectExpr>("glog_fatal");
  }
  if (!Matched) {
    return;
  }
  auto ExpansionRange =
      Result.SourceManager->getExpansionRange(Matched->getBeginLoc());
  auto ExpansionBeginLoc = ExpansionRange.getBegin();
  auto SourceTxt = Lexer::getSourceText(ExpansionRange, *Result.SourceManager,
                                        getLangOpts());
  if (SourceTxt.startswith("CHECK_JUST(")) {
    diag(ExpansionBeginLoc,
         "CHECK_JUST should not be used in functions returning Maybe. Use "
         "JUST(..) instead.")
        << ExpansionRange
        << FixItHint::CreateReplacement(
               CharSourceRange::getCharRange(
                   ExpansionBeginLoc, ExpansionBeginLoc.getLocWithOffset(10)),
               "JUST");
    return;
  }
  llvm::SmallVector<std::string, 10> GlogCheckMacroNames{
      "CHECK",    "CHECK_EQ", "CHECK_LT", "CHECK_LE",
      "CHECK_NE", "CHECK_GE", "CHECK_GT", "CHECK_NOTNULL"};
  for (const auto &Name : GlogCheckMacroNames) {
    if (SourceTxt.startswith(Name + "(")) {
      diag(ExpansionBeginLoc,
           Name + " should not be used in functions returning Maybe. Use " +
               Name + "_OR_RETURN(..) instead.")
          << ExpansionRange
          << FixItHint::CreateReplacement(
                 CharSourceRange::getCharRange(
                     ExpansionBeginLoc,
                     ExpansionBeginLoc.getLocWithOffset(Name.size())),
                 Name + "_OR_RETURN");
      return;
    }
  }
  std::string Msg =
      MatchCallExpr ? "This function contains glog CHECK and is called in a "
                      "function returning Maybe."
                    : "Glog CHECK should not be used in functions returning "
                      "Maybe. Use CHECK_OR_RETURN family or JUST(..) instead.";
  diag(Matched->getBeginLoc(), Msg) << ExpansionRange;
}

} // namespace maybe
} // namespace tidy
} // namespace clang
