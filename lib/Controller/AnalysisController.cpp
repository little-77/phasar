/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>

#include <phasar/Controller/AnalysisController.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/Strategies.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEProtoAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDESolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDETypeStateAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSConstAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSFieldSensTaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSLinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSProtoAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSignAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTypeAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSUninitializedVariables.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoSolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoTaintAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoSolverTest.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h>
#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>

using namespace std;
using namespace psr;

namespace psr {

template <typename T> static set<T> vectorToSet(const vector<T> &v) {
  set<T> s;
  for_each(v.begin(), v.end(), [&s](T t) { s.insert(t); });
  return s;
}

AnalysisController::AnalysisController()
    : IRDB(PhasarConfig::VariablesMap()["module"]
               .as<std::vector<std::string>>()),
      TH(IRDB), PT(IRDB), ICF(TH, IRDB),
      EntryPoints(vectorToSet(PhasarConfig::VariablesMap()["entry-points"]
                                  .as<std::vector<std::string>>())) {
  executeAs(to_AnalysisStrategy(
      PhasarConfig::VariablesMap()["analysis-strategy"].as<std::string>()));
}

void AnalysisController::executeAs(AnalysisStrategy Strategy) {
  switch (Strategy) {
  case AnalysisStrategy::DemandDriven:
    assert(false && "AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::Incremental:
    assert(false && "AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::ModuleWise:
    assert(false && "AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::Variational:
    assert(false && "AnalysisStrategy not supported, yet!");
    break;
  case AnalysisStrategy::WholeProgram:
    executeWholeProgram();
    break;
  default:
    break;
  }
}

void AnalysisController::executeDemandDriven() {}

void AnalysisController::executeIncremental() {}

void AnalysisController::executeModuleWise() {}

void AnalysisController::executeVariational() {}

void AnalysisController::executeWholeProgram() {
  std::vector<std::string> AnalysisConfigs =
      PhasarConfig::VariablesMap()["analysis-config"]
          .as<std::vector<std::string>>();
  size_t ConfigIdx = 0;
  for (auto DataFlowAnalysisName :
       PhasarConfig::VariablesMap()["data-flow-analysis"]
           .as<std::vector<std::string>>()) {
    std::string AnalysisConfigPath =
        (ConfigIdx < AnalysisConfigs.size()) ? AnalysisConfigs[ConfigIdx] : "";
    auto DataFlowAnalysis = to_DataFlowAnalysisType(DataFlowAnalysisName);
    switch (DataFlowAnalysis) {
    case DataFlowAnalysisType::IFDSUninitializedVariables: {
      WholeProgramAnalysis<
          IFDSSolver<
              IFDSUninitializedVariables::n_t, IFDSUninitializedVariables::d_t,
              IFDSUninitializedVariables::m_t, IFDSUninitializedVariables::t_t,
              IFDSUninitializedVariables::v_t, IFDSUninitializedVariables::i_t>,
          IFDSUninitializedVariables>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSConstAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<IFDSConstAnalysis::n_t, IFDSConstAnalysis::d_t,
                     IFDSConstAnalysis::m_t, IFDSConstAnalysis::t_t,
                     IFDSConstAnalysis::v_t, IFDSConstAnalysis::i_t>,
          IFDSConstAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSTaintAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<IFDSTaintAnalysis::n_t, IFDSTaintAnalysis::d_t,
                     IFDSTaintAnalysis::m_t, IFDSTaintAnalysis::t_t,
                     IFDSTaintAnalysis::v_t, IFDSTaintAnalysis::i_t>,
          IFDSTaintAnalysis>
          WPA(IRDB, AnalysisConfigPath, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDETaintAnalysis: {
      WholeProgramAnalysis<
          IDESolver<IDETaintAnalysis::n_t, IDETaintAnalysis::d_t,
                    IDETaintAnalysis::m_t, IDETaintAnalysis::t_t,
                    IDETaintAnalysis::v_t, IDETaintAnalysis::l_t,
                    IDETaintAnalysis::i_t>,
          IDETaintAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDETypeStateAnalysis: {
      // WholeProgramAnalysis<
      //     IDESolver<IDETypeStateAnalysis::n_t, IDETypeStateAnalysis::d_t,
      //               IDETypeStateAnalysis::m_t, IDETypeStateAnalysis::t_t,
      //               IDETypeStateAnalysis::v_t, IDETypeStateAnalysis::l_t,
      //               IDETypeStateAnalysis::i_t>,
      //     IDETypeStateAnalysis>
      //     WPA(IRDB, AnalysisConfigPath, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSTypeAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<IFDSTypeAnalysis::n_t, IFDSTypeAnalysis::d_t,
                     IFDSTypeAnalysis::m_t, IFDSTypeAnalysis::t_t,
                     IFDSTypeAnalysis::v_t, IFDSTypeAnalysis::i_t>,
          IFDSTypeAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSSolverTest: {
      WholeProgramAnalysis<IFDSSolver<IFDSSolverTest::n_t, IFDSSolverTest::d_t,
                                      IFDSSolverTest::m_t, IFDSSolverTest::t_t,
                                      IFDSSolverTest::v_t, IFDSSolverTest::i_t>,
                           IFDSSolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSLinearConstantAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<
              IFDSLinearConstantAnalysis::n_t, IFDSLinearConstantAnalysis::d_t,
              IFDSLinearConstantAnalysis::m_t, IFDSLinearConstantAnalysis::t_t,
              IFDSLinearConstantAnalysis::v_t, IFDSLinearConstantAnalysis::i_t>,
          IFDSLinearConstantAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IFDSFieldSensTaintAnalysis: {
      WholeProgramAnalysis<
          IFDSSolver<
              IFDSFieldSensTaintAnalysis::n_t, IFDSFieldSensTaintAnalysis::d_t,
              IFDSFieldSensTaintAnalysis::m_t, IFDSFieldSensTaintAnalysis::t_t,
              IFDSFieldSensTaintAnalysis::v_t, IFDSFieldSensTaintAnalysis::i_t>,
          IFDSFieldSensTaintAnalysis>
          WPA(IRDB, AnalysisConfigPath, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDELinearConstantAnalysis: {
      WholeProgramAnalysis<
          IDESolver<
              IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
              IDELinearConstantAnalysis::m_t, IDELinearConstantAnalysis::t_t,
              IDELinearConstantAnalysis::v_t, IDELinearConstantAnalysis::l_t,
              IDELinearConstantAnalysis::i_t>,
          IDELinearConstantAnalysis>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IDESolverTest: {
      WholeProgramAnalysis<
          IDESolver<IDESolverTest::n_t, IDESolverTest::d_t, IDESolverTest::m_t,
                    IDESolverTest::t_t, IDESolverTest::v_t, IDESolverTest::l_t,
                    IDESolverTest::i_t>,
          IDESolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IntraMonoFullConstantPropagation: {
      WholeProgramAnalysis<
          IntraMonoSolver<IntraMonoFullConstantPropagation::n_t,
                          IntraMonoFullConstantPropagation::d_t,
                          IntraMonoFullConstantPropagation::m_t,
                          IntraMonoFullConstantPropagation::t_t,
                          IntraMonoFullConstantPropagation::v_t,
                          IntraMonoFullConstantPropagation::i_t>,
          IntraMonoFullConstantPropagation>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::IntraMonoSolverTest: {
      WholeProgramAnalysis<
          IntraMonoSolver<IntraMonoSolverTest::n_t, IntraMonoSolverTest::d_t,
                          IntraMonoSolverTest::m_t, IntraMonoSolverTest::t_t,
                          IntraMonoSolverTest::v_t, IntraMonoSolverTest::i_t>,
          IntraMonoSolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::InterMonoSolverTest: {
      WholeProgramAnalysis<
          InterMonoSolver<InterMonoSolverTest::n_t, InterMonoSolverTest::d_t,
                          InterMonoSolverTest::m_t, InterMonoSolverTest::t_t,
                          InterMonoSolverTest::v_t, InterMonoSolverTest::i_t,
                          3>,
          InterMonoSolverTest>
          WPA(IRDB, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::InterMonoTaintAnalysis: {
      WholeProgramAnalysis<
          InterMonoSolver<
              InterMonoTaintAnalysis::n_t, InterMonoTaintAnalysis::d_t,
              InterMonoTaintAnalysis::m_t, InterMonoTaintAnalysis::t_t,
              InterMonoTaintAnalysis::v_t, InterMonoTaintAnalysis::i_t, 3>,
          InterMonoTaintAnalysis>
          WPA(IRDB, AnalysisConfigPath, EntryPoints, &PT, &ICF, &TH);
    } break;
    case DataFlowAnalysisType::Plugin:
      break;
    default:
      break;
    }
  }
}

} // namespace psr

namespace std {

template <> struct hash<pair<const llvm::Value *, unsigned>> {
  size_t operator()(const pair<const llvm::Value *, unsigned> &p) const {
    std::hash<const llvm::Value *> hash_ptr;
    std::hash<unsigned> hash_unsigned;
    size_t hp = hash_ptr(p.first);
    size_t hu = hash_unsigned(p.second);
    return hp ^ (hu << 1);
  }
};

} // namespace std
