/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		consult.yap						 *
* Last rev:	8/2/88							 *
* mods:									 *
* comments:	Consulting Files in YAP					 *
*									 *
*************************************************************************/
:- system_module( '$_consult', [compile/1,
        consult/1,
        db_files/1,-
        ensure_loaded/1,
        exists_source/1,
        exo_files/1,
        (initialization)/2,
        load_files/2,
        make/0,
        make_library_index/1,
        module/2,
        prolog_load_context/2,
        reconsult/1,
        source_file/1,
        source_file/2,
        source_file_property/2,
        use_module/3], ['$add_multifile'/3,
        '$csult'/2,
        '$do_startup_reconsult'/1,
        '$elif'/2,
        '$else'/1,
        '$endif'/1,
        '$if'/2,
        '$include'/2,
        '$initialization'/1,
        '$initialization'/2,
        '$lf_opt'/3,
        '$load_files'/3,
        '$require'/2,
        '$set_encoding'/1,
        '$use_module'/3]).

:- use_system_module( '$_absf', ['$full_filename'/3]).

:- use_system_module( '$_boot', ['$clear_reconsulting'/0,
        '$init_system'/0,
        '$init_win_graphics'/0,
        '$loop'/2,
        '$system_catch'/4]).

:- use_system_module( '$_errors', ['$do_error'/2]).

:- use_system_module( '$_load_foreign', ['$import_foreign'/3]).

:- use_system_module( '$_modules', ['$add_to_imports'/3,
        '$convert_for_export'/7,
        '$extend_exports'/3]).

:- use_system_module( '$_preds', ['$current_predicate_no_modules'/3]).

/**

  \defgroup YAPConsulting Loading files into YAP
  @ingroup YAPProgramming

  We present the main predicates and directives available to load
files and to set-up the Prolog environment. We discuss

  + @ref YAPReadFiles

  + @ref YAPCompilerSettings 


@defgroup YAPReadFiles The Predicates that Read Source Files
@ingroup  YAPConsulting

@{

 */


/**

 @pred load_files(+ _Files_, + _Options_)

General implementation of the consult/1 family. Execution is controlled by the
following flags:

  + consult(+ _Mode_)
  This extension controls the type of file to load. If  _Mode_ is:
    `consult`, clauses are added to the data-base, unless from the same file;
   `reconsult`, clauses are recompiled,
   `db`, these are facts that need to be added to the data-base,
   `exo`, these are facts with atoms and integers that can be stored in a compact representation (see load_exo/1).

  + silent(+ _Bool_)
  If true, load the file without printing a message. The specified value is the default for all files loaded as a result of loading the specified files.

  + stream(+ _Input_)
  This SWI-Prolog extension compiles the data from the stream _Input_. If this option is used,  _Files_ must be a single atom which is used to identify the source-location of the loaded
clauses as well as remove all clauses if the data is re-consulted.

  This option is added to allow compiling from non-file locations such as databases, the web,  the user (see consult/1) or other servers. 

  + compilation_mode(+ _Mode_)
  This extension controls how procedures are compiled. If  _Mode_
  is `compact` clauses are compiled and no source code is stored;
  if it is `source` clauses are compiled and source code is stored;  
  if it is `assert_all` clauses are asserted into the data-base.

  + encoding(+ _Encoding_)
  Character encoding used in consulting files. Please  (see [Encoding](@ref Encoding)) for
  supported encodings.

+ expand(+ _Bool_)
  If `true`, run the
  filenames through expand_file_name/2 and load the returned
  files. Default is false, except for consult/1 which is
  intended for interactive use.

  + if(+ _Condition_)
  Load the file only if the specified  _Condition_ is
  satisfied. The value `true` the file unconditionally,
  `changed` loads the file if it was not loaded before, or has
  been modified since it was loaded the last time, `not_loaded`
  loads the file if it was not loaded before.

  + imports(+ _ListOrAll_)
  If `all` and the file is a module file, import all public
  predicates. Otherwise import only the named predicates. Each
  predicate is referred to as `\<name\>/\<arity\>`. This option has
  no effect if the file is not a module file.

  + must_be_module(+ _Bool_)
  If true, raise an error if the file is not a module file. Used by
` use_module/1 and use_module/2.

  + autoload(+ _Autoload_)
  SWI-compatible option where if  _Autoload_ is `true` undefined predicates
  are loaded on first call.

  + derived_from(+ _File_)
  SWI-compatible option to control make/0. Currently
  not supported.
*/
%
% SWI options
% autoload(true,false)
% derived_from(File) -> make
% encoding(Encoding) => implemented
				% expand(true,false)
% if(changed,true,not_loaded) => implemented
% imports(all,List) => implemented
% qcompile(true,false)
% silent(true,false)  => implemented
% stream(Stream)  => implemented
% consult(consult,reconsult,exo,db) => implemented
% compilation_mode(compact,source,assert_all) => implemented
% register(true, false) => implemented
%
load_files(Files,Opts) :-
	'$load_files'(Files,Opts,load_files(Files,Opts)).

'$lf_option'(autoload, 1, _).
'$lf_option'(derived_from, 2, false).
'$lf_option'(encoding, 3, default).
'$lf_option'(expand, 4, false).
'$lf_option'(if, 5, true).
'$lf_option'(imports, 6, all).
'$lf_option'(qcompile, 7, never).
'$lf_option'(silent, 8, _).
'$lf_option'(skip_unix_header, 9, false).
'$lf_option'(compilation_mode, 10, source).
'$lf_option'(consult, 11, reconsult).
'$lf_option'(stream, 12, _).
'$lf_option'(register, 13, true).
'$lf_option'('$files', 14, _).
'$lf_option'('$call', 15, _).
'$lf_option'('$use_module', 16, _).
'$lf_option'('$consulted_at', 17, _).
'$lf_option'('$options', 18, _).
'$lf_option'('$location', 19, _).
'$lf_option'(dialect, 20, yap).
'$lf_option'(format, 21, source).
'$lf_option'(redefine_module, 22, false).
'$lf_option'(reexport, 23, false).
'$lf_option'(sandboxed, 24, false).
'$lf_option'(scope_settings, 25, false).
'$lf_option'(modified, 26, _).
'$lf_option'('$context_module', 27, _).
'$lf_option'('$parent_topts', 28, _).
'$lf_option'(must_be_module, 29, false).

'$lf_option'(last_opt, 29).

'$lf_opt'( Op, TOpts, Val) :-
	'$lf_option'(Op, Id, _),
	arg( Id, TOpts, Val ).

