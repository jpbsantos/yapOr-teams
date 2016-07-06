:- use_module(library(lists)).

:- par_create_parallel_engine('eng1',[team(localhost,2,'test_ari_puzz.pl'),team(localhost,2,'test_ari_puzz.pl'),team(localhost,2,'test_ari_puzz.pl')]).

debug_query:- par_run_goal('eng1',map(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',map(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

map(M):- equation([ 7, 5, 14, 8, 9, 3, 4, 11, 2, 10],LT,RT), M=[LT,RT].

% P93 (***)  Arithmetic puzzle: Given a list of integer numbers, 
% find a correct way of inserting arithmetic signs such that 
% the result is a correct equation. The idea to the problem
% is from Roland Beuret. Thanx.

% Example: With the list of  numbers [2,3,5,7,11] we can form the
% equations  2-3+5+7 = 11  or  2 = (3*5+7)/11 (and ten others!).

% equation(L,LT,RT) :- L is the list of numbers which are the leaves
%    in the arithmetic terms LT and RT - from left to right. The 
%    arithmetic evaluation yields the same result for LT and RT.

equation(L,LT,RT) :-
   split(L,LL,RL),              % decompose the list L
   term(LL,LT),                 % construct the left term
   term(RL,RT),                 % construct the right term
   LT =:= RT.                   % evaluate and compare the terms

% term(L,T) :- L is the list of numbers which are the leaves in
%    the arithmetic term T - from left to right.

term([X],X).                    % a number is a term in itself
% term([X],-X).                   % unary minus
term(L,T) :-                    % general case: binary term
   split(L,LL,RL),              % decompose the list L
   term(LL,LT),                 % construct the left term
   term(RL,RT),                 % construct the right term
   binterm(LT,RT,T).            % construct combined binary term

% binterm(LT,RT,T) :- T is a combined binary term constructed from
%    left-hand term LT and right-hand term RT

binterm(LT,RT,LT+RT).
binterm(LT,RT,LT-RT).
binterm(LT,RT,LT*RT).
binterm(LT,RT,LT/RT) :- RT =\= 0.   % avoid division by zero

% split(L,L1,L2) :- split the list L into non-empty parts L1 and L2
%    such that their concatenation is L

split(L,L1,L2) :- append(L1,L2,L), L1 = [_|_], L2 = [_|_].

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_ari_puzz.pl','eng1',map(X)).

