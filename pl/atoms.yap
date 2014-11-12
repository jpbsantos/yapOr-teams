 /*************************************************************************
 *									 *
 *	 YAP Prolog 							 *
 *									 *
 *	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
 *									 *
 * Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-2014	 *
 *									 *
 *************************************************************************/

:- system_module( '$_atoms', [
			      atom_concat/2,
			      string_concat/2,
        atomic_list_concat/2,
        atomic_list_concat/3,
        current_atom/1], []).

:- use_system_module( '$_errors', ['$do_error'/2]).

/**
 * @addtogroup Predicates_on_Atoms
 *
*/									 

/** @pred  atom_concat(+ _As_,? _A_) 


The predicate holds when the first argument is a list of atoms, and the
second unifies with the atom obtained by concatenating all the atoms in
the first list.

 
*/
atom_concat(Xs,At) :-
	( var(At) ->
	   '$atom_concat'(Xs, At )
        ;
	 '$atom_concat_constraints'(Xs, 0, At, Unbound),
	 '$process_atom_holes'(Unbound)
        ).

% the constraints are of the form hole: HoleAtom, Begin, Atom, End
'$atom_concat_constraints'([At], 0, At, []) :- !.
'$atom_concat_constraints'([At0], mid(Next, At), At, [hole(At0, Next, At, end)]) :-  !.
% just slice first atom
'$atom_concat_constraints'([At0|Xs], 0, At, Unbound) :-
	atom(At0), !,
	sub_atom(At, 0, _Sz, L, At0 ),
	sub_atom(At, _, L, 0, Atr ), %remainder
	'$atom_concat_constraints'(Xs, 0, Atr, Unbound).
% first hole: Follow says whether we have two holes in a row, At1 will be our atom
'$atom_concat_constraints'([At0|Xs], 0, At, [hole(At0, 0, At, Next)|Unbound]) :-
	 '$atom_concat_constraints'(Xs, mid(Next,_At1), At, Unbound).
% end of a run
'$atom_concat_constraints'([At0|Xs], mid(end, At1), At, Unbound) :-
	atom(At0), !,
	sub_atom(At, Next, _Sz, L, At0),
	sub_atom(At, 0, Next, Next, At1),
	sub_atom(At, _, L, 0, Atr), %remainder
	'$atom_concat_constraints'(Xs, 0, Atr, Unbound).
'$atom_concat_constraints'([At0|Xs], mid(Next,At1), At, Next, [hole(At0, Next, At, Follow)|Unbound]) :-
	 '$atom_concat_constraints'(Xs, mid(Follow, At1), At, Unbound).

'$process_atom_holes'([]).
'$process_atom_holes'([hole(At0, Next, At1, End)|Unbound]) :- End == end, !,
	sub_atom(At1, Next, _, 0, At0),
	 '$process_atom_holes'(Unbound).
'$process_atom_holes'([hole(At0, Next, At1, Follow)|Unbound]) :-
	sub_atom(At1, Next, Sz, _Left, At0),
	Follow is Next+Sz,
	 '$process_atom_holes'(Unbound).

	  
/** @pred  atomic_list_concat(+ _As_,? _A_) 


The predicate holds when the first argument is a list of atomic terms, and
the second unifies with the atom obtained by concatenating all the
atomic terms in the first list. The first argument thus may contain
atoms or numbers.

 
*/
atomic_list_concat(L,At) :-
	atomic_concat(L, At).
	
/** @pred  atomic_list_concat(? _As_,+ _Separator_,? _A_)

Creates an atom just like atomic_list_concat/2, but inserts
 _Separator_ between each pair of atoms. For example:

~~~~~{.prolog}
?- atomic_list_concat([gnu, gnat], `, `, A).

A = `gnu, gnat`
~~~~~

YAP emulates the SWI-Prolog version of this predicate that can also be
used to split atoms by instantiating  _Separator_ and  _Atom_ as
shown below.

~~~~~{.prolog}
?- atomic_list_concat(L, -, 'gnu-gnat').

L = [gnu, gnat]
~~~~~

 
*/
atomic_list_concat(L, El, At) :-
	var(El), !,
	'$do_error'(instantiation_error,atom_list_concat(L,El,At)).
atomic_list_concat(L, El, At) :-
	nonvar(L), !,
	'$add_els'(L,El,LEl),
	atomic_concat(LEl, At).
