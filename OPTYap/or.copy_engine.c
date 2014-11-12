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

/* ------------------ **
**      Includes      **
** ------------------ */

#include "Yap.h"
#ifdef YAPOR_COPY
#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#include "Yatom.h"
#include "YapHeap.h"
#include "or.macros.h"
#ifdef TABLING
#include "tab.macros.h"
#else
#include "opt.mavar.h"
#endif /* TABLING */



/* ------------------------------------- **
**      Local functions declaration      **
** ------------------------------------- */

static void share_private_nodes(int worker_q);



/* ---------------------- **
**      Local macros      **
** ---------------------- */

#define INCREMENTAL_COPY 1
#if INCREMENTAL_COPY
#define COMPUTE_SEGMENTS_TO_COPY_TO(Q)                                   \
	if (REMOTE_top_cp(Q) == GLOBAL_root_cp)	                         \
          REMOTE_start_global_copy(Q) = (CELL) (H0);                     \
	else                                                             \
          REMOTE_start_global_copy(Q) = (CELL) (REMOTE_top_cp(Q)->cp_h); \
        REMOTE_end_global_copy(Q)   = (CELL) (B->cp_h);                  \
        REMOTE_start_local_copy(Q)  = (CELL) (B);                        \
        REMOTE_end_local_copy(Q)    = (CELL) (REMOTE_top_cp(Q));         \
        REMOTE_start_trail_copy(Q)  = (CELL) (REMOTE_top_cp(Q)->cp_tr);  \
        REMOTE_end_trail_copy(Q)    = (CELL) (TR)
#else
#define COMPUTE_SEGMENTS_TO_COPY_TO(Q)                                   \
        REMOTE_start_global_copy(Q) = (CELL) (H0);                       \
        REMOTE_end_global_copy(Q)   = (CELL) (HR);                       \
        REMOTE_start_local_copy(Q)  = (CELL) (B);                        \
        REMOTE_end_local_copy(Q)    = (CELL) (GLOBAL_root_cp);           \
        REMOTE_start_trail_copy(Q)  = (CELL) (GLOBAL_root_cp->cp_tr);    \
        REMOTE_end_trail_copy(Q)    = (CELL) (TR)
#endif

#define P_COPY_GLOBAL_TO(Q)                                                         \
        memcpy((void *) (worker_offset(Q) + REMOTE_start_global_copy(Q)),           \
               (void *) REMOTE_start_global_copy(Q),                                \
               (size_t) (REMOTE_end_global_copy(Q) - REMOTE_start_global_copy(Q)))
#define Q_COPY_GLOBAL_FROM(P)                                                       \
        memcpy((void *) LOCAL_start_global_copy,                                    \
               (void *) (worker_offset(P) + LOCAL_start_global_copy),               \
               (size_t) (LOCAL_end_global_copy - LOCAL_start_global_copy))

#define P_COPY_LOCAL_TO(Q)                                                          \
        memcpy((void *) (worker_offset(Q) + REMOTE_start_local_copy(Q)),            \
               (void *) REMOTE_start_local_copy(Q),                                 \
               (size_t) (REMOTE_end_local_copy(Q) - REMOTE_start_local_copy(Q)))
#define Q_COPY_LOCAL_FROM(P)                                                        \
        memcpy((void *) LOCAL_start_local_copy,                                     \
               (void *) (worker_offset(P) + LOCAL_start_local_copy),                \
               (size_t) (LOCAL_end_local_copy - LOCAL_start_local_copy))

#define P_COPY_TRAIL_TO(Q)                                                          \
        memcpy((void *) (worker_offset(Q) + REMOTE_start_trail_copy(Q)),            \
               (void *) REMOTE_start_trail_copy(Q),                                 \
               (size_t) (REMOTE_end_trail_copy(Q) - REMOTE_start_trail_copy(Q)))
#define Q_COPY_TRAIL_FROM(P)                                                        \
        memcpy((void *) LOCAL_start_trail_copy,                                     \
               (void *) (worker_offset(P) + LOCAL_start_trail_copy),                \
               (size_t) (LOCAL_end_trail_copy - LOCAL_start_trail_copy))



/* -------------------------- **
**      Global functions      **
** -------------------------- */

