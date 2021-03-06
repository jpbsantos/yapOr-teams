:- system_module( '$_utils', [callable/1,
        current_op/3,
        nb_current/2,
        nth_instance/3,
        nth_instance/4,
        op/3,
        prolog/0,
        recordaifnot/3,
        recordzifnot/3,
        simple/1,
        subsumes_term/2], ['$getval_exception'/3]).

:- use_system_module( '$_boot', ['$live'/0]).

:- use_system_module( '$_errors', ['$do_error'/2]).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                     %%
%%                   The YapTab/YapOr/OPTYap systems                   %%
%%                                                                     %%
%% YapTab extends the Yap Prolog engine to support sequential tabling  %%
%% YapOr extends the Yap Prolog engine to support or-parallelism       %%
%% OPTYap extends the Yap Prolog engine to support or-parallel tabling %%
%%                                                                     %%
%%                                                                     %%
%%      Yap Prolog was developed at University of Porto, Portugal      %%
%%                                                                     %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- meta_predicate 
   parallel(0),
   parallel_findall(?,0,?),
   parallel_findfirst(?,0,?),
   parallel_once(0).



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                           or_statistics/0                           %%
%%                          opt_statistics/0                           %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

or_statistics :-
   current_output(Stream),
   or_statistics(Stream).

opt_statistics :-
   current_output(Stream),
   opt_statistics(Stream).



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                           or_statistics/2                           %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% should match with code in OPTYap/opt.preds.c
or_statistics(total_memory,[BytesInUse,BytesAllocated]) :-
   '$c_get_optyap_statistics'(0,BytesInUse,BytesAllocated).
