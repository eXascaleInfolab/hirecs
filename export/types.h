//! \brief Base types for the High Resolution Hierarchical Clustering with Stable State (HiReCS) library
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! > 	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-07-14

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>  // int16_t
#include <limits>  // Type limits
#include <vector>
#include <list>
#include <memory>  // unique_ptr, ...
#include <type_traits>  // conditional
#include <unordered_map>
#include <atomic>  // Atomic operations, inc

namespace hirecs {

using std::numeric_limits;
using std::vector;
using std::list;
using std::unique_ptr;
using std::conditional;
using std::atomic;
using std::unordered_map;


//! \brief Scalar Weight type definition
//!
//! \tparam UNSIGNED=true bool  - whether weight is unsigned (leads to simplified calculations)
template<bool UNSIGNED=true>
struct LinkWeight {
	//! Links weight type
	//using Type = typename conditional<UNSIGNED, uint16_t, int16_t>::type;
	using Type = float;  // RawLinkWeight type
	// Whether weights are unsigned (leads to simplified clusters calculations)
	constexpr static bool  IS_UNSIGNED = UNSIGNED;  //!< \copydoc UNSIGNED
};

using AccWeight = double;  //!< Total (accumulated) weight >= 64 bit, always signed
//! Reserved value of AccWeight for the uninitialized instances
// ATTENTION: fless() depends on current ACCWEIGHT_NONE == lowest()
constexpr AccWeight  ACCWEIGHT_NONE = numeric_limits<AccWeight>::lowest();
//! Max value of AccWeight (internally also used as a flag)
constexpr AccWeight  ACCWEIGHT_MAX = numeric_limits<AccWeight>::max();

using FItemsNum = float;  //!< Fractional number of items

// ATTENTION: AccId depends on Id, currently Id is limited upto 32 bits
using Id = uint32_t;  //!< Id of nodes, up to 4G
//! Reserved value of Id for the uninitialized instances
constexpr Id  ID_NONE = numeric_limits<Id>::max();

// Links declaration ----------------------------------------------------------
//! Container for Link items
template<typename LinkT>
using Links = vector<LinkT>;

template<typename LinksT>
class Node;

template<typename WeightT, bool WEIGHTED>
struct Link;

//! Simple (unweighted => symmetric) link
template<typename WeightT>
using SimpleLink = Link<WeightT, false>;

//! Weighted link
template<typename WeightT>
using WeightedLink = Link<WeightT, true>;

//! \brief Link declaration (weighted by default)
//!
//! \tparam WeightT  - Weight type (scalar, vector, etc.)
//! \tparam WEIGHTED bool  - whether the link is weighted or not
template<typename WeightT, bool WEIGHTED=true>
struct Link {
	using WeightType = WeightT;  //!< \copydoc WeightT
	using WeightValT = typename WeightT::Type;  //!< \copydoc WeightT::Type
	using DestT = Node<Links<Link>>;  //!< \copydoc Node<Links<Link>>

	DestT*  dest;  //!< Destination node
	//! Total accumulative outbound weight on this link (to the dest id)
	WeightValT weight;

    //! \brief Weighted link constructor
    //!
    //! \param ldest DestT*  - (destination) linked node
    //! \param lweight WeightValT  - link weight
	Link(DestT* ldest, WeightValT lweight=SimpleLink<WeightT>::weight)
	: dest(ldest), weight(lweight)  {}
};

//! \brief Simple (unweighted => symmetric) Link specialization
//!
//! \tparam WeightT  - Weight type (scalar, vector, etc.)
//! \tparam WEIGHTED bool  - whether the link is weighted or not
template<typename WeightT>
struct Link<WeightT, false> {
	using WeightType = WeightT;  //!< \copydoc WeightT
	using WeightValT = typename WeightT::Type;  //!< \copydoc WeightT::Type
	using DestT = Node<Links<Link>>;  //!< \copydoc Node<Links<Link>>

	//! Destination node
	DestT*  dest;
	//! Total accumulative outbound weight on this link (to the dest id)
	static constexpr WeightValT  weight = 1;

    //! \brief Unweighted link constructor
    //!
    //! \param ldest DestT*  - destination node / cluster
	Link(DestT* ldest)
	: dest(ldest)  {}
};

// Accumulating links
template<typename LinksT>
class Cluster;

//! \brief Accumulative link declaration
//!
//! \tparam LinksT  - cluster's links type
template<typename LinksT>
struct AccLink {
	using DestT = Cluster<LinksT>;  //!< \copydoc Cluster<LinksT>

