/************************************************************************
**                                                                     **
**                   The YapTab/YapOr/OPTYap systems                   **
**                                                                     **
** YapTab extends the Yap Prolog engine to support sequential tabling  **
** YapOr extends the Yap Prolog engine to support or-parallelism       **
** OPTYap extends the Yap Prolog engine to support or-parallel tabling **
**                                                                     **
**                                                                     **
**      Yap Prolog was developed at University of Porto, Portugal      **
**                                                                     **
************************************************************************/

/************************************
**      Includes & Prototypes      **
************************************/

#include "Yap.h"
#if defined(YAPOR) || defined(TABLING)
#include "Yatom.h"
#include "YapHeap.h"
#include "SWI-Prolog.h"
#ifdef YAPOR
#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#include "or.macros.h"
#endif /* YAPOR */
#ifdef TABLING
#include "tab.macros.h"
#endif /* TABLING */

#ifdef TABLING
static Int p_freeze_choice_point( USES_REGS1 );
static Int p_wake_choice_point( USES_REGS1 );
static Int p_abolish_frozen_choice_points_until( USES_REGS1 );
static Int p_abolish_frozen_choice_points_all( USES_REGS1 );
static Int p_table( USES_REGS1 );
static Int p_tabling_mode( USES_REGS1 );
static Int p_abolish_table( USES_REGS1 );
static Int p_abolish_all_tables( USES_REGS1 );
static Int p_show_tabled_predicates( USES_REGS1 );
static Int p_show_table( USES_REGS1 );
static Int p_show_all_tables( USES_REGS1 );
static Int p_show_global_trie( USES_REGS1 );
static Int p_show_statistics_table( USES_REGS1 );
static Int p_show_statistics_tabling( USES_REGS1 );
static Int p_show_statistics_global_trie( USES_REGS1 );
#endif /* TABLING */

#ifdef YAPOR_TEAMS
static Int p_testing(  USES_REGS1 );
static Int p_create_team(  USES_REGS1 );
static Int p_create_comm(  USES_REGS1 );
static Int p_run_goal(  USES_REGS1 );
static Int p_get_results(  USES_REGS1 );
static Int p_parallel_barrier(  USES_REGS1 ); 
static Int p_results_barrier(  USES_REGS1 ); 
#ifdef YAPOR_MPI
static Int p_mpi_parallel_barrier(  USES_REGS1 ); 
static Int p_mpi_parallel_new_answer( USES_REGS1 );
static Int p_mpi_get_results(  USES_REGS1 );
#endif
#endif
#ifdef YAPOR
static Int p_parallel_mode( USES_REGS1 );
static Int p_yapor_start( USES_REGS1 );
static Int p_yapor_workers( USES_REGS1 );
static Int p_worker( USES_REGS1 );
static Int p_parallel_new_answer( USES_REGS1 );
static Int p_parallel_get_answers( USES_REGS1 );
static Int p_show_statistics_or( USES_REGS1 );
#endif /* YAPOR */
#if defined(YAPOR) && defined(TABLING)
static Int p_show_statistics_opt( USES_REGS1 );
#endif /* YAPOR && TABLING */
static Int p_get_optyap_statistics( USES_REGS1 );

#ifdef YAPOR
static inline realtime current_time(void);
#endif /* YAPOR */

#ifdef TABLING
static inline struct page_statistics show_statistics_table_entries(IOSTREAM *out);
#if defined(THREADS_FULL_SHARING) || defined(THREADS_CONSUMER_SHARING)
static inline struct page_statistics show_statistics_subgoal_entries(IOSTREAM *out);
#endif /* THREADS_FULL_SHARING || THREADS_CONSUMER_SHARING */
static inline struct page_statistics show_statistics_subgoal_frames(IOSTREAM *out);
static inline struct page_statistics show_statistics_dependency_frames(IOSTREAM *out);
static inline struct page_statistics show_statistics_subgoal_trie_nodes(IOSTREAM *out);
static inline struct page_statistics show_statistics_subgoal_trie_hashes(IOSTREAM *out);
static inline struct page_statistics show_statistics_answer_trie_nodes(IOSTREAM *out);
static inline struct page_statistics show_statistics_answer_trie_hashes(IOSTREAM *out);
#if defined(THREADS_FULL_SHARING)
static inline struct page_statistics show_statistics_answer_ref_nodes(IOSTREAM *out);
#endif /* THREADS_FULL_SHARING */
static inline struct page_statistics show_statistics_global_trie_nodes(IOSTREAM *out);
static inline struct page_statistics show_statistics_global_trie_hashes(IOSTREAM *out);
#endif /* TABLING */
#ifdef YAPOR
static inline struct page_statistics show_statistics_or_frames(IOSTREAM *out);
static inline struct page_statistics show_statistics_query_goal_solution_frames(IOSTREAM *out);
static inline struct page_statistics show_statistics_query_goal_answer_frames(IOSTREAM *out);
#endif /* YAPOR */
#if defined(YAPOR) && defined(TABLING)
static inline struct page_statistics show_statistics_suspension_frames(IOSTREAM *out);
#ifdef TABLING_INNER_CUTS
static inline struct page_statistics show_statistics_table_subgoal_solution_frames(IOSTREAM *out);
static inline struct page_statistics show_statistics_table_subgoal_answer_frames(IOSTREAM *out);
#endif /* TABLING_INNER_CUTS */
#endif /* YAPOR && TABLING */



/************************************
**      Macros & Declarations      **
************************************/

struct page_statistics {
#ifdef USE_PAGES_MALLOC
  long pages_in_use;    /* same as struct pages (opt.structs.h) */
#endif /* USE_PAGES_MALLOC */
  long structs_in_use;  /* same as struct pages (opt.structs.h) */
  long bytes_in_use;
};

#define PgEnt_bytes_in_use(STATS)  STATS.bytes_in_use

#ifdef USE_PAGES_MALLOC
#ifdef DEBUG_TABLING
#define CHECK_PAGE_FREE_STRUCTS(STR_TYPE, PAGE)                                            \
        { pg_hd_ptr pg_hd;                                                                 \
          STR_TYPE *aux_ptr;                                                               \
          long cont = 0;                                                                   \
          pg_hd = PgEnt_first(PAGE);                                                       \
          while (pg_hd) {                                                                  \
            aux_ptr = PgHd_first_str(pg_hd);                                               \
            while (aux_ptr) {                                                              \
	      cont++;                                                                      \
	      aux_ptr = aux_ptr->next;                                                     \
	    }                                                                              \
	    pg_hd = PgHd_next(pg_hd);                                                      \
	  }                                                                                \
	  TABLING_ERROR_CHECKING(CHECK_PAGE_FREE_STRUCTS, PgEnt_strs_free(PAGE) != cont);  \
	}
#else
#define CHECK_PAGE_FREE_STRUCTS(STR_TYPE, PAGE)
#endif /* DEBUG_TABLING */
#define INIT_PAGE_STATS(STATS)                                  \
        PgEnt_pages_in_use(STATS) = 0;                          \
        PgEnt_strs_in_use(STATS) = 0
#define INCREMENT_PAGE_STATS(STATS, PAGE)                       \
        PgEnt_pages_in_use(STATS) += PgEnt_pages_in_use(PAGE);  \
        PgEnt_strs_in_use(STATS) += PgEnt_strs_in_use(PAGE)
#define INCREMENT_AUX_STATS(STATS, BYTES, PAGES)                \
        BYTES += PgEnt_bytes_in_use(STATS);                     \
        PAGES += PgEnt_pages_in_use(STATS)
#define SHOW_PAGE_STATS_MSG(STR_NAME)          "  " STR_NAME "   %10ld bytes (%ld pages and %ld structs in use)\n"
#define SHOW_PAGE_STATS_ARGS(STATS, STR_TYPE)  PgEnt_strs_in_use(STATS) * sizeof(STR_TYPE), PgEnt_pages_in_use(STATS), PgEnt_strs_in_use(STATS)
#else /* !USE_PAGES_MALLOC */
#define CHECK_PAGE_FREE_STRUCTS(STR_TYPE, PAGE)
#define INIT_PAGE_STATS(STATS)                                  \
        PgEnt_strs_in_use(STATS) = 0
#define INCREMENT_PAGE_STATS(STATS, PAGE)                       \
        PgEnt_strs_in_use(STATS) += PgEnt_strs_in_use(PAGE)
#define INCREMENT_AUX_STATS(STATS, BYTES, PAGES)                \
        BYTES += PgEnt_bytes_in_use(STATS)
#define SHOW_PAGE_STATS_MSG(STR_NAME)          "  " STR_NAME "   %10ld bytes (%ld structs in use)\n"
#define SHOW_PAGE_STATS_ARGS(STATS, STR_TYPE)  PgEnt_strs_in_use(STATS) * sizeof(STR_TYPE), PgEnt_strs_in_use(STATS)
#endif /* USE_PAGES_MALLOC */


