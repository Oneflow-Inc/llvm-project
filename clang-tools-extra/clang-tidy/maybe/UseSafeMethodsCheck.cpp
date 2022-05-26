//===--- UseSafeMethodsCheck.cpp - clang-tidy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UseSafeMethodsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/Casting.h"

#include <map>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace maybe {

// a mapping from unsafe method (type name (without template parameters), method name (unqualified)) to safe function
// i.e. {{class, method}, func} means to replace `obj.method(args...)` to `JUST(func(obj, args...))` where type of obj is class
static const std::map<std::pair<std::string, std::string>, std::string> UnsafeMethods {
  {{"std::vector", "at"}, "VectorAt"},
  {{"std::deque", "at"}, "VectorAt"},
  {{"std::array", "at"}, "VectorAt"},
  {{"std::basic_string", "at"}, "VectorAt"},
  {{"std::map", "at"}, "MapAt"},
  {{"std::unordered_map", "at"}, "MapAt"},
};

static const std::string UnqualifiedMaybeTypeName = "Maybe";
static const std::string QualifiedMaybeTypeName = "oneflow::" + UnqualifiedMaybeTypeName;
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

  if(Ret->getQualifiedNameAsString() != QualifiedMaybeTypeName) {
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

  const auto *This = Expr->getImplicitObjectArgument()->IgnoreParenImpCasts();

  auto GetSource = [&SM = *Result.SourceManager, &LO = Result.Context->getLangOpts()](SourceRange SR){
    return std::string(Lexer::getSourceText(CharSourceRange::getTokenRange(SR), SM, LO));
  };
  
  std::string ArgStr;

  bool IsOriginalArgPointer = false;
  if (This->getType()->isPointerType()) {
    IsOriginalArgPointer = true;
    const auto *OThis = llvm::dyn_cast<CXXOperatorCallExpr>(This);
    if (OThis && OThis->getOperator() == OverloadedOperatorKind::OO_Arrow) {
      ArgStr = "*" + GetSource(OThis->getArg(0)->getSourceRange());
    } else {
      ArgStr = "*" + GetSource(This->getSourceRange());
    }
  } else {
    ArgStr = GetSource(This->getSourceRange());
  }
  
  for (const auto *Arg : Expr->arguments()) {
    ArgStr += ", " + GetSource(Arg->getSourceRange());
  }

  std::string FixStr = JustMacroName + "(" + 
                       std::string(Info->second) + "(" + ArgStr + "))";

  std::string AdditionalMsg;

  if (Info->first.second == "at") {
    const auto Index = [&]() -> std::string {
      for (const auto *Arg : Expr->arguments()) {
        return GetSource(Arg->getSourceRange());
      }
      // NOTE(jianhao): unreachable
      return "unknown index";
    }();
    const auto AnotherFixStr = (IsOriginalArgPointer ? ("(" + ArgStr + ")") : ArgStr) + "[" + Index + "]";
    AdditionalMsg =  "However, if you are very sure that no bound-checking is needed here, " + AnotherFixStr + " is the better choice.";
  }

  diag(Expr->getExprLoc(), "Unsafe method `%0` is called in function `%1` which returns `%2`, please try to replace it with `%3`. %4")
    << Call->getQualifiedNameAsString()
    << Func->getQualifiedNameAsString()
    << UnqualifiedMaybeTypeName
    << Info->second
    << AdditionalMsg
    << FixItHint::CreateReplacement(Expr->getSourceRange(), FixStr);

}

} // namespace maybe
} // namespace tidy
} // namespace clang
