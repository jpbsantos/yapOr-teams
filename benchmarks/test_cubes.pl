:- par_create_parallel_engine('eng1',[team(localhost,2,'test_cubes.pl'),team(localhost,2,'test_cubes.pl'),team(localhost,2,'test_cubes.pl')]).

debug_query:- par_run_goal('eng1',cubes(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',cubes(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

cubes(Sol):- cubes(7,Qs), solve(Qs,[],Sol).

cubes(7,[q(p(0,4),p(0,9),p(7,10)),
q(p(8,13),p(11,7),p(13,3)),
q(p(8,9),p(4,8),p(10,11)),
q(p(2,8),p(0,3),p(1,0)),
q(p(5,8),p(10,11),p(0,1)),
q(p(13,11),p(12,4),p(8,2)),
q(p(4,2),p(12,10),p(12,13)),
q(p(4,7),p(11,8),p(2,8)),
q(p(9,4),p(2,8),p(12,4)),
q(p(1,8),p(10,2),p(7,10))
]).


solve([],Rs,Rs).
solve([C|Cs],Ps,Rs):-
   set(C,P),
   check(Ps,P),
   solve(Cs,[P|Ps],Rs).

check([],_).
check([q(A1,B1,C1,D1)|Ps],P):-
   P = q(A2,B2,C2,D2),
   A1 =\= A2, B1 =\= B2, C1 =\= C2, D1 =\= D2,
   check(Ps,P).

set(q(P1,P2,P3),P):- rotate(P1,P2,P).
set(q(P1,P2,P3),P):- rotate(P2,P1,P).
set(q(P1,P2,P3),P):- rotate(P1,P3,P).
set(q(P1,P2,P3),P):- rotate(P3,P1,P).
set(q(P1,P2,P3),P):- rotate(P2,P3,P).
set(q(P1,P2,P3),P):- rotate(P3,P2,P).

rotate(p(C1,C2),p(C3,C4),q(C1,C2,C3,C4)).
rotate(p(C1,C2),p(C3,C4),q(C1,C2,C4,C3)).
rotate(p(C1,C2),p(C3,C4),q(C2,C1,C3,C4)).
rotate(p(C1,C2),p(C3,C4),q(C2,C1,C4,C3)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_cubes.pl','eng1',cubes(X)).
