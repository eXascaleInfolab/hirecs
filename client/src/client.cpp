//! \brief Client for the High Resolution Hierarchical Clustering with Stable State (HiReCS)
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! >	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-11-02
#include <cstdio>
#include <utility>  // make_pair
#include <iostream>  // Input file processing
#include <fstream>
#include <limits>  //  numeric_limits
#include <stdexcept>  // Arguments processing
#include "client.h"

using std::vector;
using std::pair;
using std::move;
using std::ifstream;
using std::domain_error;
using std::invalid_argument;


// Formatting helpers ---------------------------------------------------------
template<typename ItemsT>
string itemsToStr(const ItemsT& els, char delim=' ', bool strict=false
	, const string& prefix="", const string& suffix="")
{
	string str;

	if(!els.empty()) {
		str = prefix;
		for(auto c: els)
			str += to_string(c->id) += delim;
		str.pop_back();
		str += suffix;
	} else if(!strict)
		str = "-";

	return str;
}

template<typename LinksT>
string linksToStr(const LinksT& ls)
{
	string str;

	if(!ls.empty()) {
		for(const auto& ln: ls)
			str += to_string(ln.dest->id).append(" ");
	} else str = "-";

	return str;
}

//! \brief Prints cluster links to stdout
//!
//! \param cl ClusterT&  - cluster to be processed
//! \param initial=false bool  - initial (first call) for the current level
//! \param params=nullptr void  - additional processing parameters
//! \return void
template<typename ClusterT>
void outpClsLinksJSON(ClusterT& cl, bool initial=false, void* params=nullptr)
{
	//,“levels”: [
	//	{  // Specification of the clusters on this level including selflink
	//		<cl_id>: {
	//			<cl_id1>: <weight1>,
	//			...
	//		}, ...
	//	}, ...
	//]
	printf(!initial ? ",{\"%u\":{" : "{\"%u\":{", cl.id);
	// Output selfweight as separate link, as first item if exists
	size_t  i = 0;
	if(cl.selfWeight()) {
		printf("\"%u\":%G", cl.id, cl.selfWeight());
		++i;
	}
	for(const auto& ln: cl.links)
		printf(i++ ? ",\"%u\":%G" : "\"%u\":%G", ln.dest->id, ln.weight);
	fputs("}}", stdout);
}

// Input arguments processing -------------------------------------------------
void classifyArgs(int argc, char* argv[], vector<string>& opts, vector<string>& files)
{
	opts.clear();
	files.clear();
	if(argc < 2)
		return;

	for(auto i = 1; i < argc; ++i) {
		if(argv[i][0] == '-')
			opts.push_back(argv[i] + 1);  // Skip '-'
		else files.push_back(argv[i]);
	}

#ifdef DEBUG
	fprintf(stderr, "-Arguments are classified:\n-  Options:");
	// Output options arguments
	if(!opts.empty())
		for(const auto& arg: opts)
			fputs((" " + arg).c_str(), stderr);
	else fprintf(stderr, " -");
	fprintf(stderr, "\n-  Files:");
	// Output files arguments
	if(!files.empty())
		for(const auto& arg: files)
			fputs((" " + arg).c_str(), stderr);
	else fprintf(stderr, " -");
	fprintf(stderr, "\n");
#endif // DEBUG
}

// Conditional operations -----------------------------------------------------
//! Accessory Operations - template argument depended wrappers
template<bool SIMPLE=false>  // For Nodes / non scoped storage
struct Operations {
	template<typename WeightT, typename LinkT>
	static void addLink(Links<LinkT>& links, Id id, WeightT weight=0)
	{
		links.push_back(LinkT(id, weight));
	}
};

template<>
template<typename WeightT, typename LinkT>
void Operations<true>::addLink(Links<LinkT>& links, Id id, WeightT weight)
{
	links.push_back(LinkT(id));
}