	//! Destination cluster
	DestT*  dest;
	//! Total accumulative outbound weight on this link (to the dest id)
	AccWeight  weight;

    //! \brief Accumulating Link link constructor
    //!
    //! \param lc DestT*  - (destination) linked cluster
    //! \param lweight AccWeight  - link weight
	AccLink(DestT* ldest, AccWeight lweight=0)
	: dest(ldest), weight(lweight)  {}  // Note: inline constructor for better performance
};

// Cluster declaration --------------------------------------------------------
//! Clustering Candidates (Nodes / Clusters)
template<typename CandidateT>
using Candidates = vector<CandidateT>;

//! Container of Cluster/Node items
template<typename ItemT>
using Items = vector<ItemT>;

//! Clusterability flag
enum class Clusterable: uint8_t {
	NONE = 0,  // Element is not clusterable (gain < 0 / no candidates, skipped)
	NONMUTUAL = 0b10,  // Elements without mutual gain, candidates for the propagation
	PASSIVE = 0b001,  // Element does not initiates clustering, can be clusered only from
				// another element, because it's too heavy (to decrease the entropy)
	PASSIVE_FIXED = 0b101,  // PASSIVE that can't be approved to become clusterable (except FULL mode)
	PASSIVE_CFIXED = 0b1101,  // PASSIVE that fixed by the chain
	SINGLE = 0b011,  // Single best mutual candidate exists (takes part in max gain calc)
	MULTIPLE = 0b111,  // Multiple best mutual candidates exists
	UNDEFINED = 0b1111  // Element clusterability have not been defined yet
};

//! \brief Clustering Context
//!
//! \tparam ItemT  - items type (Cluster, Node)
template<typename ItemT>
struct Context
{
	Clusterable  clusterable;  //!< Whether it can be clusterized (max gain >=0)
	Candidates<ItemT*>  cands;  //!< Clustering candidates (bidirectional reqs), sorted
	Candidates<ItemT*>  reqs;  //!< Clustering to another nodes (CANDS CHAINs), sorted
	AccWeight  weight;  //!< Total weight of the cluster in both directions
	AccWeight  cpg;  //!< Positive complemented gain, used only on clustering
	AccWeight  gmax;  //!< Max gain (gain of each candidate)

	Context();


    //! Tidy memory from all reqs including cands
	void tidyAllReqs() {
		if(!cands.empty()) {
			cands.clear();
			cands.shrink_to_fit();
		}
		if(!reqs.empty()) {
			reqs.clear();
			reqs.shrink_to_fit();
		}
	}

    //! \brief Whether any clustering reqs (including cands) exist
    //!
    //! \return bool  - no any reqs or cands exist
	bool noreqs() const {
		return cands.empty() && reqs.empty();
	}
};

template<bool NONSYMMETRIC, typename LinksT>
class HierarchyImpl;

//! \brief Cluster Interface
//!
//! \tparam LinksT  - links type
template<typename LinksT>
class ClusterI {
public:
	const Id  id;  //!< Cluster id
	Items<Cluster<LinksT>*>  owners;  //!< Owner clusters (more than one in case of clusters overlapping)

    //! \brief ClusterI constructor
    //!
    //! \param cid Id  - cluster id
	ClusterI(Id cid)
	: id(cid), owners()  {}

	ClusterI(const ClusterI&)=delete;
	ClusterI(ClusterI&&)=default;

	virtual ~ClusterI()  {}

	ClusterI& operator=(const ClusterI&)=delete;
	ClusterI& operator=(ClusterI&&)=default;

    //! \brief Self (internal) weight of the cluster
    //!
    //! \return AccWeight  - self weight of the cluster
	virtual AccWeight selfWeight() const = 0;

    //! \brief Set self (internal) weight of the cluster
    //!
    //! \param weight AccWeight  - self weight of the cluster
    //! \return virtual void
	virtual void selfWeight(AccWeight weight) = 0;

    //! \brief Descendant clusters / nodes
    //!
    //! \return const virtual Items<ClusterI*>*  - descendants or nullptr
	virtual const Items<ClusterI*>* descs() const = 0;

