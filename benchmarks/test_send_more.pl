:-use_module(library(lists)).

:- par_create_parallel_engine('eng1',[team(localhost,2,'test_send_more.pl'),team(localhost,2,'test_send_more.pl'),team(localhost,2,'test_send_more.pl')]).

debug_query:- par_run_goal('eng1',map(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',map(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

map(M):- sendmoremoney(SEND,MORE,MONEY), M=[SEND,MORE,MONEY].

d1_9(1).
d1_9(2).
d1_9(3).
d1_9(4).
d1_9(5).
d1_9(6).
d1_9(7).
d1_9(8).
d1_9(9).
 
digit(0).
digit(X) :- d1_9(X).
 
distinct([]).
distinct([X|Xs]) :- not(member(X, Xs)), distinct(Xs).
 
sendmoremoney(SEND,MORE,MONEY) :-
   d1_9(S), digit(E), digit(N), digit(D), d1_9(M), digit(O), digit(R), digit(Y),
   distinct([S, E, N, D, M, O, R, Y]),
   SEND is 1000*S + 100*E + 10*N + D,
   MORE is 1000*M + 100*O + 10*R + E,
   MONEY is 10000*M + 1000*O + 100*N + 10*E + Y,
   MONEY is SEND + MORE.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_send_more.pl','eng1',map(X)).