// Client implementation ------------------------------------------------------
template<typename LinksT>
void Client::processNodes(Nodes<LinksT>& nodes, bool symmetric, bool validate
	, bool fast, float modProfitMarg, char outfmt, uint8_t extoutp)
{
	// Output input data
#ifdef DEBUG
	fprintf(stderr, "-Nodes:\n");
	for(const auto& n: nodes)
		fprintf(stderr, "-Node #%2u: %s\n", n.id, linksToStr(n.links).c_str());
	fprintf(stderr, "\n");
#endif  // DEBUG
	auto hier = cluster(move(nodes), symmetric, validate, fast, modProfitMarg);

	// Output result
	using ClusterItemsT = typename decltype(hier)::element_type::ClusterItemsT;
	using RawLevel = unordered_map<Id, ClusterItemsT>;
	//using RawLevel = vector<pair<Id, ClusterItemsT>>;
	RawLevel  lev = {{ID_NONE, hier->root()}};
	fprintf(stderr, "-Root size: %lu\n", hier->root().size());

	if(outfmt == 't') {
		// Text format for log files
		printf("\n -Clusters:\n");
		for(Id i = 0; !lev.empty(); ++i) {
			RawLevel  nlev;
			printf("----- Clusers level #%u -------------------------------------------------------\n", i);
			for(const auto& g: lev) {
//				if(g.second.empty())
//					continue;
				printf("-- Sibling nodes OCl #%u --------------------------------------------\n", g.first);
				for(auto c: g.second) {
					printf("-Cluster #%u  ownersNum: %lu\n\towners: %s\n\tdes %s\n%s"
						, c->id, c->owners.size()
						, itemsToStr(c->owners).c_str()
						, itemsToStr(c->des, ' ', true, c->des.front()->descs()
							? "(cls): " : "(nds): " ).c_str()
						, c->des.front()->core() ? (string("\tcore: ")
							+= to_string(c->des.front()->core()->id).append("\n"))
							.c_str() : ""
					);
					if(c->des.front()->descs())
						nlev.emplace(c->id, (ClusterItemsT&)c->des);
				}
			}
			lev = move(nlev);
			nlev.clear();
		}
		// Write summary
		printf("-Nodes: %lu, clusers (communities): %lu, roots: %lu, mod: %G\n"
			, hier->nodes().size(), hier->clusters().size(), hier->root().size()
			, hier->score().modularity);
	} else {
		if(outfmt != 'c' && outfmt != 'j')
			throw domain_error("processNodes(), unexpected output format\n");

		if(outfmt == 'j') {
			// JSON format
			// Note: puts() appends newline implicitly, that is why fputs is used
			fputs(itemsToStr(hier->root(), ',', true, "{\"root\":[", "],\"clusters\":{").c_str(), stdout);
			size_t  j = 0;
			for(const auto& c: hier->clusters())
				printf("%s\"%u\":{%s%s%s%s}"
					, j++ ? "," : "", c.id
					, (!c.owners.empty()
						? itemsToStr(c.owners, ',', true, "\"owners\":[", "],").c_str()
						: "")
					, itemsToStr(c.des, ',', true, "\"des\":[", "]").c_str()
					, c.des.front()->descs() ? "" : ",\"leafs\":true"
					, c.des.front()->core() ? (string(",\"core\":")
						+= to_string(c.des.front()->core()->id)).c_str() : ""
				);
			putchar('}');
			if(extoutp && !hier->root().empty()) {
				// Unwrap root clusters
				//,“communities”: {  // Specification of the nodes (final leafs) for the clusters
				//		<cl_id>: {
				//			<nd_id1>: <share>,
				//			...
				//		}, ...
				//}
				fputs(",\"communities\":{", stdout);
				size_t  j = 0;
				for(const auto cl: hier->root()) {
					// Cluster id
					printf(j++ ? "},\"%u\":{" : "\"%u\":{", cl->id);
					// Nodes shares
					ClusterNodes<LinksT>  cns;
					hier->unwrap(*cl, cns);
					size_t  i = 0;
					for(const auto icn: cns)
						printf(i++ ? ",\"%u\":%G" : "\"%u\":%G", icn.first->id, icn.second);
				}
				fputs("}}", stdout);
				if(extoutp >= 2) {
					//,“levels”: [
					//	{  // Specification of the clusters on this level including selflink
					//		<cl_id>: {
					//			<cl_id1>: <weight1>,
					//			...
					//		}, ...
					//	}, ...
					//]
					fputs(",\"levels\":[", stdout);
					while(hier->traverseNextLevel(outpClsLinksJSON<Cluster<LinksT>>))
						putchar(',');
					putchar(']');
				}
			}
			printf(",\"nodes\":%lu,\"mod\":%G}", hier->nodes().size()
				, hier->score().modularity);
		} else {
			// CSV like format
			printf("# Clusters output format:\n");
			printf("# <cluster_id1>> [owners: <owner_id1> ...;] [des: <des_id1> ...;] [leafs: <leaf_id1> ...]\n");
			// Write all clusters, root are nodes without owner
			for(const auto& c: hier->clusters()) {
				printf("%u> %s%s%s%s\n", c.id, (!c.owners.empty()
					? itemsToStr(c.owners, ' ', true, "owners: ", "; ").c_str()
						: "")
					, itemsToStr(c.des, ' ', true, "des: ").c_str()
					, c.des.front()->descs() ? "" : "; leafs: true"
					, c.des.front()->core() ? (string("; core: ")
						+= to_string(c.des.front()->core()->id)).c_str() : ""
				);
			}
			printf("# Nodes: %lu, clusers: %lu, roots: %lu, mod: %G\n"
				, hier->nodes().size(), hier->clusters().size(), hier->root().size()
				, hier->score().modularity);
		}
	}

//		// Output input data
//		if(hier->nodes()) {
//			printf("\nNodes with links:\n");
//			for(const auto& n: *hier->nodes())
//				printf("Node #%2u: %s\n", n.id, linksToStr(n.links).c_str());
//		}
	// Here Clusters destructors output will be under DEBUG
	printf("\n");
}