#if defined(THREADS) && defined(TABLING)
#define GET_ALL_PAGE_STATS(STATS, STR_TYPE, _PAGES)                    \
        LOCK(GLOBAL_ThreadHandlesLock);                                \
        CHECK_PAGE_FREE_STRUCTS(STR_TYPE, GLOBAL##_PAGES);             \
        INCREMENT_PAGE_STATS(STATS, GLOBAL##_PAGES);                   \
        { int wid;                                                     \
          for (wid = 0; wid < MAX_THREADS; wid++) {                    \
            if (! Yap_local[wid])                                      \
              break;                                                   \
            if (REMOTE_ThreadHandle(wid).in_use) {                     \
              CHECK_PAGE_FREE_STRUCTS(STR_TYPE, REMOTE##_PAGES(wid));  \
              INCREMENT_PAGE_STATS(STATS, REMOTE##_PAGES(wid));        \
            }                                                          \
          }                                                            \
	}                                                              \
        UNLOCK(GLOBAL_ThreadHandlesLock)
#else
#define GET_ALL_PAGE_STATS(STATS, STR_TYPE, _PAGES)                    \
        CHECK_PAGE_FREE_STRUCTS(STR_TYPE, GLOBAL##_PAGES);             \
        INCREMENT_PAGE_STATS(STATS, GLOBAL##_PAGES)
#endif

#define GET_PAGE_STATS(STATS, STR_TYPE, _PAGES)                                 \
        INIT_PAGE_STATS(STATS);                                                 \
        GET_ALL_PAGE_STATS(STATS, STR_TYPE, _PAGES);                            \
        PgEnt_bytes_in_use(STATS) = PgEnt_strs_in_use(STATS) * sizeof(STR_TYPE)
#define SHOW_PAGE_STATS(OUT_STREAM, STR_TYPE, _PAGES, STR_NAME)	                                       \
        { struct page_statistics stats;                                                                \
          GET_PAGE_STATS(stats, STR_TYPE, _PAGES);	                                               \
	  Sfprintf(OUT_STREAM, SHOW_PAGE_STATS_MSG(STR_NAME), SHOW_PAGE_STATS_ARGS(stats, STR_TYPE));  \
          return stats;                                                                                \
        }



/*******************************
**      Global functions      **
*******************************/

void Yap_init_optyap_preds(void) {
#ifdef TABLING
  Yap_InitCPred("freeze_choice_point", 1, p_freeze_choice_point, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("wake_choice_point", 1, p_wake_choice_point, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("abolish_frozen_choice_points", 1, p_abolish_frozen_choice_points_until, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("abolish_frozen_choice_points", 0, p_abolish_frozen_choice_points_all, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_table", 3, p_table, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_tabling_mode", 3, p_tabling_mode, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_abolish_table", 2, p_abolish_table, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("abolish_all_tables", 0, p_abolish_all_tables, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("show_tabled_predicates", 1, p_show_tabled_predicates, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_show_table", 3, p_show_table, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("show_all_tables", 1, p_show_all_tables, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("show_global_trie", 1, p_show_global_trie, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_table_statistics", 3, p_show_statistics_table, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("tabling_statistics", 1, p_show_statistics_tabling, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("global_trie_statistics", 1, p_show_statistics_global_trie, SafePredFlag|SyncPredFlag);
#endif /* TABLING */
#ifdef YAPOR
  Yap_InitCPred("parallel_mode", 1, p_parallel_mode, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_yapor_start", 1, p_yapor_start, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_yapor_workers", 1, p_yapor_workers, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_worker", 0, p_worker, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_parallel_new_answer", 1, p_parallel_new_answer, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_parallel_get_answers", 1, p_parallel_get_answers, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("or_statistics", 1, p_show_statistics_or, SafePredFlag|SyncPredFlag);
#endif /* YAPOR */
#ifdef YAPOR_TEAMS
  Yap_InitCPred("testing", 1, p_testing, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_parallel_barrier", 1, p_parallel_barrier, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_mpi_parallel_barrier", 1, p_mpi_parallel_barrier, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_results_barrier", 1, p_results_barrier, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_create_team", 2, p_create_team, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_get_results", 2, p_get_results, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_create_comm", 3, p_create_comm, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_run_parallel_goal", 3, p_run_goal, SafePredFlag|SyncPredFlag);
#ifdef YAPOR_MPI
  Yap_InitCPred("$c_mpi_parallel_new_answer", 1, p_mpi_parallel_new_answer, SafePredFlag|SyncPredFlag);
  Yap_InitCPred("$c_mpi_get_results", 2, p_mpi_get_results, SafePredFlag|SyncPredFlag);
#endif
#endif
#if defined(YAPOR) && defined(TABLING)
  Yap_InitCPred("opt_statistics", 1, p_show_statistics_opt, SafePredFlag|SyncPredFlag);
#endif /* YAPOR && TABLING */
  Yap_InitCPred("$c_get_optyap_statistics", 3, p_get_optyap_statistics, SafePredFlag|SyncPredFlag);
}


#ifdef YAPOR
void finish_yapor(void) {
  GLOBAL_execution_time = current_time() - GLOBAL_execution_time;
  GLOBAL_parallel_mode = PARALLEL_MODE_ON;
  return;
}
#endif /* YAPOR */



/***********************************
**      Tabling C Predicates      **
***********************************/

#ifdef TABLING
static Int p_freeze_choice_point( USES_REGS1 ) {
  if (IsVarTerm(Deref(ARG1))) {
    Int offset = freeze_current_cp();
    return Yap_unify(ARG1, MkIntegerTerm(offset));
  }
  return (FALSE);
}


static Int p_wake_choice_point( USES_REGS1 ) {
  Term term_offset = Deref(ARG1);
  if (IsIntegerTerm(term_offset))
    wake_frozen_cp(IntegerOfTerm(term_offset));
  return (FALSE);
}


static Int p_abolish_frozen_choice_points_until( USES_REGS1 ) {
  Term term_offset = Deref(ARG1);
  if (IsIntegerTerm(term_offset))
    abolish_frozen_cps_until(IntegerOfTerm(term_offset));
  return (TRUE);
}


static Int p_abolish_frozen_choice_points_all( USES_REGS1 ) {
  abolish_frozen_cps_all();
  return (TRUE);
}


static Int p_table( USES_REGS1 ) {
  Term mod, t, list;
  PredEntry *pe;
  Atom at;
  int arity;
  tab_ent_ptr tab_ent;
#ifdef MODE_DIRECTED_TABLING
  int* mode_directed = NULL;
#endif /* MODE_DIRECTED_TABLING */
  
  mod = Deref(ARG1);
  t = Deref(ARG2);
  list = Deref(ARG3);

  if (IsAtomTerm(t)) {
    at = AtomOfTerm(t);
    pe = RepPredProp(PredPropByAtom(at, mod));
    arity = 0;
  } else if (IsApplTerm(t)) {
    at = NameOfFunctor(FunctorOfTerm(t));
    pe = RepPredProp(PredPropByFunc(FunctorOfTerm(t), mod));
    arity = ArityOfFunctor(FunctorOfTerm(t));
  } else
    return (FALSE);
  if (list != TermNil) {  /* non-empty list */
#ifndef MODE_DIRECTED_TABLING
    Yap_Error(INTERNAL_COMPILER_ERROR, TermNil, "invalid tabling declaration for %s/%d (mode directed tabling not enabled)", AtomName(at), arity);
    return(FALSE);
#else 
    /*************************************************************************************
      The mode operator declaration is reordered as follows:
         1. arguments with mode 'index'         (any number)
         2. arguments with mode 'min' and 'max' (any number, following the original order)
         3. arguments with mode 'all'           (any number)
         4. arguments with mode 'sum' or 'last' (only one of the two is allowed)
         5. arguments with mode 'first'         (any number)
    *************************************************************************************/
    int pos_index = 0;
    int pos_min_max = 0;
    int pos_all = 0;
    int pos_sum_last = 0;
    int pos_first = 0;
    int i;
    int *aux_mode_directed;

    aux_mode_directed = malloc(arity * sizeof(int));
    for (i = 0; i < arity; i++) {
      int mode = IntOfTerm(HeadOfTerm(list));
      if (mode == MODE_DIRECTED_INDEX)
	pos_index++;
      else if (mode == MODE_DIRECTED_MIN || mode == MODE_DIRECTED_MAX)
	pos_min_max++;
      else if (mode == MODE_DIRECTED_ALL)
	pos_all++;
      else if (mode == MODE_DIRECTED_SUM || mode == MODE_DIRECTED_LAST) {
	if (pos_sum_last) {
	  free(aux_mode_directed);
	  Yap_Error(INTERNAL_COMPILER_ERROR, TermNil, "invalid tabling declaration for %s/%d (more than one argument with modes 'sum' and/or 'last')", AtomName(at), arity);
	  return(FALSE);
	} else
	  pos_sum_last = 1;
      }
      aux_mode_directed[i] = mode;
      list = TailOfTerm(list);
    }
    pos_first = pos_index + pos_min_max + pos_all + pos_sum_last;
    pos_sum_last = pos_index + pos_min_max + pos_all;
    pos_all = pos_index + pos_min_max;
    pos_min_max = pos_index;
    pos_index = 0;
    ALLOC_BLOCK(mode_directed, arity * sizeof(int), int);
    for (i = 0; i < arity; i++) {
      int aux_pos = 0;
      if (aux_mode_directed[i] == MODE_DIRECTED_INDEX)
	aux_pos = pos_index++;	
      else if (aux_mode_directed[i] == MODE_DIRECTED_MIN || aux_mode_directed[i] == MODE_DIRECTED_MAX)
	aux_pos = pos_min_max++;
      else if (aux_mode_directed[i] == MODE_DIRECTED_ALL)
	aux_pos = pos_all++;		
      else if (aux_mode_directed[i] == MODE_DIRECTED_SUM || aux_mode_directed[i] == MODE_DIRECTED_LAST)
	aux_pos = pos_sum_last++;	
      else if(aux_mode_directed[i] == MODE_DIRECTED_FIRST)
	aux_pos = pos_first++;
      mode_directed[aux_pos] = MODE_DIRECTED_SET(i, aux_mode_directed[i]);
    }
    free(aux_mode_directed);
#endif /* MODE_DIRECTED_TABLING */
  }
  if (pe->PredFlags & TabledPredFlag)
    return (TRUE);  /* predicate already tabled */
  if (pe->cs.p_code.FirstClause)
    return (FALSE);  /* predicate already compiled */
  pe->PredFlags |= TabledPredFlag;
  new_table_entry(tab_ent, pe, at, arity, mode_directed);
  pe->TableOfPred = tab_ent;
  return (TRUE);
}


static Int p_tabling_mode( USES_REGS1 ) {
  Term mod, t, tvalue;
  tab_ent_ptr tab_ent;

  mod = Deref(ARG1);
  t = Deref(ARG2);
  if (IsAtomTerm(t))
    tab_ent = RepPredProp(PredPropByAtom(AtomOfTerm(t), mod))->TableOfPred;
  else if (IsApplTerm(t))
    tab_ent = RepPredProp(PredPropByFunc(FunctorOfTerm(t), mod))->TableOfPred;
  else
    return (FALSE);
  tvalue = Deref(ARG3);
  if (IsVarTerm(tvalue)) {
    t = TermNil;
    if (IsMode_LocalTrie(TabEnt_flags(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomLocalTrie), t);
    else if (IsMode_GlobalTrie(TabEnt_flags(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomGlobalTrie), t);
    if (IsMode_ExecAnswers(TabEnt_flags(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomExecAnswers), t);
    else if (IsMode_LoadAnswers(TabEnt_flags(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomLoadAnswers), t);
    if (IsMode_Batched(TabEnt_flags(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomBatched), t);
    else if (IsMode_Local(TabEnt_flags(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomLocal), t);
    t = MkPairTerm(MkAtomTerm(AtomDefault), t);
    t = MkPairTerm(t, TermNil);
    if (IsMode_LocalTrie(TabEnt_mode(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomLocalTrie), t);
    else if (IsMode_GlobalTrie(TabEnt_mode(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomGlobalTrie), t);
    if (IsMode_ExecAnswers(TabEnt_mode(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomExecAnswers), t);
    else if (IsMode_LoadAnswers(TabEnt_mode(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomLoadAnswers), t);
    if (IsMode_Batched(TabEnt_mode(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomBatched), t);
    else if (IsMode_Local(TabEnt_mode(tab_ent)))
      t = MkPairTerm(MkAtomTerm(AtomLocal), t);
    Bind((CELL *) tvalue, t);
    return(TRUE);
  } else if (IsIntTerm(tvalue)) {
    Int value = IntOfTerm(tvalue);
    if (value == 1) {  /* batched */
      SetMode_Batched(TabEnt_flags(tab_ent));
      if (! IsMode_Local(yap_flags[TABLING_MODE_FLAG])) {
	SetMode_Batched(TabEnt_mode(tab_ent));
	return(TRUE);
      }
    } else if (value == 2) {  /* local */
      SetMode_Local(TabEnt_flags(tab_ent));
      if (! IsMode_Batched(yap_flags[TABLING_MODE_FLAG])) {
	SetMode_Local(TabEnt_mode(tab_ent));
	return(TRUE);
      }
    } else if (value == 3) {  /* exec_answers */
      SetMode_ExecAnswers(TabEnt_flags(tab_ent));
      if (! IsMode_LoadAnswers(yap_flags[TABLING_MODE_FLAG])) {
	SetMode_ExecAnswers(TabEnt_mode(tab_ent));
	return(TRUE);
      }
    } else if (value == 4) {  /* load_answers */
      SetMode_LoadAnswers(TabEnt_flags(tab_ent));
      if (! IsMode_ExecAnswers(yap_flags[TABLING_MODE_FLAG])) {
	SetMode_LoadAnswers(TabEnt_mode(tab_ent));
	return(TRUE);
      }
    } else if (value == 5) {  /* local_trie */
      SetMode_LocalTrie(TabEnt_flags(tab_ent));
      if (! IsMode_GlobalTrie(yap_flags[TABLING_MODE_FLAG])) {
	SetMode_LocalTrie(TabEnt_mode(tab_ent));
	return(TRUE);
      }
    } else if (value == 6) {  /* global_trie */
      SetMode_GlobalTrie(TabEnt_flags(tab_ent));
      if (! IsMode_LocalTrie(yap_flags[TABLING_MODE_FLAG])) {
	SetMode_GlobalTrie(TabEnt_mode(tab_ent));
	return(TRUE);
      }
    }    
  }
  return (FALSE);
}


static Int p_abolish_table( USES_REGS1 ) {
  Term mod, t;
  tab_ent_ptr tab_ent;

  mod = Deref(ARG1);
  t = Deref(ARG2);
  if (IsAtomTerm(t))
    tab_ent = RepPredProp(PredPropByAtom(AtomOfTerm(t), mod))->TableOfPred;
  else if (IsApplTerm(t))
    tab_ent = RepPredProp(PredPropByFunc(FunctorOfTerm(t), mod))->TableOfPred;
  else
    return (FALSE);
  abolish_table(tab_ent);
  return (TRUE);
}


static Int p_abolish_all_tables( USES_REGS1 ) {
  tab_ent_ptr tab_ent;

  tab_ent = GLOBAL_root_tab_ent;
  while (tab_ent) {
    abolish_table(tab_ent);
    tab_ent = TabEnt_next(tab_ent);
  }
  return (TRUE);
}


static Int p_show_tabled_predicates( USES_REGS1 ) {
  IOSTREAM *out;
  tab_ent_ptr tab_ent;
  Term t = Deref(ARG1);

  if (IsVarTerm(t) || !IsAtomTerm(t))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t))))
    return FALSE;
  tab_ent = GLOBAL_root_tab_ent;
  Sfprintf(out, "Tabled predicates\n");
  if (tab_ent == NULL)
    Sfprintf(out, "  NONE\n");
  else while(tab_ent) {
    Sfprintf(out, "  %s/%d\n", AtomName(TabEnt_atom(tab_ent)), TabEnt_arity(tab_ent));
    tab_ent = TabEnt_next(tab_ent);
  }
  PL_release_stream(out);
  return (TRUE);
}


static Int p_show_table( USES_REGS1 ) {
  IOSTREAM *out;
  Term mod, t;
  tab_ent_ptr tab_ent;
  Term t1 = Deref(ARG1);

  if (IsVarTerm(t1) || !IsAtomTerm(t1))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t1))))
    return FALSE;
  mod = Deref(ARG2);
  t = Deref(ARG3);
  if (IsAtomTerm(t))
    tab_ent = RepPredProp(PredPropByAtom(AtomOfTerm(t), mod))->TableOfPred;
  else if (IsApplTerm(t))
    tab_ent = RepPredProp(PredPropByFunc(FunctorOfTerm(t), mod))->TableOfPred;
  else {
    PL_release_stream(out);
    return (FALSE);
  }
  show_table(tab_ent, SHOW_MODE_STRUCTURE, out);
  PL_release_stream(out);
  return (TRUE);
}


static Int p_show_all_tables( USES_REGS1 ) {
  IOSTREAM *out;
  tab_ent_ptr tab_ent;
  Term t = Deref(ARG1);

  if (IsVarTerm(t) || !IsAtomTerm(t))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t))))
    return FALSE;
  tab_ent = GLOBAL_root_tab_ent;
  while(tab_ent) {
    show_table(tab_ent, SHOW_MODE_STRUCTURE, out);
    tab_ent = TabEnt_next(tab_ent);
  }
  PL_release_stream(out);
  return (TRUE);
}


static Int p_show_global_trie( USES_REGS1 ) {
  IOSTREAM *out;
  Term t = Deref(ARG1);

  if (IsVarTerm(t) || !IsAtomTerm(t))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t))))
    return FALSE;
  show_global_trie(SHOW_MODE_STRUCTURE, out);
  PL_release_stream(out);
  return (TRUE);
}


static Int p_show_statistics_table( USES_REGS1 ) {
  IOSTREAM *out;
  Term mod, t;
  tab_ent_ptr tab_ent;
  Term t1 = Deref(ARG1);

  if (IsVarTerm(t1) || !IsAtomTerm(t1))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t1))))
    return FALSE;
  mod = Deref(ARG2);
  t = Deref(ARG3);
  if (IsAtomTerm(t))
    tab_ent = RepPredProp(PredPropByAtom(AtomOfTerm(t), mod))->TableOfPred;
  else if (IsApplTerm(t))
    tab_ent = RepPredProp(PredPropByFunc(FunctorOfTerm(t), mod))->TableOfPred;
  else {
    PL_release_stream(out);
    return (FALSE);
  }
  show_table(tab_ent, SHOW_MODE_STATISTICS, out);
  PL_release_stream(out);
  return (TRUE);
}


static Int p_show_statistics_tabling( USES_REGS1 ) {
  struct page_statistics stats;
  long bytes, total_bytes = 0;
#ifdef USE_PAGES_MALLOC
  long total_pages = 0;
#endif /* USE_PAGES_MALLOC */
  IOSTREAM *out;
  Term t = Deref(ARG1);

  if (IsVarTerm(t) || !IsAtomTerm(t))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t))))
    return FALSE;
  bytes = 0;
  Sfprintf(out, "Execution data structures\n");
  stats = show_statistics_table_entries(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#if defined(THREADS_FULL_SHARING) || defined(THREADS_CONSUMER_SHARING)
  stats = show_statistics_subgoal_entries(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#endif /* THREADS_FULL_SHARING || THREADS_CONSUMER_SHARING */
  stats = show_statistics_subgoal_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_dependency_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  Sfprintf(out, "  Memory in use (I):               %10ld bytes\n\n", bytes);
  total_bytes += bytes;
  bytes = 0;
  Sfprintf(out, "Local trie data structures\n");
  stats = show_statistics_subgoal_trie_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_answer_trie_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_subgoal_trie_hashes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_answer_trie_hashes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#if defined(THREADS_FULL_SHARING)
  stats = show_statistics_answer_ref_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#endif /* THREADS_FULL_SHARING */
  Sfprintf(out, "  Memory in use (II):              %10ld bytes\n\n", bytes);
  total_bytes += bytes;
  bytes = 0;
  Sfprintf(out, "Global trie data structures\n");
  stats = show_statistics_global_trie_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_global_trie_hashes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  Sfprintf(out, "  Memory in use (III):             %10ld bytes\n\n", bytes);
  total_bytes += bytes;
#ifdef USE_PAGES_MALLOC
  Sfprintf(out, "Total memory in use (I+II+III):    %10ld bytes (%ld pages in use)\n",
          total_bytes, total_pages);
  Sfprintf(out, "Total memory allocated:            %10ld bytes (%ld pages in total)\n",
          PgEnt_pages_in_use(GLOBAL_pages_alloc) * Yap_page_size, PgEnt_pages_in_use(GLOBAL_pages_alloc));
#else 
  Sfprintf(out, "Total memory in use (I+II+III):    %10ld bytes\n", total_bytes);
#endif /* USE_PAGES_MALLOC */
  PL_release_stream(out);
  return (TRUE);
}


static Int p_show_statistics_global_trie( USES_REGS1 ) {
  IOSTREAM *out;
  Term t = Deref(ARG1);

  if (IsVarTerm(t) || !IsAtomTerm(t))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t))))
    return FALSE;
  show_global_trie(SHOW_MODE_STATISTICS, out);
  PL_release_stream(out);
  return (TRUE);
}
#endif /* TABLING */