void make_root_choice_point(void) {

#ifdef YAPOR_TEAMS
  if (worker_id == 0  && comm_rank != 0 && !GLOBAL_mpi_active ) {
      B = LOCAL_top_cp = GLOBAL_root_cp = OrFr_node(GLOBAL_root_or_fr) = COMM_root_cp;
      B->cp_tr = TR = ((choiceptr) (out_worker_offset(0,COMM_translation_array(0)) + (CELL)(B)))->cp_tr;
  } else if (worker_id == 0  && comm_rank == 0) {
      LOCAL_top_cp = GLOBAL_root_cp = OrFr_node(GLOBAL_root_or_fr) = B;
      COMM_root_cp = LOCAL_top_cp;
#else
  if (worker_id == 0) {
    LOCAL_top_cp = GLOBAL_root_cp = OrFr_node(GLOBAL_root_or_fr) = B;
#endif
  } else {
    printf("ROOT %d\n",getpid());
    B = LOCAL_top_cp = GLOBAL_root_cp;
    if(worker_id != 0)
     B->cp_tr = TR = ((choiceptr) (worker_offset(0) + (CELL)(B)))->cp_tr;
  }
  //printf("%d (%d,%d) ##############################------------------------------------------------------------------------- %p   root_cp %p     -- %p\n",comm_rank,team_id,worker_id,GLOBAL_root_or_fr,COMM_root_cp,LOCAL_top_cp);
  //  B->cp_h = H0;
  B->cp_ap = GETWORK;
  B->cp_or_fr = GLOBAL_root_or_fr;
  LOCAL_top_or_fr = GLOBAL_root_or_fr;
  LOCAL_load = 0;
  Set_LOCAL_prune_request(NULL);
  BRANCH(worker_id, 0) = 0;
#ifdef TABLING_INNER_CUTS
  LOCAL_pruning_scope = NULL;
#endif /* TABLING_INNER_CUTS */
#ifdef TABLING
  LOCAL_top_cp_on_stack = LOCAL_top_cp;
  adjust_freeze_registers();
#endif /* TABLING */
   printf("                                                                                 %d MAKE ROOT %p (%d,%d)\n",getpid(),LOCAL_top_cp,comm_rank,worker_id);
  return;
}


void free_root_choice_point(void) {
  B = LOCAL_top_cp->cp_b;
  printf("                                                                                 %d FREE ROOT %p (%d,%d)\n",getpid(),LOCAL_top_cp,comm_rank,worker_id);
#ifdef TABLING
  LOCAL_top_cp_on_stack =
#endif /* TABLING */
  LOCAL_top_cp = GLOBAL_root_cp = OrFr_node(GLOBAL_root_or_fr) = (choiceptr) LOCAL_LocalBase;
  return;
}


int p_share_work(void) {
  int a = 10;
  printf("P_SHARE (%d,%d)\n", team_id, worker_id); 


  int worker_q = LOCAL_share_request;

  if (! BITMAP_member(OrFr_members(REMOTE_top_or_fr(worker_q)), worker_id) ||
      B == REMOTE_top_cp(worker_q) ||
      (LOCAL_load <= GLOBAL_delayed_release_load  && OrFr_nearest_livenode(LOCAL_top_or_fr) == NULL)) {
    /* refuse sharing request */
    REMOTE_reply_signal(LOCAL_share_request) = no_sharing;
    LOCAL_share_request = MAX_WORKERS;
    PUT_OUT_REQUESTABLE(worker_id);
    return 0;
  }
  /* sharing request accepted */
  COMPUTE_SEGMENTS_TO_COPY_TO(worker_q);
  REMOTE_q_fase_signal(worker_q) = Q_idle;
  REMOTE_p_fase_signal(worker_q) = P_idle;
#ifndef TABLING
  /* wait for incomplete installations */
  while (LOCAL_reply_signal != worker_ready);
#endif /* TABLING */
  LOCAL_reply_signal = sharing;
  REMOTE_reply_signal(worker_q) = sharing;
  share_private_nodes(worker_q);
  if(Get_LOCAL_prune_request())
    CUT_send_prune_request(worker_q, Get_LOCAL_prune_request()); 
  REMOTE_reply_signal(worker_q) = nodes_shared;
  /* copy local stack ? */
  LOCK(REMOTE_lock_signals(worker_q));
  if (REMOTE_q_fase_signal(worker_q) < local) {
    REMOTE_p_fase_signal(worker_q) = local;              
    UNLOCK(REMOTE_lock_signals(worker_q));
    P_COPY_LOCAL_TO(worker_q);
  } else {
    UNLOCK(REMOTE_lock_signals(worker_q));
    goto sync_with_q;
  }
  /* copy global stack ? */
  LOCK(REMOTE_lock_signals(worker_q));
  if (REMOTE_q_fase_signal(worker_q) < global) {
    REMOTE_p_fase_signal(worker_q) = global;
    UNLOCK(REMOTE_lock_signals(worker_q));
    P_COPY_GLOBAL_TO(worker_q);
  } else {
    UNLOCK(REMOTE_lock_signals(worker_q));
    goto sync_with_q;
  }
  /* copy trail stack ? */
  LOCK(REMOTE_lock_signals(worker_q));
  if (REMOTE_q_fase_signal(worker_q) < trail) {
    REMOTE_p_fase_signal(worker_q) = trail;
    UNLOCK(REMOTE_lock_signals(worker_q));
    P_COPY_TRAIL_TO(worker_q);
  } else UNLOCK(REMOTE_lock_signals(worker_q));

sync_with_q:



  REMOTE_reply_signal(worker_q) = copy_done;
  while (LOCAL_reply_signal == sharing);
  
   //printf("\n \n------\nAAA  base %p    %p   %p  %d\n",GLOBAL_team_area_pointer(team_id),(GLOBAL_team_area_pointer(team_id) + (0 * Yap_worker_area_size)),REMOTE(0),REMOTE(0)); 
  while (REMOTE_reply_signal(worker_q) != worker_ready);
  LOCAL_share_request = MAX_WORKERS;
  PUT_IN_REQUESTABLE(worker_id);
  //printf("FIM P SHARE (%d,%d) -> (%p)\n", team_id, worker_id, worker_q);
  return 1;
}


int q_share_work(int worker_p) {
  register tr_fr_ptr aux_tr;
  register CELL aux_cell;
  
   //printf("%d Q_SHARE (%d,%d)\n", getpid(),team_id, worker_id); 
   //printf("(%d,%d) ::-- \n", team_id, worker_id); 

  LOCK_OR_FRAME(LOCAL_top_or_fr);
  if (REMOTE_prune_request(worker_p)) {
    /* worker p with prune request */
    UNLOCK_OR_FRAME(LOCAL_top_or_fr);
    return FALSE;
  }
 
  YAPOR_ERROR_CHECKING(q_share_work, Get_OrFr_pend_prune_cp(LOCAL_top_or_fr) && BRANCH_LTT(worker_p, OrFr_depth(LOCAL_top_or_fr)) < OrFr_pend_prune_ltt(LOCAL_top_or_fr));
  /* there is no pending prune with worker p at right --> safe move to worker p branch */
  CUT_reset_prune_request();
  if(Get_LOCAL_prune_request()){
   UNLOCK_OR_FRAME(LOCAL_top_or_fr);
   return FALSE;
  }
  BRANCH(worker_id, OrFr_depth(LOCAL_top_or_fr)) = BRANCH(worker_p, OrFr_depth(LOCAL_top_or_fr));
  UNLOCK_OR_FRAME(LOCAL_top_or_fr);
 //printf("WWWWWWWWWWWWWWWWWWW Q_SHARE (%d,%d) ::--  %d   |%p  %p|\n", team_id, worker_id, LOCAL_p_fase_signal,LOCAL_top_cp->cp_tr,TR); 
  /* unbind variables */
  aux_tr = LOCAL_top_cp->cp_tr;
  TABLING_ERROR_CHECKING(q_share_work, TR < aux_tr);
  while (aux_tr != TR) {
    aux_cell = TrailTerm(--TR);
    /* check for global or local variables */
    if (IsVarTerm(aux_cell)) {
      RESET_VARIABLE(aux_cell);
#ifdef TABLING
    } else if (IsPairTerm(aux_cell)) {
      aux_cell = (CELL) RepPair(aux_cell);
      if (IN_BETWEEN(LOCAL_TrailBase, aux_cell, LOCAL_TrailTop)) {
	/* avoid frozen segments */
        TR = (tr_fr_ptr) aux_cell;
	TABLING_ERROR_CHECKING(q_share_work, TR > (tr_fr_ptr) LOCAL_TrailTop);
	TABLING_ERROR_CHECKING(q_share_work, TR < aux_tr);
      }
#endif /* TABLING */
#ifdef MULTI_ASSIGNMENT_VARIABLES
    } else if (IsApplTerm(aux_cell)) {
      CELL *aux_ptr = RepAppl(aux_cell);
      Term aux_val = TrailTerm(--aux_tr);
      *aux_ptr = aux_val;
#endif /* MULTI_ASSIGNMENT_VARIABLES */
    }
  }
 //printf("WWWWWWWWWWWWWWWWWWW  2 Q_SHARE (%d,%d) ::--  %d\n", team_id, worker_id, LOCAL_p_fase_signal); 
  OPTYAP_ERROR_CHECKING(q_share_work, LOCAL_top_cp != LOCAL_top_cp_on_stack);
  OPTYAP_ERROR_CHECKING(q_share_work, YOUNGER_CP(B_FZ, LOCAL_top_cp));
  YAPOR_ERROR_CHECKING(q_share_work, LOCAL_reply_signal != worker_ready);

  /* make sharing request */
  LOCK_WORKER(worker_p);
  if (BITMAP_member(GLOBAL_bm_idle_workers, worker_p) || 
      REMOTE_share_request(worker_p) != MAX_WORKERS) {
    /* worker p is idle or has another request */
    UNLOCK_WORKER(worker_p);
    return FALSE;
  }
  REMOTE_share_request(worker_p) = worker_id;
  UNLOCK_WORKER(worker_p);
  // printf("WWWWWWWWWWWWWWWWWWW  3 Q_SHARE (%d,%d) ::--  %d\n", team_id, worker_id, LOCAL_p_fase_signal); 
  /* wait for an answer */
  while (LOCAL_reply_signal == worker_ready);
  if (LOCAL_reply_signal == no_sharing) {
    /* sharing request refused */
    LOCAL_reply_signal = worker_ready;
    return FALSE;
  }
  //printf("WWWWWWWWWWWWWWWWWWW  4 Q_SHARE (%d,%d) ::--  %d\n", team_id, worker_id, LOCAL_p_fase_signal); 
  //printf("?????????????me GLOBAL %p  %p TRAIL   %p  %p  LOCAL %p %p\n",LOCAL_start_global_copy,LOCAL_end_global_copy,LOCAL_start_trail_copy,LOCAL_end_trail_copy,LOCAL_start_local_copy,LOCAL_end_local_copy);
  /* copy trail stack ? */
  LOCK(LOCAL_lock_signals);
  if (LOCAL_p_fase_signal > trail) {
    LOCAL_q_fase_signal = trail;
    UNLOCK(LOCAL_lock_signals);
    //printf("??? trail Q_SHARE (%d,%d) %::--\n", team_id, worker_id); 
    Q_COPY_TRAIL_FROM(worker_p);
    //printf("??? copy_trail Q_SHARE (%d,%d) %::--\n", team_id, worker_id); 
  } else {
    UNLOCK(LOCAL_lock_signals);
    goto sync_with_p;
  }

  /* copy global stack ? */
  LOCK(LOCAL_lock_signals);
  if (LOCAL_p_fase_signal > global) {
    LOCAL_q_fase_signal = global;
    UNLOCK(LOCAL_lock_signals);
    Q_COPY_GLOBAL_FROM(worker_p);
  } else {
    UNLOCK(LOCAL_lock_signals);
    goto sync_with_p;
  }

  /* copy local stack ? */
  while (LOCAL_reply_signal < nodes_shared);
  LOCK(LOCAL_lock_signals);
  if (LOCAL_p_fase_signal > local) {
    LOCAL_q_fase_signal = local;
    UNLOCK(LOCAL_lock_signals);
    Q_COPY_LOCAL_FROM(worker_p);
  } else UNLOCK(LOCAL_lock_signals);

sync_with_p:
#ifdef TABLING
  REMOTE_reply_signal(worker_p) = worker_ready;
#else
  REMOTE_reply_signal(worker_p) = copy_done;
#endif /* TABLING */
  while (LOCAL_reply_signal != copy_done);

#if INCREMENTAL_COPY
  /* install fase --> TR and LOCAL_top_cp->cp_tr are equal */
  aux_tr = ((choiceptr) LOCAL_start_local_copy)->cp_tr;
  TR = ((choiceptr) LOCAL_end_local_copy)->cp_tr;
  Yap_NEW_MAHASH((ma_h_inner_struct *)HR);
  while (TR != aux_tr) {
    aux_cell = TrailTerm(--aux_tr);
    if (IsVarTerm(aux_cell)) {
      if (aux_cell < LOCAL_start_global_copy || EQUAL_OR_YOUNGER_CP((choiceptr)LOCAL_end_local_copy, (choiceptr)aux_cell)) {
	YAPOR_ERROR_CHECKING(q_share_work, (CELL *)aux_cell < H0);
	YAPOR_ERROR_CHECKING(q_share_work, (ADDR)aux_cell > LOCAL_LocalBase);
#ifdef TABLING
        *((CELL *) aux_cell) = TrailVal(aux_tr);
#else
        *((CELL *) aux_cell) = *((CELL *) (worker_offset(worker_p) + aux_cell));
#endif /* TABLING */
      }
#ifdef TABLING 
    } else if (IsPairTerm(aux_cell)) {
      aux_cell = (CELL) RepPair(aux_cell);
      if (IN_BETWEEN(LOCAL_TrailBase, aux_cell, LOCAL_TrailTop)) {
        /* avoid frozen segments */
        aux_tr = (tr_fr_ptr) aux_cell;
      }
#endif /* TABLING */
#ifdef MULTI_ASSIGNMENT_VARIABLES
    } else if (IsApplTerm(aux_cell)) {
      CELL *cell_ptr = RepAppl(aux_cell);
      if (((CELL *)aux_cell < LOCAL_top_cp->cp_h || 
          EQUAL_OR_YOUNGER_CP(LOCAL_top_cp, (choiceptr)aux_cell)) &&
	  !Yap_lookup_ma_var(cell_ptr)) {
	/* first time we found the variable, let's put the new value */
#ifdef TABLING
        *cell_ptr = TrailVal(aux_tr);
#else
        *cell_ptr = *((CELL *) (worker_offset(worker_p) + (CELL)cell_ptr));
#endif /* TABLING */
      }
      /* skip the old value */
      aux_tr--;
#endif /* MULTI_ASSIGNMENT_VARIABLES */
    }
  }
#endif /* incremental */

  /* update registers and return */
  PUT_OUT_ROOT_NODE(worker_id);
#ifndef TABLING
  REMOTE_reply_signal(worker_p) = worker_ready;
#endif /* TABLING */
  TR = (tr_fr_ptr) LOCAL_end_trail_copy;

//printf(" GLOBAL %p --- %p TRAIL   %p --- %p  LOCAL %p --- %p\n",LOCAL_start_global_copy,LOCAL_end_global_copy,LOCAL_start_trail_copy,LOCAL_end_trail_copy,LOCAL_start_local_copy,LOCAL_end_local_copy);
//printf(" GLOBAL %p --- %p TRAIL   %p --- %p  LOCAL %p --- %p\n",LOCAL_start_global_copy + worker_offset(worker_p),LOCAL_end_global_copy+ worker_offset(worker_p),LOCAL_start_trail_copy+ worker_offset(worker_p),LOCAL_end_trail_copy+ worker_offset(worker_p),LOCAL_start_local_copy+ worker_offset(worker_p),LOCAL_end_local_copy+ worker_offset(worker_p));


//printf("\n \n------\n base 1- %p   2- %p   %p %d \n",GLOBAL_team_area_pointer(team_id),(GLOBAL_team_area_pointer(team_id) + (1 * Yap_worker_area_size)),worker_offset(0),worker_offset(0                   
 

  LOCAL_reply_signal = worker_ready;
  PUT_IN_REQUESTABLE(worker_id);
#ifdef TABLING
  adjust_freeze_registers();
#endif /* TABLING */
  //printf("FIM Q_SHARE (%d,%d) <- (%d)\n", team_id, worker_id,worker_p); 
  LOCAL_is_team_share = 55;
  return TRUE;
}


/* ------------------------- **
**      Local functions      **
** ------------------------- */

static
void share_private_nodes(int worker_q) {
  choiceptr sharing_node = B;

#ifdef DEBUG_OPTYAP
  OPTYAP_ERROR_CHECKING(share_private_nodes, YOUNGER_CP(LOCAL_top_cp, LOCAL_top_cp_on_stack));
  { choiceptr aux_cp = B;
    while (aux_cp != LOCAL_top_cp) {
      OPTYAP_ERROR_CHECKING(share_private_nodes, YOUNGER_CP(LOCAL_top_cp, aux_cp));
      OPTYAP_ERROR_CHECKING(share_private_nodes, EQUAL_OR_YOUNGER_CP(LOCAL_top_cp_on_stack, aux_cp));
      aux_cp = aux_cp->cp_b;
    }
  }
#endif /* DEBUG_OPTYAP */

#ifdef TABLING
  /* check if the branch is already shared */
  if (EQUAL_OR_YOUNGER_CP(LOCAL_top_cp_on_stack, sharing_node)) {
    or_fr_ptr or_frame;
    sg_fr_ptr sg_frame;
    dep_fr_ptr dep_frame;

#ifdef DEBUG_OPTYAP
    { or_fr_ptr aux_or_fr;
      aux_or_fr = LOCAL_top_or_fr;
      while (aux_or_fr != REMOTE_top_or_fr(worker_q)) {
	OPTYAP_ERROR_CHECKING(share_private_nodes, YOUNGER_CP(OrFr_node(REMOTE_top_or_fr(worker_q)), OrFr_node(aux_or_fr)));
        aux_or_fr = OrFr_next_on_stack(aux_or_fr);
      }
    }
#endif /* DEBUG_OPTYAP */

    /* update old shared nodes */
    or_frame = LOCAL_top_or_fr;
    while (or_frame != REMOTE_top_or_fr(worker_q)) {
      LOCK_OR_FRAME(or_frame);
      BRANCH(worker_q, OrFr_depth(or_frame)) = BRANCH(worker_id, OrFr_depth(or_frame));
      OrFr_owners(or_frame)++;
      if (BITMAP_member(OrFr_members(or_frame), worker_id))
        BITMAP_insert(OrFr_members(or_frame), worker_q);
      UNLOCK_OR_FRAME(or_frame);
      or_frame = OrFr_next_on_stack(or_frame);
    }

    /* update worker Q top subgoal frame */
    sg_frame = LOCAL_top_sg_fr;
    while (sg_frame && YOUNGER_CP(SgFr_gen_cp(sg_frame), sharing_node)) {
      sg_frame = SgFr_next(sg_frame);
    }
    REMOTE_top_sg_fr(worker_q) = sg_frame;

    /* update worker Q top dependency frame */
    dep_frame = LOCAL_top_dep_fr;
    while (YOUNGER_CP(DepFr_cons_cp(dep_frame), sharing_node)) {
      dep_frame = DepFr_next(dep_frame);
    }
    REMOTE_top_dep_fr(worker_q) = dep_frame;

    /* update worker Q top shared nodes */
    REMOTE_top_cp_on_stack(worker_q) = REMOTE_top_cp(worker_q) = LOCAL_top_cp;
    REMOTE_top_or_fr(worker_q) = LOCAL_top_or_fr;
  } else
#endif /* TABLING */
  {
    int depth;
    bitmap bm_workers;
    or_fr_ptr or_frame, previous_or_frame;
#ifdef TABLING
    choiceptr consumer_cp, next_node_on_branch;
    dep_fr_ptr dep_frame;
    sg_fr_ptr sg_frame;
    CELL *stack, *stack_limit;

    /* find top dependency frame above current choice point */
    dep_frame = LOCAL_top_dep_fr;
    while (EQUAL_OR_YOUNGER_CP(DepFr_cons_cp(dep_frame), sharing_node)) {
      dep_frame = DepFr_next(dep_frame);
    }
    /* initialize tabling auxiliary variables */ 
    consumer_cp = DepFr_cons_cp(dep_frame);
    next_node_on_branch = NULL;
    stack_limit = (CELL *)TR;
    stack = (CELL *)LOCAL_TrailTop;
#endif /* TABLING */

    /* initialize auxiliary variables */
    BITMAP_clear(bm_workers);
    BITMAP_insert(bm_workers, worker_id);
    BITMAP_insert(bm_workers, worker_q);
    previous_or_frame = NULL;
    depth = OrFr_depth(LOCAL_top_or_fr);

    /* sharing loop */
#ifdef TABLING
    while (YOUNGER_CP(sharing_node, LOCAL_top_cp_on_stack)) {
#else
    while (sharing_node != LOCAL_top_cp) {
#endif /* TABLING */

#ifdef DEBUG_OPTYAP
      if (next_node_on_branch) {
        choiceptr aux_cp = B;
        while (aux_cp != next_node_on_branch) {
	  OPTYAP_ERROR_CHECKING(share_private_nodes, sharing_node == aux_cp);
	  OPTYAP_ERROR_CHECKING(share_private_nodes, YOUNGER_CP(next_node_on_branch, aux_cp));
          aux_cp = aux_cp->cp_b;
        }
      } else {
        choiceptr aux_cp = B;
        while (aux_cp != sharing_node) {
	  OPTYAP_ERROR_CHECKING(share_private_nodes, YOUNGER_CP(sharing_node, aux_cp));
          aux_cp = aux_cp->cp_b;
        }
      }
#endif /* DEBUG_OPTYAP */

      ALLOC_OR_FRAME(or_frame);
      if (previous_or_frame) {
#ifdef TABLING
        OrFr_next_on_stack(previous_or_frame) =
#endif /* TABLING */
        OrFr_nearest_livenode(previous_or_frame) = OrFr_next(previous_or_frame) = or_frame;
      }
      previous_or_frame = or_frame;
      depth++;
      INIT_LOCK(OrFr_lock(or_frame));
      OrFr_node(or_frame) = sharing_node;
      OrFr_alternative(or_frame) = sharing_node->cp_ap;
      OrFr_pend_prune_cp(or_frame) = NULL;
      OrFr_nearest_leftnode(or_frame) = LOCAL_top_or_fr;
      OrFr_qg_solutions(or_frame) = NULL;
      //printf(" (%p)    ------  %p ------ getwork %p\n", or_frame, sharing_node,GETWORK);
#ifdef TABLING_INNER_CUTS
      OrFr_tg_solutions(or_frame) = NULL;
#endif /* TABLING_INNER_CUTS */
#ifdef TABLING
      OrFr_suspensions(or_frame) = NULL;
      OrFr_nearest_suspnode(or_frame) = or_frame;
      OrFr_owners(or_frame) = 2;
      if (next_node_on_branch)
        BITMAP_clear(OrFr_members(or_frame));
      else
#endif /* TABLING */
        OrFr_members(or_frame) = bm_workers;

      YAPOR_ERROR_CHECKING(share_private_nodes, sharing_node->cp_ap == GETWORK || sharing_node->cp_ap == GETWORK_SEQ);
      if (sharing_node->cp_ap && YAMOP_SEQ(sharing_node->cp_ap)) {
        sharing_node->cp_ap = GETWORK_SEQ;
      } else {
        sharing_node->cp_ap = GETWORK;
      }
      sharing_node->cp_or_fr = or_frame;
      sharing_node = sharing_node->cp_b;

#ifdef TABLING
      /* when next_node_on_branch is not NULL the **
      ** sharing_node belongs to a frozen branch. */   
      if (YOUNGER_CP(consumer_cp, sharing_node)) {
        /* frozen stack segment */
        if (! next_node_on_branch)
          next_node_on_branch = sharing_node;
        STACK_PUSH_UP(or_frame, stack);
        STACK_CHECK_EXPAND(stack, stack_limit);
        STACK_PUSH_UP(sharing_node, stack);
        STACK_CHECK_EXPAND(stack, stack_limit);
        sharing_node = consumer_cp;
        dep_frame = DepFr_next(dep_frame);
        consumer_cp = DepFr_cons_cp(dep_frame);
      } else if (consumer_cp == sharing_node) {
        dep_frame = DepFr_next(dep_frame);
        consumer_cp = DepFr_cons_cp(dep_frame);
      }
      if (next_node_on_branch == sharing_node)
        next_node_on_branch = NULL;
#endif /* TABLING */
      OPTYAP_ERROR_CHECKING(share_private_nodes, next_node_on_branch && YOUNGER_CP(next_node_on_branch, sharing_node));
    }

    /* initialize last or-frame pointer */
    or_frame = sharing_node->cp_or_fr;
    if (previous_or_frame) {
#ifdef TABLING
      OrFr_next_on_stack(previous_or_frame) =
#endif /* TABLING */
      OrFr_nearest_livenode(previous_or_frame) = OrFr_next(previous_or_frame) = or_frame;
    }

#ifdef TABLING
    /* update or-frames stored in auxiliary stack */
    while (STACK_NOT_EMPTY(stack, (CELL *)LOCAL_TrailTop)) {
      next_node_on_branch = (choiceptr) STACK_POP_DOWN(stack);
      or_frame = (or_fr_ptr) STACK_POP_DOWN(stack);
      OrFr_nearest_livenode(or_frame) = OrFr_next(or_frame) = next_node_on_branch->cp_or_fr;
    }
#endif /* TABLING */

    /* update depth */
    if (depth >= MAX_BRANCH_DEPTH)
      Yap_Error(INTERNAL_ERROR, TermNil, "maximum depth exceded (share_private_nodes)");
    or_frame = B->cp_or_fr;
#ifdef TABLING
    previous_or_frame = LOCAL_top_cp_on_stack->cp_or_fr;
    while (or_frame != previous_or_frame) {
#else
    while (or_frame != LOCAL_top_or_fr) {
#endif /* TABLING */
      unsigned int branch;
      if (OrFr_alternative(or_frame)) {
        branch = YAMOP_OR_ARG(OrFr_alternative(or_frame)) + 1;
      } else {
        branch = 1;
      }
      branch |= YAMOP_CUT_FLAG;  /* in doubt, assume cut */
      BRANCH(worker_id, depth) = BRANCH(worker_q, depth) = branch;
      OrFr_depth(or_frame) = depth--;
      or_frame = OrFr_next_on_stack(or_frame);
    }

    YAPOR_ERROR_CHECKING(share_private_nodes, depth != OrFr_depth(LOCAL_top_or_fr));

#ifdef DEBUG_OPTYAP
    { or_fr_ptr aux_or_fr = B->cp_or_fr;
      choiceptr aux_cp;
      while (aux_or_fr != LOCAL_top_cp_on_stack->cp_or_fr) {
        aux_cp = OrFr_node(aux_or_fr);
	OPTYAP_ERROR_CHECKING(share_private_nodes, OrFr_next(aux_or_fr) != aux_cp->cp_b->cp_or_fr);
	OPTYAP_ERROR_CHECKING(share_private_nodes, OrFr_nearest_livenode(aux_or_fr) != aux_cp->cp_b->cp_or_fr);
        aux_or_fr = OrFr_next_on_stack(aux_or_fr);
      }
      aux_or_fr = B->cp_or_fr;
      while (aux_or_fr != LOCAL_top_cp_on_stack->cp_or_fr) {
        or_fr_ptr nearest_leftnode = OrFr_nearest_leftnode(aux_or_fr);
        aux_cp = OrFr_node(aux_or_fr);
        while (OrFr_node(nearest_leftnode) != aux_cp) {
	  OPTYAP_ERROR_CHECKING(share_private_nodes, YOUNGER_CP(OrFr_node(nearest_leftnode), aux_cp));
          aux_cp = aux_cp->cp_b;
        }
        aux_or_fr = OrFr_next_on_stack(aux_or_fr);
      }
    }
#endif /* DEBUG_OPTYAP */

    /* update old shared nodes */
    while (or_frame != REMOTE_top_or_fr(worker_q)) {
      LOCK_OR_FRAME(or_frame);
      BRANCH(worker_q, OrFr_depth(or_frame)) = BRANCH(worker_id, OrFr_depth(or_frame));
#ifdef TABLING
      OrFr_owners(or_frame)++;
      if (BITMAP_member(OrFr_members(or_frame), worker_id))
#endif /* TABLING */
        BITMAP_insert(OrFr_members(or_frame), worker_q);
      UNLOCK_OR_FRAME(or_frame);
      or_frame = OrFr_next_on_stack(or_frame);
    }

    LOCK_OR_FRAME(REMOTE_top_or_fr(worker_q));
    or_fr_ptr old_top = REMOTE_top_or_fr(worker_q);
    Set_REMOTE_top_cp(worker_q,B);
    Set_LOCAL_top_cp(B);
    REMOTE_top_or_fr(worker_q) = LOCAL_top_or_fr = Get_LOCAL_top_cp()->cp_or_fr;
    UNLOCK_OR_FRAME(old_top);  

#ifdef TABLING
    /* update subgoal frames in the maintained private branches */
    sg_frame = LOCAL_top_sg_fr;
    while (sg_frame && YOUNGER_CP(SgFr_gen_cp(sg_frame), B)) {
      choiceptr top_cp_on_branch;
      top_cp_on_branch = SgFr_gen_cp(sg_frame);
      while (YOUNGER_CP(top_cp_on_branch, B)) {
        top_cp_on_branch = top_cp_on_branch->cp_b;
      }
      SgFr_gen_top_or_fr(sg_frame) = top_cp_on_branch->cp_or_fr;
      sg_frame = SgFr_next(sg_frame);
    }
    /* update worker Q top subgoal frame */
    REMOTE_top_sg_fr(worker_q) = sg_frame;
    /* update subgoal frames in the recently shared branches */
    while (sg_frame && YOUNGER_CP(SgFr_gen_cp(sg_frame), LOCAL_top_cp_on_stack)) {
      SgFr_gen_worker(sg_frame) = MAX_WORKERS;
      SgFr_gen_top_or_fr(sg_frame) = SgFr_gen_cp(sg_frame)->cp_or_fr;
      sg_frame = SgFr_next(sg_frame);
    }

    /* update dependency frames in the maintained private branches */
    dep_frame = LOCAL_top_dep_fr;
    while (YOUNGER_CP(DepFr_cons_cp(dep_frame), B)) {
      choiceptr top_cp_on_branch;
      top_cp_on_branch = DepFr_cons_cp(dep_frame);
      while (YOUNGER_CP(top_cp_on_branch, B)) {
        top_cp_on_branch = top_cp_on_branch->cp_b;
      }
      DepFr_top_or_fr(dep_frame) = top_cp_on_branch->cp_or_fr;
      dep_frame = DepFr_next(dep_frame);
    }
    /* update worker Q top dependency frame */
    REMOTE_top_dep_fr(worker_q) = dep_frame;
    /* update dependency frames in the recently shared branches */
    while (YOUNGER_CP(DepFr_cons_cp(dep_frame), LOCAL_top_cp_on_stack)) {
      DepFr_top_or_fr(dep_frame) = DepFr_cons_cp(dep_frame)->cp_or_fr;
      dep_frame = DepFr_next(dep_frame);
    }
#endif /* TABLING */

#ifdef DEBUG_OPTYAP
    { dep_fr_ptr aux_dep_fr = LOCAL_top_dep_fr;
      while(aux_dep_fr != GLOBAL_root_dep_fr) {
        choiceptr top_cp_on_branch;
        top_cp_on_branch = DepFr_cons_cp(aux_dep_fr);
        while (YOUNGER_CP(top_cp_on_branch, B)) {
          top_cp_on_branch = top_cp_on_branch->cp_b;
        }
	OPTYAP_ERROR_CHECKING(share_private_nodes, top_cp_on_branch->cp_or_fr != DepFr_top_or_fr(aux_dep_fr));
        aux_dep_fr = DepFr_next(aux_dep_fr);
      }
    }
#endif /* DEBUG_OPTYAP */

    /* update top shared nodes */
#ifdef TABLING
    REMOTE_top_cp_on_stack(worker_q) = LOCAL_top_cp_on_stack = 
#endif /* TABLING */
    REMOTE_top_cp(worker_q) = LOCAL_top_cp = B;
    REMOTE_top_or_fr(worker_q) = LOCAL_top_or_fr = LOCAL_top_cp->cp_or_fr;
  }

#ifdef TABLING_INNER_CUTS
  /* update worker Q pruning scope */
  if (LOCAL_pruning_scope && EQUAL_OR_YOUNGER_CP(LOCAL_top_cp, LOCAL_pruning_scope)) {
    REMOTE_pruning_scope(worker_q) = LOCAL_pruning_scope;
    PUT_IN_PRUNING(worker_q);
  } else {
    PUT_OUT_PRUNING(worker_q);
    REMOTE_pruning_scope(worker_q) = NULL;
  }
#endif /* TABLING_INNER_CUTS */

  /* update worker Q prune request */
  if (LOCAL_prune_request) {
    CUT_send_prune_request(worker_q, LOCAL_prune_request);
  }

  /* update load and return */
  REMOTE_load(worker_q) = LOCAL_load = 0;
  return;
}
#endif /* YAPOR_COPY */

#ifdef YAPOR_TEAMS

/*
static inline
void PUT_IDLE_TEAM(int team_num) {
  LOCK(GLOBAL_locks_bm_idle_teams);
  BITMAP_insert(GLOBAL_bm_idle_teams, team_num);
  UNLOCK(GLOBAL_locks_bm_idle_teams);
  return;
}

static inline
void PUT_BUSY_TEAM(int team_num) {
  LOCK(GLOBAL_locks_bm_idle_teams);
  BITMAP_delete(GLOBAL_bm_idle_teams, team_num);
  UNLOCK(GLOBAL_locks_bm_idle_teams);
  return;
}

*/
static inline
void SCH_refuse_team_share_request_if_any(void)  {
 // CACHE_REGS
  if (GLOBAL_share_team_request(team_id) != MAX_WORKERS) {
    //printf("(%d) REFUSE %d  \n",GLOBAL_share_team_request(team_id));
    GLOBAL_team_reply_signal(COMM_translation_array(GLOBAL_share_team_request(team_id))) = no_sharing;
    GLOBAL_share_team_request(team_id) = MAX_WORKERS;
    //PUT_OUT_REQUESTABLE(worker_id);
  }
  return;
}

int select_team(){

int team_p = rand() % COMM_number_teams;

while (team_p == comm_rank)
   team_p = rand() % COMM_number_teams;

return team_p;

}

/*

int select_team(){
int i;
bitmap idle_workers = GLOBAL_bm_idle_teams;
BITMAP_insert(idle_workers,team_id);

  for (i = 0; i < GLOBAL_number_teams; i++) 
    if (!BITMAP_member(idle_workers, i) && REMOTE_TEAM(0,i)->optyap_data_.load > 60){
       //printf("load %d  (%d) (%c,%d)\n",REMOTE_TEAM(0,i)->optyap_data_.load,i,PRINT_TEAM); 
      return i;
    }

return MAX_WORKERS;
}
*/

int mpi_team_get_work(){

printf("%d MPI GET WORK (%d,%d)\n",getpid(),comm_rank,worker_id);

PUT_IN_FINISHED(worker_id);

GLOBAL_mpi_load(comm_rank) = 0;

int team_p = 0;

 int test;
 int i;
 int count = 0;
 int flag = 0;
 MPI_Status status;
 //int *msg = malloc(3*sizeof(int));
 int send_msg[3];
 send_msg[0] = 1;
 send_msg[1] = 1;
 send_msg[2] = 8;
 int recv_msg[3];
 MPI_Request *request;
 
 test =1;
 int waiting_for_response = 0;
 int number_teams;
 MPI_Comm_size(MPI_COMM_WORLD,&number_teams);


//printf( "TEAM %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %d\n",getpid(),comm_rank,team_id,worker_id);


 while(1){
   MPI_Iprobe(MPI_ANY_SOURCE, 44, MPI_COMM_WORLD, &flag, &status );
   if(flag){
     MPI_Recv( &recv_msg, 3, MPI_INT, MPI_ANY_SOURCE, 44, MPI_COMM_WORLD, &status );
     printf("GET WORK MENSAGEM %d: %d %d \n",getpid(),recv_msg[0],recv_msg[1]);
     if(recv_msg[0] == 2){
       if(mpi_team_q_share_work(recv_msg[1])){
         send_msg[0] = 4;
         send_msg[1] = comm_rank;
         for (i = 0; i < number_teams; i++)
            if(GLOBAL_mpi_load(i) && i!=comm_rank)
               MPI_Send(&send_msg, 3, MPI_INT, i, 44, MPI_COMM_WORLD);
        //OUT ROOT
        PUT_OUT_ROOT_NODE(worker_id);
        //BUSY WORKER
        LOCK(GLOBAL_locks_bm_idle_workers);
        BITMAP_delete(GLOBAL_bm_idle_workers, worker_id);
        UNLOCK(GLOBAL_locks_bm_idle_workers);
        //FREE WORKERS
        BITMAP_clear(GLOBAL_bm_free_workers);
        //MARK AS FREE WORKER
        LOCK(GLOBAL_lock_free_workers);
        BITMAP_insert(GLOBAL_bm_free_workers, worker_id);
        UNLOCK(GLOBAL_lock_free_workers);
        } 
        return 1;
     }
     if(recv_msg[0] == 1){
       GLOBAL_mpi_load(recv_msg[1]) = 0;
       send_msg[0] = 3;
       send_msg[1] = comm_rank;
       MPI_Send(&send_msg, 3, MPI_INT, recv_msg[1], 44, MPI_COMM_WORLD);
     }
     if(recv_msg[0] == 3){
        GLOBAL_mpi_load(recv_msg[1]) = 0;
        waiting_for_response = 0;
     }
     if(recv_msg[0] == 4){
        GLOBAL_mpi_load(recv_msg[1]) = 1;
     }
     if(recv_msg[0] == 5){
       if(comm_rank == 0){
         if(recv_msg[1] == 1){
           waiting_for_response = 0;
         }
         else {
           for (i = 0; i < number_teams; i++) {
             if(i!=comm_rank){
                send_msg[0] = 6;
                send_msg[1] = comm_rank;
                MPI_Send(&send_msg, 3, MPI_INT, i, 44, MPI_COMM_WORLD);
             }   
           }
          return 0; 
         }        
       } else {
       send_msg[0] = 5;
       send_msg[1] = 0;
       MPI_Send(&send_msg, 3, MPI_INT, (comm_rank+1)%number_teams, 44, MPI_COMM_WORLD);
       }
     }
     if(recv_msg[0] == 6){
        return 0;
     }
   }
   if(!waiting_for_response){
     //printf("(%d) RESPONSE  %d\n",comm_rank,GLOBAL_mpi_load(0));
     for (i = 0; i < number_teams; i++) {
       if(GLOBAL_mpi_load(i) && i!=comm_rank){
       send_msg[0] = 1;
       send_msg[1] = comm_rank;
       printf("GET MENSAGEM: %d %d %d\n",send_msg[0],send_msg[1],send_msg[2]);
       MPI_Send(&send_msg, 3, MPI_INT, i, 44, MPI_COMM_WORLD);
       waiting_for_response = 1;
       break;
       }
     }
    if(waiting_for_response == 0 && comm_rank == 0){
       send_msg[0] = 5;
       send_msg[1] = 0;
       MPI_Send(&send_msg, 3, MPI_INT, 1, 44, MPI_COMM_WORLD);
       waiting_for_response = 1;
    }
   }
 }



} 


int team_get_work(){

PUT_IN_FINISHED(worker_id);

int team_p = 0;

printf("NON MPI                                           ############################## (%d,%d) TEAM GET WORK\n",team_id,worker_id);

    
    LOCK(COMM_locks_bm_idle_teams);
    BITMAP_insert(COMM_bm_idle_teams, comm_rank);
    UNLOCK(COMM_locks_bm_idle_teams); 
    LOCK_TEAM(team_id);
    SCH_refuse_team_share_request_if_any();
    UNLOCK_TEAM(team_id);


  while(1){
    if(!BITMAP_same(COMM_bm_present_teams,COMM_bm_idle_teams)){
    //if(team_id == 2  && !BITMAP_same(COMM_bm_present_teams(GLOBAL_comm_number(team_id)),COMM_bm_idle_teams(GLOBAL_comm_number(team_id)))){
      team_p = select_team();
      if(team_q_share_work(team_p)){
        //printf("GOT %d                                            ############################## (%d,%d) TEAM GET WORK\n",team_p,team_id,worker_id);
        //BUSY TEAM
        LOCK(COMM_locks_bm_idle_teams);
        BITMAP_delete(COMM_bm_idle_teams, comm_rank);
        UNLOCK(COMM_locks_bm_idle_teams); 
        //OUT ROOT
        PUT_OUT_ROOT_NODE(worker_id);
        //BUSY WORKER
        LOCK(GLOBAL_locks_bm_idle_workers);
        BITMAP_delete(GLOBAL_bm_idle_workers, worker_id);
        UNLOCK(GLOBAL_locks_bm_idle_workers);
        //FREE WORKERS
        BITMAP_clear(GLOBAL_bm_free_workers);
        //MARK AS FREE WORKER
        LOCK(GLOBAL_lock_free_workers);
        BITMAP_insert(GLOBAL_bm_free_workers, worker_id);
        UNLOCK(GLOBAL_lock_free_workers);
        //BITMAP_clear(GLOBAL_bm_finished_workers);
        //GLOBAL_AUX(team_id) = 1;
        return TRUE;
      }
      else if (BITMAP_same(COMM_bm_present_teams,COMM_bm_idle_teams))
        return FALSE;
    }
    else{
      return FALSE;
    }

  }

} 








void invalidar_alternativas(choiceptr b, choiceptr root,int team_q){
  int i=1,j;
  int share = 0, count=0;
  choiceptr aux;

  //printf("INVALIDWORK  %p    GETWORK %p  %p\n",INVALIDWORK,GETWORK,root->cp_ap);

  while(b != root){
    if(share){
     if(b->cp_ap != INVALIDWORK){
      if(b->cp_ap == GETWORK){
        LOCK_OR_FRAME(b->cp_or_fr); 
        if(OrFr_alternative(b->cp_or_fr) == NULL){                      
         GLOBAL_team_array(team_q,i) = INVALIDWORK;                  
       } else {                                                          
        GLOBAL_team_array(team_q,i) = OrFr_alternative(b->cp_or_fr);
        OrFr_alternative(b->cp_or_fr) = NULL;
        share = 0;  
        count++;
      }                     
      UNLOCK_OR_FRAME(b->cp_or_fr);     
    }else {
     aux = b->cp_ap;
     b->cp_ap = INVALIDWORK;
     share = 0;
     count++;                                              
     GLOBAL_team_array(team_q,i)  = aux;       
   }                  		                                                                                            	
  } else
  GLOBAL_team_array(team_q,i) = INVALIDWORK;
    } else{
  if(b->cp_ap != INVALIDWORK || (b->cp_ap == GETWORK && OrFr_alternative(b->cp_or_fr) != NULL)){
   share = 1;                 		                                                                                            	
   GLOBAL_team_array(team_q,i) = INVALIDWORK;
  } else
  GLOBAL_team_array(team_q,i) = INVALIDWORK;
  }

  //printf("(%d) %p  ----   %p ----------------------B = %p \n",i,GLOBAL_team_array(team_q,i),b->cp_ap,b);

  b=b->cp_b;
  i++;   
  }



if(root->cp_ap!=GETWORK)
  printf("INV ERRO !!!  %p --- %p \n",root->cp_ap,GLOBAL_team_array(team_q,1));



GLOBAL_team_array(team_q,0) = i;
//printf("-> -> -> %d  %d\n",count,i);

//for( j=i; j>0 ; j--)
//printf(" %d ------- %p\n",j,GLOBAL_team_array(team_q,j));
}


int team_p_share_work(){
  int rank_q = GLOBAL_share_team_request(team_id);
  int team_q = COMM_translation_array(rank_q);
  int worker_q = 0;


 // printf("------------------------------------TEAM P SHARE  (%d,%d) --- %d\n",team_id,worker_id,team_q);

  /* sharing request accepted */

  CELL start_global_copy = (CELL) (H0);                       
  CELL end_global_copy   = (CELL) (HR);                        
  CELL start_local_copy  = (CELL) (B);                        
  CELL end_local_copy    = (CELL) (GLOBAL_root_cp);           
  CELL start_trail_copy  = (CELL) (GLOBAL_root_cp->cp_tr);    
  CELL end_trail_copy    = (CELL) (TR);

  while (GLOBAL_team_reply_signal(team_id) != worker_ready);
  GLOBAL_team_reply_signal(team_id) = sharing;
  GLOBAL_team_reply_signal(team_q) = sharing;
                                                   
memcpy((void *) (out_worker_offset(team_q,0) + start_global_copy),           
      (void *) start_global_copy,                                
      (size_t) (end_global_copy - start_global_copy));


if(GLOBAL_root_cp->cp_ap!=GETWORK)
  printf("%d COPY INV ERRO !!!  %p --- \n",team_id,GLOBAL_root_cp->cp_ap);
                                                   
memcpy((void *) (out_worker_offset(team_q,0) + start_local_copy),            
      (void *) start_local_copy,                                 
      (size_t) (end_local_copy - start_local_copy));

if(GLOBAL_root_cp->cp_ap!=GETWORK)
  printf("%d COPY INV ERRO !!!  %p --- \n",team_id,GLOBAL_root_cp->cp_ap);

                                             
memcpy((void *) (out_worker_offset(team_q,0) + start_trail_copy),            
      (void *) start_trail_copy,                                 
      (size_t) (end_trail_copy - start_trail_copy));

  
  LOCK(REMOTE_TEAM(team_q,0)->optyap_data_.lock);

  REMOTE_TEAM(team_q,0)->optyap_data_.trail_copy.end = end_trail_copy;
  REMOTE_TEAM(team_q,0)->optyap_data_.trail_copy.start = start_trail_copy;

  REMOTE_TEAM(team_q,0)->optyap_data_.global_copy.end = end_global_copy;
  REMOTE_TEAM(team_q,0)->optyap_data_.global_copy.start = start_global_copy;

  REMOTE_TEAM(team_q,0)->optyap_data_.local_copy.end = end_local_copy;
  REMOTE_TEAM(team_q,0)->optyap_data_.local_copy.start = start_local_copy;

  REMOTE_TEAM(team_q,0)->optyap_data_.is_team_share = 1;

  UNLOCK(REMOTE_TEAM(team_q,0)->optyap_data_.lock);
 
  GLOBAL_team_reply_signal(team_q) = nodes_shared;

//  REMOTE_TEAM(0,1)->optyap_data_.top_choice_point = Get_LOCAL_top_cp();
//  REMOTE_TEAM(0,1)->optyap_data_.top_choice_point = B;

  invalidar_alternativas(B,end_local_copy,team_q);
  
  


  GLOBAL_share_team_request(team_id) = MAX_WORKERS;
  GLOBAL_team_reply_signal(team_q) = copy_done;
  while (GLOBAL_team_reply_signal(team_id) == sharing);
  while (GLOBAL_team_reply_signal(team_q) != worker_ready);

  // printf("TEAM FIM P SHARE (%d,%d) -> (%d) \n",team_id,worker_id,team_q);

  return 1;
}


void install_array(choiceptr b, choiceptr root){
int i = 1;

if(root->cp_ap!=GETWORK)
  printf("%d ANT INV ERRO !!!  %p --- \n",team_id,root->cp_ap);

//printf(" install array \n");

//int i = GLOBAL_team_array(team_id,0);
//printf("*** total %d\n",i);
// i--;
 while(b != root){
 //printf("ANTES \n");
// printf("%p instalar %p   (%d)\n",b, b->cp_ap,i);
 b->cp_ap = GLOBAL_team_array(team_id,i);
 GLOBAL_team_array(team_id,i) = NULL;
 //printf("(%d,%d) %p instalar %p   (%d)\n",team_id,worker_id,b, b->cp_b,i);
 b=b->cp_b;
 i++;   
 }

if(root->cp_ap!=GETWORK)
  printf("%d 2 INV ERRO !!!  %p --- \n",team_id,root->cp_ap);

}



int team_q_share_work(int rank_p){

int team_p = COMM_translation_array(rank_p);  

 //printf("--------------------------------------0 TEAM Q SHARE  (%d,%d)  <<--  (%d)\n",team_id,worker_id,team_p);

//printf(" (%d,%d) (%d) %d  %d %d\n",team_id,worker_id,team_p,BITMAP_member(COMM_bm_idle_teams(GLOBAL_comm_number(team_id)), rank_p),GLOBAL_share_team_request(team_p),REMOTE_share_request(team_p));
 LOCK_TEAM(team_p);
 //printf("1 TEAM Q SHARE  (%d,%d)  --  (%d)\n",team_id,worker_id,team_p);
 if (BITMAP_member(COMM_bm_idle_teams, rank_p) || 
     GLOBAL_share_team_request(team_p) != MAX_WORKERS) {
    /* worker p is idle or has another request */
   // printf("NO TEAM Q SHARE  (%d,%d)  --  (%d)\n",team_id,worker_id,team_p);
    UNLOCK_TEAM(team_p);
    return FALSE;
  }
  //printf("TEAM Q SHARE  (%d,%d)  --  (%d)\n",team_id,worker_id,team_p);
  GLOBAL_share_team_request(team_p)  = comm_rank;
  UNLOCK_TEAM(team_p);
  //printf("Q 2 LOCK  \n");

  while (GLOBAL_team_reply_signal(team_id) == worker_ready);
  if (GLOBAL_team_reply_signal(team_id) == no_sharing) {
    GLOBAL_team_reply_signal(team_id) = worker_ready;
    return FALSE;
  }


  //while (GLOBAL_team_reply_signal(team_id) != copy_done);



 // int n = memcmp((void *) LOCAL_start_local_copy, (void *) (out_worker_offset(0,0)+LOCAL_start_local_copy),(size_t) (LOCAL_end_local_copy - LOCAL_start_local_copy));
  
  GLOBAL_team_reply_signal(team_p) = copy_done;
  while (GLOBAL_team_reply_signal(team_id) != copy_done);

  if(GLOBAL_root_cp->cp_ap!=GETWORK)
  printf("A ----------%d COPY INV ERRO !!!  %p --- \n",team_id,GLOBAL_root_cp->cp_ap);

  if(GLOBAL_root_cp != LOCAL_end_local_copy)
  printf("--------------------------- %d --- \n",team_id);


  //printf("--------------------------------------0 TEAM Q SHARE  (%d,%d)  --  (%d)\n",team_id,worker_id,team_p);
  
  install_array(LOCAL_start_local_copy,LOCAL_end_local_copy);
  GLOBAL_root_cp = LOCAL_end_local_copy;
 
  if(GLOBAL_root_cp->cp_ap!=GETWORK)
  printf("B ----------%d COPY INV ERRO !!!  %p --- \n",team_id,GLOBAL_root_cp->cp_ap);


  GLOBAL_team_reply_signal(team_p) = worker_ready;
  GLOBAL_team_reply_signal(team_id) = worker_ready;


  LOCAL_top_or_fr = GLOBAL_root_or_fr;
  //PUT_IN_REQUESTABLE(worker_id);
  TR = (tr_fr_ptr) LOCAL_end_trail_copy;

  //printf("TEAM FIM Q SHARE (%d,%d) \n",team_id,worker_id);
  //GLOBAL_total_share++;
  return TRUE;

}


void mpi_invalidar_alternativas(choiceptr b, choiceptr root,int* team_array){
  int i=1,j;
  int share = 0, count=0;
  choiceptr aux;

  //printf("INVALIDWORK  %p    GETWORK %p  %p\n",INVALIDWORK,GETWORK,root->cp_ap);

  while(b != root){
    if(share){
     if(b->cp_ap != INVALIDWORK){
      if(b->cp_ap == GETWORK){
        LOCK_OR_FRAME(b->cp_or_fr); 
        if(OrFr_alternative(b->cp_or_fr) == NULL){                      
         team_array[i] = INVALIDWORK;                  
       } else {                                                          
        team_array[i] = OrFr_alternative(b->cp_or_fr);
        OrFr_alternative(b->cp_or_fr) = NULL;
        share = 0;  
        count++;
      }                     
      UNLOCK_OR_FRAME(b->cp_or_fr);     
    }else {
     aux = b->cp_ap;
     b->cp_ap = INVALIDWORK;
     share = 0;
     count++;                                              
     team_array[i]  = aux;       
   }                  		                                                                                            	
  } else
  team_array[i] = INVALIDWORK;
    } else{
  if(b->cp_ap != INVALIDWORK || (b->cp_ap == GETWORK && OrFr_alternative(b->cp_or_fr) != NULL)){
   share = 1;                 		                                                                                            	
   team_array[i] = INVALIDWORK;
  } else
  team_array[i] = INVALIDWORK;
  }

  //printf("(%d) %p  ----   %p ----------------------B = %p \n",i,team_array[i],b->cp_ap,b);

  b=b->cp_b;
  i++;   
  }


if(root->cp_ap!=GETWORK)
  printf("INV ERRO !!!  %p --- %p \n",root->cp_ap,team_array[1]);

team_array[0] = i;
printf("-> -> -> %d  %d\n",count,i);

//for( j=i; j>0 ; j--)
//printf(" %d ------- %p\n",j,GLOBAL_team_array(team_q,j));
}


void mpi_install_array(choiceptr b, choiceptr root, int * team_array){
int i = 1;

if(root->cp_ap!=GETWORK)
  printf("%d ANT INV ERRO !!!  %p --- \n",team_id,root->cp_ap);

//printf(" install array \n");

//int i = GLOBAL_team_array(team_id,0);
//printf("*** total %d\n",i);
// i--;
 while(b != root){
 //printf("ANTES \n");
// printf("%p instalar %p   (%d)\n",b, b->cp_ap,i);
 b->cp_ap = team_array[i];
 //GLOBAL_team_array(team_id,i) = NULL;
 //printf("(%d,%d) %p instalar %p   (%d)\n",team_id,worker_id,b, b->cp_b,i);
 b=b->cp_b;
 i++;   
 }

if(root->cp_ap!=GETWORK)
  printf("%d 2 INV ERRO !!!  %p --- \n",team_id,root->cp_ap);

}

int mpi_team_q_share_work(int worker_p){

printf("%d ---------------MENSAGEM QQQ ----  %p   \n",getpid(),comm_rank);  
int msg[6];
 MPI_Status status;


MPI_Recv( &msg, 6, MPI_INT, worker_p, 44, MPI_COMM_WORLD, &status );


 CELL start_global_copy = msg[0];                    
 CELL end_global_copy   = msg[1];                      
 CELL start_local_copy  = msg[2];                      
 CELL end_local_copy    = msg[3];           
 CELL start_trail_copy  = msg[4];    
 CELL end_trail_copy    = msg[5]; 

//printf("H0 %p  %p   \n",msg[0],H0);  
//printf("HR %p  %p   \n",msg[1],HR);
//printf("B %p  %p   \n",msg[2],B);
//printf("root %p  %p   \n",msg[3],GLOBAL_root_cp);
//printf("cp_tr %p  %p   \n",msg[4],GLOBAL_root_cp->cp_tr);    
//printf("TR %p  %p   \n",msg[5],TR);    

//receber
int *team_array = malloc(100*sizeof(int));
MPI_Recv(team_array, 100, MPI_INT, worker_p, 10, MPI_COMM_WORLD,&status );
//printf("RECEBIDO---1\n");

MPI_Recv( start_global_copy, (size_t) end_global_copy-start_global_copy, MPI_BYTE, worker_p, 5, MPI_COMM_WORLD, &status );
//printf("RECEBIDO---\n");
//MPI_Send( start_global_copy, (size_t) end_global_copy-start_global_copy, MPI_BYTE, worker_p, 5, MPI_COMM_WORLD );


MPI_Recv( start_local_copy, (size_t) end_local_copy-start_local_copy, MPI_BYTE, worker_p, 6, MPI_COMM_WORLD, &status );
//MPI_Send( start_local_copy,(size_t) end_local_copy-start_local_copy, MPI_BYTE, worker_p, 6, MPI_COMM_WORLD );



MPI_Recv(  start_trail_copy, (size_t) end_trail_copy-start_trail_copy, MPI_BYTE, worker_p, 7, MPI_COMM_WORLD, &status );
//MPI_Send( start_trail_copy,(size_t) end_trail_copy-start_trail_copy, MPI_BYTE, worker_p, 7, MPI_COMM_WORLD );


mpi_install_array(start_local_copy, GLOBAL_root_cp, team_array);

//sleep(100);
  //choiceptr a = (choiceptr) LOCAL_start_local_copy;
 // printf("RECEBIDO--- %p %p\n",a, a->cp_tr);
  //install_array(LOCAL_start_local_copy,LOCAL_end_local_copy);
 GLOBAL_root_cp = end_local_copy;
 
 LOCAL_top_or_fr = GLOBAL_root_or_fr;
 TR = (tr_fr_ptr) end_trail_copy;
 B = start_local_copy;
 LOCAL_is_team_share = 1;

printf("FIM--------------MENSAGEM QQQ ----  %p   %d\n",comm_rank,LOCAL_is_team_share);  

  return TRUE;

}


int mpi_team_p_share_work(int worker_q){

printf("%d ---------------MENSAGEM PPP ----  %d  \n",getpid(),worker_q);  


  int copy_info[6];
  MPI_Status status;

  /* sharing request accepted */

  CELL start_global_copy = copy_info[0] = (CELL) (H0);                       
  CELL end_global_copy   = copy_info[1] = (CELL) (HR);                        
  CELL start_local_copy  = copy_info[2] = (CELL) (B);                        
  CELL end_local_copy    = copy_info[3] = (CELL) (GLOBAL_root_cp);           
  CELL start_trail_copy  = copy_info[4] = (CELL) (GLOBAL_root_cp->cp_tr);    
  CELL end_trail_copy    = copy_info[5] = (CELL) (TR);


MPI_Send( &copy_info, 6, MPI_INT, worker_q, 44, MPI_COMM_WORLD );

   //Term a = YAP_ReadBuffer("queens(S)",NULL);
   //YAP_RunGoal(a);
   //Yap_DebugPlWrite(a);

//enviar 

int *team_array = malloc(100*sizeof(int));

mpi_invalidar_alternativas(B, GLOBAL_root_cp,team_array);

CELL *buff = malloc((size_t) (end_global_copy - start_global_copy));
CELL *buff2 = malloc((size_t) (end_local_copy - start_local_copy));
CELL *buff3 = malloc((size_t) (end_trail_copy - start_trail_copy));

//printf("ENVIADO--- 1\n");
MPI_Send(team_array, 100, MPI_INT, worker_q, 10, MPI_COMM_WORLD );
//printf("ENVIADO---  2\n");

MPI_Send( start_global_copy, (size_t) (end_global_copy - start_global_copy), MPI_BYTE, worker_q, 5, MPI_COMM_WORLD );
//printf("ENVIADO---\n");
/*
MPI_Recv( buff, (size_t) end_global_copy-start_global_copy, MPI_BYTE, worker_q, 5, MPI_COMM_WORLD, &status );

printf("VOU VERIFICAR--- global\n");

CELL *buff_aux = buff;
CELL *mem      = start_global_copy;
int count = 0;

   while(mem < end_global_copy){
     if(*buff_aux != *mem)
        printf("FALHA !!! \n");
     mem++;
     buff_aux++;
     count ++;
   }
     
*/
//printf("FIM VERIFICAR--- global  \n");  

MPI_Send( start_local_copy, (size_t) (end_local_copy - start_local_copy), MPI_BYTE, worker_q, 6, MPI_COMM_WORLD );
/*MPI_Recv( buff2, (size_t) end_local_copy-start_local_copy, MPI_BYTE, worker_q, 6, MPI_COMM_WORLD, &status );

printf("VOU VERIFICAR--- local\n");

buff_aux = buff2;
mem      = start_local_copy;
count = 0;

   while(mem < end_local_copy){
     if(*buff_aux != *mem)
        printf("FALHA !!! 2\n");
     mem++;
     buff_aux++;
     count ++;
   }
     

printf("FIM VERIFICAR--- local  %d\n",count);  

*/
MPI_Send( start_trail_copy, (size_t) (end_trail_copy - start_trail_copy), MPI_BYTE, worker_q, 7, MPI_COMM_WORLD );
/*MPI_Recv( buff3, (size_t) end_trail_copy-start_trail_copy, MPI_BYTE, worker_q, 7, MPI_COMM_WORLD, &status );

printf("VOU VERIFICAR--- trail\n");

buff_aux = buff3;
mem      = start_trail_copy;
count = 0;

   while(mem < end_trail_copy){
     if(*buff_aux != *mem)
        printf("FALHA !!! 3\n");
     mem++;
     buff_aux++;
     count ++;
   }
     
*/
//printf("FIM VERIFICAR--- trail  \n"); 

/*
                                                   
memcpy((void *) (out_worker_offset(team_q,0) + start_global_copy),           
      (void *) start_global_copy,                                
      (size_t) (end_global_copy - start_global_copy));



                                                   
memcpy((void *) (out_worker_offset(team_q,0) + start_local_copy),            
      (void *) start_local_copy,                                 
      (size_t) (end_local_copy - start_local_copy));



                                             
memcpy((void *) (out_worker_offset(team_q,0) + start_trail_copy),            
      (void *) start_trail_copy,                                 
      (size_t) (end_trail_copy - start_trail_copy));

*/
  
/*

  REMOTE_TEAM(team_q,0)->optyap_data_.trail_copy.end = end_trail_copy;
  REMOTE_TEAM(team_q,0)->optyap_data_.trail_copy.start = start_trail_copy;

  REMOTE_TEAM(team_q,0)->optyap_data_.global_copy.end = end_global_copy;
  REMOTE_TEAM(team_q,0)->optyap_data_.global_copy.start = start_global_copy;

  REMOTE_TEAM(team_q,0)->optyap_data_.local_copy.end = end_local_copy;
  REMOTE_TEAM(team_q,0)->optyap_data_.local_copy.start = start_local_copy;

  REMOTE_TEAM(team_q,0)->optyap_data_.is_team_share = 1;


 



  invalidar_alternativas(B,end_local_copy,team_q);
  
*/  


  return 1;
}



#endif


