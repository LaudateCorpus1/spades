//***************************************************************************
//* Copyright (c) 2011-2013 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

/*
 * pe_io.hpp
 *
 *  Created on: Mar 15, 2012
 *      Author: andrey
 */

#ifndef PE_IO_HPP_
#define PE_IO_HPP_


#include "bidirectional_path.hpp"
#include "io/osequencestream.hpp"

namespace path_extend {

using namespace debruijn_graph;

class ContigWriter {
protected:
    DECL_LOGGER("PathExtendIO")

protected:
	const Graph& g_;

    size_t k_;

	string ToString(const BidirectionalPath& path) const{
		stringstream ss;
		if (!path.Empty()) {
			ss <<g_.EdgeNucls(path[0]).Subseq(0, k_).str();
		}

		for (size_t i = 0; i < path.Size(); ++i) {
			int gap = i == 0 ? 0 : path.GapAt(i);
			if (gap > (int) k_) {
				for (size_t j = 0; j < gap - k_; ++j) {
					ss << "N";
				}
				ss << g_.EdgeNucls(path[i]).str();
			} else {
				int overlapLen = (int) k_ - gap;
				if (overlapLen >= (int) g_.length(path[i]) + (int) k_) {
					continue;
				}

				ss << g_.EdgeNucls(path[i]).Subseq(overlapLen).str();
			}
		}
		return ss.str();
	}

    Sequence ToSequence(const BidirectionalPath& path) const {
        SequenceBuilder result;

        if (!path.Empty()) {
            result.append(g_.EdgeNucls(path[0]).Subseq(0, k_));
        }
        for (size_t i = 0; i < path.Size(); ++i) {
            result.append(g_.EdgeNucls(path[i]).Subseq(k_));
        }

        return result.BuildSequence();
    }

    void MakeIDS(PathContainer& paths,
                 map<BidirectionalPath*, string >& ids,
                 map<BidirectionalPath*, vector<string> >& next_ids) const {
        int counter = 1;
        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
            if (iter.get()->Size() == 0)
                continue;

            BidirectionalPath* p = iter.get();
            BidirectionalPath* cp = iter.getConjugate();
            string name = io::MakeContigId(counter++, p->Length() + k_, p->Coverage(), p->GetId());
            ids.insert(make_pair(p, name));
            ids.insert(make_pair(cp, name + "'"));
            next_ids.insert(make_pair(p, vector<string>()));
            next_ids.insert(make_pair(cp, vector<string>()));
        }
    }

    void FindPathsOrder(PathContainer& paths, multimap<VertexId, BidirectionalPath*>& starting) const {
        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
            if (iter.get()->Size() == 0)
                            continue;

            BidirectionalPath* path = iter.get();
            DEBUG(g_.int_id(g_.EdgeStart(path->Front())) << " -> " << path->Size() << ", " << path->Length());
            starting.insert(make_pair(g_.EdgeStart(path->Front()), path));

            path = iter.getConjugate();
            DEBUG(g_.int_id(g_.EdgeStart(path->Front())) << " -> " << path->Size() << ", " << path->Length());
            starting.insert(make_pair(g_.EdgeStart(path->Front()), path));
        }
    }

    void VerifyIDS(PathContainer& paths,
                 map<BidirectionalPath*, string >& ids,
                 map<BidirectionalPath*, vector<string> >& next_ids,
                 multimap<VertexId, BidirectionalPath*>& starting) const {

        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
            if (iter.get()->Size() == 0)
                continue;

            BidirectionalPath* p = iter.get();
            VertexId v = g_.EdgeEnd(p->Back());
            size_t count = 0;
            DEBUG("Node " << ids[p] << " is followed by: ");
            for (auto v_it = starting.lower_bound(v); v_it != starting.upper_bound(v); ++v_it) {
                DEBUG("Vertex: " << ids[v_it->second]);
                ++count;
                auto it = find(next_ids[p].begin(), next_ids[p].end(), ids[v_it->second]);

                VERIFY(it != next_ids[p].end());
            }
            for (auto it = next_ids[p].begin(); it != next_ids[p].end(); ++it) {
                DEBUG("Next ids: " << *it);
            }
            VERIFY(count == next_ids[p].size());

            p = iter.getConjugate();
            v = g_.EdgeEnd(p->Back());
            count = 0;
            DEBUG("Node " << ids[p] << " is followed by: ");
            for (auto v_it = starting.lower_bound(v); v_it != starting.upper_bound(v); ++v_it) {
                DEBUG("Vertex: " << ids[v_it->second]);
                ++count;
                auto it = find(next_ids[p].begin(), next_ids[p].end(), ids[v_it->second]);
                VERIFY(it != next_ids[p].end());
            }
            for (auto it = next_ids[p].begin(); it != next_ids[p].end(); ++it) {
                DEBUG("Next ids: " << *it);
            }
            VERIFY(count == next_ids[p].size());
        }
    }

    void ConstructFASTG(PathContainer& paths,
            map<BidirectionalPath*, string >& ids,
            map<BidirectionalPath*, vector<string> >& next_ids) const {

        MakeIDS(paths, ids, next_ids);

        multimap<VertexId, BidirectionalPath*> starting;
        //set<VertexId> visited;
        //queue<BidirectionalPath*> path_queue;
        FindPathsOrder(paths, starting);

        DEBUG("RESULT");
        for (auto it = starting.begin(); it != starting.end(); ++it){
            BidirectionalPath* path = it->second;
            DEBUG(g_.int_id(it->first) << " -> " << path->Size() << ", " << path->Length());
        }

        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
            if (iter.get()->Size() == 0)
                            continue;

            BidirectionalPath* path = iter.get();
            VertexId v = g_.EdgeEnd(path->Back());
            DEBUG("FINAL VERTEX " << g_.int_id(v));
            DEBUG("Node " << ids[path] << " is followed by: ");

            for (auto v_it = starting.lower_bound(v); v_it != starting.upper_bound(v); ++v_it) {
                BidirectionalPath* next_path = v_it->second;
                DEBUG(ids[next_path]);
                next_ids[path].push_back(ids[next_path]);
            }

            path = iter.getConjugate();
            v = g_.EdgeEnd(path->Back());
            DEBUG("FINAL VERTEX " << g_.int_id(v));
            DEBUG("Node " << ids[path] << " is followed by: ");

            for (auto v_it = starting.lower_bound(v); v_it != starting.upper_bound(v); ++v_it) {
                BidirectionalPath* next_path = v_it->second;
                DEBUG(ids[next_path]);
                next_ids[path].push_back(ids[next_path]);
            }
        }

        VerifyIDS(paths, ids, next_ids, starting);
    }