/*********************************
**      YapOr C Predicates      **
*********************************/
#ifdef YAPOR_TEAMS

#include "mpi.h" 
static Int p_mpi_parallel_barrier( USES_REGS1 ) {
  int comm_id = IntOfTerm(Deref(ARG1));   

  int a;

  printf("*************_________________BARRIER __________ BARRIER _______%d_____ BARRIER ______________ BARRIER_______________************* %p\n",comm_id,R_COMM_mpi_comm(comm_id));

  MPI_Recv(&a,1,MPI_INT,0,21,R_COMM_mpi_comm(comm_id),MPI_STATUS_IGNORE);

  printf("*************_________________BARRIER __________ BARRIER ____________ BARRIER ______________ BARRIER_______________*************\n");

  return(TRUE);

}

static Int p_parallel_barrier( USES_REGS1 ) {
  int comm_id = IntOfTerm(Deref(ARG1));   

  //printf("\n  \n ******************************************************** ANTES P_PARALLEL_BARRIER  %p  %p******************************************************** \n",R_COMM_bm_present_teams(comm_id),R_COMM_bm_running_teams(comm_id));

  while(!BITMAP_same(R_COMM_bm_present_teams(comm_id),R_COMM_bm_running_teams(comm_id)));

  //printf("DEPOSI P_PARALLEL_BARRIER %d   \n",comm_id );


  return(TRUE);

}

