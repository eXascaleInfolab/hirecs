//! \brief Clustering interface of the High Resolution Hierarchical Clustering with Stable State (HiReCS) library
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! >	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-09-07

#ifndef CLUSTER_H
#define CLUSTER_H

#include <unordered_map>
#include <initializer_list>
#include "types.h"

namespace hirecs {

using std::unordered_map;
using std::initializer_list;


// External Interface for Data Input ------------------------------------------
//! \brief External Input Link
//!
//! \tparam WEIGHTED bool  - whether the link is weighted or not
//! \tparam UNSIGNED bool  - whether the link is signed or not
template<bool WEIGHTED=true, bool UNSIGNED=true>
struct InpLink {
    //! \copydoc LinkWeight<UNSIGNED>::Type
	using Weight = typename LinkWeight<UNSIGNED>::Type;
	//! \copydoc WEIGHTED
	constexpr static bool  IS_WEIGHTED = WEIGHTED;
	//! \copydoc SimpleLink<LinkWeight<UNSIGNED>>::weight
	constexpr static Weight  DEFAULT_WEIGHT =
		SimpleLink<LinkWeight<UNSIGNED>>::weight;

	Id  id;  //!< Dest node id
	Weight  weight;  //!< Link weight

    //! \brief Weighted InpLink constructor
    //!
    //! \param lid Id  - dest node id
    //! \param lweight=DEFAULT_WEIGHT Weight  - link weight
	InpLink(Id lid, Weight lweight=DEFAULT_WEIGHT)
	: id(lid), weight(lweight)  {}
};

//! \brief External Unweighted (Simple) Input Link specialization
//!
//! \tparam UNSIGNED bool  - whether the link is signed or not
template<bool UNSIGNED>
struct InpLink<false, UNSIGNED> {
    //! \copydoc LinkWeight<UNSIGNED>::Type
	using Weight = typename LinkWeight<UNSIGNED>::Type;
	//! Link is unweighted
	constexpr static bool  IS_WEIGHTED = false;
	//! \copydoc SimpleLink<LinkWeight<UNSIGNED>>::weight
	constexpr static Weight  weight =
		SimpleLink<LinkWeight<UNSIGNED>>::weight;

	Id  id;  //!< Dest node id

    //! \brief Unweighted InpLink constructor
    //!
    //! \param lid Id  - dest node id
	InpLink(Id lid)
	: id(lid)  {}
};

//! \brief Nodes Graph to couple nodes externally
//! \note Back links must always exist even with zero weight
//!
//! \tparam WEIGHTED bool  - whether the links are weighted or not
//! \tparam UNSIGNED bool  - whether the links are signed or not
template<bool WEIGHTED=true, bool UNSIGNED=true>
class Graph {
// TODO: Implement for Bipartie graphs
//	// All this is Description or Extension that can be also moved to the hierarchy
//	struct Component {  // ~ concepts
//		Id  id;
//		float  weight;  // 0..1
//	};
//	list<Component>  Components;
//	unordered_map<Component*, string>  ComponentTitles;
//	unordered_map<NodeT*, vector_ordered<Component*>> NodeComponents;
//	unordered_map<NodeT*, string>  NodeTitles;
public:
    //! \copydoc Link<LinkWeight<UNSIGNED>, WEIGHTED>
    //! \note Use only Unsigned links
	using LinkT = Link<LinkWeight<UNSIGNED>, WEIGHTED>;
	using LinksT = Links<LinkT>;  //!< \copydoc Links<LinkT>
	using NodeT = Node<LinksT>;  //!< \copydoc Node<LinksT>
	using NodesT = Nodes<LinksT>;  //!< \copydoc Nodes<LinksT>
	using Ids = Items<Id>;  //!< \copydoc Items<Id>
	//! \copydoc InpLink<WEIGHTED, UNSIGNED>
	using InpLinkT = InpLink<WEIGHTED, UNSIGNED>;
	using InpLinksT = Links<InpLinkT>;  //!< \copydoc Links<InpLinkT>

	constexpr static bool  IS_WEIGHT = WEIGHTED;  //!< \copydoc WEIGHTED

    //! Graph nodes with links formed from input data to be clustered
	NodesT  nodes;