Client::Client()
: m_outfmpt('t'), m_extoutp(false), m_validate(true), m_fast(false), m_reorder(false)
, m_modProfitMarg(-0.999), m_inpfile(), m_nodesNum(0), m_nodesStartId(ID_NONE)
, m_graphPtr(nullptr)
{}

bool Client::parseArgs(int argc, char *argv[])
{
	if(argc < 2)
		return false;
	// Parse input data and perform clustering
	vector<string>  opts;
	vector<string>  files;
	classifyArgs(argc, argv, opts, files);
	// Check files
	if(files.empty())
		throw domain_error("Output file name is expected to be provided");
	m_inpfile = files.front();

	// Check and apply options
	for(const auto& opt: opts)
		switch(opt[0]) {
		case 'o':
			if(opt.length() < 2 || (opt.length() >= 3 && opt[2] != 'e' && opt[2] != 'd'))
				throw domain_error("Unexpected option is provided: -" + opt + "\n");
			m_outfmpt = opt[1];
			if(opt.length() >= 3)
				m_extoutp = opt[2] == 'e' ? 1 : 2;
			break;
		case 'c':
			m_validate = false;
			break;
		case 'f':
			m_fast = true;
			break;
		case 'r':
			m_reorder = true;
			break;
		case 'm':
			m_modProfitMarg = stof(opt.substr(1));
			break;
		default:
			throw invalid_argument("Unexpected option is provided: -" + opt + "\n");
		}

	return true;
}

