//
// Created by lab42 on 8/26/15.
//

#include "extension_chooser2015.hpp"
namespace path_extend {
using namespace std;

int ExtensionChooser2015::CountMedian(vector<pair<int, double> >& histogram) const{
    double dist = 0.0;
    double sum = 0.0;
    double sum2 = 0.0;
    for (size_t j = 0; j< histogram.size(); ++j) {
        sum += histogram[j].second;
    }
    size_t i = 0;
    for (; i < histogram.size(); ++i) {
        sum2 += histogram[i].second;
        if (sum2 * 2 > sum)
            break;
    }
    if (i >= histogram.size()) {
        WARN("Count median error");
        i = histogram.size() - 1;
    }
    return (int) round(histogram[i].first);
}

void ExtensionChooser2015::CountAvrgDists(const BidirectionalPath& path, EdgeId e, std::vector<pair<int, double>> & histogram) const {
    for (int j = (int) path.Size() -1; j >= 0; --j) {
        if (unique_edges_->IsUnique(path.At(j))) {
            std::vector<int> distances;
            std::vector<double> weights;
            GetDistances(path.At(j), e, distances, weights);
            if (distances.size() > 0) {
                AddInfoFromEdge(distances, weights, histogram, g_.length(path.At(j)));
            }
//we are not interested on info over an unique edge now
            break;
        }
    }
}
void ExtensionChooser2015::FindBestFittedEdges(const BidirectionalPath& path, const set<EdgeId>& edges, EdgeContainer& result) const {
    vector<pair<double, pair<EdgeId, int >>> to_sort;
    for (EdgeId e : edges) {
        std::vector <pair<int, double>> histogram;
        CountAvrgDists(path, e, histogram);
        double sum = 0.0;
        for (size_t j = 0; j < histogram.size(); ++j) {
            TRACE(histogram[j].first << " " << histogram[j].second);
            sum += histogram[j].second;
        }
        DEBUG("edge " << g_.int_id(e) << " weight " << sum);
        //TODO reconsider threshold
        if (sum <= cl_weight_threshold_) {
            DEBUG("Edge " << g_.int_id(e)  << " weight "<< sum <<  " failed absolute weight threshold " << cl_weight_threshold_);
            continue;
        }
        int gap = CountMedian(histogram);
        //TODO reconsider condition
        int cur_is = wc_->lib().GetISMax();
        if (gap < cur_is * -1 || gap > cur_is * 2) {
            DEBUG("Edge " << g_.int_id(e)  << " gap "<< gap <<  " failed insert size conditions, IS= " << cur_is);
            continue;
        }

        //Here check about ideal info removed
        to_sort.push_back(make_pair(sum, make_pair(e, gap)));

    }
//descending order, reverse iterators;
    sort(to_sort.rbegin(), to_sort.rend());
    for(size_t j = 0; j < to_sort.size(); j++) {
        if (j == 0 || to_sort[j].first* relative_weight_threshold_ > to_sort[j - 1].first) {
            result.push_back(EdgeWithDistance(to_sort[j].second.first, to_sort[j].second.second));

            DEBUG("Edge " << g_.int_id(to_sort[j].second.first) << " gap " << to_sort[j].second.second << " weight "<< to_sort[j].first <<  " passed absolute weight threshold " << cl_weight_threshold_);
        } else {
            DEBUG ("Edge " << g_.int_id(to_sort[j].second.first) << " weight " << to_sort[j].first << " failed relative weight threshold " << relative_weight_threshold_);
            DEBUG("other removed");
            break;
        }
    }

}
set<EdgeId> ExtensionChooser2015::FindCandidates(const BidirectionalPath& path) const {
    set<EdgeId> jumping_edges;
    //PairedInfoLibraries libs = wc_->getLibs();
//TODO: multiple libs?
    const auto& lib = wc_->lib();
    //for (auto lib : libs) {
        //todo lib (and FindJumpEdges) knows its var so it can be counted there
        int is_scatter = int(math::round(double(lib.GetIsVar()) * is_scatter_coeff_));
        DEBUG("starting..., path.size" << path.Size() );
        DEBUG("is_unique_ size " << unique_edges_->size());
        //insted of commented, just break after first unique
        for (int i = (int) path.Size() - 1; i >= 0/* && path.LengthAt(i) - g_.length(path.At(i)) <=  lib.GetISMax() */; --i) {
            DEBUG("edge ");
            DEBUG(path.At(i).int_id());
            set<EdgeId> jump_edges_i;
            if (unique_edges_->IsUnique(path.At(i))) {
                DEBUG("Is Unique Ok");
                lib.FindJumpEdges(path.At(i), jump_edges_i,
                                   std::max((int)0, (int )g_.length(path.At(i)) - (int) lib.GetISMax() - is_scatter),
//TODO: Reconsider limits
                        //FIXME do we need is_scatter here?
                        //FIXME or just 0, inf?
                                   int(g_.length(path.At(i)) + 2 * lib.GetISMax() + is_scatter),
                                   0);
                DEBUG("Jump edges found");
                for (EdgeId e : jump_edges_i) {
                    if (unique_edges_->IsUnique(e) ) {
                        if ( e == path.At(i) || e->conjugate() == path.At(i)) {
                            DEBUG("skipping info on itself or conjugate " << e.int_id());
                        } else {
                            jumping_edges.insert(e);
                        }
                    }
                }
//we are not interested on info over an unique edge now
                break;
            }
        }
    //}
    DEBUG("found " << jumping_edges.size() << " jump edges");
    return jumping_edges;
}

}
