:- par_create_parallel_engine('eng1',[team(localhost,2,'test_map_colouting.pl'),team(localhost,2,'test_map_colouting.pl'),team(localhost,2,'test_map_colouting.pl')]).

debug_query:- par_run_goal('eng1',map(S),S),
              par_get_answers('eng1',exact(all),ListOfAnswers,NumOfAnswers),
              writeln(ListOfAnswers).


time_query:- statistics(walltime,[InitTime,_]),
             par_run_goal('eng1',map(S),benchmark),
             statistics(walltime,[EndTime,_]),
             Time is EndTime - InitTime,
             write('time: '),write(Time),writeln(' ms').


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

map(M):- map_problem(15,M),
         map_colours(C), 
         colour_map(M,C).

map_problem(15,[
country(1,LIE,[SWZ,AUS]),
country(2,YUG,[HUN,ALB,MAC,CRO,BOS,BUL,RUM]),
country(3,SPN,[FRN,AND,POR]),
country(4,FIN,[SWD,NOR]),
country(5,MAC,[ALB,YUG,GRC,BUL]),
country(6,DEN,[GMY]),
country(7,CRO,[HUN,YUG,BOS,SLV]),
country(8,HUN,[AUS,CRO,YUG,SLV,RUM,UKR]),
country(9,BUL,[MAC,YUG,GRC,RUM]),
country(10,GRC,[ALB,MAC,BUL]),
country(11,AND,[FRN,SPN]),
country(12,SWD,[FIN,NOR]),
country(13,ALB,[MAC,YUG,GRC]),
country(14,ITA,[FRN,SWZ,AUS,SLV]),
country(15,CZR,[GMY,POL,AUS]),
country(16,UKR,[POL,HUN,MLD,RUM]),
country(17,SWZ,[FRN,LIE,GMY,AUS,ITA]),
country(18,POR,[SPN]),
country(19,POL,[GMY,CZR,UKR]),
country(20,AUS,[LIE,SWZ,GMY,HUN,CZR,ITA,SLV]),
country(21,BOS,[CRO,YUG]),
country(22,RUM,[HUN,YUG,BUL,MLD,UKR]),
country(23,FRN,[SWZ,SPN,AND,GMY,ITA]),
country(24,NOR,[FIN,SWD]),
country(25,NTH,[GMY]),
country(26,GMY,[NTH,FRN,SWZ,POL,AUS,CZR,DEN]),
country(27,MLD,[RUM,UKR]),
country(28,SLV,[AUS,HUN,ITA,CRO]),
country(31,SPN1,[FRN1,AND1]),
country(32,FIN1,[SWD1,NOR1]),
country(33,MAC1,[ALB1,GRC1,BUL1]),
country(35,CRO1,[HUN1,SLV1]),
country(36,HUN1,[CRO1,SLV1,RUM1,UKR1]),
country(37,BUL1,[MAC1,GRC1,RUM1]),
country(38,GRC1,[ALB1,MAC1,BUL1]),
country(39,AND1,[FRN1,SPN1]),
country(40,SWD1,[FIN1,NOR1]),
country(41,ALB1,[MAC1,GRC1]),
country(42,ITA1,[FRN1,SWZ1,SLV1]),
country(44,UKR1,[HUN1,MLD1,RUM1]),
country(45,SWZ1,[FRN1,ITA1]),
country(50,RUM1,[HUN1,BUL1,MLD1,UKR1]),
country(51,FRN1,[SWZ1,SPN1,AND1,ITA1]),
country(52,NOR1,[FIN1,SWD1]),
country(55,MLD1,[RUM1,UKR1]),
country(56,SLV1,[HUN1,ITA1,CRO1])
]).

map_colours([red, green, blue]).

colour_map([],_).
colour_map([Country|Map], Colourlst) :- 
    colour_country(Country, Colourlst), 
    colour_map(Map, Colourlst).

colour_country(country(_,C,AdjacentCs), Colourlst) :- 
    map_del(C, Colourlst, CL), 
    map_subset(AdjacentCs, CL).

map_subset([],_).
map_subset([C|Cs], Colourlst) :-
    map_del(C, Colourlst, _),
    map_subset(Cs, Colourlst).

map_del(X, [X|L], L).
map_del(X, [Y|L1], [Y|L2] ) :-
    map_del(X, L1, L2).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- generate_indexing_code('test_map_colouting.pl','eng1',map(X)).
