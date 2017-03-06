-- Copyright 2017 Marco Gribaudo <marco.gribaudo@polimi.it>
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

-- Model specification

-- DAG definition: it is encoded as an array of stages.
-- Each stage is spcified by the following parameters:
--    name   (String)   : name of the stage
--    tasks  (Int)      : number of tasks in this stage
--    distr  (Distrib.) : distribution of the task running time
--                        Distributions are characterized by the following data:
--                             type   : the name of the distribution to be generated
--                             params : parameters specific to the chosen distribution type
--    pre    (String[]) : name of the stages that must be completed for this one to start
--    post   (String[]) : name of the stages that can start after this one has finished
--
Stages = {
	{ name = "M", tasks = 0, distr = {type = "exp", params = {rate = 1/1000.0}}, pre = {}, post = {} }
};

-- Number of computation nodes in the system
Nodes = 20;
-- Number of users accessing the system
Users = 10000;
-- Distribution of the think time for the users. This element is a distribution with the same
-- format as the task running times
-- UThinkTimeDistr = {type = "erl", params = {lambda = 1/1000.0, k = 10}};
UThinkTimeDistr = {type = "weib", params = {k = 4, lambda = 0.4}};

-- -- -- -- -- -- -- -- -- -- -- --
-- -- -- -- supported distributions:
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "exp"		: Exponential distribution
-- -- Params: rate		: -- rate of the exponential distribution
-- -- --
-- -- -- Example        : {type = "exp", params = {rate = 0.1}}
-- -- -- 				  Generates an exponential random variable with average 10
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "erl"		: Erlang distribution
-- -- Params: k			: -- number of stages
-- -- 		  lambda	: -- rate of each stage
-- -- --
-- -- -- Example        : {type = "erl", params = {k = 4, lambda = 0.4}}
-- -- -- 				  Generates a four stages Ernalng random variable with average 10, and c.v. = 0.5
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "det"		: Deterministic distribution
-- -- Params: val		: -- value returned
-- -- --
-- -- -- Example        : {type = "det", params = {val = 10.0}}
-- -- -- 				  Always generates 10
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "unif"	: Uniform distribution
-- -- Params: min		: -- minimum value
-- -- 		  max		: -- maximum value
-- -- --
-- -- -- Example        : {type = "det", params = {min = 5.0, max = 15.0}}
-- -- -- 				  Generates a uniform random number between 5 and 15 (average 10)
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "HSE"		 : Hyper Sum of Erlangs: a probabilistic choice of a set of sum of erlang distributions
-- -- Params: {<BLOCK>, ...} : -- an sequence of 1 or more blocks describing a probabilistic choice
-- --             <BLOCK> -> {prob, <STAGE>, ...} -- prob		  : is the probability of the block
-- --											  -- <STAGE>, ... : is a sequence of 1 or more stages
-- --			  <STAGE> -> {[k], lambda} -- The description of an Erlang block:
-- --											  -- [k]	: an optional number of stages. k=1 if missing
-- --											  -- lambda	: the rate of each stage of the corresponding Erlang
--[[
-- -- -- Example        : {type   = "HSE",
						   params = {{0.5,{k = 2, lambda = 0.5},
						   			      {k = 4, lambda = 1}},
						   			 {0.5,{lambda = 1/15.0}}
						   }
						  }
-- -- -- 				  Generates a random variable that:
-- -- --						 50% of the times is the sum of the Erlangs: Erlang(2, 1/2)+Erlang(4, 1)
-- -- --						 50% of the times is exponential, with rate 1/15
-- -- --
-- -- -- The "HSE" distribution can be used also to generate:
-- -- -- 	1) Exponential
-- -- --	2) Erlang
-- -- --	3) Hyper-exponential
-- -- --	4) Hypo-exponential
-- -- --	5) Hyper-Erlang
-- -- -- with a more compact description with respect to a PH type distribution.
-- -- -- Examples:
-- -- -- 	1) Exponential of rate 0.1
-- -- --	   {type   = "HSE", params = {{1.0,{lambda = 0.1}}} } 
-- -- -- 	2) Erlang with 4 stages of rate 0.4
-- -- --	   {type   = "HSE", params = {{1.0,{k = 4, lambda = 0.4}}} }
-- -- -- 	3) Hyper-exponential of rate 0.2 (with prob 0.5) and 1/15.0 (with prob 0.5)
-- -- --	   {type   = "HSE", params = {{0.5,{lambda = 0.05}}, {0.5,{lambda = 1/15.0}}} }	   
-- -- -- 	4) Hypo-exponential of rates 0.5 and 0.125
-- -- --	   {type   = "HSE", params = {{1.0,{lambda = 0.5},{lambda = 0.125}}} } 						   
-- -- -- 	5) Hyper-Erlang of rate 1, 5 stages (with prob 0.5) and 0.2, 3 stages (with prob 0.5)
-- -- --	   {type   = "HSE", params = {{0.5,{k = 5, lambda = 1.0}}, {0.5,{k = 3, lambda = 0.2}}} }	   
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "PH"		: Phase Type distribution
-- -- Params: alpha		: -- Initial state probability vector
-- -- 		  Q			: -- transition matrix of the phase-type distribution
--[[
-- -- -- Example        : {type   = "PH",
						   params = {
								alpha = {1.0, 0.0, 0.0}, 
								Q = {
									 {-0.2,  0.2,  0.0},
									 { 0.0, -0.4,  0.2},
									 { 0.0,  0.0, -0.2}
								}
							  }
							}
]]--
-- -- -- 				  Generates a random number that follows a PH distribution with 3 states.
-- -- --				  Always starst from state 1, goes to state 2 after 1/0.2, then
-- -- --                  It either finish or goes to state 3 after 1/0.4, with both alternatives equally prob.
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "MAP"		: Markov Arrival Process
-- -- Params: P0		: -- Initial state probability vector
-- -- 		  D0		: -- transition matrix of changes of state that do not produce arrivals
-- -- 		  D1		: -- transition matrix of changes of state that produce arrivals
--[[
-- -- -- Example        : {type   = "MAP",
						   params = {
						   		P0 = {1.0/11.0,  10.0/11.0},
						   		D0 = {{-11.0,  10.0},
						   			  {  1.0, -11.0}},
						   		D1 = {{  1.0,   0.0},
						   			  {  0.0,  10.0}},
							  }
							}
]]--
-- -- -- 				  Generates a Markov Modulated Poisson Process, with two states.
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "norm"	: Normal distribution
-- -- Params: mu		: -- average
-- -- 		  sigma		: -- standard deviation
-- -- --
-- -- -- Example        : {type = "norm", params = {mu = 0, sigma = 1.0}}
-- -- -- 				  Generates a standard normal distribution
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "Pnorm"	: Positive normal distribution
-- -- Params: mu		: -- average
-- -- 		  sigma		: -- standard deviation
-- --		  tech		: -- technique to ensure positivity:
-- --						 "trunc" - Truncate: reject negative samples, and resamples until
-- --								   a positive one is found. This is the default technique.
-- --						 "abs"   - Absorbing barrier: a negative sample is replaced by 0.
-- --						 "refl"  - Reflecting barrier: the absolute value of the sample is returned.
-- -- --
-- -- -- Example        : {type = "Pnorm", params = {mu = 0, sigma = 1.0, tech = "refl"}}
-- -- -- 				  Generates the absolute value of the standard normal distribution
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "lognorm"	: Log-normal distribution
-- -- Params: mu		: -- location
-- -- 		  sigma		: -- scale
-- -- --
-- -- -- Example        : {type = "lognorm", params = {mu = 0, sigma = 1.0}}
-- -- -- 				  Generates a log-normal distribution with mean e^0.5 and varianve e(e-1)
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "pareto"	: Log-normal distribution
-- -- Params: xm		: -- scale
-- -- 		  alpha		: -- shape
-- -- --
-- -- -- Example        : {type = "pareto", params = {xm = 1.0, alpha = 3.0}}
-- -- -- 				  Generates a pareto distribution with mean 1.5 and varianve 1.5)
-- -- -- -- -- -- -- -- -- -- -- --
-- -- Type:   "weib"	: Weibull distribution
-- -- Params: k			: -- shape
-- -- 		  lambda	: -- scape
-- -- --
-- -- -- Example        : {type = "weib", params = {k = 4, lambda = 0.4}}
-- -- -- 				  Generates a weibull random variable with average 0.4*Gamma(1.25)