'$load_files'(Files, Opts, Call) :-
	( '$nb_getval'('$lf_status', OldTOpts, fail), nonvar(OldTOpts) ->
	  '$lf_opt'(silent, OldTOpts, OldVerbosity),
	  '$lf_opt'(autoload, OldTOpts, OldAutoload)
	;
	  true ),
	'$check_files'(Files,load_files(Files,Opts)),
	'$lf_option'(last_opt, LastOpt),
	functor( TOpts, opt, LastOpt ),
	( source_location(ParentF, Line) -> true ; ParentF = user_input, Line = -1 ),
	'$lf_opt'('$location', TOpts, ParentF:Line),
	'$lf_opt'('$files', TOpts, Files),
	'$lf_opt'('$call', TOpts, Call),
	'$lf_opt'('$options', TOpts, Opts),
	'$lf_opt'('$parent_topts', TOpts, OldTOpts),
	'$process_lf_opts'(Opts,TOpts,Files,Call),
	'$lf_default_opts'(1, LastOpt, TOpts),
	'$check_use_module'(Call,UseModule),
	'$lf_opt'('$use_module', TOpts, UseModule),
        '$current_module'(M0),
	( '$lf_opt'(silent, TOpts, Verbosity),
	  var(Verbosity) ->
	  Verbosity = OldVerbosity
	;
	  true
	),
	( '$lf_opt'(autoload, TOpts, Autoload),
	  var(Autoload) ->
	  Autoload = OldAutoload
	;
	  true
	),
	% make sure we can run consult
	'$init_system',
	'$lf'(Files, M0, Call, TOpts).

'$check_files'(Files, Call) :-
	var(Files), !,
	'$do_error'(instantiation_error, Call).
'$check_files'(M:Files, Call) :- !,
	(var(M)
	->
	'$do_error'(instantiation_error, Call)
	;
	 atom(M)
	->
	 '$check_files'(Files,Call)
	;
	 '$do_error'(type_error(atom,M), Call)
	).
'$check_files'(Files, Call) :-
	( ground(Files)
	->
	 true
	;
	'$do_error'(instantiation_error, Call)
	).

'$process_lf_opts'(V, _, _, Call) :-
	var(V), !,
	'$do_error'(instantiation_error,Call).
'$process_lf_opts'([], _, _, _).
'$process_lf_opts'([Opt|Opts],TOpt,Files,Call) :-
	Opt =.. [Op, Val],
	ground(Val),
	'$lf_opt'(Op, TOpt, Val),
	'$process_lf_opt'(Op, Val,Call), !, 
	'$process_lf_opts'(Opts, TOpt, Files, Call).
'$process_lf_opts'([Opt|_],_,_,Call) :-
	'$do_error'(domain_error(unimplemented_option,Opt),Call).

'$process_lf_opt'(autoload, Val, Call) :-
	( Val == false -> true ;
	    Val == true -> true ;
	    '$do_error'(domain_error(unimplemented_option,autoload(Val)),Call) ).
'$process_lf_opt'(derived_from, File, Call) :-
	( atom(File) -> true ;  '$do_error'(type_error(atom,File),Call) ).
'$process_lf_opt'(encoding, Encoding, _Call) :-
	atom(Encoding).
'$process_lf_opt'(expand, Val, Call) :-
	( Val == true -> '$do_error'(domain_error(unimplemented_option,expand),Call) ;
	    Val == false -> true ;
	    '$do_error'(domain_error(unimplemented_option,expand(Val)),Call) ).
'$process_lf_opt'(if, If, Call) :-
	( If == changed -> true ;
	    If == true -> true ;
	    If == not_loaded -> true ;
	    '$do_error'(domain_error(unimplemented_option,if),Call) ).
'$process_lf_opt'(imports, Val, Call) :-
	( Val == all -> true ;
	    var(Val) -> Val = all ;
	    is_list(Val) -> ( ground(Val) -> true ; '$do_error'(instantiation_error,Call) ) ;
	    '$do_error'(domain_error(unimplemented_option,imports(Val)),Call) ).
'$process_lf_opt'(qcompile, Val,Call) :-
	( Val == true -> '$do_error'(domain_error(unimplemented_option,expand),Call) ;
	    Val == false -> true ;
	    '$do_error'(domain_error(unimplemented_option,expand(Val)),Call) ).
'$process_lf_opt'(silent, Val, Call) :-
	( Val == false -> true ;
	    Val == true -> true ;
	    '$do_error'(domain_error(unimplemented_option,silent(Val)),Call) ).
'$process_lf_opt'(skip_unix_header, Val, Call) :-
	( Val == false -> true ;
	    Val == true -> true ;
	    '$do_error'(domain_error(unimplemented_option,skip_unix_header(Val)),Call) ).
'$process_lf_opt'(compilation_mode, Val, Call) :-
( Val == source -> true ;
	    Val == compact -> true ;
	    Val == assert_all -> true ;
	    '$do_error'(domain_error(unimplemented_option,compilation_mode(Val)),Call) ).
'$process_lf_opt'(consult, Val , Call) :-
	( Val == reconsult -> true ;
	    Val == consult -> true ;
	    Val == exo -> true ;
	    Val == db -> true ;
	    '$do_error'(domain_error(unimplemented_option,consult(Val)),Call) ).
'$process_lf_opt'(reexport, Val , Call) :-
	( Val == true -> true ;
	    Val == false -> true ;
	    '$do_error'(domain_error(unimplemented_option,reexport(Val)),Call) ).
'$process_lf_opt'(must_be_module, Val , Call) :-
	( Val == true -> true ;
	    Val == false -> true ;
	    '$do_error'(domain_error(unimplemented_option,must_be_module(Val)),Call) ).
'$process_lf_opt'(stream, Val, Call) :-
	( current_stream(_,_,Val) -> true ;
	    '$do_error'(type_error(stream,Val),Call) ).
'$process_lf_opt'(register, Val, Call) :-
	( Val == false -> true ;
	    Val == true -> true ;
	    '$do_error'(domain_error(unimplemented_option,register(Val)),Call) ).
'$process_lf_opt'('$context_module', Val, Call) :-
	( atom(File) -> true ;  '$do_error'(type_error(atom,File),Call) ).


'$lf_default_opts'(I, LastOpt, _TOpts) :- I > LastOpt, !.
'$lf_default_opts'(I, LastOpt, TOpts) :-
	I1 is I+1,
	arg(I, TOpts, A),
	( nonvar(A) -> true ;
	  '$lf_option'(_Name, I, A)
	),
	'$lf_default_opts'(I1, LastOpt, TOpts).


	  
'$check_use_module'(use_module(_), use_module(_)) :- !.
'$check_use_module'(use_module(_,_), use_module(_)) :- !.
'$check_use_module'(use_module(M,_,_), use_module(M)) :- !.
'$check_use_module'(_, load_files) :- !.

