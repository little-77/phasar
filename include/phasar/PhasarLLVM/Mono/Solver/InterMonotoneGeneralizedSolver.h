/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * InterMonotoneSolver.h
 *
 *  Created on: 15.06.2018
 *      Author: nicolas
 */

#pragma once

// #include <deque>
#include <iostream>
#include <utility> // std::make_pair, std::pair
#include <vector>
#include <functional> // std::greater
#include <set>
#include <map>

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/Mono/InterMonotoneProblem.h>
#include <phasar/Utils/Macros.h>
#include <phasar/PhasarLLVM/Mono/Values/ValueBase.h>
#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>
#include <phasar/Utils/LLVMShorthands.h>

namespace psr {

/*
 *  N = Node of the CFG
 *  V = Values in the std::set of the edges
 *  M = Method type of the CFG
 *  C =
 *  I = CFG/ICFG type (must be a inherited class of CFG<N,M>)
 */

template <typename N, typename V, typename M, typename C, typename I,
          typename Context>
class InterMonotoneGeneralizedSolver {
private:
  template <typename T1, typename T2>
  void InterMonotoneGeneralizedSolver_check() {
    // static_assert(std::is_base_of<ValueBase<T1, T2, V>, V>::value, "Template class V must be a sub class of ValueBase<T1, T2, V> with T1, T2 templates\n");
    static_assert(std::is_base_of<ContextBase<N, V, Context>, Context>::value, "Template class Context must be a sub class of ContextBase<N, V, Context>\n");
    static_assert(std::is_base_of<CFG<N, M>, I>::value, "Template class I must be a sub class of CFG<N, M>\n");
  }

protected:
  using analysis_t = MonoMap<N, MonoMap<Context, MonoSet<V>>>;
  using edge_t = std::pair<N, N>;
  using priority_t = unsigned int;

  // DEBUG : Only for test purpose, should be a special fonction
  // To think later, probably require to provide an implementation of
  // this fonction
  struct lessEdgeN {
    bool lessN(const edge_t& lhs, const edge_t& rhs) const {
      return std::stol(getMetaDataId(lhs)) < std::stol(getMetaDataId(rhs));
    }

    bool operator() (const edge_t& lhs, const edge_t& rhs) const {
      return lhs.first < rhs.first || (!(rhs.first < lhs.first) && lhs.second < rhs.second);
    }
  };
  // DEBUG

  InterMonotoneProblem<N, V, M, C, I> &IMProblem;
  std::map<std::pair<priority_t, Context>, std::set<edge_t, lessEdgeN>, std::greater<std::pair<priority_t, Context>>> Worklist;

  using WL_first_it_t = typename decltype(Worklist)::iterator;
  using WL_second_const_it_t = typename decltype(Worklist)::mapped_type::const_iterator;
  analysis_t Analysis;
  I &ICFG;

  Context current_context;
  priority_t current_priority = 0;
  WL_first_it_t current_it_on_priority;
  WL_second_const_it_t current_it_on_edge;
  std::set<edge_t> call_edges;

//TODO: initialize the Analysis std::map with different contexts
  // void initialize_with_context() {
  //   for ( const auto& seed : IMProblem.initialSeeds() ) {
  //     for ( const auto& context : seed.second ) {
  //       Analysis[seed.first][context.first]
  //         .insert(context.second.begin(), context.second.end());
  //     }
  //   } // Seeds
  // }

  void initialize() {
    for ( const auto& seed : IMProblem.initialSeeds() ) {
      Analysis[seed.first][current_context]
          .insert(seed.second.begin(), seed.second.end());
    }
  }

  virtual void analyse_function(M method) {
    analyse_function(method, current_context, current_priority);
  }

  virtual void analyse_function(M method, Context& new_context) {
    analyse_function(method, new_context, current_priority);
  }

  virtual void analyse_function(M method, Context& new_context, priority_t new_priority) {
      std::vector<edge_t> edges =
            ICFG.getAllControlFlowEdges(method);
      auto current_pair = std::make_pair(new_priority, new_context);
        Worklist[current_pair].insert(edges.begin(), edges.end());
  }

  bool isIntraEdge(const edge_t& edge) const {
    return ICFG.getMethodOf(edge.first) == ICFG.getMethodOf(edge.second);
  }

  bool isCallEdge(const edge_t& edge) const {
    return call_edges.count(edge);
  }

  bool isReturnEdge(const edge_t& edge) const {
    return !isIntraEdge(edge) && ICFG.isExitStmt(edge.first);
  }

  virtual void getNext() {
    // We assure before using it that WL is not empty
    current_it_on_priority =  Worklist.begin();
    current_priority = current_it_on_priority->first.first;
    current_context = current_it_on_priority->first.second;
    current_it_on_edge = Worklist.cbegin()->second.cbegin();
  }

  virtual bool isWLempty() const noexcept {
    return Worklist.empty() || Worklist.cbegin()->second.empty();
  }

  virtual void eraseWL() {
    if ( call_edges.count(*current_it_on_edge) )
      call_edges.erase(*current_it_on_edge);

    auto& inside_set = current_it_on_priority->second;
    inside_set.erase(current_it_on_edge);

    if ( inside_set.empty() )
      Worklist.erase(current_it_on_priority);
  }

public:
  InterMonotoneGeneralizedSolver(InterMonotoneProblem<N, V, M, C, I> &IMP, Context& context, M method)
      : IMProblem(IMP), ICFG(IMP.getICFG()),
        current_context(context) {
        initialize();
        analyse_function(method);
      }

