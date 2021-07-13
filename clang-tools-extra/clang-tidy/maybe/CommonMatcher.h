#include "clang/ASTMatchers/ASTMatchers.h"

namespace clang {
namespace tidy {
namespace maybe {

namespace {

using namespace clang::ast_matchers;
using namespace clang::ast_matchers::internal;

AST_MATCHER_REGEX(QualType, matchesNameForType, RegExp) {
  if (Node.isNull()) {
    return false;
  }
  const std::string &Name = Node.getAsString();
  return RegExp->match(Name);
}

} // namespace

} // namespace maybe
} // namespace tidy
} // namespace clang