'$lf'(V,_,Call, _ ) :- var(V), !,
	'$do_error'(instantiation_error,Call).
'$lf'([], _, _, _) :- !.
'$lf'(M:X, _, Call, TOpts) :- !,
	(
	  atom(M)
	->
	  '$lf'(X, M, Call, TOpts)
	  ;
	  '$do_error'(type_error(atom,M),Call)
	).
'$lf'([F|Fs], Mod, Call, TOpts) :- !,
	% clean up after each consult
	( '$lf'(F,Mod,Call, TOpts), fail ;
	  '$lf'(Fs, Mod, Call, TOpts) ).
'$lf'(user, Mod, _, TOpts) :- !,
	'$do_lf'(Mod, user_input, user_input, TOpts).
'$lf'(user_input, Mod, _, TOpts) :- !,
	'$do_lf'(Mod, user_input, user_input, TOpts).
'$lf'(File, Mod, Call, TOpts) :-
	'$lf_opt'(stream, TOpts, Stream),
	( var(Stream) ->
	  /* need_to_open_file */
	  '$full_filename'(File, Y, Call),
	  open(Y, read, Stream)
        ;
	  true
        ), !,
	'$lf_opt'(reexport, TOpts, Reexport),
	'$lf_opt'(if, TOpts, If),
	( var(If) -> If = true ; true ),
	'$lf_opt'(imports, TOpts, Imports),
	'$start_lf'(If, Mod, Stream, TOpts, File, Imports),
	close(Stream).
'$lf'(X, _, Call, _) :-
	'$do_error'(permission_error(input,stream,X),Call).

'$start_lf'(not_loaded, Mod, Stream, TOpts, UserFile, Imports) :-
	'$file_loaded'(Stream, Mod, Imports, TOpts), !,
	'$lf_opt'('$options', TOpts, Opts),
	'$lf_opt'('$location', TOpts, ParentF:Line),
	'$loaded'(Stream, UserFile, Mod, ParentF, Line, not_loaded, _File, _Dir, Opts),
	'$reexport'( TOpts, ParentF, Imports, _File ).
'$start_lf'(changed, Mod, Stream, TOpts, UserFile, Imports) :-
	'$file_unchanged'(Stream, Mod, Imports, TOpts), !,
	'$lf_opt'('$options', TOpts, Opts),
	'$lf_opt'('$location', TOpts, ParentF:Line),
	'$loaded'(Stream, UserFile, Mod, ParentF, Line, changed, _File, _Dir, Opts),
	'$reexport'( TOpts, ParentF, Imports, _File ).
'$start_lf'(_, Mod, Stream, TOpts, File, _) :-
	'$do_lf'(Mod, Stream, File, TOpts).


/**

@pred ensure_loaded(+ _F_) is iso

When the files specified by  _F_ are module files,
ensure_loaded/1 loads them if they have note been previously
loaded, otherwise advertises the user about the existing name clashes
and prompts about importing or not those predicates. Predicates which
are not public remain invisible.

When the files are not module files, ensure_loaded/1 loads them
if they have not been loaded before, and naes nothing otherwise.

 _F_ must be a list containing the names of the files to load.
*/
ensure_loaded(Fs) :-
	'$load_files'(Fs, [if(not_loaded)],ensure_loaded(Fs)).

compile(Fs) :-
	'$load_files'(Fs, [], compile(Fs)).

/**
 @pred [ _F_ ]
 @pred consult(+ _F_)


Adds the clauses written in file  _F_ or in the list of files  _F_
to the program.

In YAP consult/1 does not remove previous clauses for
the procedures defined in other files than _F_, but since YAP-6.4.3 it will redefine all procedures defined in _F_.

All code in YAP is compiled, and the compileer generates static
procedures by default. In case you need to manipulate the original
code, the expanded version of the original source code is available by
calling source/0 or by enabling the source flag.

*/
% consult(Fs) :-
% 	'$has_yap_or',
% 	'$do_error'(context_error(consult(Fs),clause),query).
consult(V) :-
	var(V), !,
	'$do_error'(instantiation_error,consult(V)).
consult(M0:Fs) :- !,
	'$consult'(Fs, M0).
consult(Fs) :-
	'$current_module'(M0),
	'$consult'(Fs, M0).

'$consult'(Fs,Module) :-
	'$access_yap_flags'(8, 2), % SICStus Prolog compatibility
	!,
	'$load_files'(Module:Fs,[],consult(Fs)).
'$consult'(Fs, Module) :-
	'$load_files'(Module:Fs,[consult(consult)],consult(Fs)).


/**

@pred [ - _F_ ]
@pred reconsult(+ _F_ )
@pred compile(+ _F_ )

Updates the program by replacing the
previous definitions for the predicates defined in  _F_. It differs from consult/1
in that it only multifile/1 predicates are not reset in a reconsult. Instead, consult/1
sees all predicates as multifile.

YAP also offers no difference between consult/1 and compile/1. The two
are  implemented by the same exact code.

Example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
?- [file1, -file2, -file3, file4].
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
will consult `file1` `file4` and reconsult `file2` and
`file3`. That is, it could be written as:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
?- consult(file1),
   reconsult( [file2, file3],
   consult( [file4] ).
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

*/
reconsult(Fs) :-
	'$load_files'(Fs, [], reconsult(Fs)).


/* exo_files(+ _Files_)

Load compactly a database of facts with equal structure, see @cite
x. Useful when wanting to read in a very compact way database tables,
it saves space by storing data, not a compiled program. The idea was
introduced in @cite y, but never implemented because often indexing
just takes more room. It was redefined recebtly by exploiting
different forms of indexing, as shown in @cite x.

@note implementation

  The function Yap_ExoLookup() is the mai interface betwwen the WAM
  and exo components. The algorithms are straightforward, that is,
  mostly hash-tables but have close to linear performance..
*/

exo_files(Fs) :-
	'$load_files'(Fs, [consult(exo), if(not_loaded)], exo_files(Fs)).