    //! \brief Pointer to the cluster core among des() if exists
    //!
    //! \return virtual ClusterI*  - cluster core or nullptr
	virtual ClusterI* core() const = 0;
};

//! \brief Cluster declaration
//!
//! \tparam LinksT  - links type
template<typename LinksT>
class Cluster: public ClusterI<LinksT> {
	friend class HierarchyImpl<true, LinksT>;
	friend class HierarchyImpl<false, LinksT>;

	static atomic<Id>  m_uid;  //!< Global nodes Id counter
public:
	using AccLinksT = Links<AccLink<LinksT>>;  //!< \copydoc Links<AccLink<LinksT>>
	using WeightValT = typename LinksT::value_type::WeightType::Type;  //!< \copydoc WeightType::Type

	using ClusterI<LinksT>::id;  //!< \copydoc ClusterI<LinksT>::id
	using ClusterI<LinksT>::owners;  //!< \copydoc ClusterI<LinksT>::owners

	// Note: links here are by value, because C++11 provides efficient operations of containers inside containers (rvalue ref movement)
	AccLinksT  links;  //!< Accumulated cluster links. Sorted by dest
public:
	Items<ClusterI<LinksT>*>  des;  //!< Descendant clusters / nodes
private:
	AccWeight  m_sweight;  // Self weight of the cluster
	// ATTENTION: cluster weight can be stored / used only for the unsigned links
	//  or a graph represented and calculated as 2 subgrahs with unsigned links
	unique_ptr<Context<Cluster>>  m_context;
	ClusterI<LinksT>*  m_core;  //!<  Core of the cluster if exists (contains in des)
public:
    //! \brief Cluster constructor
    //!
    //! \param linksNum  - number of links
	Cluster(Id linksNum=0);

	Cluster(const Cluster&)=delete;
	Cluster(Cluster&&)=default;

	Cluster& operator=(const Cluster&)=delete;
	Cluster& operator=(Cluster&&)=default;

    //! \copydoc ClusterI<LinksT>::selfWeight() const
	AccWeight selfWeight() const  { return m_sweight; }

    //! \copydoc ClusterI<LinksT>::selfWeight(AccWeight weight)
	void selfWeight(AccWeight weight)  { m_sweight = weight; }

    //! \copydoc ClusterI<LinksT>::descs() const
	const Items<ClusterI<LinksT>*>* descs() const
	{ return &des; }

    //! \copydoc ClusterI<LinksT>::core() const
	ClusterI<LinksT>* core() const
	{ return m_core; }
};

// Node declaration -----------------------------------------------------------
//! \brief Node declaration
//! \note Back links must always exist even with zero weight
//!
//! \tparam LinksT  - items links type
template<typename LinksT>
class Node: public ClusterI<LinksT> {
	friend class HierarchyImpl<true, LinksT>;
	friend class HierarchyImpl<false, LinksT>;
	friend class Cluster<LinksT>;
public:
	using ClusterT = Cluster<LinksT>;  //!< \copydoc Cluster<LinksT>
	using WeightValT = typename LinksT::value_type::WeightType::Type;  //!< \copydoc WeightType::Type

	using ClusterI<LinksT>::id;  //!< \copydoc ClusterI<LinksT>::id
	using ClusterI<LinksT>::owners;  //!< \copydoc ClusterI<LinksT>::owners

	// Note: links here are by value, because C++11 provides efficient operations of containers inside containers (rvalue ref movement)
	//! \note
	//! 	- positive self link weight => tend to be clustered with any node
	//! 	- negative self link weight => tend to be separate node (root). ATTENTION: is not supported
	//! 	- zero self link weight is possible, but such link should be avoided,
	//!  		because increases complexity and is senseless except for the back links
	LinksT  links;  //!< Node links. Sorted by dest
private:
	// Note: total network weight is: Sum(m_w + m_sw (to be considered twice as other links)) / 2
	WeightValT  m_sweight;  // Self weight of the node
	unique_ptr<Context<Node>>  m_context;
public:
    //! \brief Node constructor
    //!
    //! \param nid  - node id
    //! \param linksNum  - number of links
	Node(Id nid, Id linksNum=0);

	Node(const Node&)=delete;
	Node(Node&&)=default;

	~Node()  {}

	Node& operator=(const Node&)=delete;
	Node& operator=(Node&&)=default;

