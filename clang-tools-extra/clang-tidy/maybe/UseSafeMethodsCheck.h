//===--- UseSafeMethodsCheck.h - clang-tidy ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MAYBE_USESAFEMETHODSCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MAYBE_USESAFEMETHODSCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace maybe {

/// Checks unsafe method calls like `.at()` for containers
class UseSafeMethodsCheck : public ClangTidyCheck {
public:
  UseSafeMethodsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace maybe
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MAYBE_USESAFEMETHODSCHECK_H
