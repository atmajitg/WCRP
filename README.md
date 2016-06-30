# Forgetting and Student Abilities

This is the version of WCRP we used in our [EDM2016 paper](http://arxiv.org/pdf/1604.02416v1.pdf).

This branch contains a slight extension to BKT. It allows for forgetting and individualization of guess and slip probabilities. 
In the paper we compared variants of WCRP against [our own implementation](https://github.com/mmkhajah/dkt) of Deep Knowledge Tracing.

If you're trying to replicate the results of the [NIPS2014 paper](http://www.cs.colorado.edu/~mozer/Research/Selected%20Publications/reprints/LindseyKhajahMozer2014.pdf),
do not use this branch. 


# WCRP

WCRP is a Weighted Chinese Restaurant Process model for inferring skill labels in Bayesian Knowledge Tracing. 

Check out the [paper](http://papers.nips.cc/paper/5554-automatic-discovery-of-cognitive-skills-to-improve-the-prediction-of-student-learning) for more information. 


## Downloading and compiling

WCRP is written in C++. It depends on the [Boost](http://www.boost.org/) and [GNU GSL](http://www.gnu.org/software/gsl/) libraries. 

After installing Boost and GNU GSL, run

    git clone https://github.com/robert-lindsey/WCRP.git
    cd WCRP
    mkdir build
    cd build
    cmake ..
    make

You should see two executable files in build/bin: find_skills and cross_validation. 
You can view the command line options for each via the command line argument --help. 

## Usage 


#### Finding the most likely skill assignments

The command

    ./bin/find_skills --datafile ../datasets/spanish_dataset.txt --savefile map_estimate_skills.txt --map_estimate 

will run the MCMC algorithm on the data in spanish_dataset.txt using default settings. It'll then save the maximum a posteriori (MAP) estimate of the skill assignments to the file map_estimate_skills.txt. The ith entry in map_estimate_skills.txt is the skill ID of item i. 


#### Sampling the posterior distribution over skill assignments 

The command

    ./bin/find_skills --datafile ../datasets/spanish_dataset.txt --savefile sampled_skills.txt --iterations 1000 --burn 500

will run the MCMC algorithm on the data in spanish_dataset.txt for 1000 iterations and discard the first 500 iterations as burn-in. 
It'll produce sampled_skills.txt which will have 500 lines, one per post burn-in iteration. 
The goal of the MCMC algorithm is to draw samples from a probability distribution over skill assignments conditioned on the observed student data. 
Each line in sampled_skills.txt is a sample from that distribution.

The skill IDs are sample-specific: you can't count on them being the same across samples because they only denote the partitioning of items into skills given the state of the Markov chain. 
The number of skills will typically vary between samples too.


#### Running cross validation simulations on heldout students 

The command

    ./bin/cross_validation --datafile ../datasets/spanish_dataset.txt --savefile predictions.txt  --foldfile ../splits/spanish_splits.txt 

will produce the file predictions.txt containing the expected posterior probability of recall for each of the trials of the students in a heldout set of students. There will be one line per replication-fold-student-trial. 


## Data format 

#### Student responses

WCRP assumes that your student data are in a space-delimited text file with one row per trial. 
The columns should correspond to a trial's student ID, item ID, and whether the student produced a correct response in the trial. 
The IDs should be integers beginning at 0, and the trials for each student should be ordered from least to most recent. 
An example data file is available [here](https://github.com/robert-lindsey/WCRP/blob/master/datasets/spanish_dataset.txt)

#### (Optional) Expert-provided skills  

Our model's nonparametric prior distribution over skill labels can leverage skill labels provided by a human domain expert. 
If you want to provide them to our model, create a text file with one line per item. 
The ith line should be the expert-provided skill ID for the ith item. 
You can provide the file to WCRP via the command line argument --expertfile.
An example file is available [here](https://github.com/robert-lindsey/WCRP/blob/master/datasets/spanish_expert_labels.txt)

The parameter beta in the model controls how much the prior is drawn toward the expert-provided skills.
A value of 0 will have the model ignore the expert-provided skills, and as beta approaches 1 the model will deterministically use
the expert-provided skills. 
The command line options of WCRP allow you to hold beta constant at a specified value or to have the model give
beta the Bayesian treatment by treating it as another random nuisance variable. 


#### K-fold cross validation assignments

The executable cross_validation runs K-fold cross validation on your dataset.
It requires a space-delimited text file ("foldfile") with one row per cross validation simulation you want to run.
There should be one column per student in your dataset. 
Each entry indicates the fold number of the student in that replication. For example, 

    0 0 1 1 2 2
    0 1 2 0 1 2

denotes that in the first replication, students 0 and 1 are in fold 0, students 2 and 3 are in fold 1, and students 4 and 5 are in fold 2. The second replication has students 0 and 3 in fold 0, students 1 and 4 in fold 1, and students 2 and 5 in fold 3. 
The files we used are available [here](https://github.com/robert-lindsey/WCRP/tree/master/splits). 


## License and citation

This code is released under the [MIT License](https://github.com/robert-lindsey/WCRP/blob/master/LICENSE.md).

Please cite our paper in your publications if it helps your research: 

    @paper{edm2016howdeep,
    	author = {Khajah, Mohammad and Lindsey, Robert V and Mozer, Michael C},
    	title = {How deep is knowledge tracing?},
    	conference = {Educational Data Mining 2016},
    	year = {2016}
    }