//#include "mpi.h" 

static Int p_create_team( USES_REGS1 ) {
  int n_workers = IntOfTerm(Deref(ARG1));   

  //printf("C_CREATE_TEAM %d\n",n_workers);

  

  GLOBAL_counter_teams++;

  //MPI_Comm_size(MPI_COMM_WORLD, &everyone); 

#ifdef YAPOR_MPI
  #include "mpi.h" 
  //MPI_Init(NULL, NULL);
  MPI_Comm everyone;
  MPI_Comm spawn;
 
  int rank = 100;

  MPI_Comm_spawn("yap", MPI_ARGV_NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &everyone, MPI_ERRCODES_IGNORE); 
  MPI_Comm_rank(everyone, &rank); 
  printf(" EEEEEE  %d EEEEE  %d\n",rank,getpid());

  MPI_Comm everyone2;
  MPI_Comm spawn2;
  int rank2 = 100;

  MPI_Comm_spawn("yap", MPI_ARGV_NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &everyone2, MPI_ERRCODES_IGNORE); 
  MPI_Comm_rank(everyone2, &rank2); 
  printf(" 2EEEEEE  %d EEEEE  %d\n",rank2,getpid());
#endif

  //Yap_init_yapor_teams_workers(n_workers,GLOBAL_counter_teams);
  

  Term counter_teams;
  counter_teams = MkIntTerm(GLOBAL_counter_teams);
  
  Yap_unify(ARG2, counter_teams);

  return(TRUE);

}

char* 
term2string(char** ptr, size_t* size, const YAP_Term t) {
  int missing = YAP_ExportTerm(t, *ptr, *size);
  while(missing < 1){
   // expand_buffer(ptr, size);
    missing = YAP_ExportTerm(t, *ptr, *size);
  }
  return *ptr;
}

static Int p_run_goal( USES_REGS1 ) {
  int i;
 // printf("C_RUN_GOAL \n");

  

  int tid = IntOfTerm(Deref(ARG2));

  int type = IntOfTerm(Deref(ARG3));

 // printf("2 C_RUN_GOAL %d\n",tid);

  
 if(type){
  Term term_goal = Deref(ARG1);
  size_t size;
  char *goal = malloc(300*sizeof(char));
  term2string( &goal, &size,term_goal );
  MPI_Send(goal,300,MPI_CHAR,0,0,R_COMM_mpi_comm(tid));




  } 
  else {
  Term db_key = YAP_Record(Deref(ARG1));
  BITMAP_clear(R_COMM_bm_idle_teams(tid));
  GLOBAL_data_base_key(R_COMM_translation_array(tid,0)) = db_key;
  //for(i = 0; i < R_COMM_number_teams(tid); i ++) {
   GLOBAL_flag(R_COMM_translation_array(tid,0)) = 1;
   //printf("%d AVISAR %d  %d\n",COMM_number_teams(tid),COMM_translation_array(tid,i),GLOBAL_flag(2));
  //}
  }

  return(TRUE);
}



/*
 * Converts a string with a ascci representation of a term into a Prolog term.
 */
YAP_Term 
string2term(char** ptr, size_t *size) {
  YAP_Term t;

  t = YAP_ImportTerm( *ptr );
  if ( t==FALSE ) {
    //write_msg(__FUNCTION__,__FILE__,__LINE__,"FAILED string2term>>>>size:%lx %d\n",t,*size);
    exit(1);
  }
  return t;
}                   

static Int p_create_comm( USES_REGS1 ) {

printf(" __________________________________________________________________________ \n"); 


//sleep(100);

  int mpi = 1;
  Term list = Deref(ARG1);
  int tid , i;
  int number_teams = IntOfTerm(Deref(ARG2));
  int comm = GLOBAL_counter_comms;
  MPI_Comm everyone;
  int rank;
  char * goal;
  char * res;
  size_t size;
  goal = malloc(300*sizeof(char));
  //res = term2string( &goal, &size,list );

  //YAP_Term out = YAP_ImportTerm(res);
  //Yap_DebugPlWrite(out);
  
  /*
  for(i=0;i<300;i++)
   printf("|%c|\n",res[i]);
  */

  //YAP_ExportTerm(list,goal,size);
  //printf("----------   %s  --------------- %s\n",goal,list); 


if(mpi){
  int workers_per_team[100];
  //printf("----------   %s  ---------------\n",goal); 
  printf("INIT SPAWN WORKER MASTER %d\n",getpid());
  MPI_Comm_spawn("yap", MPI_ARGV_NULL, number_teams, MPI_INFO_NULL, 0, MPI_COMM_SELF, &everyone, MPI_ERRCODES_IGNORE); 
  R_COMM_mpi_comm(comm) = everyone;
  R_COMM_number_teams(comm) = number_teams;
  printf("INIT SPAWN WORKER MASTER %d %d  %p\n",getpid(),number_teams,R_COMM_mpi_comm(comm));

    for(i = 0; i < number_teams; i ++){
    workers_per_team[i] = IntOfTerm(HeadOfTerm(list));
    list = TailOfTerm(list);
  }


  MPI_Bcast(&number_teams, 1, MPI_INT,  MPI_ROOT , everyone);
  MPI_Bcast(workers_per_team,number_teams,MPI_INT,MPI_ROOT,everyone);
  //exit(0);
  //MPI_Send(goal,300,MPI_CHAR,0,0,everyone);
  MPI_Comm_rank(everyone, &rank); 
  printf(" EEEEEE  %d EEEEE  %d\n",rank,getpid());
  } 
else{
  GLOBAL_counter_comms ++;
  BITMAP_clear(R_COMM_bm_present_teams(comm));
  BITMAP_clear(R_COMM_bm_idle_teams(comm));

  //printf("number teams %d\n",number_teams);   

  for(i = 0; i < number_teams; i ++){
  BITMAP_insert(R_COMM_bm_present_teams(comm),i);
  }
  BITMAP_copy(R_COMM_bm_finished_teams(comm),R_COMM_bm_present_teams(comm));	

  INIT_LOCK(R_COMM_locks_bm_running_teams(comm));
  INIT_LOCK(R_COMM_locks_bm_present_teams(comm));      
  INIT_LOCK(R_COMM_locks_bm_finished_teams(comm)); 
  INIT_LOCK(R_COMM_locks_bm_idle_teams(comm)); 
 

  for(i = 0; i < number_teams; i ++){
    int n_workers_team = IntOfTerm(HeadOfTerm(list));

    tid = ++GLOBAL_counter_teams;

    Yap_init_yapor_teams_workers(n_workers_team,tid,comm,i);

    //BITMAP_insert(R_COMM_bm_present_teams(comm),i);
    R_COMM_translation_array(comm,i) = tid;
    GLOBAL_comm_rank(tid) = i; 
//    printf(" __________________________________________________________________________ %d  --->  %d \n",i,tid); 
    GLOBAL_comm_number(tid) = comm;
    list = TailOfTerm(list);
  }
  


  R_COMM_number_teams(comm) = number_teams;    
  //BITMAP_copy(R_COMM_bm_finished_teams(comm),R_COMM_bm_present_teams(comm));	
  R_COMM_finished(comm) = 0; 
	 
}
  

  Term comm2 = MkIntTerm(comm);
  Yap_unify(ARG3, comm2);
  //Yap_unify(ARG3, out);	

  return(TRUE);
}

