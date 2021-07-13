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
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace maybe {

void GlogFatalCheck::registerMatchers(MatchFinder *Finder) {
  auto MatchedFunctionDecl = functionDecl(
      isDefinition(), returns(matchesNameForType("^Maybe<.*>$")),
      hasDescendant(cxxTemporaryObjectExpr(
                        hasTypeLoc(loc(asString("google::LogMessageFatal"))))
                        .bind("glog_fatal")));
  Finder->addMatcher(MatchedFunctionDecl, this);
}

void GlogFatalCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *Matched =
          Result.Nodes.getNodeAs<CXXTemporaryObjectExpr>("glog_fatal")) {
    diag(Matched->getBeginLoc(),
         "Glog CHECK should not be used in functions returning Maybe. Use "
         "CHECK_OR_RETURN family or JUST(..) instead.")
        << Matched->getSourceRange();
  }
}

} // namespace maybe
} // namespace tidy
} // namespace clang
