// Mostly copy from UnusedReturnValueCheck
//
#include "UnusedMaybeCheck.h"
#include "../utils/OptionsUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;
using namespace clang::ast_matchers::internal;

namespace clang {
namespace tidy {
namespace bugprone {

namespace {

AST_MATCHER_REGEX(QualType, matchesNameForType, RegExp) {
  if (Node.isNull()) {
    return false;
  }
  const std::string &Name = Node.getAsString();
  return RegExp->match(Name);
}

} // namespace

UnusedMaybeCheck::UnusedMaybeCheck(llvm::StringRef Name,
                                   ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

void UnusedMaybeCheck::registerMatchers(MatchFinder *Finder) {
  auto MatchedCallExpr = expr(ignoringImplicit(ignoringParenImpCasts(
      callExpr(callee(functionDecl(returns(matchesNameForType("^Maybe<.*>$")))))
          .bind("match"))));

  auto UnusedInCompoundStmt =
      compoundStmt(forEach(MatchedCallExpr),
                   // The checker can't currently differentiate between the
                   // return statement and other statements inside GNU statement
                   // expressions, so disable the checker inside them to avoid
                   // false positives.
                   unless(hasParent(stmtExpr())));
  auto UnusedInIfStmt =
      ifStmt(eachOf(hasThen(MatchedCallExpr), hasElse(MatchedCallExpr)));
  auto UnusedInWhileStmt = whileStmt(hasBody(MatchedCallExpr));
  auto UnusedInDoStmt = doStmt(hasBody(MatchedCallExpr));
  auto UnusedInForStmt =
      forStmt(eachOf(hasLoopInit(MatchedCallExpr),
                     hasIncrement(MatchedCallExpr), hasBody(MatchedCallExpr)));
  auto UnusedInRangeForStmt = cxxForRangeStmt(hasBody(MatchedCallExpr));
  auto UnusedInCaseStmt = switchCase(forEach(MatchedCallExpr));

  Finder->addMatcher(
      traverse(TK_AsIs,
               stmt(anyOf(UnusedInCompoundStmt, UnusedInIfStmt,
                          UnusedInWhileStmt, UnusedInDoStmt, UnusedInForStmt,
                          UnusedInRangeForStmt, UnusedInCaseStmt))),
      this);
}

void UnusedMaybeCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *Matched = Result.Nodes.getNodeAs<CallExpr>("match")) {
    diag(Matched->getBeginLoc(), "This function returns Maybe but the return "
                                 "value is ignored. Wrap it with JUST(..)?")
        << Matched->getSourceRange()
        << FixItHint::CreateInsertion(Matched->getBeginLoc(), "JUST(")
        << FixItHint::CreateInsertion(Matched->getEndLoc(), ")");
  }
}

} // namespace bugprone
} // namespace tidy
} // namespace clang
