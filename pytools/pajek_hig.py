#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
\descr: Pajek to HiReCS format convertor

Pajek format: http://gephi.github.io/users/supported-graph-formats/pajek-net-format/
.hig format: http://www.lumais.com/docs/hig_format.hig

(c) HiReCS (High Resolution Hierarchical Clustering with Stable State library)
\author: Artem Lutov <luart@ya.ru>
\organizations: eXascale lab <http://exascale.info/>, ScienceWise <http://sciencewise.info/>, Lumais <http://www.lumais.com/>
\date: 2015-10
"""

import sys

def outName(finpName):
	"""Returns output filename by input filename"""
	i = finpName.rfind('.')
	if i != -1:
		finpName = finpName[0:i]
	return finpName + '.hig'

def formGraphHeader(fout, args):
	"""Parse command line arguments and write Graph header section"""
	weighted = True
	resdub = False  # Resolve duplications
	if args:
		assert isinstance(args, (tuple, list))
		normalize = False
		# Parce args
		for marg in args:
			if marg[0] != '-':
				raise ValueError('Unexpected argument: ' + args[0])
			for arg in marg[1:]:
				if arg == 's':
					weighted = False
				elif arg == 'n':
					normalize = True
				elif arg == 'r':
					resdub = True
				else:
					raise ValueError('Unexpected argument: ' + arg)
		fout.write("/Graph weighted:{0} normalize:{1}\n\n".format(int(weighted), int(normalize)))
	return weighted, resdub

def saveNodes(fout, vertNum):
	"""Write nodes header section (nodes number) to the .hgc file
		/Nodes [<nodes_number>  [<start_id>]]
		start_id is always 1 for pajek format
	"""
	fout.write('/Nodes {0} 1\n\n'.format(vertNum))

def parseLink(link, weighted):
	"""Parse single link in Pajek format
	return (dest_id, weight)
	"""
	link = link.split()
	if not link or len(link) > 2:
		raise ValueError(' '.join(('Invalid format of the link specification:', link)))
	weight = '1'
	if weighted and len(link) > 1:
		weight = link[1]
	return (link[0], weight)

def parseLinks(links, weighted):
	"""Parse node links in Pajek format
	return [(dest_id, weight), ...]
	"""
	links = links.split()
	if weighted:
		links = [(v, 1) for v in links]
	return links

def saveLinks(fout, links, weighted):
	"""Save links to the current section"""
	for ndlinks in links.items():
		val = ndlinks[1]
		# Consider that duplicates might were resolved
		if isinstance(val, dict):
			val = val.items()

		if weighted:
			text = ' '.join([':'.join(v) for v in val])
		else:
			# Skip weights
			text = ' '.join([v[0] for v in val])
		fout.write('{0}> {1}\n'.format(ndlinks[0], text))

def pajekToHgc(finpName, *args):
	"""Convert Pajek file into the HiReCS input file format line by line,
	processing edges and arcs Pajek section as a single line in the .hgc
	"""
	#vertNum, edges, arcs = loadPajek()
	#saveHgc(foutName(finpName), vertNum, edges, arcs)
	with open(finpName, 'r') as finp:
		print('File {0} is opened, converting...'.format(finpName))
		# Create output file
		foutName = outName(finpName)
		with open(foutName, 'w') as fout:
			print('File {0} is created, filling...'.format(foutName))
			fout.write('# Converted from {0}\n'.format(finpName))
			weighted, resdub = formGraphHeader(fout, args)
			# Sections Enumeration
			SECT_NONE = 0
			SECT_VRTS = 1
			SECT_EDGS = 2  # Single edge  per line
			SECT_ARCS = 3
			SECT_EDGL = 4  # Edges list
			SECT_ARCL = 5  # Arcs list

			def sectionName(sect):
				if sect == SECT_NONE:
					return '<NONE>'
				elif sect == SECT_VRTS:
					return 'vertices'
				elif sect == SECT_EDGS:
					return 'edges'
				elif sect == SECT_ARCS:
					return 'arcs'
				elif sect == SECT_EDGL:
					return 'edgeslist'
				elif sect == SECT_ARCL:
					return 'arcslist'
				else:
					return '<UNDEFINED>'

			sect = SECT_NONE
			vertNum = None  # Number of verteces
			links = {}  # {src: [(dst, weight), ...]}
			arcs = {}  # Selflinks in case of Edges processing

			# Outpurt sections
			OSECT_NONE = 0
			OSECT_NODES = 1
			OSECT_EDGS = 1
			OSECT_ARCS = 1
			outsect = OSECT_NONE

			for ln in finp:
				# Skip comments
				ln = ln.lstrip()
				if not ln or ln.startswith('%'):
					continue
				# Process content
				if ln[0] != '*':
					# Process section body
					if sect == SECT_NONE:
						raise SyntaxError('Invalid file format: section header is expected')
					elif sect == SECT_VRTS:
						continue  # Skip vertices annotations
					else:
						# Body of the links section
						ln = ln.split(None, 1)
						if len(ln) < 2:
							raise SyntaxError(''.join(('At least 2 ids are expected in the "'
								, sectionName(sect), '" section items: ', ln)))
						node = int(ln[0])
						if sect == SECT_EDGS or sect == SECT_ARCS:
							#print('links: ', links)
							ndlinks = links.get(node, [] if not resdub else {})
							link = parseLink(ln[1], weighted)
							if sect == SECT_ARCS or link[0] != ln[0]:
								if not resdub:
									ndlinks.append(parseLink(ln[1], weighted))
								else:
									ndlinks[link[0]] = link[1]
							else:
								arcs[node] = (link,)  # Make a tuple
							if ndlinks:
								links[node] = ndlinks
						elif sect == SECT_EDGL or sect == SECT_ARCL:
							saveLinks(fout, {node: parseLinks(ln[1], weighted)}, weighted)
						else:
							raise RuntimeError(''.join(('Logical error: unexpected "'
								, sectsectionName(sect), '" section')))
				else:
					# Process section header
					ln = ln[1:].strip().split(None, 2)
					if not ln:
						raise ValueError('Invalid section name (empty)')
					sectName = ln[0].lower()
					if sect == SECT_NONE and sectName != 'vertices':
						raise ValueError(''.join(('Invalid section: "', sectName
							, '", "vertices" is expected')))
					elif sect != SECT_NONE and sectName == 'vertices':
						raise ValueError('Invalid section, "vertices" is not expected')
					else:
						# Save parced data if required
						if links:
							if sect == SECT_EDGS or sect == SECT_ARCS:
								saveLinks(fout, links, weighted)
							else:
								raise RuntimeError(''.join(('Logical error: unsaved data in "'
									, sectsectionName(sect), '" section')))
							links = {}
						# Set working section
						if sectName == 'vertices':
							sect = SECT_VRTS
							#if len(ln) > 1:
							try:
								vertNum = int(ln[1])
							except (ValueError, IndexError):
								raise SyntaxError('Number of vertices must be specified')
							saveNodes(fout, vertNum)
						elif sectName == 'edges':
							fout.write('\n/Edges\n')
							sect = SECT_EDGS
						elif sectName == 'arcs':
							fout.write('\n/Arcs\n')
							sect = SECT_ARCS
						elif sectName == 'edgeslist':
							fout.write('\n/Edges\n')
							sect = SECT_EDGL
						elif sectName == 'arcslist':
							fout.write('\n/Arcs\n')
							sect = SECT_ARCL
						else:
							raise ValueError('Unexpected section: ' + sectName)
			# Save remained parced data if required
			if links:
				if sect == SECT_EDGS or sect == SECT_ARCS:
					saveLinks(fout, links, weighted)
				else:
					raise RuntimeError(''.join(('Logical error: unsaved data in "'
						, sectsectionName(sect), '" section')))
				links = {}
			if arcs:
				fout.write('\n/Arcs\n')
				saveLinks(fout, arcs, weighted)
			print('Data is converted')


if __name__ == '__main__':
	if len(sys.argv) > 1:
		pajekToHgc(*sys.argv[1:])
	else:
		print('\n'.join(('Usage: {0} <pajek_filename> [-rsn]',
			'  -r  - resolve duplicated links from the .pjk',
			'  -s  - simple format of links (unweighted graph)',
			'  -n  - perform links normalization on processing'))
			.format(sys.argv[0]))
