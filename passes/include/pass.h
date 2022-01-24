
#ifndef _PASS_H
#define _PASS_H

#include <set>
#include <map>

#if LLVM_VERSION >= 110
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasAnalysis.h"

#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"

#include "llvm/Support/Regex.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SpecialCaseList.h"
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/IR/InlineAsm.h"

#include "llvm/Transforms/Utils/Local.h"

#include "llvm/Transforms/Scalar.h"
#else
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/AliasAnalysis.h>

#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ADT/Statistic.h>

#include <llvm/Support/Regex.h>
#include <llvm/Support/CommandLine.h>
#include "llvm/Support/SpecialCaseList.h"
#include <llvm/Analysis/LoopInfo.h>

#include <llvm/IR/InlineAsm.h>

#include <llvm/Transforms/Utils/Local.h>

#include <llvm/Transforms/Scalar.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#endif /* _PASS_H */
