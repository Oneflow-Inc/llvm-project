//===--- UseSafeMethodsCheck.cpp - clang-tidy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UseSafeMethodsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Lex/Lexer.h"

#include <map>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace maybe {

// a mapping from unsafe method (type name (without template parameters), method name (unqualified)) to safe function
// i.e. {{class, method}, func} means to replace `obj.method(args...)` to `JUST(func(obj, args...))` where type of obj is class
static const std::map<std::pair<std::string, std::string>, std::string> UnsafeMethods {
  {{"std::vector", "at"}, "oneflow::VectorAt"},
  {{"std::deque", "at"}, "oneflow::VectorAt"},
  {{"std::array", "at"}, "oneflow::VectorAt"},
  {{"std::basic_string", "at"}, "oneflow::VectorAt"},
  {{"std::map", "at"}, "oneflow::MapAt"},
  {{"std::unordered_map", "at"}, "oneflow::MapAt"},
};

static const std::string MaybeTypeName = "oneflow::Maybe";
static const std::string JustMacroName = "JUST";

void UseSafeMethodsCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(cxxMemberCallExpr(
    thisPointerType(
      hasCanonicalType(
        hasDeclaration(
          cxxRecordDecl().bind("obj")
        )
      )
    ),
    callee(
      cxxMethodDecl().bind("call")
    ),
    forFunction(
      functionDecl(
        returns(
          hasDeclaration(
            cxxRecordDecl().bind("ret")
          )
        )
      ).bind("func")
    )
  ).bind("expr"), this);
}

void UseSafeMethodsCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *Ret = Result.Nodes.getNodeAs<CXXRecordDecl>("ret");
  if(!Ret) {
    return;
  }

  if(Ret->getQualifiedNameAsString() != MaybeTypeName) {
    return;
  }

  const auto *Call = Result.Nodes.getNodeAs<CXXMethodDecl>("call");
  if(!Call) {
    return;
  }

  const auto *Obj = Result.Nodes.getNodeAs<CXXRecordDecl>("obj");
  if(!Obj) {
    return;
  }

  auto Info = UnsafeMethods.find(std::make_pair(Obj->getQualifiedNameAsString(), Call->getNameAsString()));
  if(Info == UnsafeMethods.end()) {
    return;
  }

  const auto *Expr = Result.Nodes.getNodeAs<CXXMemberCallExpr>("expr");
  if(!Expr) {
    return;
  }

  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!Func) {
    return;
  }

  auto GetSource = [&SM = *Result.SourceManager, &LO = Result.Context->getLangOpts()](SourceRange SR){
    return Lexer::getSourceText(CharSourceRange::getTokenRange(SR), SM, LO);
  };

  auto ObjStr = GetSource(Expr->getImplicitObjectArgument()->getSourceRange());

  std::string ArgStr;
  for (const auto *Arg : Expr->arguments()) {
    ArgStr += ", " + std::string(GetSource(Arg->getSourceRange()));
  }

  std::string FixStr = JustMacroName + "(" + std::string(Info->second) + "(" + std::string(ObjStr) + ArgStr + "))";

  diag(Expr->getExprLoc(), "Unsafe method `%0` is called in function `%1` which returns `%2`, please try to replace it with `%3`")
    << Call->getQualifiedNameAsString()
    << Func->getQualifiedNameAsString()
    << MaybeTypeName
    << Info->second
    << FixItHint::CreateReplacement(Expr->getSourceRange(), FixStr);
}

} // namespace maybe
} // namespace tidy
} // namespace clang
