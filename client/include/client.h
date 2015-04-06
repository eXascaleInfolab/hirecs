//! \brief Client for the High Resolution Hierarchical Clustering with Stable State (HiReCS)
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! >	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-11-02
#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "hirecs.hpp"

using std::string;
using namespace hirecs;


//! \brief Client of the clustering library.
//! Prepares input data for the clustering based on console input
//! \details Typical usage:
//! \code{.cpp}
//! Client  client;
//!	if(client.parseArgs(argc, argv))
//!		client.process();
//!	else client.usage(argv[0]);
//!	return 0;
//! \endcode
class Client {
public:
	Client();
	Client(const Client& c)=delete;
	Client& operator=(const Client& c)=delete;

    //! \brief Parse arguments from console and convert to internal representation
    //!
    //! \param argc int  - number of parameters
    //! \param argv[] char*  - array of string parameters
    //! \return bool  - whether parameters are successfully processed
	bool parseArgs(int argc, char *argv[]);

    //! \brief Output usage into stdout
    //!
    //! \param filename[] const char  - executable filename
    //! \return void
	void usage(const char filename[]) const;


    //! \brief Load data according to inputed parameters and perform the clustering
    //!
    //! \return void
	void process();

    //! \brief Build hierarchy from nodes
    //! 	Output results to stdout, stderr
    //!
    //! \param nodes Nodes<LinksT>&  - nodes with links to be processed
    //! \param symmetric bool  - whether links are symmetric (undirected)
    //! \param validate=true bool  - validate and fix links or skipped validation
    //! \param fast=false bool  - perform quazy-mutual clustering
    //! 	(long convergence) or quazi-optimal
    //! \param modProfitMarg=-0.999 float  - profit margin of modularity for
    //! 	early termination of the clustering. Note: should be <~ -0.01 for
    //! 	non strict clustering
    //! \param outfmt='t' char  - output hierarchy format
    //! \param extoutp=0 bool  - extended output hierarchy format
    //!     1  - show inter-cluster links
    //!     2  - unwrap root clusters to nodes
    //! \return void
	template<typename LinksT>
	static void processNodes(Nodes<LinksT>& nodes, bool symmetric
		, bool validate=true, bool fast=false, float modProfitMarg=-0.999
		, char outfmt='t', uint8_t extoutp=0);
protected:
    //! .hig file sections, similar to Pajec format, but more compact and readable
	enum class FileSection
	{
		NONE,
		GRAPH,
		NODES,
		EDGES,  //  Undirected links
		ARCS  //  Directed links
	};

	//! \brief Extend the Graph by links parsing
	//! \tparam WEIGHTED bool  - whether the link is weighted or not
	//! \param line string&  - input string to be parsed for links creation
    //! \param directed bool  - directed (arcs) / undirected (edges) links
	//! \return void
	template<bool WEIGHTED=true>
	void parseLinks(string& line, bool directed);

	//! \brief Performs clustering of the graph into hierarchy
	//! \tparam WEIGHTED bool  - whether the link is weighted or not
	template<bool WEIGHTED=true>
	void processGraph();
private:
	// User defined parameters
	char  m_outfmpt;  // Hierarchy output format
	uint8_t  m_extoutp;  // Extended hierarchy output format
	bool  m_validate;  // Validate links (and fix) / skip validation
	bool  m_fast;  // Perform strictly mutual / quazi-mutual (faster) clustering
	bool  m_reorder;  // Shuffle (rand reorder) nodes and links
	float  m_modProfitMarg;  // Profit margin for early terminaition of clustering
	string  m_inpfile;
	// File reader attributes
	Id  m_nodesNum;
	Id  m_nodesStartId;
	void *m_graphPtr;
};

#endif // CLIENT_H