/**

@pred load_db(+ _Files_)


Load a database of ground facts. All facts must take up the same amount of storage, so that
 a fact $I$ can be accessed at position _P[I-1]_. This representation thus stores the facts as a huge continuous array, the so-called mega clause. 

See \cite for a motivation for this technique. YAP implements this
optimization by default whenever it loads a large number of facts (see
Yap_BuildMegaClause(PredEntry *ap) ) for the details. On the other
hand, loading the data-base will cause fragmentation because
individual facts facts need extra headers ands tails, and because
often new atoms will be stored in the Symbol Table, see
LookupAtom(const char *atom). The main advantage of load_db/1 is
that it allocates the necessary memory only once. Just doing this
 may halve total memory usage in large in-memory database-oriented applications.
 
@note Implementation 

YAP implements load_db/1 as a two-step non-optimised process. First,
   it counts the nmuber of facts and checks their size. Second, it
   allocates and fills the memory. The first step of the algorithm is
   implemented by dbload_get_space(), and the second by
   dbload_add_facts().

   db_files/1 itself is just a call to load_files/2. 
*/
db_files(Fs) :-
	'$load_files'(Fs, [consult(db), if(not_loaded)], exo_files(Fs)).


'$csult'(Fs, M) :-
	'$extract_minus'(Fs, MFs), !,
	'$load_files'(M:MFs,[],[M:Fs]).
'$csult'(Fs, M) :-
	'$load_files'(M:Fs,[consult(consult)],[M:Fs]).

'$extract_minus'([], []).
'$extract_minus'([-F|Fs], [F|MFs]) :-
	'$extract_minus'(Fs, MFs).


'$do_lf'(ContextModule, Stream, UserFile, TOpts) :-
	'$lf_opt'('$context_module', TOpts, ContextModule),
	'$msg_level'( TOpts, Verbosity),
%	format( 'I=~w~n', [Verbosity=UserFile] ),
	'$lf_opt'(encoding, TOpts, Encoding),
	'$set_encoding'(Stream, Encoding),
	% export to process
	b_setval('$lf_status', TOpts),
	'$reset_if'(OldIfLevel),
	% take care with [a:f], a is the ContextModule
	'$current_module'(SourceModule, ContextModule),
	'$lf_opt'(consult, TOpts, Reconsult0),
	'$lf_opt'('$options', TOpts, Opts),
	'$lf_opt'('$location', TOpts, ParentF:Line),
	'$loaded'(Stream, UserFile, SourceModule, ParentF, Line, Reconsult, File, Dir, Opts),
	working_directory(OldD, Dir),
	H0 is heapused, '$cputime'(T0,_),
	'$set_current_loop_stream'(OldStream, Stream),
	'$swi_current_prolog_flag'(generate_debug_info, GenerateDebug),
	'$lf_opt'(compilation_mode, TOpts, CompMode),
	'$comp_mode'(OldCompMode, CompMode),
	( get_value('$syntaxcheckflag',on) -> '$init_style_check'(File) ; true ),
	recorda('$initialisation','$',_),
	( Reconsult \== consult ->
	    '$start_reconsulting'(File),
	    '$start_consult'(Reconsult,File,LC),
	    '$remove_multifile_clauses'(File),
	    StartMsg = reconsulting,
	    EndMsg = reconsulted
	    ;
	    '$start_consult'(Reconsult,File,LC),
	    ( File \= user_input, File \= [] -> '$remove_multifile_clauses'(File) ; true ),
	    StartMsg = consulting,
	    EndMsg = consulted
	),
	print_message(Verbosity, loading(StartMsg, File)),
	'$lf_opt'(skip_unix_header , TOpts, SkipUnixHeader),
	( SkipUnixHeader == true->
	    '$skip_unix_header'(Stream)
	;
	    true
	),
	'$loop'(Stream,Reconsult),
	H is heapused-H0, '$cputime'(TF,_), T is TF-T0,
	'$current_module'(Mod, SourceModule),
	print_message(Verbosity, loaded(EndMsg, File, Mod, T, H)),
	'$end_consult',
	( 
	    Reconsult = reconsult ->
	    '$clear_reconsulting'
	;
	    true
	),
	'$set_current_loop_stream'(Stream, OldStream),
	'$swi_set_prolog_flag'(generate_debug_info, GenerateDebug),
	'$comp_mode'(_CompMode, OldCompMode),
	working_directory(_,OldD),
	% surely, we were in run mode or we would not have included the file!
	nb_setval('$if_skip_mode',run),
	% back to include mode!
	nb_setval('$if_level',OldIfLevel),
	'$lf_opt'('$use_module', TOpts, UseModule),
	'$bind_module'(Mod, UseModule),
	'$lf_opt'(imports, TOpts, Imports),
	'$import_to_current_module'(File, ContextModule, Imports, _, TOpts),
	'$reexport'( TOpts, ParentF, Imports, File ),
	( LC == 0 -> prompt(_,'   |: ') ; true),
	'$exec_initialisation_goals',
	% format( 'O=~w~n', [Mod=UserFile] ),
	!.

% are we in autoload and autoload_flag is false?
'$msg_level'( TOpts, Verbosity) :-
	'$lf_opt'(autoload, TOpts, AutoLoad),
	AutoLoad == true,
	'$swi_current_prolog_flag'(verbose_autoload, false), !,
	Verbosity = silent.
'$msg_level'( _TOpts, Verbosity) :-
	'$swi_current_prolog_flag'(verbose_load, false), !,
	Verbosity = silent.
'$msg_level'( _TOpts, Verbosity) :-
	'$swi_current_prolog_flag'(verbose, silent), !,
	Verbosity = silent.
'$msg_level'( TOpts, Verbosity) :-
	'$lf_opt'(silent, TOpts, Silent),
	Silent == true, !,
	Verbosity = silent.
'$msg_level'( _TOpts, informational).

'$reset_if'(OldIfLevel) :-
	'$nb_getval'('$if_level', OldIfLevel, fail), !,
	nb_setval('$if_level',0).
'$reset_if'(0) :-
	nb_setval('$if_level',0).

'$get_if'(Level0) :-
	'$nb_getval'('$if_level', Level, fail), !,
	Level0 = Level.
'$get_if'(0).

'$bind_module'(_, load_files).
'$bind_module'(Mod, use_module(Mod)).

'$import_to_current_module'(File, ContextModule, Imports, RemainingImports, TOpts) :-
	\+ recorded('$module','$module'(File, _Module, _ModExports, _),_),
	% enable loading C-predicates from a different file
	recorded( '$load_foreign_done', [File, M0], _),
	'$import_foreign'(File, M0, ContextModule ),
	fail.
	
'$import_to_current_module'(File, ContextModule, Imports, RemainingImports, TOpts) :-
	recorded('$module','$module'(File, Module, ModExports, _),_),
	Module \= ContextModule, !,
	'$lf_opt'('$call', TOpts, Call),
	'$convert_for_export'(Imports, ModExports, Module, ContextModule, TranslationTab, RemainingImports, Goal),
	'$add_to_imports'(TranslationTab, Module, ContextModule).
'$import_to_current_module'(_, _, _, _, _).

