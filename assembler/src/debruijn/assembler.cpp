#include <cassert>
#include <iostream>
#include <list>
#include <cstdio>
#include <vector>
#include <ctime>
#include <array>
#include <string>
#include "../parser.hpp"
#include "debruijn.hpp"

using namespace std;

pair<string,string> filenames = make_pair("./data-short/s_6_1.fastq.gz", "./data-short/s_6_2.fastq.gz");

#define MPSIZE 100
#define K 11


bool seq_test() {
	Seq<10,long long> s("AACCGGTTAA");
	cerr << s.str() << endl;
	if (s.str() != "ACGTACGTAC") {
		cerr << s.str();
		return false;
	}
	assert((s << 'A').str() == "CGTACGTACA");
	assert((s << 'T').str() == "CGTACGTACT");
	assert(s.tail<9>().str() == "CGTACGTAC");
	assert(s.head<9>().str() == "ACGTACGTA");
	return true;
}

int main(int argc, char *argv[]) {

	std::cerr << "Hello, I am assembler!" << std::endl;

	time_t now = time(NULL);

	seq_test();
	// simple de Bruijn graph
	DeBruijn<K> graph;
	// start parsing...
	FASTQParser<MPSIZE>* fqp = new FASTQParser<MPSIZE>();
	fqp->open(filenames.first, filenames.second);
	int cnt = 0;
	while (!fqp->eof()) {
		MatePair<MPSIZE> mp = fqp->read(); // is it copy? :)
		if (mp.id != -1) { // don't have 'N' in reads
			Seq<K> head = mp.seq1.head<K>();
			Seq<K> tail;
			for (size_t i = K; i < MPSIZE; ++i) {
				tail = head << mp.seq1[i];
				graph.addEdge(head, tail);
				head = tail;
			}
			cnt++;
		}
	}
	cout << "Total nodes: " << graph.size() << endl;
	cout << "seconds: " << (time(NULL) - now) << endl;
	fqp->close();

	return 0;
}