static Int p_results_barrier( USES_REGS1 ) {



  int id = IntOfTerm(Deref(ARG1));

  //printf("\n \n \n \n **********************************************************************************************************\n ++++++++++++++++++++++++++++++++++++++++  barrier GET_RESULTS %d +++++++++++++++++++++++++++++++++++++++++++++++\n **************************************************************************************************** \n",R_COMM_finished(id));

  while(!R_COMM_finished(id));
  R_COMM_finished(id) = 0;
  //printf(" --- FINISHED %d\n",COMM_finished(id));
  return (TRUE);

}


static Int p_get_results( USES_REGS1 ) {



  int id = IntOfTerm(Deref(ARG1));
  Term t = TermNil;   
  int i;

  //printf("\n \n \n \n **********************************************************************************************************\n ++++++++++++++++++++++++++++++++++++++++  GET_RESULTS %d +++++++++++++++++++++++++++++++++++++++++++++++\n **************************************************************************************************** \n",COMM_finished(id));

  while(!R_COMM_finished(id));
  for(i=0; i<R_COMM_number_teams(id); i++){
    int team = R_COMM_translation_array(id,i);
    if (OrFr_qg_solutions(GLOBAL_optyap_team_data.global_optyap_data_[team].root_or_frame)) {
    qg_ans_fr_ptr aux_answer1, aux_answer2;
    aux_answer1 = SolFr_first(OrFr_qg_solutions(GLOBAL_optyap_team_data.global_optyap_data_[team].root_or_frame));
    while (aux_answer1) {
      t = MkPairTerm(AnsFr_answer(aux_answer1), t);
      aux_answer2 = aux_answer1;
      aux_answer1 = AnsFr_next(aux_answer1);
      FREE_QG_ANSWER_FRAME(aux_answer2);
    }
    FREE_QG_SOLUTION_FRAME(OrFr_qg_solutions(GLOBAL_optyap_team_data.global_optyap_data_[team].root_or_frame));
    OrFr_qg_solutions(GLOBAL_optyap_team_data.global_optyap_data_[team].root_or_frame) = NULL;
   }
 }

  R_COMM_finished(id) = 0;
  Yap_unify(ARG2, t);
  return (TRUE);

}


#ifdef YAPOR_MPI
static Int p_mpi_get_results( USES_REGS1 ) {
  int id = IntOfTerm(Deref(ARG1));
  printf("-----------------------------------  %d ---------------------------------\n",id);  
  int a = 1;
  int recv_msg;
  char buf [5000] [256];
  int i, j;
  Term t =  TermNil;
  Term aux = NULL;
  MPI_Recv( &recv_msg, 1, MPI_INT, MPI_ANY_SOURCE, 34, R_COMM_mpi_comm(id), MPI_STATUS_IGNORE);

  printf("MENSAGEM -----------------------------  %d   %d---------------------------------  \n",recv_msg,R_COMM_number_teams(id));

  //sleep(1000);

  Yap_StartSlots( PASS_REGS1 );
  //aux = YAP_ReadBuffer("a(X)",NULL);
  
  for(i=0; i< R_COMM_number_teams(id); i++)
     MPI_Send(&a, 1, MPI_INT, i, 34, R_COMM_mpi_comm(id));

  for(i=0; i< R_COMM_number_teams(id); i++){
     printf("\n \n RESULTADOS %d\n \n",i);
     MPI_Recv( &buf, 5000*256, MPI_CHAR, i, 34, R_COMM_mpi_comm(id), MPI_STATUS_IGNORE);
     for (j=0;j<5000;j++){
      if(buf[j][0] == '\0')
         break;
      //printf("--- %s\n",buf[j]);
      int l;
     /* for(l=0;l<256;l++){
         printf("--- %c\n",buf[j][l]);
         if(buf[j][l] == '\0')
            printf("tenho termin\n");
      }*/
      aux = YAP_ReadBuffer(buf[j],NULL);
      //Yap_DebugPlWrite(aux);
      t = MkPairTerm(aux, t);
     }
     printf("\n \n FIM FIM FIM  \n \n");
      //Yap_DebugPlWrite(t);
  }

  printf("\n \n ANTES ANTES ANTES \n \n");
  //Yap_DebugPlWrite(t);
  printf("\n \n FIM FIM FIM  \n \n");
  Yap_unify(ARG2, t);
  return (TRUE);

}
#endif 

#endif /* YAPOR_TEAMS */

#ifdef YAPOR
static Int p_parallel_mode( USES_REGS1 ) {
  Term t;
  t = Deref(ARG1);
  if (IsVarTerm(t)) {
    Term ta;
    if (GLOBAL_parallel_mode == PARALLEL_MODE_OFF) 
      ta = MkAtomTerm(Yap_LookupAtom("off"));
    else if (GLOBAL_parallel_mode == PARALLEL_MODE_ON) 
      ta = MkAtomTerm(Yap_LookupAtom("on"));
    else /* PARALLEL_MODE_RUNNING */
      ta = MkAtomTerm(Yap_LookupAtom("running"));
    Bind((CELL *)t, ta);
    return(TRUE);
  }
  if (IsAtomTerm(t) && GLOBAL_parallel_mode != PARALLEL_MODE_RUNNING) {
    char *s;
    s = RepAtom(AtomOfTerm(t))->StrOfAE;
    if (strcmp(s,"on") == 0) {
      GLOBAL_parallel_mode = PARALLEL_MODE_ON;
      return(TRUE);
    }
    if (strcmp(s,"off") == 0) {
      GLOBAL_parallel_mode = PARALLEL_MODE_OFF;
      return(TRUE);
    }
    return(FALSE); /* PARALLEL_MODE_RUNNING */
  }
  return(FALSE);
}



static Int p_testing( USES_REGS1 ) {
  Term t;
  t = Deref(ARG1);
 
  Yap_StartSlots( PASS_REGS1 );
  Term bb = YAP_ReadBuffer("queens(S)",NULL);
  
  return(FALSE);
}


static Int p_yapor_start( USES_REGS1 ) {
  printf("START START START START (%d,%d)  %d\n",team_id,worker_id,comm_rank);
#ifdef TIMESTAMP_CHECK
  GLOBAL_timestamp = 0;
#endif /* TIMESTAMP_CHECK */
  BITMAP_delete(GLOBAL_bm_idle_workers, 0);
  BITMAP_clear(GLOBAL_bm_invisible_workers);
  BITMAP_clear(GLOBAL_bm_requestable_workers);
#ifdef TABLING_INNER_CUTS
  BITMAP_clear(GLOBAL_bm_pruning_workers);
#endif /* TABLING_INNER_CUTS */
  make_root_choice_point();
  GLOBAL_parallel_mode = PARALLEL_MODE_RUNNING;
  GLOBAL_execution_time = current_time();
  //BITMAP_clear(GLOBAL_bm_finished_workers);
#ifdef YAPOR_TEAMS
  BITMAP_clear(GLOBAL_bm_free_workers);
  LOCK(GLOBAL_lock_free_workers);
  BITMAP_insert(GLOBAL_bm_free_workers, worker_id);
  UNLOCK(GLOBAL_lock_free_workers);
  //BITMAP_clear(R_COMM_bm_finished_teams(GLOBAL_comm_number(team_id)));
  BITMAP_clear(COMM_bm_finished_teams);
  //printf("START START START START (%d,%d)\n",team_id,worker_id);
#ifdef YAPOR_MPI
  //MPI_Barrier(MPI_COMM_WORLD);
  int results = IntOfTerm(Deref(ARG1)); 
  printf("START START START START (%d,%d)  res = %d\n",team_id,worker_id,results);
  int cp_tr[3];
  cp_tr[0] = GLOBAL_root_cp;
  cp_tr[1] = GLOBAL_root_cp->cp_tr;
  cp_tr[2] = R_COMM_query_has_results(comm_rank) = results;
  MPI_Bcast(&cp_tr, 3, MPI_INT, 0, MPI_COMM_WORLD);
#endif
  PUT_OUT_ROOT_NODE(worker_id);
#endif
  printf("XXXXX START START START START (%d,%d) %p\n",team_id,worker_id,cp_tr[2]);
  return (TRUE);
}


static Int p_yapor_workers( USES_REGS1 ) {
#ifdef YAPOR_THREADS
  return Yap_unify(MkIntegerTerm(GLOBAL_number_workers),ARG1);
#else
  return FALSE;
#endif /* YAPOR_THREADS */
}


static Int p_worker( USES_REGS1 ) {
  CurrentModule = USER_MODULE;
  P = GETWORK_FIRST_TIME;
  return TRUE;
}


static Int p_parallel_new_answer( USES_REGS1 ) {
  qg_ans_fr_ptr actual_answer;
  or_fr_ptr leftmost_or_fr;

//if(worker_id != 0)
//  printf("(%d) NEW ANSWER %d   |%p| |%p|\n",comm_rank,getpid(),B,B->cp_ap);


  ALLOC_QG_ANSWER_FRAME(actual_answer);
  AnsFr_answer(actual_answer) = Deref(ARG1);
  AnsFr_next(actual_answer) = NULL;
  leftmost_or_fr = CUT_leftmost_or_frame();
  LOCK_OR_FRAME(leftmost_or_fr);
  if (Get_LOCAL_prune_request()) {
    UNLOCK_OR_FRAME(leftmost_or_fr);
    FREE_QG_ANSWER_FRAME(actual_answer);
  } else {
    CUT_store_answer(leftmost_or_fr, actual_answer);
    UNLOCK_OR_FRAME(leftmost_or_fr);
  }

  return (TRUE);
}

#ifdef YAPOR_MPI

