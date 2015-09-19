#include "benchmark.h"
#include "utils.h"

#define TESTS 100
using namespace std;

/**
 * Benchmarks the time taken to construct the hash from the reference sequence
 */
void benchmark_construction(genome &g) {

	struct timeval start, end;
	struct timezone tzp;

	gettimeofday(&start, &tzp);
	g.construct_hash();
	gettimeofday(&end, &tzp);

	print_time_elapsed("Constructing Hash: ", &start, &end);

	return;
}

/**
 * Benchmarks random search over a genome
 */
void benchmark_search(genome &g, int num_patterns, int pattern_len) {

	struct timeval start, end;
	struct timezone tzp;

	gettimeofday(&start, &tzp);
	for (int i = 0; i < num_patterns; i++) {
		std::string temp = generate_random_string(pattern_len);
		auto vec = g.search(temp);
	}
	gettimeofday(&end, &tzp);

	std::string message = "Searching " + std::to_string(num_patterns)
			+ " patterns of length " + std::to_string(pattern_len) + "...\t";
	print_time_elapsed(message, &start, &end);

	return;
}

/**
 * Benchmarks random SNPs over a genome
 */
void benchmark_snp(genome &g) {

	struct timeval start, end;
	struct timezone tzp;

	std::vector<std::pair<long, char>> random = generate_random_inserts(TESTS,
			g.get_length());

	gettimeofday(&start, &tzp);

	for (std::vector<std::pair<long, char>>::iterator i = random.begin();
			i != random.end(); i++) {
		long position = i->first;
		char character = i->second;
		g.snp_at(position, std::string(1, character));
	}
	gettimeofday(&end, &tzp);

	std::string message = "Inserting " + std::to_string(TESTS)
			+ " SNPs at random locations" + "...\t";
	print_time_elapsed(message, &start, &end);

	return;
}

/**
 * Benchmarks random inserts over a genome
 */
void benchmark_insert(genome &g) {

	struct timeval start, end;
	struct timezone tzp;

	std::vector<std::pair<long, char>> random = generate_random_inserts(TESTS, g.get_length());

	gettimeofday(&start, &tzp);

	for (std::vector<std::pair<long, char>>::iterator i = random.begin();
			i != random.end(); i++) {
		long position = i->first;
		char character = i->second;
		if (position > K && g.get_length() - K) { //To avoid corner cases in insert_at which haven't yet been handled
			g.insert_at(std::string(1, character), position);
		}
	}
	gettimeofday(&end, &tzp);

	std::string message = "Inserting " + std::to_string(TESTS)
			+ " strings at random locations" + "...\t";
	print_time_elapsed(message, &start, &end);

	return;
}

/**
 * Parses the input file which contains the edits to perform on a genome
 * and transforms the required fields into a tuple<strng, string, string>
 *
 * Example input lines:
 * 	I 4439799 T
 * 	S 2261415 C
 * 	D 494753 494753
 *
 * The parsed output generated by this helper function is used by benchmark_edits()
 */
void parse_edit_file(std::vector<std::tuple<std::string, std::string, std::string>> &edits, std::string edits_file_path) {

	std::string g;
	std::ifstream edits_file(edits_file_path);
	
	std::string edit;
	if (edits_file.is_open()) {
		while (!edits_file.eof())
    	{
    		std::getline(edits_file,edit);
    		if(edit.length()>0){
				std::stringstream edit_stream(edit);
				std::string e;
				std::vector<std::string> edit_details;
				while (std::getline(edit_stream, e, ' ')) {
					edit_details.push_back(e);
				}
				edits.push_back(make_tuple(edit_details[0], edit_details[1], edit_details[2]));
    		}
    	}
    } else {
    	//TODO: Change to logging
		std::cerr << "[benchmark.cpp][parse_edit_file()] Failed to open file " << edits_file_path << std::endl;
		exit(-1);
	}

	return;
}

/**
 * Benchmarks the time taken to perform all the edits specified in the edits_file
 * to the genome.
 *
 * Edits could refer to Insertions(I), Deletions(D), or Substitutions(S):
 * I 4439799 T
 * S 2261415 C
 * D 494753 494753
 *
 */