void Client::usage(const char filename[]) const
{
	printf("Usage: %s [-o{t,c,j}] [-f] [-r] [-m<float>] <adjacency_matrix.hig>\n"
		"  -o  - output data format. Default: t\n"
		"    t  - text like representation for logs\n"
		"    c  - CSV like representation for parcing\n"
		"    j  - JSON represenation\n"
		"    je  - extended JSON represenation (j + unwrap root clusters to nodes)\n"
		"    jd  - detaile JSON represenation (je + show inter-cluster links)\n"
		"  -c  - clean links, skip links validation\n"
		"  -f  - fast quazy-mutual clustering (faster). Default: strictly-mutual (better)\n"
		"  -r  - rand reorder (shuffle) nodes and links on nodes construction\n"
		"  -m<float>  - modularity profit margin for early exit"
		", float E [-1, 1]. Default: -0.999, but on practice >~= 0\n"
		"    -1  - skip stderr tracing after each iteration. Recommended: 1E-6 or 0\n"
		, filename);
}

template<bool WEIGHTED>
void Client::processGraph()
{
	if(!m_graphPtr)
		throw domain_error("Graph should be existed\n");
	auto graph = reinterpret_cast<Graph<WEIGHTED>*>(m_graphPtr);

	processNodes(graph->finalize(), !graph->directed(), m_validate
		, m_fast, m_modProfitMarg, m_outfmpt, m_extoutp);

	// Finalize processing
	delete graph;
	m_graphPtr = nullptr;
}

template<bool WEIGHTED>
void Client::parseLinks(string& line, bool directed)
{
	using GraphT = Graph<WEIGHTED>;

	// Grate Graph if required
	if(!m_graphPtr) {
		m_graphPtr = new GraphT(m_nodesNum, m_reorder);
		if(m_nodesStartId != ID_NONE)
			reinterpret_cast<GraphT*>(m_graphPtr)
				->addNodes(m_nodesStartId, m_nodesStartId + m_nodesNum);
	}

	// Parse links
	constexpr char  spaces[] = " \t";
	// Fetch node id
	size_t  pos;
	Id  nid = stoul(line, &pos);

	// Fetch links
	pos = line.find_first_not_of(spaces, line.find('>', pos));
	if(pos == string::npos)
		return;
	++pos;
	const auto  lineLen = line.length();
	typename GraphT::InpLinksT  links;
	using Link = typename GraphT::InpLinkT;
	using Weight = typename Link::Weight;
	constexpr unsigned char  SYM_DIGITS_MAX = numeric_limits<Weight>::digits10 + 2;  // 2 ("0", ".") + 6 digits for float = 8

	while(pos < lineLen) {
		pos = line.find_first_not_of(spaces, pos);
		// Fetch dest id
		size_t  offs;
		Id  did = stoul(line.substr(pos, SYM_DIGITS_MAX), &offs);
		pos += offs;
		// Next is : or space
		// Fetch link weight
		Weight  weight = 0;
		bool  weightAssigned = false;
		if(WEIGHTED && pos < lineLen && line[pos] == ':') {
			weight = stof(line.substr(++pos, SYM_DIGITS_MAX), &offs);
			weightAssigned = true;
			pos += offs;
		}
		offs = line.find_first_of(spaces, pos);
		if(offs != pos && offs != string::npos && (WEIGHTED || line[pos] != ':')) {
			constexpr size_t  ctxSize = SYM_DIGITS_MAX;
			size_t  pbeg = 0;
			if(pos > ctxSize)
				pbeg = pos - ctxSize;
			size_t  pend = pos + ctxSize + 1;
			if(pend > line.size())
				pend = line.size();

			throw domain_error(to_string(pos).insert(0
				, "Invalid value format in pos: ").append(", context(+/-PRECISION_DIG symbols): ")
				+= line.substr(pbeg, pend - pbeg) += '\n');
		}
		pos = offs;
		if(weightAssigned)
			Operations<!Link::IS_WEIGHTED>::addLink(links, did, weight);
		else Operations<true>::addLink<Weight>(links, did);
		if(pos == string::npos)
			pos = lineLen;
	}

	// Store links in the Graph
	if(!links.empty()) {
		GraphT  &graph = *reinterpret_cast<GraphT*>(m_graphPtr);
		if(m_nodesStartId != ID_NONE) {
			if(directed)
				graph.template addNodeLinks<true>(nid, links);
			else graph.template addNodeLinks<false>(nid, links);
		} else if(directed)
			graph.template addNodeAndLinks<true>(nid, links);
		else graph.template addNodeAndLinks<false>(nid, links);
	}
}