static Int p_mpi_parallel_new_answer( USES_REGS1 ) {
  qg_ans_fr_ptr actual_answer;
  or_fr_ptr leftmost_or_fr;


 //printf("(%d,%d) NEW ANSWER %d   |%p| |%p|  %d\n",comm_rank,worker_id,getpid(),B,B->cp_ap,IntOfTerm(ASP[0]));

  Term db_ref = Yap_StoreTermInDB(Deref(ARG1),10);
 
  ALLOC_QG_ANSWER_FRAME(actual_answer);
  AnsFr_answer(actual_answer) = db_ref;
  AnsFr_next(actual_answer) = NULL;
  leftmost_or_fr = CUT_leftmost_or_frame();
  LOCK_OR_FRAME(leftmost_or_fr);
  if (Get_LOCAL_prune_request()) {
    UNLOCK_OR_FRAME(leftmost_or_fr);
    FREE_QG_ANSWER_FRAME(actual_answer);
  } else {
    CUT_store_answer(leftmost_or_fr, actual_answer);
    UNLOCK_OR_FRAME(leftmost_or_fr);
  }

  return (TRUE);
}

#endif

static Int p_parallel_get_answers( USES_REGS1 ){
  Term t = TermNil;

  if (OrFr_qg_solutions(LOCAL_top_or_fr)) {
    qg_ans_fr_ptr aux_answer1, aux_answer2;
    aux_answer1 = SolFr_first(OrFr_qg_solutions(LOCAL_top_or_fr));
    while (aux_answer1) {
      t = MkPairTerm(AnsFr_answer(aux_answer1), t);
      aux_answer2 = aux_answer1;
      aux_answer1 = AnsFr_next(aux_answer1);
      FREE_QG_ANSWER_FRAME(aux_answer2);
    }
    FREE_QG_SOLUTION_FRAME(OrFr_qg_solutions(LOCAL_top_or_fr));
    OrFr_qg_solutions(LOCAL_top_or_fr) = NULL;
  }
  Yap_unify(ARG1, t);
  return (TRUE);
}


static Int p_show_statistics_or( USES_REGS1 ) {
  struct page_statistics stats;
  long bytes, total_bytes = 0;
#ifdef USE_PAGES_MALLOC
  long total_pages = 0;
#endif /* USE_PAGES_MALLOC */
  IOSTREAM *out;
  Term t = Deref(ARG1);

  if (IsVarTerm(t) || !IsAtomTerm(t))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t))))
    return FALSE;
  bytes = 0;
  Sfprintf(out, "Execution data structures\n");
  stats = show_statistics_or_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  Sfprintf(out, "  Memory in use (I):               %10ld bytes\n\n", bytes);
  total_bytes += bytes;
  bytes = 0;
  Sfprintf(out, "Cut support data structures\n");
  stats = show_statistics_query_goal_solution_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_query_goal_answer_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  Sfprintf(out, "  Memory in use (II):              %10ld bytes\n\n", bytes);
  total_bytes += bytes;
#ifdef USE_PAGES_MALLOC
  Sfprintf(out, "Total memory in use (I+II):        %10ld bytes (%ld pages in use)\n",
          total_bytes, total_pages);
  Sfprintf(out, "Total memory allocated:            %10ld bytes (%ld pages in total)\n",
          PgEnt_pages_in_use(GLOBAL_pages_alloc) * Yap_page_size, PgEnt_pages_in_use(GLOBAL_pages_alloc));
#else 
  Sfprintf(out, "Total memory in use (I+II):        %10ld bytes\n", total_bytes);
#endif /* USE_PAGES_MALLOC */
  PL_release_stream(out);
  return (TRUE);
}
#endif /* YAPOR */



/**********************************
**      OPTYap C Predicates      **
**********************************/

#if defined(YAPOR) && defined(TABLING)
static Int p_show_statistics_opt( USES_REGS1 ) {
  struct page_statistics stats;
  long bytes, total_bytes = 0;
#ifdef USE_PAGES_MALLOC
  long total_pages = 0;
#endif /* USE_PAGES_MALLOC */
  IOSTREAM *out;
  Term t = Deref(ARG1);

  if (IsVarTerm(t) || !IsAtomTerm(t))
    return FALSE;
  if (!(out = Yap_GetStreamHandle(AtomOfTerm(t))))
    return FALSE;
  bytes = 0;
  Sfprintf(out, "Execution data structures\n");
  stats = show_statistics_table_entries(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#if defined(THREADS_FULL_SHARING) || defined(THREADS_CONSUMER_SHARING)
  stats = show_statistics_subgoal_entries(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#endif /* THREADS_FULL_SHARING || THREADS_CONSUMER_SHARING */
  stats = show_statistics_subgoal_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_dependency_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_or_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_suspension_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  Sfprintf(out, "  Memory in use (I):               %10ld bytes\n\n", bytes);
  total_bytes += bytes;
  bytes = 0;
  Sfprintf(out, "Local trie data structures\n");
  stats = show_statistics_subgoal_trie_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_answer_trie_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_subgoal_trie_hashes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_answer_trie_hashes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#if defined(THREADS_FULL_SHARING)
  stats = show_statistics_answer_ref_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#endif /* THREADS_FULL_SHARING */
  Sfprintf(out, "  Memory in use (II):              %10ld bytes\n\n", bytes);
  total_bytes += bytes;
  bytes = 0;
  Sfprintf(out, "Global trie data structures\n");
  stats = show_statistics_global_trie_nodes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_global_trie_hashes(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  Sfprintf(out, "  Memory in use (III):             %10ld bytes\n\n", bytes);
  total_bytes += bytes;
  bytes = 0;
  Sfprintf(out, "Cut support data structures\n");
  stats = show_statistics_query_goal_solution_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_query_goal_answer_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#ifdef TABLING_INNER_CUTS
  stats = show_statistics_table_subgoal_solution_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
  stats = show_statistics_table_subgoal_answer_frames(out);
  INCREMENT_AUX_STATS(stats, bytes, total_pages);
#endif /* TABLING_INNER_CUTS */
  Sfprintf(out, "  Memory in use (IV):              %10ld bytes\n\n", bytes);
  total_bytes += bytes;
#ifdef USE_PAGES_MALLOC
  Sfprintf(out, "Total memory in use (I+II+III+IV): %10ld bytes (%ld pages in use)\n",
          total_bytes, total_pages);
  Sfprintf(out, "Total memory allocated:            %10ld bytes (%ld pages in total)\n",
          PgEnt_pages_in_use(GLOBAL_pages_alloc) * Yap_page_size, PgEnt_pages_in_use(GLOBAL_pages_alloc));
#else 
  Sfprintf(out, "Total memory in use (I+II+III+IV): %10ld bytes\n", total_bytes);
#endif /* USE_PAGES_MALLOC */
  PL_release_stream(out);
  return (TRUE);
}
#endif /* YAPOR && TABLING */


static Int p_get_optyap_statistics( USES_REGS1 ) {
  struct page_statistics stats;
  Int value, bytes = 0, structs = -1;
  Term tbytes, tstructs;

  value = IntOfTerm(Deref(ARG1));
#ifdef TABLING
  if (value == 0 || value == 1) {  /* table_entries */
    GET_PAGE_STATS(stats, struct table_entry, _pages_tab_ent);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#if defined(THREADS_FULL_SHARING) || defined(THREADS_CONSUMER_SHARING)
  if (value == 0 || value == 16) {  /* subgoal_entries */
    GET_PAGE_STATS(stats, struct subgoal_entry, _pages_sg_ent);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#endif
  if (value == 0 || value == 2) {  /* subgoal_frames */
    GET_PAGE_STATS(stats, struct subgoal_frame, _pages_sg_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 3) {  /* dependency_frames */
    GET_PAGE_STATS(stats, struct dependency_frame, _pages_dep_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 6) {  /* subgoal_trie_nodes */
    GET_PAGE_STATS(stats, struct subgoal_trie_node, _pages_sg_node);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 8) {  /* subgoal_trie_hashes */
    GET_PAGE_STATS(stats, struct subgoal_trie_hash, _pages_sg_hash);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 7) {  /* answer_trie_nodes */
    GET_PAGE_STATS(stats, struct answer_trie_node, _pages_ans_node);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 9) {  /* answer_trie_hashes */
    GET_PAGE_STATS(stats, struct answer_trie_hash, _pages_ans_hash);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#if defined(THREADS_FULL_SHARING)
  if (value == 0 || value == 17) {  /* answer_ref_nodes */
    GET_PAGE_STATS(stats, struct answer_ref_node, _pages_ans_ref_node);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#endif
  if (value == 0 || value == 10) {  /* global_trie_nodes */
    GET_PAGE_STATS(stats, struct global_trie_node, _pages_gt_node);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 11) {  /* global_trie_hashes */
    GET_PAGE_STATS(stats, struct global_trie_hash, _pages_gt_hash);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#endif /* TABLING */
#ifdef YAPOR
  if (value == 0 || value == 4) {  /* or_frames */
    GET_PAGE_STATS(stats, struct or_frame, _pages_or_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 12) {  /* query_goal_solution_frames */
    GET_PAGE_STATS(stats, struct query_goal_solution_frame, _pages_qg_sol_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 13) {  /* query_goal_answer_frames */
    GET_PAGE_STATS(stats, struct query_goal_answer_frame, _pages_qg_ans_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#endif /* YAPOR */
#if defined(YAPOR) && defined(TABLING)
  if (value == 0 || value == 5) {  /* suspension_frames */
    GET_PAGE_STATS(stats, struct suspension_frame, _pages_susp_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#ifdef TABLING_INNER_CUTS
  if (value == 0 || value == 14) {  /* table_subgoal_solution_frames */
    GET_PAGE_STATS(stats, struct table_subgoal_solution_frame, _pages_tg_sol_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
  if (value == 0 || value == 15) {  /* table_subgoal_answer_frames */
    GET_PAGE_STATS(stats, struct table_subgoal_answer_frame, _pages_tg_ans_fr);
    bytes += PgEnt_bytes_in_use(stats);
    if (value != 0) structs = PgEnt_strs_in_use(stats);
  }
#endif /* TABLING_INNER_CUTS */
#endif /* YAPOR && TABLING */

  if (value == 0) {  /* total_memory */
#ifdef USE_PAGES_MALLOC
    structs = PgEnt_pages_in_use(GLOBAL_pages_alloc) * Yap_page_size;
#else
    structs = bytes;
#endif /* USE_PAGES_MALLOC */
  }
  if (structs == -1)
    return (FALSE);
  tbytes = Deref(ARG2);
  tstructs = Deref(ARG3);
  if (IsVarTerm(tbytes)) {
    Bind((CELL *) tbytes, MkIntTerm(bytes));
  } else if (IsIntTerm(tbytes) &&  IntOfTerm(tbytes) != bytes)
    return (FALSE);
  if (IsVarTerm(tstructs)) {
    Bind((CELL *) tstructs, MkIntTerm(structs));
  } else if (IsIntTerm(tstructs) &&  IntOfTerm(tstructs) != structs)
    return (FALSE);
  return (TRUE);
}



/******************************
**      Local functions      **
******************************/

#ifdef YAPOR
static inline realtime current_time(void) {
#define TIME_RESOLUTION 1000000
  struct timeval tempo;
  gettimeofday(&tempo, NULL);
  return ((realtime)tempo.tv_sec + (realtime)tempo.tv_usec / TIME_RESOLUTION);
  /* to get time as Yap */
  /*
  double now, interval;
  Yap_cputime_interval(&now, &interval);
  return ((realtime)now);
  */
}
#endif /* YAPOR */


#ifdef TABLING
static inline struct page_statistics show_statistics_table_entries(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct table_entry, _pages_tab_ent, "Table entries:                ");
}


#if defined(THREADS_FULL_SHARING) || defined(THREADS_CONSUMER_SHARING)
static inline struct page_statistics show_statistics_subgoal_entries(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct subgoal_entry, _pages_sg_ent, "Subgoal entries:              ");
}
#endif /* THREADS_FULL_SHARING || THREADS_CONSUMER_SHARING */


static inline struct page_statistics show_statistics_subgoal_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct subgoal_frame, _pages_sg_fr, "Subgoal frames:               ");
}


static inline struct page_statistics show_statistics_dependency_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct dependency_frame, _pages_dep_fr, "Dependency frames:            ");
}


static inline struct page_statistics show_statistics_subgoal_trie_nodes(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct subgoal_trie_node, _pages_sg_node, "Subgoal trie nodes:           ");
}


static inline struct page_statistics show_statistics_subgoal_trie_hashes(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct subgoal_trie_hash, _pages_sg_hash, "Subgoal trie hashes:          ");
}


static inline struct page_statistics show_statistics_answer_trie_nodes(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct answer_trie_node, _pages_ans_node, "Answer trie nodes:            ");
}


static inline struct page_statistics show_statistics_answer_trie_hashes(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct answer_trie_hash, _pages_ans_hash, "Answer trie hashes:           ");
}


#if defined(THREADS_FULL_SHARING)
static inline struct page_statistics show_statistics_answer_ref_nodes(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct answer_ref_node, _pages_ans_ref_node, "Answer ref nodes:             ");
}
#endif /* THREADS_FULL_SHARING */


static inline struct page_statistics show_statistics_global_trie_nodes(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct global_trie_node, _pages_gt_node, "Global trie nodes:            ");
}


static inline struct page_statistics show_statistics_global_trie_hashes(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct global_trie_hash, _pages_gt_hash, "Global trie hashes:           ");
}
#endif /* TABLING */


#ifdef YAPOR
static inline struct page_statistics show_statistics_or_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct or_frame, _pages_or_fr, "Or-frames:                    ");
}


static inline struct page_statistics show_statistics_query_goal_solution_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct query_goal_solution_frame, _pages_qg_sol_fr, "Query goal solution frames:   ");
}


static inline struct page_statistics show_statistics_query_goal_answer_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct query_goal_answer_frame, _pages_qg_ans_fr, "Query goal answer frames:     ");
}
#endif /* YAPOR */


