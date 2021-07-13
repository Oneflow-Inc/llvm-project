#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MAYBE_UNUSEDCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MAYBE_UNUSEDCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace maybe {

/// Detects function calls where the return Maybe is unused.
///
class UnusedCheck : public ClangTidyCheck {
public:
  UnusedCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace maybe
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MAYBE_UNUSEDCHECK_H

