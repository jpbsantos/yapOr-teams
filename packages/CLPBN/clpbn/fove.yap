
/************************************************

  (GC) First Order Variable Elimination Interface
 
**************************************************/

:- module(clpbn_fove,
          [fove/3,
           check_if_fove_done/1,
           set_horus_flag/2,
           init_fove_solver/4,
           run_fove_solver/3,
           finalize_fove_solver/1
          ]).


:- use_module(library('clpbn/display'),
          [clpbn_bind_vals/3]).


:- use_module(library('clpbn/dists'),
          [get_dist_params/2]).


:- use_module(library(pfl),
          [factor/5,
           skolem/2
          ]).


:- load_foreign_files(['horus'], [], init_predicates).


:- set_horus_flag(use_logarithms, false).
%:- set_horus_flag(use_logarithms, true).



fove([[]], _, _) :- !.
fove([QueryVars], AllVars, Output) :-
  writeln(queryVars:QueryVars),
  writeln(allVars:AllVars),
	init_fove_solver(_, AllVars, _, ParfactorGraph),
	run_fove_solver([QueryVars], LPs, ParfactorGraph),
	finalize_fove_solver(ParfactorGraph),
	clpbn_bind_vals([QueryVars], LPs, Output).


init_fove_solver(_, AllAttVars, _, fove(ParfactorGraph, DistIds)) :-
  writeln(allattvars:AllAttVars), writeln(''),
	get_parfactors(Parfactors),
	get_dist_ids(Parfactors, DistIds0),
	sort(DistIds0, DistIds),
	get_observed_vars(AllAttVars, ObservedVars),
  writeln(factors:Parfactors:'\n'),
	writeln(evidence:ObservedVars:'\n'),
  create_lifted_network(Parfactors,ObservedVars,ParfactorGraph).


:- table get_parfactors/1.
%
% enumerate all parfactors and enumerate their domain as tuples.
%
% output is list of pf(
%        Id: an unique number
%        Ks: a list of keys, also known as the pf formula [a(X),b(Y),c(X,Y)]
%        Vs: the list of free variables [X,Y]
%        Phi: the table following usual CLP(BN) convention
%        Tuples: tuples with all ground bindings for variables in Vs, of the form [fv(x,y)]
%
get_parfactors(Factors) :-
	findall(F, is_factor(F), Factors).


is_factor(pf(Id, Ks, Rs, Phi, Tuples)) :-
	factor(Id, Ks, Vs, Table, Constraints),
  get_ranges(Ks,Rs),
	Table \= avg,
	gen_table(Table, Phi),
	all_tuples(Constraints, Vs, Tuples).


get_ranges([],[]).
get_ranges(K.Ks, Range.Rs) :-	!,
	skolem(K,Domain),
  length(Domain,Range),
  get_ranges(Ks, Rs).


gen_table(Table, Phi) :-
	( is_list(Table)
          -> 
	  Phi = Table
          ;
	  call(user:Table, Phi)
	).


all_tuples(Constraints, Tuple, Tuples) :-
	setof(Tuple, Constraints^run(Constraints), Tuples).


run([]).
run(Goal.Constraints) :-
  user:Goal,
  run(Constraints).


get_dist_ids([], []).
get_dist_ids(pf(Id, _, _, _, _).Parfactors, Id.DistIds) :-
  get_dist_ids(Parfactors, DistIds).


get_observed_vars([], []).
get_observed_vars(V.AllAttVars, [K:E|ObservedVars]) :-
  writeln('checking ev for':V),
	clpbn:get_atts(V,[key(K),evidence(E)]), !,
  writeln('evidence!!!':K:E),
	get_observed_vars(AllAttVars, ObservedVars).
get_observed_vars(V.AllAttVars, ObservedVars) :-
	clpbn:get_atts(V,[key(K)]), !,
  writeln('no evidence for':V:K),
	get_observed_vars(AllAttVars, ObservedVars).


get_query_vars([], []).
get_query_vars(E1.L1, E2.L2) :-
  get_query_vars_2(E1,E2),
	get_query_vars(L1, L2).


get_query_vars_2([], []).
get_query_vars_2(V.AttVars, [RV|RVs]) :-
	clpbn:get_atts(V,[key(RV)]), !,
	get_query_vars_2(AttVars, RVs).


get_dists_parameters([], []).
get_dists_parameters([Id|Ids], [dist(Id)|DistsInfo]) :-
	get_dist_params(Id, Params),
	get_dists_parameters(Ids, DistsInfo).


run_fove_solver(QueryVarsAtts, Solutions, fove(ParfactorGraph, DistIds)) :-
  % TODO set_parfactor_graph_params
  writeln(distIds:DistIds),
  %get_dists_parameters(DistIds, DistParams),
  %writeln(distParams:DistParams),
  get_query_vars(QueryVarsAtts, QueryVars),
  writeln(queryVars:QueryVars),
  run_lifted_solver(ParfactorGraph, QueryVars, Solutions).


finalize_fove_solver(fove(ParfactorGraph, _)) :-
  free_parfactor_graph(ParfactorGraph).