#if defined(YAPOR) && defined(TABLING)
static inline struct page_statistics show_statistics_suspension_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct suspension_frame, _pages_susp_fr, "Suspension frames:            ");
}


#ifdef TABLING_INNER_CUTS
static inline struct page_statistics show_statistics_table_subgoal_solution_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct table_subgoal_solution_frame, _pages_tg_sol_fr, "Table subgoal solution frames:");
}


static inline struct page_statistics show_statistics_table_subgoal_answer_frames(IOSTREAM *out) {
  SHOW_PAGE_STATS(out, struct table_subgoal_answer_frame, _pages_tg_ans_fr, "Table subgoal answer frames:  ");
}
#endif /* TABLING_INNER_CUTS */
#endif /* YAPOR && TABLING */
#endif /* YAPOR || TABLING */

#ifdef YAPOR_TEAMS

#include <ilocals.h>

void Yap_init_yapor_teams_workers(int n_workers, int my_team_id, int comm, int rank) {
  CACHE_REGS
  int i;
  int son;

  team_id = my_team_id;
  OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];
  Yap_init_global_optyap_data(0,n_workers,0,0);
  GLOBAL_start_area = GLOBAL_team_area_pointer(my_team_id);
  OPT = &GLOBAL_optyap_team_data.global_optyap_data_[0];
  team_id = 0;
  

  
  GLOBAL_team_area_pointer(my_team_id + 1) = GLOBAL_team_area_pointer(my_team_id) + (n_workers * Yap_worker_area_size);
  GLOBAL_local_id(my_team_id + 1) = GLOBAL_local_id(my_team_id) + n_workers; 

//printf("(%d) team area %p  local_id  %d\n",my_team_id,GLOBAL_team_area_pointer(my_team_id),GLOBAL_local_id(my_team_id));

  son = fork();
  if (son == -1)
      Yap_Error(FATAL_ERROR, TermNil, "fork error (Yap_init_yapor_workers)");
  if (son != 0){
      return;
  }
  else {
      LOCK(GLOBAL_lock_worker_pid); 
      GLOBAL_worker_pid(GLOBAL_worker_pid_counter)=getpid();
      GLOBAL_worker_pid_counter++;
      //printf("%d ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇçç %d %d\n",GLOBAL_worker_pid_counter,getpid(),GLOBAL_worker_pid(GLOBAL_worker_pid_counter-1));
      UNLOCK(GLOBAL_lock_worker_pid); 
      worker_id = 0;
      team_id = my_team_id;
      //COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;
  }

  GLOBAL_number_workers = n_workers;
  comm_rank = rank;
  COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;
  OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];
  
  //Yapor_teams_alloc_new_team(n_workers);



  //Yap_init_global_optyap_data(0,n_workers,0,0);
  Yap_init_root_frames();

   Yapor_teams_remap_team_memory();

  LOCAL = REMOTE(worker_id);
  memcpy(REMOTE(worker_id), Yap_local, sizeof(struct worker_local));
   InitWorker(worker_id);
//printf("A (%d,%d) ---------------    %p  %p  %p\n", team_id, worker_id,LOCAL, REMOTE(worker_id), Yap_local+1);
//----------------------------------------------
  // Yap_InitYaamRegs2( worker_id, team_id );
  //Yap_InitYaamRegs( worker_id);
//----------------------------------------------
  comm_rank = rank;
  COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;
  OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];
//printf("B (%d,%d) ---------------(%p)    %p  %p  %p\n", team_id, worker_id,B,LOCAL, REMOTE(worker_id), Yap_local+1);

 // printf("(%d,%d) COMM1 %p  %d \n",team_id,worker_id,COMM1, comm);

  for (i = 1; i < n_workers; i++) {
    son = fork();
    if (son == -1)
      Yap_Error(FATAL_ERROR, TermNil, "fork error (Yap_init_yapor_workers)");
    if (son == 0) { 
      /* new worker */
      LOCK(GLOBAL_lock_worker_pid); 
      GLOBAL_worker_pid(GLOBAL_worker_pid_counter)=getpid();
      GLOBAL_worker_pid_counter++;
      //printf("(%d,%d) ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇçç %d %d\n",team_id,i,GLOBAL_worker_pid_counter,getpid(),GLOBAL_worker_pid(GLOBAL_worker_pid_counter-1));
      UNLOCK(GLOBAL_lock_worker_pid); 
      worker_id = i;
      team_id = my_team_id;
      //printf("! estou a espera %d  %d    GLOBAL_local_id(team_id) %d\n",worker_id,team_id,GLOBAL_local_id(team_id));
      Yapor_teams_remap_team_memory(n_workers);   
       
      InitWorker(worker_id);
       //printf("2 estou a espera %d \n",worker_id); 
      memcpy(REMOTE(worker_id), Yap_local, sizeof(struct worker_local));
       //printf("3 estou a espera %d\n",worker_id); 
      Yap_InitYaamRegs2( worker_id, team_id );
 //printf("4 estou a espera %d\n",worker_id); 
      LOCAL = REMOTE(worker_id);
  //printf("5 estou a espera %d\n",comm); 
      COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;
      OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];
      break;
    } else{
      //GLOBAL_worker_pid(i) = son;
      //printf ("Estou a lançar (%d) -- %d\n",worker_id,i);
    }
  }
  
      CurrentModule = USER_MODULE;
      GLOBAL_parallel_mode = PARALLEL_MODE_ON;
      P = GETWORK_FIRST_TIME;
      Yap_exec_absmi(FALSE);
      Yap_Error(INTERNAL_ERROR, TermNil, "abstract machine unexpected exit (YAP_Init)");
}


