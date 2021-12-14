// TODO: add header

#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/LangOptions.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_os_ostream.h"
#include "clang/AST/DeclTemplate.h"
#include "llvm/Support/raw_ostream.h"

namespace maybe {

inline llvm::Optional<std::string> GetNameOfClassTemplate(clang::QualType type) {
    if (const auto &tempType = type->getAs<clang::TemplateSpecializationType>()) {
        if (clang::TemplateDecl *decl = tempType->getTemplateName().getAsTemplateDecl()) {
            return decl->getQualifiedNameAsString();
        }
    } else if (const auto &recordDecl = type->getAsRecordDecl()) {
        return recordDecl->getQualifiedNameAsString();
    }

    return {};
}

inline bool MatchNameOfClassTemplate(clang::QualType type, llvm::StringRef name) {
    if (const auto& res = GetNameOfClassTemplate(type)) {
        return res.getValue() == name;
    }

    return false;
}

inline llvm::Optional<std::pair<std::string, std::string>> GetMethodNameInClassTemplate(const clang::FunctionDecl* decl) {
    if (const auto *md = llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {
        if (!md->isStatic()) {
            if(const auto &tempName = GetNameOfClassTemplate(md->getThisObjectType())) {
                return std::make_pair(tempName.getValue(), decl->getNameAsString());
            }
        }
    }

    return {};
}

inline bool MatchMethodNameInClassTemplate(const clang::FunctionDecl* decl, llvm::StringRef name, llvm::StringRef method) {
    if (const auto& res = GetMethodNameInClassTemplate(decl)) {
        return res->first == name && res->second == method;
    }

    return false;
}

inline llvm::raw_ostream& DebugOuts() {
    #if defined(NDEBUG)
        return llvm::nulls();
    #else
        return llvm::outs();
    #endif
}

} // namespace maybe