    //! \brief Grapth constructor
    //!
    //! \param nodesNum  - estimated number of nodes
    //! \param shuffle=false bool  - shuffle (rand reorder) nodes and links
	Graph(Id nodesNum=0, bool shuffle=false);

    //! \brief Reinitialize the Graph
    //! \note existent nodes will be reseted
    //!
    //! \param nodesNum  - estimated number of nodes
    //! \param shuffle=false bool  - shuffle (rand reorder) nodes and links
	void reinit(Id nodesNum=0, bool shuffle=false);

    //! \brief Add new nodes to the graph
    //! Required only to preallocate nodes and decrease number of reallocations
    //!
    //! \param nodesIds const Ids&  - list of node ids to be added
    //! \return void
	void addNodes(const Ids& nodesIds);

    //! \brief Add new nodes to the graph
    //! Required only to preallocate nodes and decrease number of reallocations
    //!
    //! \param nodesIds initializer_list<Id>  - list of node ids to be added
    //! \return void
	void addNodes(initializer_list<Id> nodesIds);

    //! \brief Add solid range of new nodes to the graph
    //! Required only to preallocate nodes and decrease number of reallocations
    //!
    //! \param idBeg Id  - start id
    //! \param idEnd Id  - end id (excluding)
    //! \return void
	void addNodes(Id idBeg, Id idEnd);

    //! \brief Add node links to the Graph
    //! \note Links must point only to already existent nodes
    //!
    //! \tparam DIRECTED bool  - whether links are directed
    //! \param node Id  - source node id
    //! \param links const InpLinksT&  - node links
    //! \return void
	template<bool DIRECTED>
	void addNodeLinks(Id node, const InpLinksT& links);

    //! \brief Add node links to the Graph
    //! \note Links must point only to already existent nodes
    //!
    //! \tparam DIRECTED bool  - whether links are directed
    //! \param node Id  - source node id
    //! \param links initializer_list<InpLink<WEIGHTED, UNSIGNED>>  - node links
    //! \return void
	template<bool DIRECTED>
	void addNodeLinks(Id node, initializer_list
		<InpLink<WEIGHTED, UNSIGNED>> links);

    //! \brief Add node links to the Graph
    //! \note Node and links can denote unexistent nodes that are created in place
    //!
    //! \tparam DIRECTED bool  - whether links are directed
    //! \param node Id  - source node id
    //! \param links const InpLinksT&  - node links
    //! \return void
	template<bool DIRECTED>
	void addNodeAndLinks(Id node, const InpLinksT& links);

    //! \brief Complete initialization and fix the Graph
	//! that prevents it from the subsequent nodes/links extension
	//! and releases memory occupied by the corresponding helpers
    //!
    //! \return NodesT&  - costructed nodes with links
	NodesT& finalize();

    //! \brief Whether links of nodes are directed (nonsymmentric)
    //!
    //! \return bool  - at least one link is directed (two considering backlink)
	bool directed() const  { return m_directed; }
protected:
    //! Validate that the Graph is applicable to be extended (not finalized)
	void validateExtension();
private:
	unordered_map<Id, NodeT*>  m_idNodes;
	bool  m_finalized;
	bool  m_directed;  // Whether nodes links are directed
	bool  m_shuffle;
};

// Clustering interface -------------------------------------------------------
//! \brief Perform clustering and build the hierarchy
//!
//! \tparam LinksT  - type of items links
//!
//! \param nodes Nodes<LinksT>&&
//! \param symmetric bool  - whether links are symmetric (use simplified
//! 	calculations)
//! 	\note
//! 	    - it is skipped (handled as symmetric) for the unweighted links
//! \param validate bool  - whether to validate links consistancy
//! \param fast bool  - perform strictly mutual or quazi-mutual (faster) clustering
//! \param modProfitMarg float  - modularity profit margin to stop clusering
//! 	when modularity increases less than on this value.
//! 	-1 means the same, but mod tracing is not shown
//! \return unique_ptr<Hierarchy<LinksT>>  - resulting hierarchy
template<typename LinksT>
unique_ptr<Hierarchy<LinksT>> cluster(Nodes<LinksT>&& nodes, bool symmetric
	, bool validate=true, bool fast=false, float modProfitMarg=-0.999);
}  // hirecs

#endif // CLUSTER_H