or_statistics(or_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(4,BytesInUse,StructsInUse).
or_statistics(query_goal_solution_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(12,BytesInUse,StructsInUse).
or_statistics(query_goal_answer_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(13,BytesInUse,StructsInUse).



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                          opt_statistics/2                           %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% should match with code in OPTYap/opt.preds.c
opt_statistics(total_memory,[BytesInUse,BytesAllocated]) :-
   '$c_get_optyap_statistics'(0,BytesInUse,BytesAllocated).
opt_statistics(table_entries,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(1,BytesInUse,StructsInUse).
opt_statistics(subgoal_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(2,BytesInUse,StructsInUse).
opt_statistics(dependency_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(3,BytesInUse,StructsInUse).
opt_statistics(or_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(4,BytesInUse,StructsInUse).
opt_statistics(suspension_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(5,BytesInUse,StructsInUse).
opt_statistics(subgoal_trie_nodes,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(6,BytesInUse,StructsInUse).
opt_statistics(answer_trie_nodes,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(7,BytesInUse,StructsInUse).
opt_statistics(subgoal_trie_hashes,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(8,BytesInUse,StructsInUse).
opt_statistics(answer_trie_hashes,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(9,BytesInUse,StructsInUse).
opt_statistics(global_trie_nodes,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(10,BytesInUse,StructsInUse).
opt_statistics(global_trie_hashes,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(11,BytesInUse,StructsInUse).
opt_statistics(query_goal_solution_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(12,BytesInUse,StructsInUse).
opt_statistics(query_goal_answer_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(13,BytesInUse,StructsInUse).
opt_statistics(table_subgoal_solution_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(14,BytesInUse,StructsInUse).
opt_statistics(table_subgoal_answer_frames,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(15,BytesInUse,StructsInUse).
opt_statistics(subgoal_entries,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(16,BytesInUse,StructsInUse).
opt_statistics(answer_ref_nodes,[BytesInUse,StructsInUse]) :-
   '$c_get_optyap_statistics'(17,BytesInUse,StructsInUse).



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                             parallel/1                              %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

parallel(Goal) :-
   parallel_mode(Mode), Mode = on, !,
   (
      '$parallel_query'(Goal)
   ;
      true
   ).
parallel(Goal) :-
   (
      '$execute'(Goal),
      fail
   ;
      true
   ).

'$parallel_query'(Goal) :-
   '$c_yapor_start'(Results), 
   '$execute'(Goal),
   fail.
'$parallel_query'(_).



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                          parallel_findall/3                         %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

parallel_findall(Template,Goal,Answers) :- writeln('------------------------------Yapor.yap'),
   parallel_mode(Mode), Mode = on, !,
   (
      '$parallel_findall_query'(Template,Goal)
   ;
      '$c_parallel_get_answers'(Refs),
      '$parallel_findall_recorded'(Refs,Answers),
      eraseall(parallel_findall)
   ).
parallel_findall(Template,Goal,Answers) :-
   findall(Template,Goal,Answers).

'$parallel_findall_query'(Template,Goal) :-
   '$c_yapor_start'(Results), 
   '$execute'(Goal), writeln(Goal),
   recordz(parallel_findall,Template,Ref),
   '$c_parallel_new_answer'(Ref),
   fail.
'$parallel_findall_query'(_,_).

'$parallel_findall_recorded'([],[]) :- !.
'$parallel_findall_recorded'([Ref|Refs],[Template|Answers]):-
   recorded(parallel_findall,Template,Ref),
   '$parallel_findall_recorded'(Refs,Answers).


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                      parallel_findfirst/3                           %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

parallel_findfirst(Template,Goal,Answer) :- 
   parallel_findall(Template,(Goal,!),Answer).


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                         parallel_once/1                             %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

parallel_once(Goal) :-
   parallel_mode(Mode), Mode = on, !,
   (
      '$parallel_once_query'(Goal)
   ;
      recorded(parallel_once,Goal,Ref),
      erase(Ref)
   ).
parallel_once(Goal) :-
   once(Goal).

'$parallel_once_query'(Goal) :-
   '$c_yapor_start'(Results), 
   '$execute'(once(Goal)),
    recordz(parallel_once,Goal,_),
    fail.
'$parallel_once_query'(_).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                         YAPOR_TEAM                                  %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- dynamic yapor_team/2.

%debug_joao:- '$c_parallel_new_answer'(Ref).

%%%%% create_team %%%%%%%
create_team(Nworkers,TeamName) :- 
   '$c_create_team'(Nworkers,Id), 
   assertz(yapor_team(TeamName,Id,1)), 
   yapor_team(TeamName,A,1).

%%%%% create_comm %%%%%%%
deal_with_list([(Host,NWorkers)|XS],[Host|HostList],[NWorkers|NWokersList]):- writeln(Host),deal_with_list(XS,HostList,NWokersList).
deal_with_list([],[],[]).  

create_comm(TeamList,ComName) :-
    length(TeamList,Number_teams),
    deal_with_list(TeamList,HostList,NWokersList),
    writeln(HostList),
    writeln(NWokersList),    
   '$c_create_comm'(NWokersList,HostList,Number_teams,CommId),
    assertz(yapor_team(ComName,CommId)).


%%%%% parallel %%%%%%%
parallel(TeamName,Goal,Type) :-
    yapor_team(TeamName,Id),
   '$c_run_parallel_goal'(Goal,Id,Type).


%%%%% parallel_barrier %%%%%%%

parallel_barrier(TeamName):- 
	yapor_team(TeamName,Id), 
        '$c_parallel_barrier'(Id).
%%%%% parallel_barrier %%%%%%%

mpi_parallel_barrier(TeamName):- 
	yapor_team(TeamName,Id), 
        '$c_mpi_parallel_barrier'(Id).


%%%%% parallel_barrier %%%%%%%

results_barrier(TeamName):- 
	yapor_team(TeamName,Id), 
        '$c_results_barrier'(Id).

%%%%% mpi_get_results %%%%%%%
mpi_get_results(TeamName,Answers) :-
    yapor_team(TeamName,Id),
    '$c_mpi_get_results'(Id,Answers). 


%%%%% get_results %%%%%%%
get_results(TeamName,Answers) :-
    yapor_team(TeamName,Id), 
   '$c_get_results'(Id,Res),
   '$parallel_findall_recorded_test'(Res,Answers),
    eraseall(parallel_findall).


'$parallel_findall_recorded_test'([],[]) :- !.
'$parallel_findall_recorded_test'([Ref|Refs],[Template|Answers]):-
   recorded(parallel_findall,Template,Ref),
   '$parallel_findall_recorded_test'(Refs,Answers).

%%%%% parallel_findall_test %%%%%%%

parallel_findall_test(Template,Goal,Answers) :- 
   parallel_mode(Mode), Mode = on, !,
   '$parallel_findall_query_test'(Template,Goal).


'$parallel_findall_query_test'(Template,Goal) :-
   '$c_yapor_start'(Results), 
   '$execute'(Goal), 
   writeln(Template),
   recordz(parallel_findall,Template,Ref),
   '$c_parallel_new_answer'(Ref),
   fail.
'$parallel_findall_query_test'(_,_).

%%%%% MPI parallel_findall_test %%%%%%%

mpi_parallel_findall_test(Template,Goal,Answers) :-
   parallel_mode(Mode), Mode = on,
   '$c_yapor_start'(1),
   '$execute'(Goal), 
   %writeln(Template),
   %recordz(parallel_findall,Template,Ref),
   '$c_mpi_parallel_new_answer'(Template),
   fail.


mpi_parallel_findall_test(Template,Goal,Answers).

%%%%% parallel_findall_test_no_store_ans %%%%%%%

parallel_findall_test_no_store_ans(Template,Goal,Answers) :-
   parallel_mode(Mode),Mode = on, !,
   '$parallel_findall_query_test_no_store_ans'(Template,Goal).


'$parallel_findall_query_test_no_store_ans'(Template,Goal) :- 
   '$c_yapor_start'(Results),
   '$execute'(Goal), 
   %writeln(Goal),
   %recordz(parallel_findall,Template,Ref),
   %'$c_parallel_new_answer'(Ref),
   fail.
'$parallel_findall_query_test_no_store_ans'(_,_).

%%%%% MPI parallel_findall_test_no_store_ans %%%%%%%

mpi_parallel_findall_test_no_store_ans(Template,Goal,Answers) :-
   parallel_mode(Mode),Mode = on, !,
   '$mpi_parallel_findall_query_test_no_store_ans'(Template,Goal),writeln('AQUI_______________________________________________'),fail.


'$mpi_parallel_findall_query_test_no_store_ans'(Template,Goal) :- 
   '$c_yapor_start'(0),
   '$execute'(Goal), 
   %writeln(Goal),
   %recordz(parallel_findall,Template,Ref),
   %'$c_parallel_new_answer'(Ref),
   fail.
'$mpi_parallel_findall_query_test_no_store_ans'(_,_).

%%%%% MPI SOLVE BUG %%%%%%%
 
mpi_solve_bug(ConsultCall,TeamName):-
     yapor_team(TeamName,Id), 
     '$c_mpi_solve_bug'(Id,ConsultCall).
 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%new syntax
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

generate_indexing_code(Path,Comm,Goal):- mpi_prompt,
                                         mpi_solve_bug(Path,Comm),
                                         mpi_parallel_barrier(Comm).
generate_indexing_code(Path,Comm,Goal):- not mpi_prompt,
					 user:call(Goal).

par_run_goal(Comm,Goal,Template):-not ground(Template),parallel(Comm,mpi_parallel_findall_test(Template,Goal,L),1),!.

par_run_goal(Comm,Goal,Template):- ground(Template), parallel(Comm,mpi_parallel_findall_test_no_store_ans(Template,Goal,L),1),
                                                mpi_parallel_barrier(Comm).

par_get_answers(Comm,Mode,L,N):-Mode == exact(all), mpi_get_results(Comm,L),length(L,N).
par_get_answers(Comm,Mode,L,N):-Mode \= exact(all), write(Mode),writeln(' this mode is not currently supported').



remove([],L).
remove([team(A,B,C)|XS],[(A,B)|L]):- remove(XS,L).

par_create_parallel_engine(Comm,TeamList):-mpi_prompt,remove(TeamList,NewTeamList),create_comm(NewTeamList,Comm).