void benchmark_edits(genome &g, std::string edits_file, const long number_of_edits) {

	//TODO: Change to logging
	std::cout << "BEGIN: BENCHMARKING EDITS" << std::endl;

	benchmark_construction(g);

	std::vector<std::tuple<std::string, std::string, std::string>> edit;
	parse_edit_file(edit, edits_file);
	long ins_count = 0, del_count = 0, snp_count = 0;

	struct timeval start, end;
	struct timezone tzp;

	gettimeofday(&start, &tzp);
	long total_edits = number_of_edits;
	std::vector<long> invalid_deletes;
	long edit_index = 0;

	for (auto it : edit) {

		if (total_edits > 0) {

			if (get<0>(it) == "I") {
				g.insert_at(get<2>(it),stol(get<1>(it),nullptr,10));
				ins_count++;
			}

			if (get<0>(it) == "D") {
				//TODO: Fix corner cases and remove
				if (!g.delete_at(stol(get<1>(it),nullptr,10), stol(get<2>(it),nullptr,10) - stol(get<1>(it),nullptr,10) + 1)) {
					invalid_deletes.push_back(edit_index);
					total_edits++;
				}
				del_count++;
			}

			if (get<0>(it) == "S") {
				g.snp_at(stol(get<1>(it),nullptr,10), get<2>(it));
				snp_count++;
			}

			total_edits--;

		} else {
			std::cout << "Total edits: " << number_of_edits << std::endl;
			break;
		}
		edit_index++;
	}

	gettimeofday(&end, &tzp);

	print_time_elapsed("Edits: ", &start, &end);

	std::cout << "Total Insertions: " << ins_count << std::endl;
	std::cout << "Total Deletions: " << del_count << std::endl;
	std::cout << "Total SNPs: " << snp_count << std::endl;

	//TODO: Remove after deletion corner case is fixed
	//Store every invalid delete in a file
	std::ofstream invalid_deletes_file("/home/komal/data/invalid_deletes");
	for (auto invalid_delete : invalid_deletes)
		invalid_deletes_file << invalid_delete << "\n";
	invalid_deletes_file.close();

	//TODO: Change to logging
	std::cout << "END: BENCHMARKING EDITS" << std::endl;

}

/**
 * Parses the input file which contains the edits to perform on a genome,
 * followed by search queries, and transforms the required fields into a
 * tuple<strng, string, string>
 *
 * A typical input file would consist of sets of edits and search queries -
 * a few lines of edits, followed by a few search strings, and so on.
 * Example input lines:
 * I 442077 ATC
 * D 3975001 3975005
 * I 2052837 GCG
 * Q GCGCCAGCGTCGGCAAGGGT N 1
 * Q TGTTAACGCGCTGGCGGCGA N 1
 * Q TTCCACGCCTTACACCGTAC N 1
 *
 * The parsed output generated by this helper function is used by benchmark_search()
 *
 */
void parse_edit_file(const std::string edits_queries_file_path,
		std::vector<std::tuple<std::string, std::string, std::string>> &edits,
		std::vector<std::tuple<std::string, std::string, std::string, long>> &queries,
		long queryFrequency, long queryCount, long iterations) {

	std::ifstream edits_queries_file(edits_queries_file_path);

	if (edits_queries_file.is_open()) {

		for (int j = 0; j < iterations; j++) {

			std::string g;

			for (int i = 0; i < queryFrequency; i++) {

				std::getline(edits_queries_file, g);
				std::stringstream edit_stream(g);
				if (g.length() > 0) {
					std::string e;
					std::vector<std::string> edit_details;
					while (std::getline(edit_stream, e, ' ')) {
						edit_details.push_back(e);
					}
					edits.push_back(
							make_tuple(edit_details[0], edit_details[1], edit_details[2]));
				}
			}

			for (int i = 0; i < queryCount; i++) {

				std::getline(edits_queries_file, g);
				std::stringstream query_stream(g);
				if (g.length() > 0) {
					std::string q;
					std::vector<std::string> query_details;
					while (std::getline(query_stream, q, ' ')) {
						query_details.push_back(q);
					}
					queries.push_back(
							make_tuple(query_details[0], query_details[1],
									query_details[2], stol(query_details[3], nullptr, 10)));
				}
			}
		}
	}  else {
    	//TODO: Change to logging
		std::cerr << "[benchmark.cpp][parse_edit_file()] Failed to open file: " << edits_queries_file_path << std::endl;
		exit(-1);
	}
}