'$start_reconsulting'(F) :-
	recorda('$reconsulted','$',_),
	recorda('$reconsulting',F,_).

'$exec_initialisation_goals' :-
	nb_setval('$initialization_goals',on),
	fail.
'$exec_initialisation_goals' :-
	recorded('$blocking_code',_,R),
	erase(R),
	fail.
% system goals must be performed first 
'$exec_initialisation_goals' :-
	recorded('$system_initialisation',G,R),
	erase(R),
	G \= '$',
	once( call(G) ),
	fail.
'$exec_initialisation_goals' :-
	'$show_consult_level'(Level),
	'$current_module'(M),
	recorded('$initialisation',do(Level,_),_),
	findall(G,
	        '$fetch_init_goal'(Level, G),
		LGs),
	lists:member(G,LGs),
	% run initialization under user control (so allow debugging this stuff).
	(
	  '$system_catch'(('$user_call'(G,M) -> true), M, Error, user:'$LoopError'(Error, top)),
	  fail
	;
          OldMode = on,
	  fail
	).
'$exec_initialisation_goals' :-
	nb_setval('$initialization_goals',off).


'$fetch_init_goal'(Level, G) :-
	recorded('$initialisation',do(Level,G),R),
	erase(R),
	G\='$'.

/**
include(+ _F_) is directive

	The `include` directive adds the text files or sequence of text
	files specified by  _F_ into the files being currently consulted. It may be used
	as an replacement to consult/1 by allowing the programmer to include a data-base
	split into several files. 

*/
'$include'(V, _) :- var(V), !,
	'$do_error'(instantiation_error,include(V)).
'$include'([], _) :- !.
'$include'([F|Fs], Status) :- !,
	'$include'(F, Status),
	'$include'(Fs, Status).
'$include'(X, Status) :-
	b_getval('$lf_status', TOpts),
	'$msg_level'( TOpts, Verbosity),
	'$full_filename'(X, Y , ( :- include(X)) ),
	'$lf_opt'(stream, TOpts, OldStream),
	source_location(F, L),
	'$current_module'(Mod),
	( open(Y, read, Stream) 	->
	  true ; 
	  '$do_error'(permission_error(input,stream,Y),include(X))
	),
	'$set_current_loop_stream'(OldStream, Stream),
	H0 is heapused, '$cputime'(T0,_),
	'$loaded'(Stream, X, Mod, F, L, include, Y, _Dir, []),
	( '$nb_getval'('$included_file', OY, fail ) -> true ; OY = [] ),
	'$lf_opt'(encoding, TOpts, Encoding),
	'$set_encoding'(Stream, Encoding),
	nb_setval('$included_file', Y),
	print_message(Verbosity, loading(including, Y)),
	'$loop'(Stream,Status),
	'$set_current_loop_stream'(Stream, OldStream),
	close(Stream),
	H is heapused-H0, '$cputime'(TF,_), T is TF-T0,
	print_message(Verbosity, loaded(included, Y, Mod, T, H)),
	nb_setval('$included_file',OY).



%
% reconsult at startup...
%
'$do_startup_reconsult'(_X) :-
    '$init_win_graphics',
    fail.
'$do_startup_reconsult'(X) :-
	( '$access_yap_flags'(15, 0) ->
	  '$system_catch'(load_files(X, [silent(true)]), Module, Error, '$Error'(Error))
	;
	  '$swi_set_prolog_flag'(verbose, silent),
	  '$system_catch'(load_files(X, [silent(true),skip_unix_header(true)]),Module,_,fail)
	;
	  true
	),
	!,
	( '$access_yap_flags'(15, 0) -> true ; halt).
'$do_startup_reconsult'(_).