  ~InterMonotoneGeneralizedSolver() noexcept = default;
  InterMonotoneGeneralizedSolver(const InterMonotoneGeneralizedSolver& copy) = delete;
  InterMonotoneGeneralizedSolver(InterMonotoneGeneralizedSolver &&move) = delete;
  InterMonotoneGeneralizedSolver& operator=(const InterMonotoneGeneralizedSolver &copy) = delete;
  InterMonotoneGeneralizedSolver& operator=(InterMonotoneGeneralizedSolver&& move) = delete;

  virtual void solve() {
    while ( !isWLempty() ) {
      getNext();
      auto& edge = *current_it_on_edge;

      const auto& src = edge.first;
      const auto& dst = edge.second;

      MonoSet<V> Out;

      Context src_context(current_context);
      Context dst_context(src_context);

      if ( isCallEdge(edge) ) {
        // Handle call and call-to-ret flow
        if ( !isIntraEdge(edge) ) {
          Out = IMProblem.callFlow(src, ICFG.getMethodOf(dst),
                                                    Analysis[src][src_context]);
        } //  !isIntraEdge(edge)
        else {
          Out =
              IMProblem.callToRetFlow(src, dst, Analysis[src][src_context]);
        } // isIntraEdge(edge)

        // Even in a call-to-ret (like recursion) the context can change
        // (e.g. called with a different std::set of parameters)
        dst_context.enterFunction(src, dst, Analysis[src][src_context]);
      } // isCallEdge(edge)

      else if ( ICFG.isExitStmt(src) ) {
        // Handle return flow
        Out =
            IMProblem.returnFlow(dst, ICFG.getMethodOf(src), src,
                                   Analysis[src][src_context]);
        dst_context.exitFunction(src, dst, Analysis[src][src_context]);
      } // ICFG.isExitStmt(src)
      else {
        // Handle normal flow
        Out = IMProblem.normalFlow(src, Analysis[src][src_context]);
      }

      bool dst_context_already_exist = Analysis[dst].count(dst_context);

      // If there is no context equal to dst_context already in Analysis[dst]
      // we generate one so the next loop we'll work.
      if ( !dst_context_already_exist )
        Analysis[dst][dst_context];

      // We can have multiple context that are similar to dst_context
      // if the Comparison (in general std::less) is not strick weak order.
      // In that case, equal_range works to get every key with a similar context
      //WARNING: std::set::equal_range works here but it may be a bug from this version
      // of the lib. If it breaks, we should try to use a multiset to keep the
      // analysis results.
      auto dst_range = Analysis[dst].equal_range(dst_context);
      for ( auto& analysis_dst_it = dst_range.first;
            analysis_dst_it != dst_range.second; ++analysis_dst_it ) {

        // flowfactsstabilized = true <-> Same std::set + already visited once
        bool flowfactsstabilized =
          dst_context_already_exist
            ? IMProblem.sqSubSetEqual(Out, analysis_dst_it->second)
            : false;

        if ( !flowfactsstabilized ) {
          analysis_dst_it->second =
              IMProblem.join(analysis_dst_it->second, Out);

          if ( isIntraEdge(edge) ) {
            for (auto nprimeprime : ICFG.getSuccsOf(dst)) {
              //NOTE: current_it_on_priority->second could be changed by
              //      Worklist[std::make_pair(current_priority, analysis_dst_it->first)].
              //      That would make the context change for each context found in
              //      Analysis[dst], but there is almost 0 chance that there is more
              //      than 1 context for an intra-edge and using current_context reduce
              //      the overall number of edges inserted.
              current_it_on_priority->second.emplace(std::make_pair(dst, nprimeprime));
            }
          }
        } // unstabilized flow fact

        if ( isIntraEdge(edge) ) {
            if ( ICFG.isCallStmt(dst) ) {
            // The dst is a call stmt, we create the edge between the call and
            // the start point of the callees
            auto key = std::make_pair(current_priority+1, current_context);
            for ( auto callee : ICFG.getCalleesOfCallAt(dst) ) {
              for ( auto entry_point : ICFG.getStartPointsOf(callee) ) {
                auto new_edge = std::make_pair(dst, entry_point);
                Worklist[key].insert(new_edge);
                call_edges.insert(std::move(new_edge));
              } // entry_points of callee (~ 1 entry_point)
            } // callee of method
          }
        } // Intra edge

        if ( isCallEdge(edge) ) {
          if ( !flowfactsstabilized
            || dst_context.isUnsure() || IMProblem.recompute(ICFG.getMethodOf(dst)) ) {
            // We never computed the function or the flow facts have changed or
            // in case we want to handle some side-effects, the context does not
            // assured perfect equality or any reason we would want to restart the
            // computation of the function

            analyse_function(ICFG.getMethodOf(dst), dst_context);
          } // Compute a call
          // Computed or not, we called a callFlow or callToRetFlow so we generate the
          // exit edges to call the corresponding RetFlow

          for ( auto exit_point : ICFG.getExitPointsOf(ICFG.getMethodOf(dst)) ) {
            Worklist[std::make_pair(current_priority, dst_context)].emplace(std::make_pair(exit_point, src));
          }
        } // Is a call edge

        // Nothing to do in particular if an Exit statement
      }

      eraseWL();
    } // WL not empty
  }
};

} // namespace psr
