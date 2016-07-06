:- par_create_parallel_engine('eng1',[team(localhost,2,'test_queens_13.pl'),team(localhost,2,'test_queens_13.pl'),team(localhost,2,'test_queens_13.pl')]).

debug_query:- par_run_goal('eng1',queens(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',queens(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

queens(S) :- size(BoardSize), solve(BoardSize,[],S).

size(14).

snint(1).
snint(2).
snint(3).
snint(4).
snint(5).
snint(6).
snint(7).
snint(8).
snint(9).
snint(10).
snint(11).
snint(12).
snint(13).
snint(14).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

newsquare([square(I,J)|Rest],square(X,Y)) :-
       X is I+1,
       snint(Y),
       not_threatened(I,J,X,Y),
       safe(X,Y,Rest).
newsquare([],square(1,X)) :- snint(X).

safe(X,Y,[square(I,J)|L]) :-
       not_threatened(I,J,X,Y),
       safe(X,Y,L).
safe(X,Y,[]).

not_threatened(I,J,X,Y) :-
       I =\= X,
       J =\= Y,
       I-J =\= X-Y,
       I+J =\= X+Y.

solve(Board_size,Initial,Final) :-
       newsquare(Initial,Next),
       solve(Board_size,[Next|Initial],Final).
solve(Bs,[square(Bs,Y)|L],[square(Bs,Y)|L]) :- size(Bs).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_queens_13.pl','eng1',queens(X)).
