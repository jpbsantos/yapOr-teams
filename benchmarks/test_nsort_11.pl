:- par_create_parallel_engine('eng1',[team(localhost,2,'test_nsort_11.pl'),team(localhost,2,'test_nsort_11.pl'),team(localhost,2,'test_nsort_11.pl')]).

debug_query:- par_run_goal('eng1',nsort(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',nsort(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

nsort(L):- go_nsort([12,11,10,9,8,7,6,5,4,3,2,1],L).

go_nsort(L1,L2):-
    nsort_permutation(L1,L2),
    nsort_sorted(L2).

nsort_sorted([X,Y|Z]):-
    X =< Y,
    nsort_sorted([Y|Z]).
nsort_sorted([_]).

nsort_permutation([],[]).
nsort_permutation(L,[H|T]):-
    nsort_delete(H,L,R),
    nsort_permutation(R,T).

nsort_delete(X,[X|T],T). 
nsort_delete(X,[Y|T],[Y|T1]):- nsort_delete(X,T,T1).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_nsort_11.pl','eng1',nsort(X)).
