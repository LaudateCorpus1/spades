#pragma once
#include "sam_reader.hpp"
#include "read.hpp"
#include "include.hpp"
#include "config_struct.hpp"


namespace corrector {
typedef vector<WeightedRead> WeightedReadStorage;

class InterestingPositionProcessor {
	string contig;
	vector<int> is_interesting;
	vector<vector<size_t> > read_ids;
	WeightedReadStorage wr_storage;
//TODO:: init this consts with something more reasonable
	const int anchor_gap = 100;
	const int anchor_num = 6;
//TODO: old formula 1 scores? RECONSIDER REASONABLE INIT

	static const size_t MaxErrorCount = 6;
	//const int error_weight[MaxErrorCount] ={374864, 3853, 1171, 841, 66, 27};
	//const int error_weight[MaxErrorCount] ={100000, 1000, 1000, 1000, 1, 1};
	const int error_weight[MaxErrorCount] ={100, 10, 8, 5, 2, 1};
	unordered_map<size_t, position_description> interesting_weights;
	unordered_map<size_t, position_description> changed_weights;


public:
	InterestingPositionProcessor(){
	}
	void set_contig(string ctg) {
		contig = ctg;
		size_t len = contig.length();
		is_interesting.resize(len);
		read_ids.resize(len);
	}
	unordered_map<size_t, position_description> get_weights() const{
		return changed_weights;
	}

	inline int get_error_weight(size_t i) const {
		if (i >= MaxErrorCount)
			return 0;
		else
			return error_weight[i];
	}

	inline bool IsInteresting(size_t position) const{
		if (position >= contig.length())
			return 0;
		else return is_interesting[position];
	}

	void UpdateInterestingRead(const PositionDescriptionMap &ps);
	void UpdateInterestingPositions();
	size_t FillInterestingPositions(vector<position_description> &charts);

};
};
