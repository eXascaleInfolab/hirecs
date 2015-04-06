//! \brief High Resolution Hierarchical Clustering with Stable State (HiReCS) library, common types implementation
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! >	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-07-28

#ifndef TYPES_HPP
#define TYPES_HPP

#include "types.h"

using namespace hirecs;


// Context definitions --------------------------------------------------------
template<typename ItemT>
Context<ItemT>::Context()
: clusterable(Clusterable::UNDEFINED), cands(), reqs(), weight(ACCWEIGHT_NONE)
, cpg(ACCWEIGHT_NONE), gmax(ACCWEIGHT_NONE)
{}

// Cluster definitions --------------------------------------------------------
template<typename LinksT>
atomic<Id> Cluster<LinksT>::m_uid(0);

template<typename LinksT>
Cluster<LinksT>::Cluster(Id linksNum)
: ClusterI<LinksT>(m_uid++), links(), des(), m_sweight(0)
, m_context(new Context<Cluster>()), m_core(nullptr)
{ links.reserve(linksNum); }

// Node definitions -----------------------------------------------------------
template<typename LinksT>
Node<LinksT>::Node(Id nid, Id linksNum)
: ClusterI<LinksT>(nid), links(), m_sweight(0), m_context(new Context<Node>())
{ links.reserve(linksNum); }

// Hierarchy definitions ------------------------------------------------------
template<typename LinksT>
Hierarchy<LinksT>::Hierarchy()
: m_nodes(), m_cls(), m_root(), m_score()
{}

template<typename LinksT>
Hierarchy<LinksT>::~Hierarchy()
{}

template<typename LinksT>
void Hierarchy<LinksT>::unwrap(const Cluster<LinksT>& cl, ClusterNodes<LinksT>& clNodes) const
{
	// Note: clNodes are extended with information from this node, but not rewrote
	using ClustersLevel = unordered_map<const ClusterI<LinksT>*, Share>;
	ClustersLevel  lev({{&cl, 1}});
	// Unfold level
	Id lnum = 0;
	while(!lev.empty()) {
#ifdef DEBUG
		fprintf(stderr, "  > #%u, lev %u ---------------------------------------------\n", cl.id, lnum++);
#endif // DEBUG, tracing
		ClustersLevel  levn;
		for(const auto& il: lev) {
			auto des = il.first->descs();
			if(des) {
				if(des->empty())
					continue;
				// Form Next Level
				for(const auto cl: *des) {
					levn[cl] += il.second / (!cl->owners.empty() ? cl->owners.size() : 1);
#ifdef DEBUG
					fprintf(stderr, "  > #%u: %G\n", cl->id, levn[cl]);
#endif // DEBUG, tracing
				}
			} else {
				// Form resulting ClasterNodes
				clNodes[(Node<LinksT>*)il.first] += il.second;
#ifdef DEBUG
				fprintf(stderr, "  > %u: %G\n", il.first->id, il.second);
#endif // DEBUG, tracing
			}
		}
		lev = move(levn);
	}
}

#endif // TYPES_HPP