/**
 * Benchmarks the time taken to search for a set of query strings
 * after a certain set of updates have been performed on the genome.
 *
 * The benchmark is performed iteratively, where in each iteration,
 * "queryFrequency" number of edits are made to the genome, and
 * "queryCount" number of queries are performed on the genome.
 *
 */
void benchmark_search(genome &g, const std::string path_to_query_file, long queryFrequency, long queryCount, long iterations) {

	//TODO: Change to logging
	std::cout << "BEGIN: BENCHMARKING SEARCH" << std::endl;

	benchmark_construction(g);

	std::vector<std::tuple<std::string, std::string, std::string>> edit;
	std::vector<std::tuple<std::string, std::string, std::string, long>> query;

	parse_edit_file(path_to_query_file, edit, query, queryFrequency, queryCount, iterations);

	for (int j = 0; j < iterations; j++) {

		long c = j * queryFrequency;
		for (int i = 0; i < queryFrequency; i++) {
			if (get<0>(edit[i + c]) == "I") {
				g.insert_at(get<2>(edit[i + c]),
						stol(get<1>(edit[i + c]), nullptr, 10));
			} else if (get<0>(edit[i + c]) == "D") {
				g.delete_at(stol(get<1>(edit[i + c]), nullptr, 10),
						stol(get<2>(edit[i + c]), nullptr, 10)
								- stol(get<1>(edit[i + c]), nullptr, 10) + 1);
			}
		}

		struct timeval start, end;
		struct timezone tzp;

		gettimeofday(&start, &tzp);
		for (int i = 0; i < queryCount; i++) {
			long c = j * queryCount;
			g.search(get<1>(query[i + c]));
		}
		gettimeofday(&end, &tzp);
		std::string message = "Search Iteration " + j;
		print_time_elapsed(message, &start, &end);

	}

	//TODO: Change to logging
	std::cout << "BEGIN: BENCHMARKING SEARCH" << std::endl;
}

/**
 * Benchmarks the time taken to extract substrings of random lengths
 * from random positions on the genome.
 *
 * A typical substring input file would look like the following:
 *
 * POSITION, SUBSTR_LENGTH
 * 4015907,46
 * 1197216,4
 * 4192204,20
 * 3940536,38
 * 3887129,39
 *
 */
void benchmark_substring(genome &g, std::string substr_file_path) {

	//TODO: Change to logging
	std::cout << "BEGIN: BENCHMARKING SUBSTRING" << std::endl;

	std::vector<std::pair<long, long>> substrings;
	std::ifstream substr_file(substr_file_path);
	std::string line, pos, len;


	if (substr_file.is_open()) {
		while (!substr_file.eof())
    	{
    		std::getline(substr_file,line);
    		if(line.length()>0){
				std::stringstream stream(line);
				std::string l;
				std::vector<std::string> substr_details;
				while (std::getline(stream, l, ',')) {
					substr_details.push_back(l);
				}
				substrings.push_back(
						std::make_pair(stol(substr_details[0], nullptr, 10),
								stol(substr_details[1], nullptr, 10)));
    		}
    	}
	}

	/*
	if (substr_file.is_open()) {

		while (getline(substr_file, line)) {
			std::stringstream linestream(line);
			getline(linestream, pos, ',');
			getline(linestream, len, ',');
			substrings.push_back(
					std::make_pair(stol(pos, nullptr, 10),
							stol(len, nullptr, 10)));
			line.clear();
			pos.clear();
			len.clear();
		}
	}*/ else {
		std::cerr << "[benchmark.cpp][()] Failed to open file: " << substr_file_path << std::endl;
		exit(-1);
	}

	struct timeval start, end;
	struct timezone tzp;
	long temp;
	gettimeofday(&start, &tzp);

	for (auto substr : substrings) {
		g.read_reference_abs_at(substr.first, substr.second, temp);
	}

	gettimeofday(&end, &tzp);

	std::string message = "Extracted " + std::to_string(substrings.size())
			+ " substrings: ";
	print_time_elapsed(message, &start, &end);

	//TODO: Change to logging
	std::cout << "END: BENCHMARKING SUBSTRING" << std::endl;

}
