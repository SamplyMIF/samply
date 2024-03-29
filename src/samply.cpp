#include "defines.h"
#include "utils.h"
#include "input.h"

#include "elementdb.h"
#include "solutiondb.h"

int main(int argc, char **argv) {
	// start program
	std::chrono::high_resolution_clock::time_point start_time, current_time;
	start_time = std::chrono::high_resolution_clock::now();

	ElementDB::init();
	SolutionDB sdb;

	Input in(argc, argv);

	auto r = in().r;
	auto rdb = in().rdb;

	for (auto mp : in.matrix()) {
		Eigen::FullPivLU<Eigen::MatrixXd> lu;

		lu.compute(mp.first);
		auto K = lu.kernel();
		auto S = lu.solve(mp.second);
		auto res = mp.first * S;

		if (in().debug) {
			std::cout << "A:\n" << mp.first << std::endl;
			std::cout << "B:\n" << mp.second << std::endl;
			std::cout << "K:\n" << K << std::endl;
			std::cout << "S:\n" << S << std::endl;
		}
		

		if (res.isApprox(mp.second))
			std::cout << "OK" << std::endl;
		else
			continue;

		int num_zeroes = 0;
		int num_negs = 0;
		for (int i = 0; i < K.size(); ++i) {
			if (K(i) == 0)
				num_zeroes++;
			else if(K(i) < 0) {
				num_negs++;
			}
		}
		bool kernel_bad = false;
		if (num_zeroes == K.size()) {
			kernel_bad = true;
		}

		bool first_bad = false;
		for (int i = 0; i < S.size(); ++i) {
			if (S(i) < 0)
				first_bad = true;
		}
		if (first_bad == false)
			sdb.insert(S);

		if (in().debug == true) {
			std::cout << "Ins: " << in().cmd_input_stoichs << std::endl;
			std::cout << "Inp: " << in().cmd_input_precursors << std::endl;
			std::cout << "Kernel health: " << kernel_bad << std::endl;
			std::cout << "Solution health: " << !first_bad << std::endl;
		}
			 
		if (kernel_bad == false || in().mode == 1) {
			in.validate_weights(static_cast<int>(K.cols()), num_negs);
			auto weights = in.load(WEIGHTSCACHEPATH);

			//std::cout << "Before weights loop" << std::endl;
			for (auto &ps : weights) {
				Eigen::VectorXd p(S.size());
				//auto p = nS;
				for (int i = 0; i < S.size(); ++i) {
					//double x = S(i);
					p(i) = S(i);// math::round<double>(x);
					//std::cout << S(i) << " ";
				}
				//std::cout << std::endl;
				for (int i = 0; i < K.cols(); ++i) {
					p += ps[i] * K.col(i);
				}
				//check for negative solutions
				bool negative = false;
				for (int i = 0; i < p.size(); ++i) {
					if (p(i) < 0)
						negative = true;
					//std::cout << p(i) << " ";
				}
				//std::cout << std::endl;
				//std::cout << std::endl;
				if (negative == false) {
					sdb.insert(p);
				}
					
			}
		}
		//std::cout << S.size() << std::endl;
	}
	
	current_time = std::chrono::high_resolution_clock::now();
	auto clockval = std::chrono::duration<double>(current_time - start_time).count();
	std::cout << "DB size: " << sdb().size() << std::endl;
	std::cout << ">> Points computed in: " << clockval << "s" << std::endl << std::endl;


	if (sdb().size() > 0) {
		std::vector<int> ranks;
		for (auto s : sdb()) {
			ranks.push_back(s.second.score());
		}
		std::sort(ranks.begin(), ranks.end());
		std::cout << "Best score is: " << ranks[0] << std::endl;

		// trim rest of unnecessary points
		if (sdb().size() > in().samples)
			sdb.trim(ranks[in().samples]);

		sdb.exportcsv(r, rdb, in());
	}
	else {
		std::cout << "No solutions" << std::endl;
	}
	
	return 0;
}