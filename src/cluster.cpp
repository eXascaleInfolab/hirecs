//! \brief Interface inmplementation of the High Resolution Hierarchical Clustering with Stable State (HiReCS)
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! >	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-09-07

#include "types.hpp"
#include "hierarchy.hpp"


namespace hirecs {

// Internal Clustering implementation --------------------------------------------------
template<typename LinksT>
unique_ptr<Hierarchy<LinksT>> cluster(Nodes<LinksT>&& nodes, bool symmetric
	, bool validate, bool fast, float modProfitMarg)
{
#ifdef DEBUG
	fprintf(stderr, "  > cluster() params: symmetric: %u, validate: %u, fast: %u, modProfitMarg: %G\n"
		, symmetric, validate, fast, modProfitMarg);
#endif  // DEBUG
	return symmetric ? unique_ptr<Hierarchy<LinksT>>(new HierarchyImpl<false, LinksT>(
		move(nodes), validate, fast, modProfitMarg))
		: unique_ptr<Hierarchy<LinksT>>(new HierarchyImpl<true, LinksT>(
		move(nodes), validate, fast, modProfitMarg));
}

// Internal types aliases
// Weight
using SLinkWeight = LinkWeight<true>;  //!< Signed LinkWeight
using ULinkWeight = LinkWeight<false>;  //!< Unsigned LinkWeight

// Link
using UScalarSimpleLinks = Links<SimpleLink<ULinkWeight>>;  //!< Unweighted Unsigned Links
using ScalarSimpleLinks = Links<SimpleLink<SLinkWeight>>;  //!< Unweighted Signed Links

using UScalarLinks = Links<WeightedLink<ULinkWeight>>;  //!< Weighted Unsigned Links
using ScalarLinks = Links<WeightedLink<SLinkWeight>>;  //!< Weighted Signed Links

// Templates instantiation ----------------------------------------------------
// Clustering instantiation
template unique_ptr<Hierarchy<UScalarSimpleLinks>> cluster<UScalarSimpleLinks>
	(Nodes<UScalarSimpleLinks>&& nodes, bool symmetric, bool validate, bool fast, float modProfitMarg);
template unique_ptr<Hierarchy<ScalarSimpleLinks>> cluster<ScalarSimpleLinks>
	(Nodes<ScalarSimpleLinks>&& nodes, bool symmetric, bool validate, bool fast, float modProfitMarg);

template unique_ptr<Hierarchy<UScalarLinks>> cluster<UScalarLinks>
	(Nodes<UScalarLinks>&& nodes, bool symmetric, bool validate, bool fast, float modProfitMarg);
template unique_ptr<Hierarchy<ScalarLinks>> cluster<ScalarLinks>
	(Nodes<ScalarLinks>&& nodes, bool symmetric, bool validate, bool fast, float modProfitMarg);

}  // hirecs
