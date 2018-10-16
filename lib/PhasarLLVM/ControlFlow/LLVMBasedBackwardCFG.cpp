/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedBackwardCFG.cpp
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include <phasar/Config/Configuration.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h>

using namespace psr;
using namespace std;

namespace psr {
//getStatementId?

//same as LLVMBasedCFG
const llvm::Function *
LLVMBasedBackwardCFG::getMethodOf(const llvm::Instruction *stmt) {
  return stmt->getParent()->getParent();
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getPredsOf(const llvm::Instruction *stmt) {
  return {};
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getSuccsOf(const llvm::Instruction *stmt) {
  return {};
}

std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedBackwardCFG::getAllControlFlowEdges(const llvm::Function *fun) {
  return {};
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardCFG::getAllInstructionsOf(const llvm::Function *fun) {
  vector<const llvm::Instruction *> Instructions;
  for (auto &BB : *fun) {
    for (auto &I : BB) {
      Instructions.insert(Instructions.begin(),&I);
    }
  }
  return Instructions;
}

//LLVMBasedCFG::isStartPoint
bool LLVMBasedBackwardCFG::isExitStmt(const llvm::Instruction *stmt) {
  return (stmt == &stmt->getFunction()->front().front());
}

//LLVMBasedCFG::isExitStmt
bool LLVMBasedBackwardCFG::isStartPoint(const llvm::Instruction *stmt) {
  return llvm::isa<llvm::ReturnInst>(stmt);
}

bool LLVMBasedBackwardCFG::isFallThroughSuccessor(
    const llvm::Instruction *stmt, const llvm::Instruction *succ) {
  return false;
}

bool LLVMBasedBackwardCFG::isBranchTarget(const llvm::Instruction *stmt,
                                          const llvm::Instruction *succ) {
  return false;
}

//same as LLVMBasedCFG
std::string LLVMBasedBackwardCFG::getMethodName(const llvm::Function *fun) {
  return fun->getName().str();
}

std::string LLVMBasedBackwardCFG::getStatementId(const llvm::Instruction *stmt) {
  return llvm::cast<llvm::MDString>(
             stmt->getMetadata(MetaDataKind)->getOperand(0))
      ->getString()
      .str();
}
} // namespace psr
