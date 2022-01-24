#ifndef KSPECEM_ABILIST_H
#define KSPECEM_ABILIST_H

#include <pass.h>

using namespace llvm;

static StringRef GetGlobalTypeString(const GlobalValue &G) {
  // Types of GlobalVariables are always pointer types.
  Type *GType = G.getValueType();
  // For now we support blacklisting struct types only.
  if (StructType *SGType = dyn_cast<StructType>(GType)) {
    if (!SGType->isLiteral())
      return SGType->getName();
  }
  return "<unknown type>";
}

namespace llvm {

  class KSpecEmABIList {
    std::unique_ptr<SpecialCaseList> SCL;
    std::string func_prefix;

    public:
    KSpecEmABIList() = default;

    void set(std::unique_ptr<SpecialCaseList> List, std::string prefix) {
      SCL = std::move(List);
      func_prefix = std::move(prefix);
    }

    /// Returns whether either this function or its source file are listed in the
    /// given category.
    bool isIn(const Function &F, StringRef Category) const {
      StringRef functionName = F.getName();

      size_t dotIndex = functionName.find(".llvm.");
      if (dotIndex != (size_t)-1) {
        functionName = functionName.take_front(dotIndex);
      }
      if (functionName.startswith(func_prefix)) {
        functionName = functionName.drop_front(func_prefix.length());
      }
      return isIn(*F.getParent(), Category) ||
        SCL->inSection("dataflow", "fun", functionName, Category);
    }

    /// Returns whether this global alias is listed in the given category.
    ///
    /// If GA aliases a function, the alias's name is matched as a function name
    /// would be.  Similarly, aliases of globals are matched like globals.
    bool isIn(const GlobalAlias &GA, StringRef Category) const {
      if (isIn(*GA.getParent(), Category))
        return true;

      StringRef globalAliasName = GA.getName();
      if (globalAliasName.startswith(func_prefix)) {
        globalAliasName = globalAliasName.drop_front(func_prefix.length());
      }

      if (isa<FunctionType>(GA.getValueType()))
        return SCL->inSection("dataflow", "fun", globalAliasName, Category);

      StringRef globalTypeString = GetGlobalTypeString(GA);
      if (globalTypeString.startswith(func_prefix)) {
        globalTypeString = globalTypeString.drop_front(func_prefix.length());
      }
      return SCL->inSection("dataflow", "global", globalAliasName, Category) ||
        SCL->inSection("dataflow", "type", globalTypeString, Category);
    }

    /// Returns whether this module is listed in the given category.
    bool isIn(const Module &M, StringRef Category) const {
      StringRef moduleIdentifier = M.getModuleIdentifier();
      if (moduleIdentifier.startswith(func_prefix)) {
        moduleIdentifier = moduleIdentifier.drop_front(func_prefix.length());
      }
      return SCL->inSection("dataflow", "src", moduleIdentifier, Category);
    }
  };
} /* namespace llvm */

#endif