public:
    ContigWriter(const Graph& g): g_(g), k_(g.k()){

    }

    void writeEdges(const string& filename) const {
        INFO("Outputting edges to " << filename);
        io::osequencestream_with_data_for_scaffold oss(filename);

        set<EdgeId> included;
        for (auto iter = g_.ConstEdgeBegin(); !iter.IsEnd(); ++iter) {
            if (included.count(*iter) == 0) {
                oss.setCoverage(g_.coverage(*iter));
                oss.setID((int) g_.int_id(*iter));
                oss << g_.EdgeNucls(*iter);

                included.insert(*iter);
                included.insert(g_.conjugate(*iter));
            }
        }
        DEBUG("Contigs written");
    }


    void writePathEdges(PathContainer& paths, const string& filename) const {
		INFO("Outputting path data to " << filename);
		std::ofstream oss;
        oss.open(filename.c_str());
        int i = 1;
        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
			oss << i << endl;
			i++;
            BidirectionalPath* path = iter.get();
            if (path->GetId() % 2 != 0) {
                path = path->GetConjPath();
            }
            oss << "PATH " << path->GetId() << " " << path->Size() << " " << path->Length() + k_ << endl;
            for (size_t j = 0; j < path->Size(); ++j) {
			    oss << g_.int_id(path->At(j)) << " " << g_.length(path->At(j)) << endl;
            }
            oss << endl;
		}
		oss.close();
		DEBUG("Edges written");
	}

    void writePaths(PathContainer& paths, const string& filename) const {

        INFO("Writing contigs to " << filename);
        io::osequencestream_with_data_for_scaffold oss(filename);
        int i = 0;
        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
        	if (iter.get()->Length() <= 0){
        		continue;
        	}
        	DEBUG("NODE " << ++i);
            BidirectionalPath* path = iter.get();
            if (path->GetId() % 2 != 0) {
                path = path->GetConjPath();
            }
            path->Print();
        	oss.setID((int) path->GetId());
            oss.setCoverage(path->Coverage());
            oss << ToString(*path);
        }
        DEBUG("Contigs written");
    }

    void WritePathsToFASTG(PathContainer& paths, const string& filename, const string& fastafilename) const {
        map<BidirectionalPath*, string > ids;
        map<BidirectionalPath*, vector<string> > next_ids;

        INFO("Constructing FASTG file from paths " << filename);
        ConstructFASTG(paths, ids, next_ids);

        INFO("Writing contigs in FASTG to " << filename);
        INFO("Writing contigs in FASTQ to " << fastafilename);
        io::osequencestream_for_fastg fastg_oss(filename);
        io::osequencestream_with_id oss(fastafilename);
        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
            BidirectionalPath* path = iter.get();
            if (path->Length() <= 0){
                continue;
            }
            DEBUG(ids[path]);

            oss.setID((int) path->GetId());
            oss.setCoverage(path->Coverage());
            oss << ToString(*path);
            fastg_oss.set_header(ids[path]);
            fastg_oss << next_ids[path] << ToString(*path);
        }
    }
};


class PathInfoWriter {
protected:
    DECL_LOGGER("PathExtendIO")


public:

    void writePaths(PathContainer& paths, const string& filename){
        std::ofstream oss(filename.c_str());

        for (auto iter = paths.begin(); iter != paths.end(); ++iter) {
            iter.get()->Print(oss);
        }

        oss.close();
    }
};

}

#endif /* PE_IO_HPP_ */