'$skip_unix_header'(Stream) :-
	peek_code(Stream, 0'#), !, % 35 is ASCII for '#
	skip(Stream, 10),
	'$skip_unix_header'(Stream).
'$skip_unix_header'(_).


source_file(FileName) :-
	recorded('$lf_loaded','$lf_loaded'(FileName, _, _),_).

source_file(Mod:Pred, FileName) :-
	current_module(Mod),
	Mod \= prolog,
	'$current_predicate_no_modules'(Mod,_,Pred),
	'$owned_by'(Pred, Mod, FileName).

'$owned_by'(T, Mod, FileName) :-
	'$is_multifile'(T, Mod),
	functor(T, Name, Arity),
	setof(FileName, Ref^recorded('$multifile_defs','$defined'(FileName,Name,Arity,Mod), Ref), L),
	lists:member(FileName, L).
'$owned_by'(T, Mod, FileName) :-
	'$owner_file'(T, Mod, FileName).

prolog_load_context(directory, DirName) :- 
	source_location(F, _),
	file_directory_name(F, DirName).
prolog_load_context(file, FileName) :- 
	source_location(FileName, _).
prolog_load_context(module, X) :-
	'$nb_getval'('$consulting_file', _, fail),
	'$current_module'(X).
prolog_load_context(source, F0) :-
	source_location(F0, _) /*,
	'$input_context'(Context),
	'$top_file'(Context, F0, F) */.
prolog_load_context(stream, Stream) :- 
	'$nb_getval'('$consulting_file', _, fail),
	'$current_loop_stream'(Stream).
% return this term for SWI compatibility.
prolog_load_context(term_position, '$stream_position'(0,Line,0,0,0)) :- 
	source_location(_, Line).


% if the file exports a module, then we can
% be imported from any module.
'$file_loaded'(Stream, M, Imports, TOpts) :-
	'$file_name'(Stream, F),
	'$ensure_file_loaded'(F, M, F1),
%	format( 'IL=~w~n', [(F1:Imports->M)] ),
	'$import_to_current_module'(F1, M, Imports, _, TOpts).

'$ensure_file_loaded'(F, M, F1) :-
	recorded('$module','$module'(F1,_NM,_P,_),_),
	recorded('$lf_loaded','$lf_loaded'(F1, _, _),_),
	same_file(F1,F), !.
'$ensure_file_loaded'(F, M, F1) :-
	% loaded from the same module, but does not define a module.
	recorded('$lf_loaded','$lf_loaded'(F1, _, M),_),
	same_file(F1,F), !.
	

% if the file exports a module, then we can
% be imported from any module.
'$file_unchanged'(Stream, M, Imports, TOpts) :-
	'$file_name'(Stream, F),
	'$ensure_file_unchanged'(F, M, F1),
%	format( 'IU=~w~n', [(F1:Imports->M)] ),
	'$import_to_current_module'(F1, M, Imports, _, TOpts).

'$ensure_file_unchanged'(F, M, F1) :-
	recorded('$module','$module'(F1,_NM,_P,_),_),
	recorded('$lf_loaded','$lf_loaded'(F1,Age,_),R),
	same_file(F1,F), !,
	'$file_is_unchanged'(F, R, Age).
'$ensure_file_unchanged'(F, M, F1) :-
	recorded('$lf_loaded','$lf_loaded'(F1, Age, M),R),
	same_file(F1,F), !,
	'$file_is_unchanged'(F, R, Age).

'$file_is_unchanged'(F, R, Age) :-
        time_file64(F,CurrentAge),
        ( (Age == CurrentAge ; Age = -1)  -> true; erase(R), fail).


% inform the file has been loaded and is now available.
'$loaded'(Stream, UserFile, M, OldF, Line, Reconsult, F, Dir, Opts) :-
	'$file_name'(Stream, F0),
	( F0 == user_input, nonvar(UserFile) -> UserFile = F ; F = F0 ),
	( F == user_input -> working_directory(Dir,Dir) ; file_directory_name(F, Dir) ),
	nb_setval('$consulting_file', F ),
	( Reconsult \== consult, Reconsult \== not_loaded, Reconsult \== changed, recorded('$lf_loaded','$lf_loaded'(F, _,_),R), erase(R), fail ; var(Reconsult) -> Reconsult = consult ; true ),
	( Reconsult \== consult, recorded('$lf_loaded','$lf_loaded'(F, _, _, _, _, _, _),R), erase(R), fail ; var(Reconsult) -> Reconsult = consult ; true ),
	( F == user_input -> Age = 0 ; time_file64(F, Age) ),
	( recordaifnot('$lf_loaded','$lf_loaded'( F, Age, M), _) -> true ; true ),
	recorda('$lf_loaded','$lf_loaded'( F, M, Reconsult, UserFile, OldF, Line, Opts), _).

'$set_encoding'(Encoding) :-
	'$current_loop_stream'(Stream),
	'$set_encoding'(Stream, Encoding).

'$set_encoding'(Stream, Encoding) :-
	( Encoding == default -> true ; set_stream(Stream, encoding(Encoding)) ).

/** @pred make is det

SWI-Prolog originally included this built-in as a Prolog version of the Unix `make`
utility program. In this case the idea is to reconsult all source files that have been
changed since they were originally compiled into Prolog. YAP has a limited implementation of make/0 that
just goes through every loaded file and verifies whether reloading is needed.

*/

make :-
	recorded('$lf_loaded','$lf_loaded'(F1,_M,reconsult,_,_,_,_),_),
	'$load_files'(F1, [if(changed)],make),
	fail.
make.

make_library_index(_Directory).

'$file_name'(Stream,F) :-
	stream_property(Stream, file_name(F)), !.
'$file_name'(user_input,user_input) :- !.
'$file_name'(user_output,user_ouput) :- !.
'$file_name'(user_error,user_error) :- !.
'$file_name'(_,[]).


'$fetch_stream_alias'(OldStream,Alias) :-
	stream_property(OldStream, alias(Alias)), !.

'$require'(_Ps, _M).

'$store_clause'('$source_location'(File, _Line):Clause, File) :-
	assert_static(Clause).


'$set_current_loop_stream'(OldStream, Stream) :-
	'$current_loop_stream'(OldStream), !,
	'$new_loop_stream'(Stream).
'$set_current_loop_stream'(_OldStream, Stream) :-
	'$new_loop_stream'(Stream).

'$new_loop_stream'(Stream) :-
	(var(Stream) ->
	    nb_delete('$loop_stream')
	;
	    nb_setval('$loop_stream',Stream)
	).
	    
'$current_loop_stream'(Stream) :-
	'$nb_getval'('$loop_stream',Stream, fail).

exists_source(File) :-
	'$full_filename'(File, _AbsFile, exists_source(File)).

% reload_file(File) :-
%         ' $source_base_name'(File, Compile),
%         findall(M-Opts,
%                 source_file_property(File, load_context(M, _, Opts)),
%                 Modules),
%         (   Modules = [First-OptsFirst|Rest]
%         ->  Extra = [ silent(false),
%                       register(false)
%                     ],
%             merge_options([if(true)|Extra], OptsFirst, OFirst),
% %            debug(make, 'Make: First load ~q', [load_files(First:Compile, OFirst)]),
%             load_files(First:Compile, OFirst),
%             forall(member(Context-Opts, Rest),
%                    ( merge_options([if(not_loaded)|Extra], Opts, O),
% %                     debug(make, 'Make: re-import: ~q',
% %                           [load_files(Context:Compile, O)]),
%                      load_files(Context:Compile, O)
%                    ))
%         ;   load_files(user:Compile)
%         ).

% ' $source_base_name'(File, Compile) :-
%         file_name_extension(Compile, Ext, File),
%         user:prolog_file_type(Ext, prolog), !.
% ' $source_base_name'(File, File).

source_file_property( File0, Prop) :-
	( nonvar(File0) -> absolute_file_name(File0,File) ; File = File0 ),
	'$source_file_property'( File, Prop).

'$source_file_property'( OldF, includes(F, Age)) :-
	recorded('$lf_loaded','$lf_loaded'( F, _M, include, _File, OldF, _Line, _), _),
	recorded('$lf_loaded','$lf_loaded'( F, Age, _), _).
'$source_file_property'( F, included_in(OldF, Line)) :-
	recorded('$lf_loaded','$lf_loaded'( F, _M, include, _File, OldF, Line, _), _).
'$source_file_property'( F, load_context(OldF, Line, Options)) :-
	recorded('$lf_loaded','$lf_loaded'( F, _M, V, _File, OldF, Line, Options), _), V \== include.
'$source_file_property'( F, modified(Age)) :-
	recorded('$lf_loaded','$lf_loaded'( F, Age, _), _).
'$source_file_property'( F, module(M)) :-
	recorded('$module','$module'(F,M,_,_),_).


/**

@}

@addtogroup YAPModules

@{

**/

%
% stub to prevent modules defined within the prolog module.
%
module(Mod, Decls) :-
	'$current_module'(prolog, Mod), !,
	'$export_preds'(Decls).

'$export_preds'([]).
'$export_preds'([N/A|Decls]) :-
    functor(S, N, A),
    '$sys_export'(S, prolog),
    '$export_preds'(Decls).


% prevent modules within the kernel module...
/** @pred use_module(? _M_,? _F_,+ _L_) is directive
    SICStus compatible way of using a module

If module _M_ is instantiated, import the procedures in _L_ to the
current module. Otherwise, operate as use_module/2, and load the files
specified by _F_, importing the predicates specified in the list _L_.
*/ 

use_module(M,F,Is) :- '$use_module'(M,F,Is).

'$use_module'(M,F,Is) :-  
	var(Is), !,
	'$use_module'(M,F,all).
'$use_module'(M,F,Is) :-
	nonvar(F), !,
        strip_module(F, M0, F0),
	'$load_files'(M0:F0, [if(not_loaded),must_be_module(true),imports(Is)], use_module(M,F,Is)),
	( var(M) -> true
	;
	   absolute_file_name( F0, F1, [expand(true),file_type(prolog)] ),
	  recorded('$module','$module'(F1,M,_,_),_)
	).
'$use_module'(M,F,Is) :-
	nonvar(M), !,
	strip_module(F, M0, F0),
	(
	    recorded('$module','$module'(F1,M,_,_),_)
	->
	    '$load_files'(M0:F1, [if(not_loaded),must_be_module(true),imports(Is)], use_module(M,F,Is))
	),
        (var(F0) -> F0 = F1 ; absolute_file_name( F1, F2, [expand(true),file_type(prolog)] ) -> F2 = F0 ).
'$use_module'(M,F,Is) :-
	'$do_error'(instantiation_error,use_module(M,F,Is)).

/**

  @pred reexport(+F) is directive
  @pred reexport(+F, +Decls ) is directive
  allow a module to use and export predicates from another module

Export all predicates defined in list  _F_ as if they were defined in
the current module.

Export predicates defined in file  _F_ according to  _Decls_. The
declarations should be of the form:

<ul>
    A list of predicate declarations to be exported. Each declaration
may be a predicate indicator or of the form `` _PI_ `as`
 _NewName_'', meaning that the predicate with indicator  _PI_ is
to be exported under name  _NewName_.

    `except`( _List_) 
In this case, all predicates not in  _List_ are exported. Moreover,
if ` _PI_ `as`  _NewName_` is found, the predicate with
indicator  _PI_ is to be exported under name  _NewName_ as
before.


Re-exporting predicates must be used with some care. Please, take into
account the following observations:

<ul>
    
The `reexport` declarations must be the first declarations to
follow the  `module` declaration.

    
It is possible to use both `reexport` and `use_module`, but
all predicates reexported are automatically available for use in the
current module.

    
In order to obtain efficient execution, YAP compiles dependencies
between re-exported predicates. In practice, this means that changing a
`reexport` declaration and then  *just* recompiling the file
may result in incorrect execution.

</ul>
**/
'$reexport'( TOpts, File, Imports, OldF ) :-
	'$lf_opt'(reexport, TOpts, Reexport),
	( Reexport == false -> true ;
	  '$lf_opt'('$parent_topts', TOpts, OldTOpts),
	  '$lf_opt'('$context_module', OldTOpts, OldContextModule),
	  '$import_to_current_module'(File, OldContextModule, Imports, _, TOpts),
	  '$extend_exports'(File, Imports, OldF )
	).

/**
@}
**/

/** @defgroup  YAPCompilerSettings Directing and Configuring the Compiler
    @ingroup  YAPConsulting

  The YAP system also includes a number of primitives designed to set
  compiler parameters and to track the state of the compiler. One
  important example is the number of directivees that allow setting up
  properties of predicates. It is also possible to enable or disable
  waraanings about possible issues with the code in the program, sich
  as the occurrence .

This section presents a set of built-ins predicates designed to set the 
environment for the compiler.
  prolog_to_os_filename(+ _PrologPath_,- _OsPath_) @anchor prolog_to_os_filename



This is an SWI-Prolog built-in. Converts between the internal Prolog
pathname conventions and the operating-system pathname conventions. The
internal conventions are Unix and this predicates is equivalent to =/2
(unify) on Unix systems. On DOS systems it will change the
directory-separator, limit the filename length map dots, except for the
last one, onto underscores.

@{

*/

% add_multifile_predicate when we start consult
'$add_multifile'(Name,Arity,Module) :-
	source_location(File,_),
	'$add_multifile'(File,Name,Arity,Module).

'$add_multifile'(File,Name,Arity,Module) :-
	recorded('$multifile_defs','$defined'(File,Name,Arity,Module), _), !.
%	print_message(warning,declaration((multifile Module:Name/Arity),ignored)).
'$add_multifile'(File,Name,Arity,Module) :-
	recordz('$multifile_defs','$defined'(File,Name,Arity,Module),_), !,
	fail.
'$add_multifile'(File,Name,Arity,Module) :-
	recorded('$mf','$mf_clause'(File,Name,Arity,Module,Ref),R),
	erase(R),
	'$erase_clause'(Ref,Module),
	fail.
'$add_multifile'(_,_,_,_).

% retract old multifile clauses for current file.
'$remove_multifile_clauses'(FileName) :-
	recorded('$multifile_defs','$defined'(FileName,_,_,_),R1),
	erase(R1),
	fail.
'$remove_multifile_clauses'(FileName) :-
	recorded('$mf','$mf_clause'(FileName,_,_,Module,Ref),R),
	'$erase_clause'(Ref, Module),
	erase(R),
	fail.
'$remove_multifile_clauses'(_).



/** @pred initialization(+ _G_) is iso
The compiler will execute goals  _G_ after consulting the current
file.

Notice that the goal will execute in the calling context, not within the file context, 
In other words, the source module and execution directory will be the ones of the parent
environment. Use initialization/2 for more flexible behavior.

*/
'$initialization'(V) :-
	var(V), !,
	'$do_error'(instantiation_error,initialization(V)).
'$initialization'(C) :- number(C), !,
	'$do_error'(type_error(callable,C),initialization(C)).
'$initialization'(C) :- db_reference(C), !,
	'$do_error'(type_error(callable,C),initialization(C)).
'$initialization'(G) :-
	'$show_consult_level'(Level1),
	% it will be done after we leave the current consult level.
	Level is Level1-1,
	recordz('$initialisation',do(Level,G),_),
	fail.
'$initialization'(_).



/** @pred initialization(+ _Goal_,+ _When_)

Similar to initialization/1, but allows for specifying when
 _Goal_ is executed while loading the program-text:


    + now
      Execute  _Goal_ immediately. 

    + after_load
      Execute  _Goal_ after loading program-text. This is the same as initialization/1. 

    + restore
      Do not execute  _Goal_ while loading the program, but only when restoring a state (not implemented yet). 

*/
initialization(G,OPT) :-
	'$initialization'(G,OPT).

'$initialization'(G,OPT) :-
	( 
	   var(G)
	->
	  '$do_error'(instantiation_error,initialization(G,OPT))
	;
	   number(G)
	->
	  '$do_error'(type_error(callable,G),initialization(G,OPT))
	;
	   db_reference(G)
	->
	  '$do_error'(type_error(callable,G),initialization(G,OPT))
	;
	   var(OPT)
	->
	  '$do_error'(instantiation_error,initialization(G,OPT))
	;
	  atom(OPT)
	->
	  (
	   OPT == now
	  ->
	   fail
	  ;
	   OPT == after_load
	  ->
	   fail
	  ;
	   OPT == restore
	  ->
	   fail
	  ;
	   '$do_error'(domain_error(initialization,OPT),initialization(OPT))
	  )
	;
	  '$do_error'(type_error(OPT),initialization(G,OPT))
	).
'$initialization'(G,now) :-
	( call(G) -> true ; format(user_error,':- ~w:~w failed.~n',[M,G]) ).
'$initialization'(G,after_load) :-
	'$initialization'(G).
% ignore for now.
'$initialization'(_G,restore).

/**

@}
*/

/**

@defgroup Conditional_Compilation Conditional Compilation

@ingroup  YAPCompilerSettings

Conditional compilation builds on the same principle as
term_expansion/2, goal_expansion/2 and the expansion of
grammar rules to compile sections of the source-code
conditionally. One of the reasons for introducing conditional
compilation is to simplify writing portable code.



Note that these directives can only be appear as separate terms in the
input.  Typical usage scenarios include:


    Load different libraries on different dialects

    Define a predicate if it is missing as a system predicate

    Realise totally different implementations for a particular
part of the code due to different capabilities.

    Realise different configuration options for your software.







~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
:- if(test1).
section_1.
:- elif(test2).
section_2.
:- elif(test3).
section_3.
:- else.
section_else.
:- endif.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


@{

*/

/** @pred    if( : _Goal_) 

Compile subsequent code only if  _Goal_ succeeds.  For enhanced
portability,  _Goal_ is processed by `expand_goal/2` before execution.
If an error occurs, the error is printed and processing proceeds as if
 _Goal_ has failed.

*/
%
% This is complicated because of embedded ifs.
%
'$if'(_,top) :- !, fail.
'$if'(_Goal,_) :-
	'$get_if'(Level0),
	Level is Level0 + 1,
	nb_setval('$if_level',Level),
	( '$nb_getval'('$endif', OldEndif, fail) -> true ; OldEndif=top),
	( '$nb_getval'('$if_skip_mode', Mode, fail) -> true ; Mode = run ),
	nb_setval('$endif',elif(Level,OldEndif,Mode)),
	fail.
% we are in skip mode, ignore....
'$if'(_Goal,_) :-
	'$nb_getval'('$endif',elif(Level, OldEndif, skip), fail), !,
	nb_setval('$endif',endif(Level, OldEndif, skip)).	
% we are in non skip mode, check....
'$if'(Goal,_) :-
	('$if_call'(Goal)
	    ->
	 % we will execute this branch, and later enter skip
	 '$nb_getval'('$endif', elif(Level,OldEndif,Mode), fail),
	 nb_setval('$endif',endif(Level,OldEndif,Mode))

	;
	 % we are now in skip, but can start an elif.
	 nb_setval('$if_skip_mode',skip)
	).

/** 
@pred    else
Start `else' branch.

*/
'$else'(top) :- !, fail.
'$else'(_) :-
	'$get_if'(0), !,
	'$do_error'(context_error(no_if),(:- else)).
% we have done an if, so just skip
'$else'(_) :-
	nb_getval('$endif',endif(_Level,_,_)), !,
	nb_setval('$if_skip_mode',skip).
% we can try the elif
'$else'(_) :-
	'$get_if'(Level),
	nb_getval('$endif',elif(Level,OldEndif,Mode)),
	nb_setval('$endif',endif(Level,OldEndif,Mode)),
	nb_setval('$if_skip_mode',run).

/** @pred   elif(+ _Goal_)


Equivalent to `:- else. :-if(Goal) ... :- endif.`  In a sequence
as below, the section below the first matching elif is processed, If
no test succeeds the else branch is processed.
*/
'$elif'(_,top) :- !, fail.
'$elif'(Goal,_) :-
	'$get_if'(0),
	'$do_error'(context_error(no_if),(:- elif(Goal))).
% we have done an if, so just skip
'$elif'(_,_) :-
	 nb_getval('$endif',endif(_,_,_)), !,
	 nb_setval('$if_skip_mode',skip).
% we can try the elif
'$elif'(Goal,_) :-
	'$get_if'(Level),
	nb_getval('$endif',elif(Level,OldEndif,Mode)),
	('$if_call'(Goal)
	    ->
% we will not skip, and we will not run any more branches.
	 nb_setval('$endif',endif(Level,OldEndif,Mode)),
	 nb_setval('$if_skip_mode',run)
	;
% we will (keep) on skipping
	 nb_setval('$if_skip_mode',skip)
	).
'$elif'(_,_).

/** @pred    endif
End of conditional compilation.

*/
'$endif'(top) :- !, fail.
'$endif'(_) :-
% unmmatched endif.
	'$get_if'(0),
	'$do_error'(context_error(no_if),(:- endif)).
'$endif'(_) :-
% back to where you belong.
	'$get_if'(Level),
	nb_getval('$endif',Endif),
	Level0 is Level-1,
	nb_setval('$if_level',Level0),
	arg(2,Endif,OldEndif),
	arg(3,Endif,OldMode),
	nb_setval('$endif',OldEndif),
	nb_setval('$if_skip_mode',OldMode).


'$if_call'(G) :-
	catch('$eval_if'(G), E, (print_message(error, E), fail)).

'$eval_if'(Goal) :-
	expand_term(Goal,TrueGoal),
	once(TrueGoal).

'$if_directive'((:- if(_))).
'$if_directive'((:- else)).
'$if_directive'((:- elif(_))).
'$if_directive'((:- endif)).


'$comp_mode'(_OldCompMode, CompMode) :-
	var(CompMode), !. % just do nothing.
'$comp_mode'(OldCompMode, assert_all) :-
	'$fetch_comp_status'(OldCompMode),
	nb_setval('$assert_all',on).
'$comp_mode'(OldCompMode, source) :-
	'$fetch_comp_status'(OldCompMode),
	'$set_yap_flags'(11,1).
'$comp_mode'(OldCompMode, compact) :-
	'$fetch_comp_status'(OldCompMode),
	'$set_yap_flags'(11,0).

'$fetch_comp_status'(assert_all) :-
	'$nb_getval'('$assert_all',on, fail), !.
'$fetch_comp_status'(source) :-
	 '$access_yap_flags'(11,1), !.
'$fetch_comp_status'(compact).

/**
@}
*/