void Client::process()
{
	// Default Graph params
	bool  weighted = true;

	assert(m_graphPtr == nullptr && "m_graphPtr should be empty on start\n");
	m_nodesNum = 0;
	m_nodesStartId = ID_NONE;
	m_graphPtr = nullptr;

	constexpr char  spaces[] = " \t";
	string  line;
	ifstream  infile;
	// Set exceptions to any file IO operations
	infile.exceptions(ifstream::badbit);  // | ifstream::failbit
	infile.open(m_inpfile);
	FileSection sect = FileSection::NONE;
	while(getline(infile, line)) {
		// Skip leading spaces
		auto pos = line.find_first_not_of(spaces);
		// Skip empty lines and comments
		if(pos == string::npos || line[pos] == '#')
			continue;

		// Define file section and parse it
		if(line[pos] != '/') {
			if(sect != FileSection::EDGES && sect != FileSection::ARCS)
				continue;

			if(weighted)
				parseLinks<true>(line, sect == FileSection::ARCS);
			else parseLinks<false>(line, sect == FileSection::ARCS);
		} else {
			// Extract section name and convert to lowercase
			auto pose = line.find_first_of(spaces, ++pos);
			string  title = line.substr(pos, pose - pos);
			if(title.empty())
				throw domain_error("Invalid (empty) section header\n");
			for(size_t i = 0; i != title.length(); ++i)
				title[i] = tolower(title[i]);
			// Remove tail comment if exists
			pos = line.find("#", pose);
			if(pos != string::npos)
				line.resize(pos);

			// Define current section
			if(!title.compare("graph")) {
				if(sect != FileSection::NONE)
					throw domain_error(
						"Unexpected section: Graph section must be first one\n");

				sect = FileSection::GRAPH;
				// Load weighted attribute
				if(pose != string::npos) {
					// Process attrib weighted
					title = "weighted:";
					pos = line.find(title, pose + 1);
					if(pos != string::npos) {
						// Fetch value
						pos = line.find_first_not_of(spaces, pos + title.length());
						weighted = stoi(line.substr(pos
							, line.find_first_of(spaces, pos + 1)));
					}
				}
			} else if(!title.compare("nodes")) {
				if(sect != FileSection::NONE && sect != FileSection::GRAPH)
					throw domain_error(
						"Unexpected section: Nodes section must be first one"
						" or after the Graph section\n");

				sect = FileSection::NODES;
				// Load Nodes attributes
				if(pose != string::npos) {
					// nodesNum
					pos = line.find_first_not_of(spaces, pose + 1);
					if(pos != string::npos) {
						m_nodesNum = stoul(line.substr(pos
							, pose = line.find_first_of(spaces, pos + 1)));
						// nodesStartId
						if(pose != string::npos) {
							pos = line.find_first_not_of(spaces, pose + 1);
							if(pos != string::npos)
								m_nodesStartId = stoul(line.substr(pos
									, line.find_first_of(spaces, pos + 1)));
						}
					}
				}
			} else if(!title.compare("edges"))
				sect = FileSection::EDGES;
			else if(!title.compare("arcs"))
				sect = FileSection::ARCS;
			else throw out_of_range(
				title.insert(0, "Unknown section is used: ") += '\n');
		}
	}

	// Perfom clustering
	if(weighted)
		processGraph<true>();
	else processGraph<false>();

	assert(m_graphPtr == nullptr  && "Graph must be released after processing\n");
}
