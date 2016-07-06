:- par_create_parallel_engine('eng1',[team(localhost,2,'test_ham.pl'),team(localhost,2,'test_ham.pl'),team(localhost,2,'test_ham.pl')]).

debug_query:- par_run_goal('eng1',ham(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',ham(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ham(H):- cycle_ham([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39],H).

cycle_ham([X|Y],[X,T|L]) :-
	chain_ham([X|Y],[],[T|L]),
	ham_edge(T,X).

chain_ham([X],L,[X|L]).
chain_ham([X|Y],K,L) :-
	ham_del(Z,Y,T),
	ham_edge(X,Z),
	chain_ham([Z|T],[X|K],L).

ham_del(X,[X|Y],Y).
ham_del(X,[U|Y],[U|Z]) :- ham_del(X,Y,Z).

ham_edge(X,Y) :-
	ham_connect(X,L),
	ham_el(Y,L).

ham_el(X,[X|_]).
ham_el(X,[_|L]) :- ham_el(X,L).

ham_connect(0,[2, 1, 34]).
ham_connect(1,[0, 2]).
ham_connect(2,[4, 3, 0, 1]).
ham_connect(3,[5, 4, 2]).
ham_connect(4,[6, 5, 2, 3]).
ham_connect(5,[6, 4, 3]).
ham_connect(6,[5, 7, 4]).
ham_connect(7,[9, 8, 6]).
ham_connect(8,[7, 9]).
ham_connect(9,[8, 11, 10, 7]).
ham_connect(10,[11, 12, 9]).
ham_connect(11,[12, 10, 9]).
ham_connect(12,[13, 11, 10]).
ham_connect(13,[15, 12, 14]).
ham_connect(14,[16, 15, 13]).
ham_connect(15,[14, 17, 16, 13]).
ham_connect(16,[17, 14, 15]).
ham_connect(17,[16, 19, 18, 15]).
ham_connect(18,[17, 20, 19]).
ham_connect(19,[21, 18, 20, 17]).
ham_connect(20,[22, 19, 21, 18]).
ham_connect(21,[23, 20, 22, 19]).
ham_connect(22,[21, 23, 20]).
ham_connect(23,[25, 22, 24, 21]).
ham_connect(24,[26, 25, 23]).
ham_connect(25,[27, 26, 23, 24]).
ham_connect(26,[28, 27, 24, 25]).
ham_connect(27,[29, 28, 25, 26]).
ham_connect(28,[27, 29, 26]).
ham_connect(29,[28, 31, 30, 27]).
ham_connect(30,[29, 31]).
ham_connect(31,[30, 33, 32, 29]).
ham_connect(32,[31, 33]).
ham_connect(33,[34, 31, 32]).
ham_connect(34,[33, 35, 10]).
ham_connect(35,[34, 36, 17, 15, 14]).
ham_connect(36,[35, 37, 9]).
ham_connect(37,[36, 38]).
ham_connect(38,[37, 39]).
ham_connect(39,[38, 0]).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_ham.pl','eng1',ham(X)).

