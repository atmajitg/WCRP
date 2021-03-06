/*
The MIT License (MIT)

Copyright (c) 2015 Robert Lindsey

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MIXTURE_WCRP_H
#define MIXTURE_WCRP_H

#include "Random.hpp"
#include "common.hpp"

typedef double(*prior_log_density_fn) (const double x);

class MixtureWCRP {

  public:

    MixtureWCRP(Random * generator,
                 const set<size_t> & train_students,
                 const vector< vector<bool> > & recall_sequences,
                 const vector< vector<size_t> > & item_sequences,
                 const vector<size_t> & provided_skill_assignments,
                 const double gamma,
                 const double init_alpha_prime,
                 const size_t num_students,
                 const size_t num_items,
                 const size_t num_subsamples);

    ~MixtureWCRP();

    void run_mcmc(const size_t num_iterations, const size_t burn, const bool infer_gamma, const bool infer_alpha_prime);

    // returns the expected posterior probability that the student responds correctly to the trial number
    double get_estimated_recall_prob(const size_t student, const size_t trial) const;

    // returns the skill assignments for each item across all samples
    // each returned vector has one entry per item denoting the skill id
    vector< vector<size_t> > get_sampled_skill_labels() const;

    // returns the skill assignments which maximized the training data log likelihood
    // the returned vector has one entry per item denoting the skill id
    vector<size_t> get_most_likely_skill_labels() const;


  protected:

    double full_data_log_likelihood(const bool is_training, size_t & trials_included) const;
    double data_log_likelihood(const vector<size_t> & students, const vector<size_t> & first_exposures) const;
    double data_log_likelihood(const size_t student, const size_t first_exposure, size_t & num_trials) const;

    bool studied_any_of(const size_t student, const vector<size_t> & items) const;
    void record_sample(const double train_ll);

    double log_seating_prob() const;

    // resample the skill assignment (table) for this item (customer)
    // see algorithm 8 from http://www.stat.purdue.edu/~rdutta/24.PDF
    void gibbs_resample_skill(const size_t item);

    double slice_resample_bkt_parameter(const size_t skill_id, double * param, const vector<size_t> & students_to_include, const vector<size_t> & first_exposures, const double cur_ll);
    double slice_resample_wcrp_param(double * param, const double cur_seating_lp, const double lower_bound, const double upper_bound, const double init_bracket, prior_log_density_fn prior_lp);

    // bootstraps calculating a student's data log likelihood by precomputed forward state
    void cache_p_hat(const size_t student, const size_t end_trial, boost::unordered_map<size_t, double> & p_hat) const;

    double skill_log_likelihood(const size_t skill_id, const vector<size_t> & affected_students, const vector<size_t> & first_exposures, const vector< boost::unordered_map<size_t, double> > & init_p_hat) const;
    double skill_log_likelihood(const size_t skill_id, const vector<size_t> & affected_students, const vector<size_t> & first_exposures) const;

    void draw_bkt_param_prior(struct bkt_parameters & params) const;
    double compute_K(const size_t item, const size_t table_id, const bool am_initializing) const;
    bool remove_item_from_table(const size_t item, const size_t table_id);
    void assign_item_to_table(const size_t item, const size_t table_id, const bool is_new_table);

    Random * generator;

    // constants
    const set<size_t> & train_students;
    const vector< vector<bool> > & recall_sequences;
    const vector< vector<size_t> > & item_sequences;
    const vector<size_t> & provided_skill_assignments;
    const size_t num_students;
    const size_t num_items;
    const size_t num_subsamples;
    const bool use_expert_labels;

    // Markov chain state
    vector<size_t> seating_arrangement; // seating_arrangement[item] = table id
    boost::unordered_map<size_t, struct bkt_parameters> parameters; // mapping b/w table id and parameter values
    double log_alpha_prime;
    double log_gamma;
    // MCMC helper variables
    size_t num_used_skills;
    boost::unordered_map<size_t, size_t> table_sizes; // table_sizes[table_id] = # of items assigned to it
    set<size_t> extant_tables;
    boost::unordered_map<size_t, boost::unordered_map<size_t, vector<size_t> > > trial_lookup; // trial_lookup[table_id][student_id] = sequence of trial #s assigned to the student-skill pair
    size_t tables_ever_instantiated;
    vector<struct bkt_parameters> prior_samples; // auxiliary variables for the non-conjugate gibbs sampler
    vector< vector<double> > singleton_skill_data_lp;

    // dataset helper variables
    vector< vector<bool> > ever_studied;				// ever_studied[student][item] = true if at any time the student studied the item (and is in the training set)
    vector<size_t> all_items;
    vector< vector<size_t> > students_who_studied;  	// students_who_studied[item] = list of TRAINING students who at any time studied the item
    vector< vector<size_t> > all_first_encounters;		// all_first_encounters[item]....
    vector< vector<size_t> > first_encounter; 			// first_encounter[student][item] = trial index the student first studied the item. ='s -1 if they never did
    vector< vector< vector<size_t> > > trials_studied;  // trials_studied[student][item] = list of all trials where the student studied the item
    size_t num_expert_provided_skills;
    vector< vector<pair<size_t, bool> > > item_and_recall_sequences; // student, trial, (item, recall)

    // these variables record the sampler state for later reporting
    vector< vector< vector<double> > > pRT_samples; // pRT_samples[student][trial][sample number]
    vector< vector<size_t> > skill_label_samples;   // skill_label_samples[sample number][item] = skill id  (note: skill ids are sample-specific)
    vector<double> train_ll_samples; // train_ll[sample number] = the training data log likelihood of that sample

};

#endif
