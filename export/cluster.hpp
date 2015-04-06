//! \brief Clustering proxy of the High Resolution Hierarchical Clustering with Stable State (HiReCS)
//! 	Constructs Graph links and nodes
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! >	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-07-16

#ifndef CLUSTER_HPP
#define CLUSTER_HPP

#include <string>
#include <cassert>
#include <stdexcept>
#include <cstdlib>  // srand
#include <ctime>  // time
#include "cluster.h"

using std::string;
using std::to_string;
using std::out_of_range;
using std::domain_error;
using std::srand;
using std::rand;
using std::time;
using namespace hirecs;


// Accessory routines ---------------------------------------------------------
//! \brief Accessory InpOperations - template argument depended wrappers
//!
//! \tparam SIMPLE=false bool  - whether operation is simple (some params are skipped)
template<bool SIMPLE=false>
struct InpOperations {
    //! \brief Add Link(dst, weight) to the src links
    //!
    //! \param src NodeT*  - source node which links are extended
    //! \param dst NodeT*  - dest node
    //! \param weight WeightT  - links weight to the dest node
    //! \param shuffle=false bool  - shuffle (rand reorder) nodes and links
    //! \return void
	template<typename NodeT, typename WeightT>
	static void addLink(NodeT* src, NodeT* dst, WeightT weight, bool shuffle=false)
	{
		if(!shuffle)
			src->links.emplace_back(dst, weight);
		else src->links.emplace(src->links.begin() + rand() % src->links.size(), dst, weight);
	}
};

//! \copydoc InpOperations::addLink
template<>
template<typename NodeT, typename WeightT>
void InpOperations<true>::addLink(NodeT* src, NodeT* dst, WeightT weight, bool shuffle)
{
	if(!shuffle)
		src->links.emplace_back(dst);
	else src->links.emplace(src->links.begin() + rand() % src->links.size(), dst);
}

//! \brief Add nodes from the user input
//!
//! \param nodes NodesT&  - internal nodes to be extended
//! \param idNodes IdNodesT&  - external inputted nodes
//! \param nodesIds const NodesIdsT&  - mapping of external node Ids into the
//! 	internal nodes
//! \param shuffle=false bool  - shuffle (rand reorder) nodes and links
//! \return void
template<typename NodesT, typename IdNodesT, typename NodesIdsT>
void acsAddNodes(NodesT& nodes, IdNodesT& idNodes, const NodesIdsT& nodesIds, bool shuffle=false)
{
	//nodes.reserve(nodesIds.size());
	// Fill nodes and mapping id -> nodePtr
	idNodes.reserve(idNodes.size() + nodesIds.size());
	for(auto id: nodesIds) {
		bool  iback = !shuffle || rand() % 2;
		if(iback)
			nodes.emplace_back(id);
		else nodes.emplace_front(id);
		auto ridn = idNodes.emplace(id, iback ? &nodes.back() : &nodes.front());
		assert(ridn.second && "acsAddNodes(), input node is duplicated");
	}
}

//! \brief Add link to the node
//!
//! \tparam DIRECTED  - whether links are directed
//! \tparam WEIGHTED  - whether links are weighted
//!
//! \param nd NodeT*  - the node to be updated
//! \param dst NodeT*  - destination node for the link
//! \param weight WeightT&&  - link weight
//! \param shuffle=false bool  - shuffle (rand reorder) nodes and links
//! \return void
template<bool DIRECTED, bool WEIGHTED, typename NodeT, typename WeightT>
void acsAddNodeLink(NodeT* nd, NodeT* dst, WeightT weight, bool shuffle=false)
{
	// ATTENTION: the weight is doubled on transition from Edges into Arcs in
	// the undirected networks
	if(dst == nd) {
		assert(!dst->selfWeight()
			&& "acsAddNodeLinks(), selfweight can be initialized just once");
		// NOTE: use double selfweight for the undirected network to compensate
		// twice increased weight from Edges i>j to Arcs i>j, j>i
		dst->selfWeight(weight * (1 + (!WEIGHTED && !DIRECTED)));
		return;
	}
	if(!DIRECTED) {
		weight /= 2;
		InpOperations<!WEIGHTED>::addLink(dst, nd, weight, shuffle);
		InpOperations<!WEIGHTED>::addLink(nd, dst, weight, shuffle);
	} else InpOperations<!WEIGHTED>::addLink(nd, dst, weight, shuffle);
}

//! \brief Add node links from the user input
//!
//! \tparam DIRECTED  - whether links are directed
//! \tparam WEIGHTED  - whether links are weighted
//!
//! \param idNodes const IdNodesT&  - external id - internal nodes mapping
//! \param src Id  - external node id
//! \param links const InpLinksT&  - external node links to be added
//! \param shuffle=false bool  - shuffle (rand reorder) nodes and links
//! \return void
template<bool DIRECTED, bool WEIGHTED, typename IdNodesT, typename InpLinksT>
void acsAddNodeLinks(const IdNodesT& idNodes, Id src, const InpLinksT& links, bool shuffle=false)
{
	// Append node links
	Id  dstId = ID_NONE;  // Required for the exception description
	try {
		auto  nd = idNodes.at(src);
		for(auto& ln: links)
			acsAddNodeLink<DIRECTED, WEIGHTED>(nd, idNodes.at(dstId = ln.id), ln.weight, shuffle);
	} catch(out_of_range& err) {
		string cause = to_string(dstId != ID_NONE ? dstId : src).insert(0
			, "acsAddNodeLinks(), the link with unexistent node is used: #")
			.append("\n").append(err.what());
		throw out_of_range(cause);
	}
}

