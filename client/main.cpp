//! \brief High Resolution Hierarchical Clustering with Stable State (HiReCS) client
//!
//! \license Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0.html
//! >	Simple explanation: https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)
//!
//! Copyright (c)
//! \authr Artem Lutov
//! \email luart@ya.ru
//! \date 2014-09-04

#include "client.h"

// Hardcoded usecases ---------------------------------------------------------
void testcase(const char* filename=nullptr)
{
	//using LinksT = WeightedLinks;

	fprintf(stderr, "-Clusterization Verifier\n");
	//Size nlinks = 0;  // Links size
	if(!filename) {
		// Test Usecases ----------------------------------------------------------
//		// Sample from wiki: http://en.wikipedia.org/wiki/Modularity_(networks)#Example_of_multiple_community_detection
//	    Nodes<LinksT>  nodes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
//		// Note: Assert should be for {1, 2, 3} - backlinks postcondition verification
//	    setNodeLinks(*nodes[$1, {1, 2};
//	    setNodeLinks(*nodes[$1, {0, 2, 3};
//	    setNodeLinks(*nodes[$1, {0, 1};
//		// Note: Assert should be for {0, 1, 5, 7} - backlinks pretcondition verification
//	    setNodeLinks(*nodes[$1, {1, 5, 7};
//	    setNodeLinks(*nodes[$1, {5, 6};
//	    setNodeLinks(*nodes[$1, {3, 4, 6};
//	    setNodeLinks(*nodes[$1, {4, 5};
//	    setNodeLinks(*nodes[$1, {3, 8, 9};
//	    setNodeLinks(*nodes[$1, {7, 9};
//	    setNodeLinks(*nodes[$1, {7, 8};

		// Simple usecase
//		using GraphT = Graph<false>;  // Unweighted
		using GraphT = Graph<true>;  // Weighted
		GraphT  graph;
//		constexpr bool  DIRECTED = false;
//		// Simple triangle
// 		graph.addNodes({0, 1, 2});
// 		graph.addNodeLinks<DIRECTED>(0, {1, 2});
// 		graph.addNodeLinks<DIRECTED>(1, {2});
//		// With separate node
// 		graph.addNodes({3});

//		// Extension to simple square
// 		graph.addNodes({3});
// 		graph.addNodeLinks<DIRECTED>(3, {1, 2});

//		// Simple square
// 		graphacsAddNodeLinks.addNodes({0, 1, 2, 3});
// 		graph.addNodeLinks<DIRECTED>(0, {1, 2});
// 		graph.addNodeLinks<DIRECTED>(3, {1, 2});

// 		// Simple pentagon (5)
// 		graph.addNodes({0, 1, 2, 3, 4});
// 		graph.addNodeLinks<DIRECTED>(0, {1, 2});
// 		graph.addNodeLinks<DIRECTED>(3, {1, 4});
// 		graph.addNodeLinks<DIRECTED>(2, {4});

// 		// Simple hexagon (6)  (8 ~ 6: orig Q0 > 0, dQ0 > 0, Q1 < 0)
// 		graph.addNodes({0, 1, 2, 3, 4, 5});
// 		graph.addNodeLinks<DIRECTED>(0, {1, 2});
// 		graph.addNodeLinks<DIRECTED>(3, {1, 5});
// 		graph.addNodeLinks<DIRECTED>(4, {2, 5});

// 		// Simple decagon (10) (orig Q0, dQ0, Q1 > 0)
// 		GraphT::Ids  ids;
// 		ids.reserve(10);
// 		for(Id i = 0; i < 10; ++i)
//			ids.push_back(i);
// 		graph.addNodes(ids);
// 		graph.addNodeLinks<DIRECTED>(0, {1, 2});
// 		graph.addNodeLinks<DIRECTED>(3, {1, 5});
// 		graph.addNodeLinks<DIRECTED>(4, {2, 6});
// 		graph.addNodeLinks<DIRECTED>(7, {5, 9});
// 		graph.addNodeLinks<DIRECTED>(8, {6, 9});

		// Basic 3x overlapping
		graph.addNodes({0, 1, 2, 3});
		using InpLinkT = GraphT::InpLinkT;
		graph.addNodeLinks<true>(0, {InpLinkT(0, 6)});
		graph.addNodeLinks<true>(1, {InpLinkT(1, 6)});
		graph.addNodeLinks<true>(3, {InpLinkT(3, 6)});
		graph.addNodeLinks<false>(2, {0, 1, 3});

//		// Basic 3x overlapping, another input functions
//		using InpLinkT = GraphT::InpLinkT;
//		GraphT  graph(4);
//		graph.addNodeAndLinks<true>(0, {InpLinkT(0, 6)});
//		graph.addNodeAndLinks<true>(1, {InpLinkT(1, 6)});
//		graph.addNodeAndLinks<true>(3, {InpLinkT(3, 6)});
//		graph.addNodeAndLinks<DIRECTED>(2, {0, 1, 3});
		Client::processNodes(graph.finalize(), !graph.directed(), true, false, -0.999);
	}
}


//! libhigac client
int main(int argc, char* argv[])
{
	Client  client;
	if(client.parseArgs(argc, argv))
		client.process();
	else client.usage(argv[0]);
	return 0;

//	if(argc == 1) {
//		// Predefined testcase
//		// Use nodes specified by default if no input file was specified
//		//testcase();  // "data/corpus_10.txt" testcase("data/c101r2.txt"); "data/corpusu.txt" "data/corpus_109u.txt" // seqgr1.txt, c101r2.txt, corpus_20; seqgr1sw.txt
//		//return 0;
//
//		// TODO: Check stdin and implement JSON processing via it
//		// Use rapidjson C++ lib, string reader for custom parsing:
//		// https://github.com/miloyip/rapidjson/blob/master/doc/sax.md#reader-reader
//		// https://github.com/miloyip/rapidjson/blob/master/doc/tutorial.md#value--document-valuedocument
//		//
//		// Data movement from Python:
//		// response = urllib2.urlopen('https://api.instagram.com/v1/tags/pizza/media/XXXXXX')
//		// # from StringIO import StringIO
//		// # io = StringIO('["streaming API"]')
//		// # json.load(io)
//		// data = json.load(response)
//		// # print data
//		// from subprocess import Popen, PIPE, STDOUT
//		// p = Popen(['grep', 'f'], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
//		// grep_stdout = p.communicate(input='one\ntwo\nthree\nfour\nfive\nsix\n')[0]
//		// # or
//		// p = subprocess.Popen(['grep','f'],stdout=subprocess.PIPE,stdin=subprocess.PIPE)
//		// p.stdin.write('one\ntwo\nthree\nfour\nfive\nsix\n')
//		//
//		// http://stackoverflow.com/questions/163542/python-how-do-i-pass-a-string-into-subprocess-popen-using-the-stdin-argument
//		// http://chimera.labs.oreilly.com/books/1230000000393/ch05.html
//		//
//		// + extend TViSC json format to specify weights type (float / uint_16 / none)
//	}
}