    //! \copydoc ClusterI<LinksT>::selfWeight() const
	AccWeight  selfWeight() const  { return m_sweight; }

    //! \copydoc ClusterI<LinksT>::selfWeight(AccWeight weight)
	void selfWeight(AccWeight weight)  { m_sweight = weight; }

    //! \copydoc ClusterI<LinksT>::descs() const
	const Items<ClusterI<LinksT>*>* descs() const
	{ return nullptr; }

    //! \copydoc ClusterI<LinksT>::core() const
	ClusterI<LinksT>* core() const
	{ return nullptr; }
};

//! Container for the items with static allocation
template<typename ItemT>
using StoredItems = list<ItemT>;  // Note: vector<unique_ptr<ItemT>> can also be used

//! Container for nodes
// Note: without unique_ptr adresses of the nodes can be invalidated on vector updating
template<typename LinksT>
using Nodes = StoredItems<Node<LinksT>>;

//! Container for clusters
// Note: without unique_ptr adresses of the clusters can be invalidated on vector updating
template<typename LinksT>
using Clusters = StoredItems<Cluster<LinksT>>;

// Hierarchy Diagnostic Tools -------------------------------------------------
//! Share of the descendant items in the owner E (0, 1]
using Share = float;

//! All nodes of the unwrapped cluster
template<typename LinksT>
using ClusterNodes = unordered_map<Node<LinksT>*, Share>;

// Hierarchy declaration ------------------------------------------------------
//! \brief Hierarchy declaration
//!
//! \tparam LinksT  - type of items' links
template<typename LinksT>
class Hierarchy {
public:
	using ClusterItemsT = Items<Cluster<LinksT>*>;  //!< \copydoc Items<Cluster<LinksT>*>
	using NodesT = Nodes<LinksT>;  //!< \copydoc Nodes<LinksT>
	using ClustersT = Clusters<LinksT>;  //!< \copydoc Clusters<LinksT>

    //! Hierarchy score / evaluation measures
	struct Score {
		float  modularity;  //!< Total final modularity

		Score(): modularity(0)  {}
	};
protected:
	NodesT  m_nodes;  //!< Leafs that are initial nodes
	ClustersT  m_cls;  //!< All clusters of the hierarchy
	ClusterItemsT  m_root;  //!< Root level, refers stored clusters m_cls
	Score  m_score;  //!< Final total score of the hierarchy

	Hierarchy();
public:
	Hierarchy(const Hierarchy&)=delete;
	Hierarchy(Hierarchy&&)=default;

	virtual ~Hierarchy();

	Hierarchy& operator=(const Hierarchy&)=delete;
	Hierarchy& operator=(Hierarchy&&)=default;

	const NodesT& nodes() const  { return m_nodes; }  //!< \copydoc m_nodes
	const ClustersT& clusters() const  { return m_cls; }  //!< \copydoc m_cls
	const ClusterItemsT& root() const  { return m_root; }  //!< \copydoc m_root
	const Score& score() const  { return m_score; }  //!< \copydoc m_score

	//! \brief Traversing Operation (callback for the traverseNextLevel())
	//!
	//! \param cl Cluster<LinksT>&  - cluster to be processed
	//! \param initial bool  - initial (first) operation call in current level
	//! \param params void*  - passed parameters
	//! \return void
	using TraverseOp = void (*)(Cluster<LinksT>& cl, bool initial, void* params);

	//! \brief Unwrap cluster to nodes
	//!
	//! \param cl const Cluster<LinksT>&  - cluster to be unwrapped
	//! \param clNodes ClusterNodes<LinksT>&  - all inner nodes wither their share (to be extended)
	//! \return void
	void unwrap(const Cluster<LinksT>& cl, ClusterNodes<LinksT>& clNodes) const;

	//! Reset traversing to start from the first bootm level of clusters
	virtual void resetTraversing()=0;

	//! \brief Traverse next hierarchy level from bottom executing specified operation
	//! 	Traversing state is saved on function return so as next level is traversed
	//! 	on the next call in a cyclic way.
	//!
	//! \param operation TraverseOp  - operation being executed on each cluster in the level
	//! \param params=nullptr void*  - callback parameters
	//! \return bool  - whether next level is exists and can be traversed
	virtual bool traverseNextLevel(TraverseOp operation, void* params=nullptr)=0;
};

}  // hirecs

#endif // TYPES_H
