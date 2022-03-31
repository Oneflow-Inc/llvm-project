//===--- NeedErrorMsgCheck.cpp - clang-tidy
//----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "NeedErrorMsgCheck.h"
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

#include <iostream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace maybe {

void NeedErrorMsgCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(returnStmt(forFunction(functionDecl(returns(
                                    matchesNameForType("^Maybe<.*>$")))))
                         .bind("returnStmt"),
                     this);
}

void NeedErrorMsgCheck::check(const MatchFinder::MatchResult &Result) {
  const ReturnStmt *Matched = Result.Nodes.getNodeAs<ReturnStmt>("returnStmt");
  // getExpansionRange returns a empty range if the code at the loc is not a
  // macro
  auto ExpansionRange =
      Result.SourceManager->getExpansionRange(Matched->getBeginLoc());
  auto SourceTxt = Lexer::getSourceText(ExpansionRange, *Result.SourceManager,
                                        getLangOpts());

  llvm::SmallVector<const CXXOperatorCallExpr *> callExprs;
  auto AddCallExpr = [&callExprs](const ArrayRef<BoundNodes> Matches) {
    for (const auto &Match : Matches) {
      const auto *Operator = Match.getNodeAs<CXXOperatorCallExpr>("operator<<");
      if (Operator) {
        callExprs.push_back(Operator);
      }
    }
  };

  auto CallExprMatcher =
      cxxOperatorCallExpr(
          hasOverloadedOperatorName("<<"),
          hasArgument(1, unless(callExpr(callee(functionDecl(
                             returns(asString("class oneflow::Error"))))))))
          .bind("operator<<");

  AddCallExpr(
      match(traverse(TK_IgnoreUnlessSpelledInSource, findAll(CallExprMatcher)),
            *Matched, *(Result.Context)));

  // Check macro names and the minimum number of serialization (operator <<)
  // among them
  llvm::StringMap<unsigned> CheckMacroNamesAndSerializeCount{
      {"CHECK_OR_RETURN", 3},    {"CHECK_EQ_OR_RETURN", 8},
      {"CHECK_LT_OR_RETURN", 8}, {"CHECK_LE_OR_RETURN", 8},
      {"CHECK_NE_OR_RETURN", 8}, {"CHECK_GE_OR_RETURN", 8},
      {"CHECK_GT_OR_RETURN", 8}, {"CHECK_NOTNULL_OR_RETURN", 3}};

  // Match statement "return Error::RuntimeError();" if serializeCount is 0
  unsigned serializeCount = 0;
  for (const auto &KV : CheckMacroNamesAndSerializeCount) {
    const auto &Name = KV.getKey().str();
    if (SourceTxt.startswith(Name + "(")) {
      serializeCount = KV.getValue();
      break;
    }
  }
  if (callExprs.size() > serializeCount) {
    return;
  }
  diag(Matched->getBeginLoc(),
       "This function maybe return error but without any error message, please "
       "specify an error message.")
      << Matched->getSourceRange();
}

} // namespace maybe
} // namespace tidy
} // namespace clang
