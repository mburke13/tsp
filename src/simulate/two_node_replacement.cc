#include "simulate/two_node_replacement.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>

#include "solve/solution.h"

using namespace std;

TwoNodeReplacement::~TwoNodeReplacement() {
}

Solution TwoNodeReplacement::GetOriginalSolution() {
	Solution solution;
	ifstream in(GetOriginalSolutionFile());
	if (in.good()) {
		in >> solution.distance;
		unsigned int n =  tsp_solver().graph()->num_nodes();
		solution.path = vector<int>(n, 0);
		for (unsigned int i = 0; i < n; ++i) {
			in >> solution.path[i];
		}
		solution.optimal = true;
	} else {
		solution = tsp_solver().ComputeSolution();
		if (solution.optimal) {
			SaveOriginalSolution(solution);
		}
	}
	in.close();
	return solution;
}

void TwoNodeReplacement::SaveOriginalSolution(const Solution& solution) const {
	ofstream out(GetOriginalSolutionFile());
	if (out.good()) {
		out << fixed << solution.distance << endl;
		for (unsigned int i = 0; i < solution.path.size(); ++i) {
			out << solution.path[i] << endl;
		}
	}
	out.close();
}

void TwoNodeReplacement::RunSimulation(TSP* tsp, ofstream& data_out,
																			 mt19937& random_gen,
																			 ImageGenerator& img_gen,
																			 int img_to_generate, int itr_num,
																			 long seed) {
	data_out << "trial,T,T',T'',T''',seed" << endl;
	tsp->BuildGraph(nearest_int_rounding());
	tsp_solver().set_graph(tsp->graph());
	Solution T(GetOriginalSolution());
	if (!T.optimal) {
		cerr << "Unable to find optimal solution for TSP " << tsp->name() << endl;
		return;
	}
	vector<int> node_list(tsp->dimension());
	iota(node_list.begin(), node_list.end(), 0);
	shuffle(node_list.begin(), node_list.end(), random_gen);
	int i = node_list[0];
	int j = node_list[1];
	for (int k = trials_start(); k < trials_end(); ++k) {
		// BEGIN Image Generation
		int path_node_1 = i;
		vector<pair<double, double>> coordinate_path_1;
		if (k == img_to_generate) {
			Coord** node_coords = tsp->GetNodeCoords();
			coordinate_path_1 = img_gen.GetCoordinatePath(node_coords, tsp->dimension(),
																										T.path, &path_node_1);
		}
		// END Image Generation

		// BEGIN Compute T'
		Coord* original_1 = tsp->ReplaceCoordRandomly(min_coord(), max_coord(),
																								  random_gen, i);
		tsp->BuildGraph(nearest_int_rounding());
		tsp_solver().set_graph(tsp->graph());
		Solution T_prime(tsp_solver().ComputeSolution());
		if (!T_prime.optimal) {
			cerr << "Unable to find optimal solution in trial " << k + 1 << endl;
			continue;
		}
    // END Compute T'

		// BEGIN Image Generation
		int path_node_2 = i;
		vector<pair<double, double>> coordinate_path_2;
		if(k == img_to_generate) {
			Coord** node_coords = tsp->GetNodeCoords();
			coordinate_path_2 = img_gen.GetCoordinatePath(node_coords, tsp->dimension(),
																										T_prime.path, &path_node_2);
			img_gen.GenerateImage("/itr_" + to_string(itr_num) +
														"_trial_" + to_string(k+1) + ".png",
														coordinate_path_1, coordinate_path_2,
														T.path, T_prime.path,
														path_node_1, path_node_2,
														T.distance, T_prime.distance);
		}
    // END Image Generation

		// BEGIN Replace Original City i
		Coord* new_1 = tsp->ReplaceCoord(original_1, i);
		// END Replace Original City i

		// BEGIN Compute T''
		Coord* original_2 = tsp->ReplaceCoordRandomly(min_coord(), max_coord(),
																							 	  random_gen, j);
		tsp->BuildGraph(nearest_int_rounding());
		tsp_solver().set_graph(tsp->graph());
		Solution T_double_prime(tsp_solver().ComputeSolution());
		if (!T_double_prime.optimal) {
			cerr << "Unable to find optimal solution in trial " << k + 1 << endl;
			continue;
		}
		// END Compute T''

		// BEGIN Replace New City i
		tsp->ReplaceCoord(new_1, i);
		// END Replace New City i

		// BEGIN Compute T'''
		tsp->BuildGraph(nearest_int_rounding());
		tsp_solver().set_graph(tsp->graph());
		Solution T_triple_prime(tsp_solver().ComputeSolution());
		if (!T_triple_prime.optimal) {
			cerr << "Unable to find optimal solution in trial " << k + 1 << endl;
			continue;
		}
		// END Compute T'''

		data_out << fixed << k + 1 << ',' << T.distance << ',' << T_prime.distance << ','
		         << T_double_prime.distance << ',' << T_triple_prime.distance << ','
						 << seed << endl;

		// BEGIN Cleanup
		delete tsp->ReplaceCoord(original_1, i);
		delete tsp->ReplaceCoord(original_2, j);
		// END Cleanup
	}
}