#include <sys/mman.h>
#include <fcntl.h>
/*
void Yapor_teams_alloc_new_team(int n_workers){

 int fd_mapfile; 

    if ((fd_mapfile = open(GLOBAL_mapfile_path, O_RDWR)) < 0)
      Yap_Error(FATAL_ERROR, TermNil, "open error ( Yap_init_yapor_stacks_memory)");
    if (lseek(fd_mapfile, GLOBAL_allocated_memory + ( n_workers * Yap_worker_area_size), SEEK_SET) < 0) 
      Yap_Error(FATAL_ERROR, TermNil, "lseek error (Yap_init_yapor_stacks_memory)");
    if (write(fd_mapfile, "", 1) < 0) 
      Yap_Error(FATAL_ERROR, TermNil, "write error (Yap_init_yapor_stacks_memory)");
    if (munmap(LOCAL_GlobalBase, (size_t)Yap_worker_area_size)== -1)
      Yap_Error(FATAL_ERROR, TermNil, "munmap error (Yap_remap_yapor_memory)");
    if (mmap(LOCAL_GlobalBase, (size_t) ( n_workers * Yap_worker_area_size), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, fd_mapfile,  GLOBAL_allocated_memory) == (void *) -1)
      Yap_Error(FATAL_ERROR, TermNil, "mmap error (Yap_init_yapor_stacks_memory)");
    GLOBAL_allocated_memory = GLOBAL_allocated_memory + (size_t) ( n_workers * Yap_worker_area_size);
    if (close(fd_mapfile) == -1)
      Yap_Error(FATAL_ERROR, TermNil, "close error (Yap_init_yapor_stacks_memory)");

}




void Yapor_teams_remap_team_memory(int n_workers) {
  int i;
  void *remap_addr = LOCAL_GlobalBase;
  int fd_mapfile;
  long remap_offset = (ADDR) remap_addr - (ADDR) Yap_local;
  if ((fd_mapfile = open(GLOBAL_mapfile_path, O_RDWR)) < 0)
    Yap_Error(FATAL_ERROR, TermNil, "open error (Yap_remap_yapor_memory)");
  if (munmap(remap_addr, (size_t)(Yap_worker_area_size)) == -1)
    Yap_Error(FATAL_ERROR, TermNil, "munmap error (Yap_remap_yapor_memory)");
  if (mmap(remap_addr, (size_t)Yap_worker_area_size, PROT_READ|PROT_WRITE, 
             MAP_SHARED|MAP_FIXED, fd_mapfile, remap_offset + Yap_worker_area_size + (i * Yap_worker_area_size)) == (void *) -1)
      Yap_Error(FATAL_ERROR, TermNil, "mmap error (Yap_remap_yapor_memory)");
  if (close(fd_mapfile) == -1)
    Yap_Error(FATAL_ERROR, TermNil, "close error (Yap_remap_yapor_memory)");

}
*/


void Yap_init_mpi_yapor_teams_workers(int n_workers, int my_team_id, int comm, int rank) {
  CACHE_REGS
  int i;
  int son;



  OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];
  Yap_init_global_optyap_data(0,n_workers,0,0);
  GLOBAL_mpi_active = 1;
  GLOBAL_team_area_pointer(my_team_id) = GLOBAL_team_area_pointer(my_team_id+1);
  GLOBAL_start_area = GLOBAL_team_area_pointer(my_team_id);

  //printf("%d A (%d,%d) ---------------    %p  %p  %p\n",getpid(), team_id, worker_id,LOCAL, REMOTE(worker_id), Yap_local+1);
  //GLOBAL_team_area_pointer(my_team_id + 1) = GLOBAL_team_area_pointer(my_team_id) + (n_workers * Yap_worker_area_size);
  //GLOBAL_local_id(my_team_id + 1) = GLOBAL_local_id(my_team_id) + n_workers; 

//printf("(%d) team area %p  local_id  %d\n",my_team_id,GLOBAL_team_area_pointer(my_team_id),GLOBAL_local_id(my_team_id));


      LOCK(GLOBAL_lock_worker_pid); 
      GLOBAL_worker_pid(GLOBAL_worker_pid_counter)=getpid();
      GLOBAL_worker_pid_counter++;
      //printf("%d ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇçç %d %d\n",GLOBAL_worker_pid_counter,getpid(),GLOBAL_worker_pid(GLOBAL_worker_pid_counter-1));
      UNLOCK(GLOBAL_lock_worker_pid); 
      worker_id = 0;
      team_id = my_team_id;
      //COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;


  GLOBAL_number_workers = n_workers;
  comm_rank = rank;
  COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;
  OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];


  
  //Yapor_teams_alloc_new_team(n_workers);

  //printf("%d B (%d,%d) ---------------    %p  %p  %p\n",getpid(), team_id, worker_id,LOCAL, REMOTE(worker_id), Yap_local+1);

  //Yap_init_global_optyap_data(0,n_workers,0,0);
  Yap_init_root_frames();

  Yapor_teams_remap_team_memory();

  LOCAL = REMOTE(worker_id);
  //memcpy(REMOTE(worker_id), Yap_local, sizeof(struct worker_local));
   //InitWorker(worker_id);

//----------------------------------------------
   //Yap_InitYaamRegs2( worker_id, team_id );
  //Yap_InitYaamRegs( worker_id);
//----------------------------------------------
  comm_rank = rank;
  COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;
  OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];
//printf("B (%d,%d) ---------------(%p)    %p  %p  %p\n", team_id, worker_id,B,LOCAL, REMOTE(worker_id), Yap_local+1);

 // printf("(%d,%d) COMM1 %p  %d \n",team_id,worker_id,COMM1, comm);

//printf("%d C (%d,%d) ---------------    %p  %p  %p\n",getpid(), team_id, worker_id,LOCAL, REMOTE(worker_id), Yap_local+1);

  for (i = 1; i < n_workers; i++) {
    son = fork();
    if (son == -1)
      Yap_Error(FATAL_ERROR, TermNil, "fork error (Yap_init_yapor_workers)");
    if (son == 0) { 
      /* new worker */
      LOCK(GLOBAL_lock_worker_pid); 
      GLOBAL_worker_pid(GLOBAL_worker_pid_counter)=getpid();
      GLOBAL_worker_pid_counter++;
      //printf("(%d,%d) ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇçç %d %d\n",team_id,i,GLOBAL_worker_pid_counter,getpid(),GLOBAL_worker_pid(GLOBAL_worker_pid_counter-1));
      UNLOCK(GLOBAL_lock_worker_pid); 
      worker_id = i;
      team_id = my_team_id;
      //printf("! estou a espera %d  %d    GLOBAL_local_id(team_id) %d\n",worker_id,team_id,GLOBAL_local_id(team_id));
      Yapor_teams_remap_team_memory(n_workers);   
       
      InitWorker(worker_id);
       //printf("2 estou a espera %d \n",worker_id); 
      memcpy(REMOTE(worker_id), Yap_local, sizeof(struct worker_local));
       //printf("3 estou a espera %d\n",worker_id); 
      Yap_InitYaamRegs2( worker_id, team_id );
 //printf("4 estou a espera %d\n",worker_id); 
      LOCAL = REMOTE(worker_id);
      comm_rank = rank; 
  //printf("5 estou a espera %d\n",comm); 
      COMM1 = &GLOBAL_optyap_team_data.comm_optyap_data_[comm] ;
      OPT = &GLOBAL_optyap_team_data.global_optyap_data_[my_team_id];
      break;
    } else{
      //GLOBAL_worker_pid(i) = son;
      //printf ("Estou a lançar (%d) -- %d\n",worker_id,i);
    }
  }


//printf("%d DD (%d,%d) ---------------    %p  %p  %p\n",getpid(), team_id, worker_id,LOCAL, REMOTE(worker_id), Yap_local+1);
      CurrentModule = USER_MODULE;
      GLOBAL_parallel_mode = PARALLEL_MODE_ON;
      P = GETWORK_FIRST_TIME;
      printf("%d                                                          ENDEREÇO CODIGO %p\n",getpid(), P);
      Yap_exec_absmi(FALSE);
      Yap_Error(INTERNAL_ERROR, TermNil, "abstract machine unexpected exit (YAP_Init)");
      
}

void Yapor_teams_remap_team_memory() {

  void *remap_addr = LOCAL_GlobalBase;
  int fd_mapfile;
  long remap_offset = (ADDR) (GLOBAL_team_area_pointer(team_id) + (worker_id * Yap_worker_area_size)) - (ADDR) Yap_local;

  printf("%d                                                          OFFSET %p   |%p  %p|\n",getpid(),remap_offset,GLOBAL_team_area_pointer(team_id),LOCAL_GlobalBase+Yap_worker_area_size);
  if ((fd_mapfile = open(GLOBAL_mapfile_path, O_RDWR)) < 0)
    Yap_Error(FATAL_ERROR, TermNil, "open error (Yap_remap_yapor_memory)");
  if (munmap(remap_addr, (size_t)(Yap_worker_area_size)) == -1)
    Yap_Error(FATAL_ERROR, TermNil, "munmap error (Yap_remap_yapor_memory)");
  if (mmap(remap_addr, (size_t)Yap_worker_area_size, PROT_READ|PROT_WRITE, 
             MAP_SHARED|MAP_FIXED, fd_mapfile, remap_offset) == (void *) -1)
      Yap_Error(FATAL_ERROR, TermNil, "mmap error (Yap_remap_yapor_memory)");
  if (close(fd_mapfile) == -1)
    Yap_Error(FATAL_ERROR, TermNil, "close error (Yap_remap_yapor_memory)");

}
#endif /* YAPOR_TEAMS */