//! \brief Add node links also adding nodes if required (not exist yet)
//!
//! \param nodes NodesT&  - internal nodes that can be extended
//! \param idNodes IdNodesT&  - external id - internal nodes mapping
//! \param src Id  - external node id
//! \param links const InpLinksT&  - external node links to be added
//! \param shuffle=false bool  - shuffle (rand reorder) nodes and links
//! \return void
template<bool DIRECTED, bool WEIGHTED, typename NodesT, typename IdNodesT, typename InpLinksT>
void acsAddNodeAndLinks(NodesT& nodes, IdNodesT& idNodes, Id src, const InpLinksT& links
	, bool shuffle=false)
{
	auto ind = idNodes.find(src);
	if(ind == idNodes.end()) {
		bool  iback = !shuffle || rand() % 2;
		if(iback)
			nodes.emplace_back(src);
		else nodes.emplace_front(src);
		auto ridn = idNodes.emplace(src, iback ? &nodes.back() : &nodes.front());
		assert(ridn.second && "acsAddNodeAndLinks(), duplicated input nodes");
		ind = ridn.first;
	}
	for(auto& ln: links) {
		auto idst = idNodes.find(ln.id);
		if(idst == idNodes.end()) {
			bool  iback = !shuffle || rand() % 2;
			if(iback)
				nodes.emplace_back(ln.id);
			else nodes.emplace_front(ln.id);
			auto ridn = idNodes.emplace(ln.id, iback ? &nodes.back() : &nodes.front());
			assert(ridn.second && "acsAddNodeAndLinks(), duplicated input nodes in links");
			idst = ridn.first;
		}
		acsAddNodeLink<DIRECTED, WEIGHTED>(ind->second, idst->second, ln.weight, shuffle);
	}
}

// External Input interfaces implementation -----------------------------------
template<bool WEIGHTED, bool UNSIGNED>
Graph<WEIGHTED, UNSIGNED>::Graph(Id nodesNum, bool shuffle)
: nodes(), m_idNodes(), m_finalized(false), m_directed(false), m_shuffle(false)
{
	m_idNodes.reserve(nodesNum);
	srand(time(nullptr));
}

template<bool WEIGHTED, bool UNSIGNED>
void Graph<WEIGHTED, UNSIGNED>::reinit(Id nodesNum, bool shuffle)
{
	nodes.clear();
	m_directed = false;
	m_idNodes.clear();
	m_idNodes.reserve(nodesNum);
	m_shuffle = shuffle;
	srand(time(nullptr));
}

template<bool WEIGHTED, bool UNSIGNED>
void Graph<WEIGHTED, UNSIGNED>::validateExtension()
{
	if(m_finalized)
		throw domain_error("addNodes(), Finalized graph can't be extended\n");
}

template<bool WEIGHTED, bool UNSIGNED>
void Graph<WEIGHTED, UNSIGNED>::addNodes(const Ids& nodesIds)
{
	validateExtension();
	acsAddNodes(nodes, m_idNodes, nodesIds, m_shuffle);
}

template<bool WEIGHTED, bool UNSIGNED>
void Graph<WEIGHTED, UNSIGNED>::addNodes(initializer_list<Id> nodesIds)
{
	validateExtension();
	acsAddNodes(nodes, m_idNodes, nodesIds, m_shuffle);
}

template<bool WEIGHTED, bool UNSIGNED>
void Graph<WEIGHTED, UNSIGNED>::addNodes(Id idBeg, Id idEnd)
{
	validateExtension();
	if(idEnd < idBeg)
		throw domain_error("addNodes(), idEnd must be >= idBeg\n");
	// Fill nodes and mapping id -> nodePtr
	m_idNodes.reserve(m_idNodes.size() + idEnd - idBeg);
	for(auto id = idBeg; id != idEnd; ++id) {
		//fprintf(stderr, "> nodes are shuffled: %d, r: %d\n", m_shuffle, rand() % 2);
		bool iback = !m_shuffle || rand() % 2;
		if(iback)
			nodes.emplace_back(id);
		else nodes.emplace_front(id);
		auto idn = m_idNodes.emplace(id, iback ? &nodes.back() : &nodes.front());
		assert(idn.second && "addNodes(), input node is duplicated");
	}
}

template<bool WEIGHTED, bool UNSIGNED>
template<bool DIRECTED>
void Graph<WEIGHTED, UNSIGNED>::addNodeLinks(Id node, const InpLinksT& links)
{
	validateExtension();
	m_directed |= DIRECTED;
	acsAddNodeLinks<DIRECTED, WEIGHTED>(m_idNodes, node, links, m_shuffle);
}

template<bool WEIGHTED, bool UNSIGNED>
template<bool DIRECTED>
void Graph<WEIGHTED, UNSIGNED>::addNodeLinks(Id node
	, initializer_list<InpLink<WEIGHTED, UNSIGNED>> links)
{
	validateExtension();
	m_directed |= DIRECTED;
	acsAddNodeLinks<DIRECTED, WEIGHTED>(m_idNodes, node, links, m_shuffle);
}

template<bool WEIGHTED, bool UNSIGNED>
template<bool DIRECTED>
void Graph<WEIGHTED, UNSIGNED> ::addNodeAndLinks(Id node, const InpLinksT& links)
{
	validateExtension();
	m_directed |= DIRECTED;
	acsAddNodeAndLinks<DIRECTED, WEIGHTED>(nodes, m_idNodes, node, links, m_shuffle);
}

template<bool WEIGHTED, bool UNSIGNED>
auto Graph<WEIGHTED, UNSIGNED>::finalize() -> NodesT&
{
	m_idNodes.clear();
	m_finalized = true;
	return nodes;
}

#endif // CLUSTER_HPP
