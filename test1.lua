
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
	{ name = "M1", tasks = 90, distr = {type   = "HSE", params = {
						   {0.21666666666666667,{k = 15, lambda = 3.1111637094343906E-4}}, 
						   {0.2,{k = 21, lambda = 6.790833524512663E-4}},
						   {0.19777777777777777,{k = 54, lambda = 0.0020991645325160587}},
						   {0.20222222222222222,{k = 469, lambda = 0.017497069394925676}},
						   {0.18333333333333332,{k = 20, lambda = 9.852098239166557E-4}}
						   } 
						  }, pre = {}, post = {"R2"} },
	{ name = "M4", tasks = 88, distr = {type   = "HSE", params = {
						   {0.14545454545454545,{k = 39, lambda = 0.001776052284135784}}, 
						   {0.15113636363636362,{k = 659, lambda = 0.03646246104524854}},
						   {0.1431818181818182,{k = 12, lambda = 0.0016638149016192413}},
						   {0.13636363636363635,{k = 21, lambda = 0.0013192121514264577}},
						   {0.14545454545454545,{k = 47, lambda = 0.007333132850749104}},
						   {0.14772727272727273,{k = 296, lambda = 0.014700521972414195}},
						   {0.13068181818181818,{k = 19, lambda = 6.734493427361483E-4}}
						   } 
						  }, pre = {}, post = {"R2"} },
	{ name = "R2", tasks = 20, distr = {type   = "HSE", params = {
						   {0.115,{k = 3, lambda = 0.348433242506812}}, 
						   {0.165,{k = 89, lambda = 1.2476635514018692}},
						   {0.155,{k = 84, lambda = 1.488}},
						   {0.13,{k = 18, lambda = 0.47928994082840237}},
						   {0.1,{k = 4, lambda = 0.03517078119715929}},
						   {0.17,{k = 41, lambda = 0.7347670250896058}},
						   {0.165,{k = 35, lambda = 0.3246960972488804}}
						   } 
						  }, pre = {"M1","M4"}, post = {"R3"} },
	{ name = "R3", tasks = 1, distr = {type   = "HSE", params = {
						   {1,{k = 394, lambda = 2.984848484848485}}
						   } 
						  }, pre = {"R2"}, post = {} }					  
};

-- Number of computation nodes in the system
Nodes = 20;
-- Number of users accessing the system
Users = 3;
-- Distribution of the think time for the users. This element is a distribution with the same
-- format as the task running times
UThinkTimeDistr = {type = "exp", params = {rate = 1/10000.0}};

-- Total number of jobs to simulate
maxJobs = 1000;
-- Coefficient for the Confidence Intervals
-- 99%	2.576
-- 98%	2.326
-- 95%	1.96
-- 90%	1.645
confIntCoeff = 1.96;

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
