// Mostly copy from UnusedReturnValueCheck
//
#include "UnusedCheck.h"
#include "../utils/OptionsUtils.h"
#include "CommonMatcher.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceLocation.h"

using namespace clang::ast_matchers;
using namespace clang::ast_matchers::internal;

namespace clang {
namespace tidy {
namespace maybe {

UnusedCheck::UnusedCheck(llvm::StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

void UnusedCheck::registerMatchers(MatchFinder *Finder) {
  // expr means `non-statement`
  auto MatchedCallExpr =
      expr(ignoringImplicit(ignoringParenImpCasts(anyOf(
          callExpr(
              callee(functionDecl(returns(matchesNameForType("^Maybe<.*>$")))),
              forCallable(
                  functionDecl(returns(matchesNameForType("^Maybe<.*>$")))))
              .bind("match-in-maybe"),
          callExpr(
              callee(functionDecl(returns(matchesNameForType("^Maybe<.*>$")))),
              unless(forCallable(
                  functionDecl(returns(matchesNameForType("^Maybe<.*>$"))))))
              .bind("match-in-normal")))));

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

void UnusedCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *Matched =
          Result.Nodes.getNodeAs<CallExpr>("match-in-maybe")) {
    diag(Matched->getBeginLoc(), "This function returns Maybe but the return "
                                 "value is ignored. Wrap it with JUST(..)?")
        << Matched->getSourceRange()
        << FixItHint::CreateInsertion(Matched->getBeginLoc(), "JUST(")
        << FixItHint::CreateInsertion(Matched->getEndLoc(), ")");
  } else if (const auto *Matched =
                 Result.Nodes.getNodeAs<CallExpr>("match-in-normal")) {
    diag(Matched->getBeginLoc(),
         "This function returns Maybe but the return "
         "value is ignored. Wrap it with CHECK_JUST(..)?")
        << Matched->getSourceRange()
        << FixItHint::CreateInsertion(Matched->getBeginLoc(), "CHECK_JUST(")
        << FixItHint::CreateInsertion(Matched->getEndLoc(), ")");
  }
}

} // namespace maybe
} // namespace tidy
} // namespace clang