atomic_list_concat(L, El, At) :-
	nonvar(At), !,
	atom_codes(At, S),
	atom_codes(El, [ElS]),
	'$split_elements'(S, ElS, SubS),
	'$atomify_list'(SubS, L).

'$add_els'([A,B|L],El,[A,El|NL]) :- !,
	'$add_els'([B|L],El,NL).
'$add_els'(L,_,L).
	
'$split_elements'(E.S, E, SubS) :- !,
	'$split_elements'(S, E, SubS).
'$split_elements'(E1.S, E, [E1|L].SubS) :- !,
	'$split_elements'(S, E, L, SubS).
'$split_elements'([], _, []).

'$split_elements'([], _, [], []).
'$split_elements'(E.S, E, [], SubS) :- !,
	'$split_elements'(S, E, SubS).
'$split_elements'(E1.S, E, E1.L, SubS) :-
	'$split_elements'(S, E, L, SubS).

'$atomify_list'([], []).
'$atomify_list'(S.SubS, A.L) :-
	atom_codes(A, S),
	'$atomify_list'(SubS, L).
	

%
% small compatibility hack

'$singletons_in_term'(T,VL) :-
	'$variables_in_term'(T,[],V10),
	'$sort'(V10, V1),
	'$non_singletons_in_term'(T,[],V20),
	'$sort'(V20, V2),	
	'$subtract_lists_of_variables'(V2,V1,VL).

'$subtract_lists_of_variables'([],VL,VL).
'$subtract_lists_of_variables'([_|_],[],[]) :- !.
'$subtract_lists_of_variables'([V1|VL1],[V2|VL2],VL) :-
	V1 == V2, !,
	'$subtract_lists_of_variables'(VL1,VL2,VL).
'$subtract_lists_of_variables'([V1|VL1],[V2|VL2],[V2|VL]) :-
	'$subtract_lists_of_variables'([V1|VL1],VL2,VL).

/** @pred  current_atom( _A_) 


Checks whether  _A_ is a currently defined atom. It is used to find all
currently defined atoms by backtracking.

 
*/
current_atom(A) :-				% check
	atom(A), !.
current_atom(A) :-				% generate
	'$current_atom'(A).
current_atom(A) :-				% generate
	'$current_wide_atom'(A).

string_concat(Xs,At) :-
	( var(At) ->
	   '$string_concat'(Xs, At )
        ;
	 '$string_concat_constraints'(Xs, 0, At, Unbound),
	 '$process_string_holes'(Unbound)
        ).

% the constraints are of the form hole: HoleString, Begin, String, End
'$string_concat_constraints'([At], 0, At, []) :- !.
'$string_concat_constraints'([At0], mid(Next, At), At, [hole(At0, Next, At, end)]) :-  !.
% just slice first string
'$string_concat_constraints'([At0|Xs], 0, At, Unbound) :-
	string(At0), !,
	sub_string(At, 0, _Sz, L, At0 ),
	sub_string(At, _, L, 0, Atr ), %remainder
	'$string_concat_constraints'(Xs, 0, Atr, Unbound).
% first hole: Follow says whether we have two holes in a row, At1 will be our string
'$string_concat_constraints'([At0|Xs], 0, At, [hole(At0, 0, At, Next)|Unbound]) :-
	 '$string_concat_constraints'(Xs, mid(Next,_At1), At, Unbound).
% end of a run
'$string_concat_constraints'([At0|Xs], mid(end, At1), At, Unbound) :-
	string(At0), !,
	sub_string(At, Next, _Sz, L, At0),
	sub_string(At, 0, Next, Next, At1),
	sub_string(At, _, L, 0, Atr), %remainder
	'$string_concat_constraints'(Xs, 0, Atr, Unbound).
'$string_concat_constraints'([At0|Xs], mid(Next,At1), At, Next, [hole(At0, Next, At, Follow)|Unbound]) :-
	 '$string_concat_constraints'(Xs, mid(Follow, At1), At, Unbound).

'$process_string_holes'([]).
'$process_string_holes'([hole(At0, Next, At1, End)|Unbound]) :- End == end, !,
	sub_string(At1, Next, _, 0, At0),
	 '$process_string_holes'(Unbound).
'$process_string_holes'([hole(At0, Next, At1, Follow)|Unbound]) :-
	sub_string(At1, Next, Sz, _Left, At0),
	Follow is Next+Sz,
	 '$process_string_holes'(Unbound).

/**
@}
*/
