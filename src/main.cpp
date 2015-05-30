#ifndef MAIN_CPP
#define MAIN_CPP

#include "common.hpp"
#include "MixtureWCRP.hpp"

using namespace std;

// reads a tab delimited file with the columns: student id, item id, skill id, recall success
// all ids are assumed to start at 0 and be contiguous
void load_dataset(const char * filename, vector<size_t> & provided_skill_assignments, vector< vector<bool> > & recall_sequences, vector< vector<size_t> > & problem_sequences, size_t & num_students, size_t & num_items, size_t & num_skills) {

	num_students=0, num_items=0, num_skills=0;
	size_t student, item, skill, recall;

	ifstream in(filename);
	if (!in.is_open()) { 
		cerr << "couldn't open " << string(filename) << endl;
		exit(EXIT_FAILURE);
	}
	
	// figure out how many students, items, and skills there are
	while (in >> student >> item >> skill >> recall) {
		num_students = max(student+1, num_students);
		num_items = max(item+1, num_items);
		num_skills = max(skill+1, num_skills);
	}
	in.close();
	cout << "dataset has " << num_students << " students, " << num_items << " items, and " << num_skills << " expert-provided skills" << endl;

	// initialize
	provided_skill_assignments.resize(num_items, -1); // skill_assignments[item index] = skill index
	recall_sequences.resize(num_students);
	problem_sequences.resize(num_students);

	// read the dataset
	in.open(filename);
	while (in >> student >> item >> skill >> recall) {
		recall_sequences[student].push_back(recall);
		problem_sequences[student].push_back(item);
		provided_skill_assignments[item] = skill;
	}
	in.close();
}


void load_splits(const char * filename, vector<vector<size_t> > & fold_nums, size_t & num_folds, const size_t num_students) {

	ifstream in(filename);
	if (!in.is_open()) { 
		cerr << "couldn't open " << string(filename) << endl;
		exit(EXIT_FAILURE);
	}
	
	num_folds = 0;
	while(!in.eof()) {
		// read a line
		string line;
		getline(in, line);
		boost::trim(line);
		if (line.empty()) break;
	
		// split on whitespace
		vector<string> fields;
		boost::split(fields, line, boost::is_any_of(" \t"));
		assert(fields.size() == num_students);
		
		vector<size_t> replication_fold_nums(fields.size());
		for (size_t student = 0; student < fields.size(); student++) {
			replication_fold_nums[student] = boost::lexical_cast<size_t>(fields[student]);
			num_folds = max(replication_fold_nums[student]+1, num_folds);
		}
		fold_nums.push_back(replication_fold_nums);
	}
	
	cout << "# replications to run = " << fold_nums.size() << endl;
	cout << "# folds per replication = " << num_folds << endl;
}


int main(int argc, char ** argv) {

	namespace po = boost::program_options;

	string datafile, outfile, foldfile;
	int tmp_num_iterations, tmp_burn, tmp_num_subsamples;
	double init_beta, init_alpha_prime;
	bool infer_beta, infer_alpha_prime, dump_skills;

	// parse the command line arguments
	po::options_description desc("Allowed options");
	desc.add_options()
        	("help", "print help message")
		("datafile", po::value<string>(&datafile), "train the model on the given data file")
		("outfile", po::value<string>(&outfile), "put results in this file")
		("foldfile", po::value<string>(&foldfile), "file with the training / test splits")
		("init_beta", po::value<double>(&init_beta), "initial value of beta")
		("fixed_alpha_prime", po::value<double>(&init_alpha_prime), "fixed value of alpha'")
		("infer_beta", "infer the value of beta")
		("num_iterations", po::value<int>(&tmp_num_iterations)->default_value(200), "number of iterations to run")
		("burn", po::value<int>(&tmp_burn)->default_value(100), "number of iterations to discard")
		("num_subsamples", po::value<int>(&tmp_num_subsamples)->default_value(2000), "number of samples to use when approximating marginal likelihood of new tables")
		("dump_skills", "save the skill assignments too")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (argc == 1 || vm.count("help")) {
		cout << desc << endl;
		return EXIT_SUCCESS;
	}

	infer_beta = vm.count("infer_beta");
	dump_skills = vm.count("dump_skills");
	if (vm.count("fixed_alpha_prime")) {
		assert(init_alpha_prime >= 0);
		infer_alpha_prime = 0;
		cout << "the code will keep alpha' fixed at " << init_alpha_prime << endl;
	}
	else {
		init_alpha_prime = -1;
		infer_alpha_prime = 1;
		cout << "the code will automatically infer the value of alpha'" << endl;
	}
	
	size_t num_iterations = (size_t) tmp_num_iterations;
	size_t burn = (size_t) tmp_burn;
	size_t num_subsamples = (size_t) tmp_num_subsamples;

	Random * generator = new Random(time(NULL));

	if (infer_beta) cout << "the code will automatically infer the value of beta" << endl;
	else cout << "the code will keep beta fixed at " << init_beta << endl;
	
	assert(init_beta >= 0 && init_beta <= 1);
	assert(num_iterations >= 0);
	assert(num_iterations > burn);

	// load the dataset
	vector<size_t> provided_skill_assignments;
	vector< vector<bool> > recall_sequences;    // recall_sequences[student][trial # i]  = recall success or failure of the ith trial we have for the student
	vector< vector<size_t> > problem_sequences; // problem_sequences[student][trial # i] = item corresponding to the ith trial we have for the student
	size_t num_students, num_items, num_skills_dataset;
	load_dataset(datafile.c_str(), provided_skill_assignments, recall_sequences, problem_sequences, num_students, num_items, num_skills_dataset);
	assert(num_students > 0 && num_items > 0);

	// load the training / test splits
	vector<vector<size_t> > fold_nums;
	size_t num_folds;
	load_splits(foldfile.c_str(), fold_nums, num_folds, num_students);
	
	for (size_t replication = 0; replication < fold_nums.size(); replication++) {
		for (size_t test_fold = 0; test_fold < num_folds; test_fold++) {
		
			set<size_t> train_students, test_students;
			for (size_t s = 0; s < num_students; s++) {
				if (fold_nums.at(replication).at(s) == test_fold && num_folds>1) test_students.insert(s);
				else train_students.insert(s);
			}

			if (num_folds > 1) assert(test_students.size() > 0);
			assert(train_students.size() > 0);
			
			// create the model
			MixtureWCRP model(generator, train_students, test_students, recall_sequences, problem_sequences, provided_skill_assignments, init_beta, init_alpha_prime, num_students, num_items, num_subsamples);

			// run the sampler
			model.run_mcmc(outfile, replication, test_fold, num_iterations, burn, infer_beta, infer_alpha_prime, dump_skills);
		}
	}

	delete generator;
	return EXIT_SUCCESS;
}

#endif