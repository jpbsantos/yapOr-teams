:- par_create_parallel_engine('eng1',[team(localhost,2,'test_knight_move.pl'),team(localhost,2,'test_knight_move.pl'),team(localhost,2,'test_knight_move.pl')]).

debug_query:- par_run_goal('eng1',map(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',map(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

map(M):- find_path(13,[1,1],M).



move([X,Y], [X1,Y1]) :- c1(X,X1), c2(Y,Y1).
move([X,Y], [X1,Y1]) :- c2(X,X1), c1(Y,Y1).

c1(X,X1) :- X > 1, X1 is X - 1.
c1(X,X1) :- X < 8, X1 is X + 1.
c2(X,X1) :- X > 2, X1 is X - 2.
c2(X,X1) :- X < 7, X1 is X + 2.

find_path(N, S, Ans) :-
path(N, [S], Ans).


path(0, L, L).
path(N, [C|Prev], Ans) :- N > 0, N1 is N - 1,
                         move(C,C1),
                         notin(Prev,C1),
path(N1, [C1 | [C | Prev]], Ans).

notin([], X).
notin([F|R], X) :- F \== X, notin(R,X).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_ari_puzz.pl','eng1',map(X)).
