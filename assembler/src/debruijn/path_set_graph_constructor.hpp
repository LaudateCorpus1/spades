#pragma once
#include "matepair_transformer.hpp"
#include "path_set.hpp"
#include "new_debruijn.hpp"
#include "graph_pack.hpp"
#include "utils.hpp"
#include "omni/omni_utils.hpp"

#include "omni/omni_tools.hpp"
#include "omni/omnigraph.hpp"

#include "omni/edges_position_handler.hpp"
#include "omni/total_labeler.hpp"
#include "path_set_stats.hpp"
#include "path_set_tools.hpp"
namespace debruijn_graph{
template <class graph_pack>
class PathSetGraphConstructor {

typedef typename graph_pack::graph_t::EdgeId EdgeId;
typedef typename graph_pack::graph_t::VertexId VertexId;
typedef typename graph_pack::graph_t Graph;
typedef vector<EdgeId > Path;
const Graph& g_;
const PairedInfoIndex<Graph>& pair_info_;

public:
PathSetGraphConstructor(graph_pack& gp, PairedInfoIndex<Graph>& clustered_index, graph_pack& new_gp): g_(gp.g), pair_info_(clustered_index) {
	PathSetIndexData<EdgeId> PII ;
	PathSetIndexData<EdgeId> PIIFilter ;

	MatePairTransformer<Graph> transformer(g_, pair_info_);
	transformer.Transform(PII);
	PathSetIndex<EdgeId> PI(PII);
//	PI.RemovePrefixes(PIIFilter_tmp);
	PI.Process(PIIFilter);


	for(auto iter = PII.begin(); iter != PII.end() ; ++iter)
	{
		DEBUG(str(*iter, gp));
//		DEBUG(tst());
	}
	DEBUG("FILTERED");
	int count = 0;

//	map<VertexId, int> long_start_vertices;
	for(auto iter = PIIFilter.begin(); iter != PIIFilter.end() ; ++iter)
	{
//		if ((old_vertices.find(gp.gp.g.EdgeEnd(iter->start)) != old_vertices.end()) && gp.gp.g.length(iter->start) > cfg::get().ds.IS) {
//			real_id.insert(make_pair(iter->id, iter->id));
//		} else
		{
			VertexId v = new_gp.g.AddVertex();
			new_gp.int_ids.AddVertexIntId(v, iter->id);
//			real_id.insert(make_pair(iter->id, iter->id));
//			old_vertices.insert(make_pair(gp.gp.g.EdgeEnd(iter->start), iter->id));
		}
	}
	map<EdgeId, int> long_starts;
	map<int, int> real_ids;
	for(auto iter = PIIFilter.begin(); iter != PIIFilter.end() ; ++iter)
	{
		if (long_starts.find(iter->start) == long_starts.end()) {
			if (iter->length - gp.g.length(iter->end) < cfg::get().ds.IS ) {
				real_ids.insert(make_pair(iter->id, iter->id));
			} else {
				long_starts.insert(make_pair(iter->start, iter->id));
				real_ids.insert(make_pair(iter->id, iter->id));
			}
		} else {
			real_ids.insert(make_pair(iter->id, long_starts[iter->start]));
			new_gp.g.DeleteVertex(new_gp.int_ids.ReturnVertexId(iter->id));
		}
	}
	DEBUG("PahtSetNumber is "<< PIIFilter.size());
	for(auto iter = PIIFilter.begin(); iter != PIIFilter.end() ; ++iter)
	{
		DEBUG(str(*iter, gp));
	}
	map<EdgeId, EdgeId> new_to_old;
	map<EdgeId, double> weight_sums;
	for(auto iter = PIIFilter.begin(); iter != PIIFilter.end() ; ++iter)
	{
		PathSet<EdgeId> first = *iter;
		EdgeId old_first_edge = iter->start;
		if (weight_sums.find(old_first_edge) == weight_sums.end())
			weight_sums.insert (make_pair(old_first_edge, 0));
		else {
			INFO(weight_sums[old_first_edge] <<" " <<first.weight <<" "<< gp.g.length(old_first_edge));
		}
		vector<PathSet<EdgeId>> extends;
		PI.FindExtension(PIIFilter,first, extends);
		if (extends.size() == 0){
			auto current_path = first.paths.begin();
			for(auto path_iter = current_path->begin(); path_iter != current_path->end(); ++path_iter) {
				old_first_edge = * path_iter;
				if (weight_sums.find(old_first_edge) == weight_sums.end())
					weight_sums.insert (make_pair(old_first_edge, 0));
				else {
					INFO(weight_sums[old_first_edge] <<" " <<first.weight)
				}
				weight_sums[old_first_edge] += first.weight;
			}
			old_first_edge = first.end;
			if (weight_sums.find(old_first_edge) == weight_sums.end())
				weight_sums.insert (make_pair(old_first_edge, 0));
			weight_sums[old_first_edge] += first.weight;
		} else {
			vector<PathSet<EdgeId>> extends;
			PI.FindExtension(PIIFilter,first, extends);
			for(size_t i = 0; i < extends.size(); i++)
			weight_sums[old_first_edge] += extends[i].weight;
		}
	}

	for(auto iter = PIIFilter.begin(); iter != PIIFilter.end() ; ++iter)
	{
		PathSet<EdgeId> first = *iter;
		vector<PathSet<EdgeId>> extends;
		PI.FindExtension(PIIFilter,first, extends);
		VertexId new_start = new_gp.int_ids.ReturnVertexId(real_ids[iter->id]);
		EdgeId old_first_edge = iter->start;
		if (first.weight / weight_sums[old_first_edge] < 0.9)
				DEBUG("low covered");
		DEBUG ("path-set numero " << first.id<< " has "<< extends.size()<<"extensions: ");
		for(size_t i = 0; i < extends.size(); i++) {
			DEBUG("to pathset "<< extends[i].id << " weight "<< extends[i].weight);
			VertexId new_end = new_gp.int_ids.ReturnVertexId(real_ids[extends[i].id]);
			DEBUG("adding edge from" << new_gp.int_ids.ReturnIntId(new_start) << " to " << new_gp.int_ids.ReturnIntId(new_end) << " of length " << gp.g.length(old_first_edge) <<" and coverage "<< gp.g.coverage(old_first_edge) << " * " << extends[i].weight / weight_sums[old_first_edge]);
			bool flag = true;
			vector<EdgeId> out_e = new_gp.g.OutgoingEdges(new_start);
			for (auto eid = out_e.begin(); eid != out_e.end(); ++eid){
				if (new_to_old[*eid] == old_first_edge && new_gp.g.EdgeEnd(*eid)== new_end) {
					//TODO: what's about coverage?
					flag = false;
					break;
				}
			}
			if ( (!flag)/* && real_ids[first.id] != first.id*/) {
				DEBUG("ignoring clone to pathset " << extends[i].id << " and vertex " << real_ids[extends[i].id]);
			} else {
				EdgeId eid = new_gp.g.AddEdge(new_start, new_end, gp.g.EdgeNucls(old_first_edge));
				WrappedSetCoverage(new_gp.g, eid, (int) (gp.g.coverage(old_first_edge) * gp.g.length(old_first_edge) *   extends[i].weight / weight_sums[old_first_edge]));
				new_gp.edge_pos.AddEdgePosition(eid, gp.edge_pos.edges_positions().find(old_first_edge)->second);
				new_to_old.insert(make_pair(eid, old_first_edge));
				DEBUG("count was "<< count);
//		    omnigraph::WriteSimple(new_gp.g, tot_labeler_after, cfg::get().output_dir  + ToString(count)+".dot", "no_repeat_graph");
				count ++ ;
			}
		}
		if (extends.size() == 0){
			VertexId new_end = new_gp.g.AddVertex();
			DEBUG("adding edge from" << new_gp.int_ids.ReturnIntId(new_start) << " to " << new_gp.int_ids.ReturnIntId(new_end) << " of length " << gp.g.length(old_first_edge) << " and coverage"<< gp.g.coverage(old_first_edge) /*<< " * "  << first.weight / weight_sums[old_first_edge]*/);

			old_first_edge = first.start;
			EdgeId eid = new_gp.g.AddEdge(new_start, new_end, gp.g.EdgeNucls(old_first_edge));
			WrappedSetCoverage(new_gp.g, eid, (int) (gp.g.coverage(old_first_edge) * gp.g.length(old_first_edge) /** first.weight / weight_sums[old_first_edge]*/));
			new_gp.edge_pos.AddEdgePosition(eid, gp.edge_pos.edges_positions().find(old_first_edge)->second);
			new_to_old.insert(make_pair(eid, old_first_edge));
			new_start = new_end;
			count++;

			if (first.paths.size() != 1 ) {
				DEBUG ("can not select between different tails:(");
			} else {
				DEBUG("singleton dead end!");
				auto current_path = first.paths.begin();
				DEBUG(current_path->size());
			//	DEBUG(first);


				for(auto path_iter = current_path->begin(); path_iter != current_path->end(); ++path_iter) {

					VertexId new_end = new_gp.g.AddVertex();
					old_first_edge = * path_iter;

					DEBUG("adding edge from" << new_gp.int_ids.ReturnIntId(new_start) << " to " << new_gp.int_ids.ReturnIntId(new_end) << " of length " << gp.g.length(old_first_edge) << " and coverage"<< gp.g.coverage(old_first_edge) /*<< " * "  << first.weight / weight_sums[old_first_edge]*/);

					EdgeId eid = new_gp.g.AddEdge(new_start, new_end, gp.g.EdgeNucls(old_first_edge));
					WrappedSetCoverage(new_gp.g, eid, (int) (gp.g.coverage(old_first_edge) * gp.g.length(old_first_edge)));
					new_gp.edge_pos.AddEdgePosition(eid, gp.edge_pos.edges_positions().find(old_first_edge)->second);
					new_to_old.insert(make_pair(eid, old_first_edge));
					new_start = new_end;
					count++;

				}
				VertexId new_end = new_gp.g.AddVertex();
				old_first_edge = first.end;
				DEBUG("adding edge from" << new_gp.int_ids.ReturnIntId(new_start) << " to " << new_gp.int_ids.ReturnIntId(new_end) << " of length " << gp.g.length(old_first_edge));


				EdgeId eid = new_gp.g.AddEdge(new_start, new_end, gp.g.EdgeNucls(old_first_edge));
				WrappedSetCoverage(new_gp.g, eid, (int) (gp.g.coverage(old_first_edge)  * gp.g.length(old_first_edge) /** first.weight / weight_sums[old_first_edge] */));
				new_gp.edge_pos.AddEdgePosition(eid, gp.edge_pos.edges_positions().find(old_first_edge)->second);
				new_to_old.insert(make_pair(eid, old_first_edge));
				new_start = new_end;
				DEBUG("and tail of length "<< iter->length);
//				omnigraph::WriteSimple(new_gp.g, tot_labeler_after, cfg::get().output_dir  + ToString(count)+".dot", "no_repeat_graph");
				count++;
			}
		}
	}
	DEBUG(count);
	INFO("adding isolated edges");

	for(auto iter = gp.g.SmartEdgeBegin(); !iter.IsEnd(); ++iter) {
		VertexId start = gp.g.EdgeStart(*iter);
		VertexId end = gp.g.EdgeEnd(*iter);
		TRACE (gp.g.CheckUniqueOutgoingEdge(start)<<" "<<  gp.g.IsDeadStart(start) <<" "<< gp.g.CheckUniqueIncomingEdge(end) <<" "<<gp.g.IsDeadEnd(end));
		if (gp.g.CheckUniqueOutgoingEdge(start) && gp.g.IsDeadStart(start) && gp.g.CheckUniqueIncomingEdge(end) && gp.g.IsDeadEnd(end) ) {
			VertexId new_start = new_gp.g.AddVertex();
			VertexId new_end = new_gp.g.AddVertex();
			EdgeId old_first_edge = *iter;
			DEBUG("adding isolated edge from" << new_gp.int_ids.ReturnIntId(new_start) << " to " << new_gp.int_ids.ReturnIntId(new_end) << " of length " << gp.g.length(old_first_edge));
			EdgeId eid = new_gp.g.AddEdge(new_start, new_end, gp.g.EdgeNucls(old_first_edge));
			WrappedSetCoverage(new_gp.g, eid, (int) (gp.g.coverage(old_first_edge)  * gp.g.length(old_first_edge) /** first.weight / weight_sums[old_first_edge] */));
			new_gp.edge_pos.AddEdgePosition(eid, gp.edge_pos.edges_positions().find(old_first_edge)->second);
			new_to_old.insert(make_pair(eid, old_first_edge));
		}
	}
}


};
}
