//===--- MaybeTIdyModule.cpp - clang-tidy ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "GlogFatalCheck.h"
#include "NeedErrorMsgCheck.h"
#include "UnusedCheck.h"
#include "UseSafeMethodsCheck.h"

namespace clang {
namespace tidy {
namespace maybe {

class MaybeModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<GlogFatalCheck>("maybe-glog-fatal");
    CheckFactories.registerCheck<UnusedCheck>("maybe-unused");
    CheckFactories.registerCheck<UseSafeMethodsCheck>("maybe-use-safe-methods");
    CheckFactories.registerCheck<NeedErrorMsgCheck>("maybe-need-error-msg");
  }
};

} // namespace maybe

// Register the MaybeTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<maybe::MaybeModule>
    X("maybe-module", "Add checks for maybe.");

// This anchor is used to force the linker to link in the generated object file
// and thus register the MaybeModule.
volatile int MaybeModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang
