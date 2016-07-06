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

//#define INCREMENTAL_COPY 1
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
    //printf("ROOT %d\n",getpid());
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
  // printf("                                                                                 %d MAKE ROOT %p (%d,%d)\n",getpid(),B,comm_rank,worker_id);
  return;
}


void free_root_choice_point(void) {
 // B = LOCAL_top_cp->cp_b;
  B = LOCAL_top_cp;
  //printf(" %p                                                                                %d FREE ROOT %p (%d,%d)\n",B->cp_depth,getpid(),LOCAL_top_cp,comm_rank,worker_id);
/*  while(a != NULL && comm_rank==0){
  printf("------- %p \n",a);
  a = a->cp_b;
  }*/   
#ifdef TABLING
  LOCAL_top_cp_on_stack =
#endif /* TABLING */
    // YAPOR_MPI alterado
  LOCAL_top_cp = OrFr_node(GLOBAL_root_or_fr) = (choiceptr) LOCAL_LocalBase;
  //YAPOR_MPI
  return;
}


int p_share_work(void) {



  int worker_q = LOCAL_share_request;

//printf("INICIO P_SHARE (%d) -> (%d)    %d\n", worker_id, worker_q,LOCAL_delegate_share_area); 

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
 // printf("FIM P_SHARE (%d) -> (%d)    %d\n", worker_id,worker_q,LOCAL_delegate_share_area); 
  return 1;
}


int q_share_work(int worker_p) {
  register tr_fr_ptr aux_tr;
  register CELL aux_cell;
  
   //printf("Q_SHARE (%d) \n", worker_id,); 
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
  //if(REMOTE_delegate_share_area(worker_p) != MAX_WORKERS || LOCAL_delegate_share_area != MAX_WORKERS)
    //  printf("(%d) -> (%d) TEM PEDIDO DE DELEg %d - %d\n",worker_p,worker_id,REMOTE_delegate_share_area(worker_p),LOCAL_delegate_share_area);
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
#ifdef YAPOR_SPLIT
     OrFr_so(or_frame) = sharing_node->cp_so;
#endif
#ifdef YAPOR_MPI
      if(sharing_node->cp_ap == INVALIDWORK)
       OrFr_alternative(or_frame) = NULL;
      else
       OrFr_alternative(or_frame) = sharing_node->cp_ap;
#else
      OrFr_alternative(or_frame) = sharing_node->cp_ap;
#endif
      OrFr_pend_prune_cp(or_frame) = NULL;
      OrFr_nearest_leftnode(or_frame) = LOCAL_top_or_fr;
      OrFr_qg_solutions(or_frame) = NULL;
      //printf(" (%d) -> %d    ------  %p -- (%p)\n", worker_id, worker_q, sharing_node,or_frame);
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

static inline
void check_messages_delegations(){

 int number_teams;
 MPI_Comm_size(MPI_COMM_WORLD,&number_teams);

//sleep(100);

   int i;
   int msg[100];
    for(i = 0; i < GLOBAL_mpi_n_arenas; i++){
     //printf("%d M %d free %d\n",i,GLOBAL_mpi_delegate_messages(i),GLOBAL_mpi_delegate_is_free(i));
     int volatile delegate_message = GLOBAL_mpi_delegate_messages(i);
     //     printf(" %d  %d\n",i, delegate_message);
        if(!GLOBAL_mpi_delegate_is_free(i) && delegate_message){

   if(delegate_message == 1 ){
       msg[0] = 2 + GLOBAL_execution_counter;
       msg[1] = comm_rank;
       msg[2] = 0;
       //  printf("???1w %d                                               ENVIADO %d   free %d\n",GLOBAL_mpi_delegate_worker_p(i),i,GLOBAL_mpi_n_free_arenas);
       if(!GLOBAL_mpi_msg_ok_sent(i))
       MPI_Send(&msg, 5, MPI_INT, GLOBAL_mpi_delegate_worker_q(i), 44, MPI_COMM_WORLD);
//array-----
       long pos = GLOBAL_mpi_delegate_arena_start(i);
       int* load_team = (size_t) pos + (size_t) (7*sizeof(int));
       int* time_stamp = (size_t) pos + (size_t) (8*sizeof(int));

       *time_stamp = GLOBAL_time_stamp;

       *load_team  = 0;
/*       if(*load_team == 0){
         printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
         *load_team = 1;
       }*/
       GLOBAL_time_stamp++;
       pos = pos + (9*sizeof(int));
       memcpy((void *) pos, (void *) get_GLOBAL_mpi_load , (size_t) number_teams*sizeof(int));


       pos = pos +(number_teams*sizeof(int))+(1*sizeof(int));
       memcpy((void *) pos, (void *) get_GLOBAL_mpi_load_time, (size_t) number_teams*sizeof(int));

      GLOBAL_mpi_load(GLOBAL_mpi_delegate_worker_q(i)) = GLOBAL_mpi_delegate_new_load(i);
      GLOBAL_mpi_load_time(GLOBAL_mpi_delegate_worker_q(i)) = GLOBAL_mpi_delegate_time(i) + 1;
      //printf("MUDAR TIME STAMP %d para %d\n",GLOBAL_mpi_load_time(GLOBAL_mpi_delegate_worker_q(i)),GLOBAL_mpi_load(GLOBAL_mpi_delegate_worker_q(i)));
//array-------
       MPI_Send(GLOBAL_mpi_delegate_arena_start(i), GLOBAL_mpi_delegate_len(i), MPI_BYTE, GLOBAL_mpi_delegate_worker_q(i), 7, MPI_COMM_WORLD);
   } else if(delegate_message == 2 ){
	msg[0] = 3 + GLOBAL_execution_counter;
	msg[1] = comm_rank;
        msg[2] = 0;
        msg[3] = GLOBAL_time_stamp;
        GLOBAL_time_stamp++;
       memcpy((void *) &msg[4], (void *) get_GLOBAL_mpi_load , (size_t) number_teams*sizeof(int));
       memcpy((void *) &msg[4+number_teams], (void *) get_GLOBAL_mpi_load_time, (size_t) number_teams*sizeof(int));
       int size = number_teams*2+4;
	MPI_Send(&msg, size, MPI_INT, GLOBAL_mpi_delegate_worker_q(i), 44, MPI_COMM_WORLD);
        GLOBAL_share_count--;
   }
  if(delegate_message == 3 && !GLOBAL_mpi_msg_ok_sent(i)){
       msg[0] = 2 + GLOBAL_execution_counter;
       msg[1] = comm_rank;
       msg[2] = 0;
       //printf("???3w %d                                               ENVIADO %d   free %d  \n",GLOBAL_mpi_delegate_worker_p(i),i,GLOBAL_mpi_n_free_arenas);
       MPI_Send(&msg, 5, MPI_INT, GLOBAL_mpi_delegate_worker_q(i), 44, MPI_COMM_WORLD);
       GLOBAL_mpi_msg_ok_sent(i) = 1;
   } else  if (delegate_message != 3) {
        GLOBAL_mpi_msg_ok_sent(i) = 0;
	GLOBAL_mpi_n_free_arenas++;
        GLOBAL_mpi_delegate_is_free(i) =  1;             
        GLOBAL_mpi_delegate_worker_q(i) = MAX_WORKERS;                 
        GLOBAL_mpi_delegate_messages(i) = 0;                 
        GLOBAL_mpi_delegate_len(i) = 0;   
        BITMAP_delete(GLOBAL_mpi_delegated_workers ,GLOBAL_mpi_delegate_worker_p(i)); 
        GLOBAL_mpi_delegate_worker_p(i) = MAX_WORKERS;     
        //printf("\n\n   %d  %d \n",comm_rank, GLOBAL_mpi_delegated_workers ); 
   }
  }
 }

}

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>  
#include <math.h>


 int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
 { 
   //printf("%ld  %ld.............. \n",t2->tv_sec,t1->tv_sec);
   long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
   result->tv_sec = diff / 1000000;
   result->tv_usec = diff % 1000000;

   return (diff<0); 
 }

 void timeval_print(struct timeval *tv)
 {
   char buffer[30];
   time_t curtime;

   printf("%ld.%06ld", tv->tv_sec, tv->tv_usec);
   curtime = tv->tv_sec;
   strftime(buffer, 30, "%m-%d-%Y  %T", localtime(&curtime));
   printf(" = %s.%06ld\n", buffer, tv->tv_usec);
 }

 
int mpi_team_get_work(){

//printf("%d MPI GET WORK (%d,%d)\n",getpid(),comm_rank,worker_id);

PUT_IN_FINISHED(worker_id);

//GLOBAL_mpi_load(comm_rank) = 0;

int team_p = 0;

 int test;
 int i;
 int count = 0;
 int flag = 0;
 MPI_Status status;
 //int *msg = malloc(3*sizeof(int));
 int send_msg[100];
 send_msg[0] = 1;
 send_msg[1] = 1;
 send_msg[2] = 8;
 int recv_msg[100];
 int msg[5];
 MPI_Request *request;
 
 test =1;
 int waiting_for_response = 0;
 int number_teams;
 MPI_Comm_size(MPI_COMM_WORLD,&number_teams);
 GLOBAL_time_stamp++;

 if(number_teams == 1)
  return 0;

 /* if(GLOBAL_mpi_n_free_arenas < GLOBAL_mpi_n_arenas){
    printf("SIM SIM\n");
   for(i = 0; i < GLOBAL_mpi_n_arenas; i++){
    if(!GLOBAL_mpi_delegate_is_free(i))
      printf("(%d) %d WAAAAAITTTT   %d\n",comm_rank,i,GLOBAL_mpi_delegate_worker_p(i));
   }
   }*/


 //_________________________________________________________________________________________________________________
 //struct timeval tvBegin, tvEnd, tvDiff;

 // begin
 //gettimeofday(&tvBegin, NULL);
 //timeval_print(&tvBegin);
 //_____________________________________________________________________________________________________________________

//printf( "TEAM %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %d ---- %d\n",getpid(),comm_rank,team_id,worker_id,GLOBAL_mpi_n_free_arenas);
  while(GLOBAL_mpi_n_free_arenas < GLOBAL_mpi_n_arenas){
    check_messages_delegations();
}
//printf("(%d) DEPOIS \n",comm_rank);
//printf("depois %d MPI GET WORK (%d,%d)\n",getpid(),comm_rank,worker_id);
//printf("TEAM %d --------------------delay msg %d  count %d -----------------------------------------| %d  %d |\n", getpid(),GLOBAL_delay_msg,GLOBAL_delay_msg_count,GLOBAL_mpi_load(0),GLOBAL_mpi_load(1));

 while(1){
   MPI_Iprobe(MPI_ANY_SOURCE, 44, MPI_COMM_WORLD, &flag, &status );
   if(flag){
     MPI_Recv( &recv_msg, 100, MPI_INT, MPI_ANY_SOURCE, 44, MPI_COMM_WORLD, &status );
     if((recv_msg[0]/10)*10 != GLOBAL_execution_counter){   
       //       printf("REJEITAR  %d -- %d\n",recv_msg[0],GLOBAL_execution_counter);   
       recv_msg[0] = 10;
     } else 
     recv_msg[0] = recv_msg[0]%10;

     //   printf("%d GET WORK MENSAGEM: (%d)  %d   %d   %d\n",comm_rank,recv_msg[0],recv_msg[1],recv_msg[2],recv_msg[3]);
     GLOBAL_msg_count--;
     if(recv_msg[0] == 2){
       int load_p;
       LOCAL_delegate_share_area = 0;
       //if(mpi_team_q_share_work(recv_msg[1],&load_p)){
       if(mpi_delegate_team_q_share_work(recv_msg[1])){
         GLOBAL_time_stamp++;
         send_msg[0] = 4 + GLOBAL_execution_counter;
         send_msg[1] = comm_rank;
         send_msg[2] = LOCAL_load;
         send_msg[3] = GLOBAL_time_stamp;
         //send_msg[4] = recv_msg[1];
         //send_msg[5] = GLOBAL_mpi_load(recv_msg[1]);
//printf("%d NOTIFICAR %d  time %d\n",comm_rank,LOCAL_load,GLOBAL_time_stamp);
	 /*  for (i = 0; i < number_teams; i++)
	    if(i!=comm_rank && i!=recv_msg[1]){
	     MPI_Send(&send_msg, 4, MPI_INT, i, 44, MPI_COMM_WORLD);
             GLOBAL_msg_count++;
	     }*/
	 if(recv_msg[1] != 0)
	  MPI_Send(&send_msg, 4, MPI_INT, 0, 44, MPI_COMM_WORLD);   
        GLOBAL_time_stamp++;
        LOCAL_delegate_share_area = MAX_WORKERS;
        GLOBAL_share_count--;
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
     else if(recv_msg[0] == 1){
       send_msg[0] = 3 + GLOBAL_execution_counter;
       send_msg[1] = comm_rank;
       //send_msg[2] = LOCAL_load;
       send_msg[2] = -1;
       send_msg[3] = GLOBAL_time_stamp;
        for(i=0; i< number_teams; i++){
            if(comm_rank != i && recv_msg[3+number_teams+i] > GLOBAL_mpi_load_time(i)){
              GLOBAL_mpi_load_time(i) = recv_msg[3+number_teams+i];
              GLOBAL_mpi_load(i) = recv_msg[3+i]; 
              //printf("(%d) #%d# mudar load %d - %d\n",comm_rank,i,GLOBAL_mpi_load(i),GLOBAL_mpi_load_time(i));
            }
        }
	GLOBAL_mpi_load(recv_msg[1]) = -1;
	GLOBAL_mpi_load_time(recv_msg[1]) = recv_msg[2];
       //GLOBAL_time_stamp++;
//send load array
       //int joao[100];
       int ll = 0;
       //for(ll=0;ll<number_teams;ll++)
          //printf("%d (%d) ******* |%d| %d -- %d\n",GLOBAL_time_stamp,comm_rank,ll,GLOBAL_mpi_load(ll),GLOBAL_mpi_load_time(ll) );
       memcpy((void *) &send_msg[4], (void *) get_GLOBAL_mpi_load , (size_t) number_teams*sizeof(int));

       memcpy((void *) &send_msg[4+number_teams], (void *) get_GLOBAL_mpi_load_time, (size_t) number_teams*sizeof(int));
       int size = number_teams*2+4;

       MPI_Send(&send_msg, size, MPI_INT, recv_msg[1], 44, MPI_COMM_WORLD);
       GLOBAL_msg_count++;

//sleep(100);
     }
     else if(recv_msg[0] == 3){
        for(i=0; i< number_teams; i++){
            if(comm_rank != i && recv_msg[4+number_teams+i] > GLOBAL_mpi_load_time(i)){
              GLOBAL_mpi_load_time(i) = recv_msg[4+number_teams+i];
              GLOBAL_mpi_load(i) = recv_msg[4+i]; 
              //printf("(%d) #%d# mudar load %d - %d\n",comm_rank,i,GLOBAL_mpi_load(i),GLOBAL_mpi_load_time(i));
            }
        }
        GLOBAL_mpi_load(recv_msg[1]) = recv_msg[2];
        GLOBAL_mpi_load_time(recv_msg[1]) = recv_msg[3];   
        waiting_for_response = 0;
     }
     else if(recv_msg[0] == 4){
      GLOBAL_mpi_load(recv_msg[1]) = recv_msg[2];
      GLOBAL_mpi_load_time(recv_msg[1]) = recv_msg[3];
      //printf("load_time (%d) - %d\n",recv_msg[1],recv_msg[3]);
      //nao enviar load do que deu trabalho
      //GLOBAL_mpi_load(msg[4]) = msg[5];
      //GLOBAL_mpi_load_time(msg[4]) = msg[6];
     }
     else if(recv_msg[0] == 5){
       //printf("SHARE COUNT %d\n",GLOBAL_share_count);
       if(comm_rank == 0){
	 //         printf("(%d)  %d ---  %d\n",recv_msg[2],recv_msg[3]+GLOBAL_msg_count,GLOBAL_share_count+recv_msg[4]);
         waiting_for_response = 0;
         if(recv_msg[2] > 0){
           GLOBAL_mpi_load(recv_msg[1]) = recv_msg[2];
           //printf("LOADDDDDDDDDDDDDDDDDDDDDDDD ---- %d --- %d\n",recv_msg[1],comm_rank);
         }
	 // else if((recv_msg[3]+GLOBAL_msg_count) == 0){
	 else if((recv_msg[4]+GLOBAL_share_count)==0){
           for (i = 0; i < number_teams; i++) {
             if(i!=comm_rank){
                send_msg[0] = 6 + GLOBAL_execution_counter;
                send_msg[1] = comm_rank;
                MPI_Send(&send_msg, 5, MPI_INT, i, 44, MPI_COMM_WORLD);
             }   
           }
	   //printf("SAIR SAIR\n");
          return 0; 
         }        
       } else {
       for (i = 0; i < comm_rank; i++) {
          GLOBAL_mpi_load(i) = 0;
       }
       send_msg[0] = 5 + GLOBAL_execution_counter;
       send_msg[2] = 0;
       GLOBAL_msg_count++;
       send_msg[3] = recv_msg[3] + GLOBAL_msg_count;
       send_msg[4] = recv_msg[4] + GLOBAL_share_count;
       MPI_Send(&send_msg, 5, MPI_INT, (comm_rank+1)%number_teams, 44, MPI_COMM_WORLD);
       }
     }
     else if(recv_msg[0] == 6){ 
       //------------------------------------------------------------------------------------------------------------------------------
       //end
       // sleep(10);
       //    gettimeofday(&tvEnd, NULL);
       //timeval_print(&tvEnd);

       // diff
       //timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
       //printf("%d ---- %ld.%06ld----- %ld\n",comm_rank, tvDiff.tv_sec, tvDiff.tv_usec,tvDiff.tv_usec/1000);
       //-------------------------------------------------------------------------------------------------------------------------
        return 0;
     }
   }
   if(!waiting_for_response){
     //printf("(%d) RESPONSE  %d\n",comm_rank,GLOBAL_mpi_load(0));
     int load = -1;
     int rank = 0;
     int termination = 1;
     for (i = 0; i < number_teams; i++) {
       if(i != comm_rank && GLOBAL_mpi_load(i) > load){
       load = GLOBAL_mpi_load(i);
       rank = i;
       termination = 0;
       }
     }    


     if(!termination && rank != comm_rank){
       send_msg[0] = 1 + GLOBAL_execution_counter;
       send_msg[1] = comm_rank;
       send_msg[2] = GLOBAL_time_stamp;
       memcpy((void *) &send_msg[3], (void *) get_GLOBAL_mpi_load , (size_t) number_teams*sizeof(int));
       memcpy((void *) &send_msg[3+number_teams], (void *) get_GLOBAL_mpi_load_time, (size_t) number_teams*sizeof(int));
       int size = number_teams*2+4;
       MPI_Send(&send_msg, size, MPI_INT, rank, 44, MPI_COMM_WORLD);
       GLOBAL_msg_count++;
       waiting_for_response = 1;
     }
     
/*    if(waiting_for_response == 0 && comm_rank == 0){
       send_msg[0] = 5;
       send_msg[1] = 0;
       GLOBAL_msg_count++;
       send_msg[3] = 0;
       send_msg[4] = 0;
       MPI_Send(&send_msg, 5, MPI_INT, 1, 44, MPI_COMM_WORLD);
       waiting_for_response = 1;
    }*/

     //if(waiting_for_response == 0 && comm_rank == 0){
    if(termination){
       int ll;
       //     for(ll=0;ll<number_teams;ll++)
       //           printf("SEND 6 (%d)** %d  |%d| -- %d\n",comm_rank,ll,GLOBAL_mpi_load(ll),GLOBAL_mpi_load_time(ll));
       for (i = 0; i < number_teams; i++) {
             if(i!=comm_rank){
                send_msg[0] = 6 + GLOBAL_execution_counter;
                send_msg[1] = comm_rank;
                MPI_Send(&send_msg, 5, MPI_INT, i, 44, MPI_COMM_WORLD);
             }
       }
       //------------------------------------------------------------------------------------------------------------------------------                                                                             
       //end                                                                                                                                                                                                        
       //gettimeofday(&tvEnd, NULL);
       //timeval_print(&tvEnd);

       // diff                                                                                                                                                                                                      
       //timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
       //printf("***%d ---- %ld.%06ld---%ld\n",comm_rank, tvDiff.tv_sec, tvDiff.tv_usec,tvDiff.tv_usec/1000);
       //-------------------------------------------------------------------------------------------------------------------------   
       return 0;
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

    //    printf("(%d) %p  ----   %p ----------------------B = %p \n",i,GLOBAL_team_array(team_q,i),b->cp_ap,b);

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

/*
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



  printf("(%d)		LTT %d   		%d\n",i,YAMOP_LTT(b->cp_b->cp_ap),b->cp_b->cp_or_fr);

  b=b->cp_b;
  i++;   
  }


if(root->cp_ap!=GETWORK)
  printf("INV ERRO !!!  %p --- %p \n",root->cp_ap,team_array[1]);

team_array[0] = i;
printf("-> -> -> %d  %d    %d\n",count,i,B->cp_depth);


}
*/

int mpi_invalidar_alternativas(choiceptr b, int share, int i, int* team_array){

choiceptr aux = NULL;
int lub = 0;
int total_lub = 0;
int j = i+1;

//printf("(%d)		TTTT\n",i);

if(b == GLOBAL_root_cp){
 team_array[0] = i;
 return lub;
}


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
        //count++;
      }                     
      UNLOCK_OR_FRAME(b->cp_or_fr);     
    }else {
     aux = b->cp_ap;
     b->cp_ap = INVALIDWORK;
     share = 0;
     //count++;                                              
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

 //printf("(%d) %p  ----   %p ----------------------  %p  %p \n",i,team_array[i],b->cp_ap,INVALIDWORK,GETWORK);

  //printf("(%d)		\n",i);

  
  total_lub = mpi_invalidar_alternativas(b->cp_b, share, j, team_array);
   //printf("(%d)		total %d\n",i,total_lub);
  if(b->cp_ap != GETWORK){
    b->cp_or_fr = total_lub;
    if(b->cp_ap != INVALIDWORK)
    lub = YAMOP_LTT(b->cp_ap);
  }

  //printf("(%d)		LTT %d   	lub %d	%d\n",i,YAMOP_LTT(b->cp_ap),lub,b->cp_or_fr);

  return total_lub + lub;
}

/*
void mpi_install_array(choiceptr b, choiceptr root, int * team_array){
int i = 1;

if(root->cp_ap!=GETWORK)
  printf("%d ANT INV ERRO !!!  %p --- \n",team_id,root->cp_ap);



 while(b != root){
 b->cp_ap = team_array[i];
 b=b->cp_b;
 i++;   
 }

if(root->cp_ap!=GETWORK)
  printf("%d 2 INV ERRO !!!  %p --- \n",team_id,root->cp_ap);

}

*/

int mpi_install_array(choiceptr b, int i, int * team_array){
int total_lub = 0;
int lub = 0;
int j = 0;

if(b == GLOBAL_root_cp)
 return lub;



 b->cp_ap = team_array[i];

 j = i + 1;
 total_lub = mpi_install_array( b->cp_b, j,team_array);

 b->cp_or_fr = total_lub;
 
 if(b->cp_ap != INVALIDWORK)
    lub = YAMOP_LTT(b->cp_ap);

 printf("----(%d)		 %p   	lub %d	%d\n",i,b->cp_ap,lub,b->cp_or_fr);

 return total_lub + lub;
}



int mpi_team_q_share_work(int worker_p, int *load_p){

//printf("%d ---------------MENSAGEM QQQ ----  %p   \n",getpid(),comm_rank);  

CELL *buff = GLOBAL_buff;
int *team_array = GLOBAL_team_array2;

int msg[6];
 MPI_Status status;
//CELL *buff = malloc(Yap_worker_area_size);
int pos = 0;

//printf("Q  1 %p  %p\n",GLOBAL_buff,GLOBAL_team_array2);
MPI_Recv( buff, Yap_worker_area_size, MPI_PACKED, worker_p, 7, MPI_COMM_WORLD, &status );
//printf("Q  2\n");

//MPI_Recv( &msg, 6, MPI_INT, worker_p, 44, MPI_COMM_WORLD, &status );

MPI_Unpack( buff, Yap_worker_area_size, &pos, msg, 6, MPI_INT, MPI_COMM_WORLD);


 CELL start_global_copy = msg[0];                    
 CELL end_global_copy   = msg[1];                      
 CELL start_local_copy  = msg[2];                      
 CELL end_local_copy    = msg[3];           
 CELL start_trail_copy  = msg[4];    
 CELL end_trail_copy    = msg[5]; 

/*
int i;
while( i < 6){ 
 printf("P | %p|\n",msg[i]);
 i++;
}
*/

//receber
//int *team_array = malloc(100*sizeof(int));
//MPI_Recv(team_array, 100, MPI_INT, worker_p, 10, MPI_COMM_WORLD,&status );
MPI_Unpack( buff, Yap_worker_area_size, &pos, team_array, 1000, MPI_INT, MPI_COMM_WORLD);
//printf("RECEBIDO---1\n");

//MPI_Recv( start_global_copy, (size_t) end_global_copy-start_global_copy, MPI_BYTE, worker_p, 5, MPI_COMM_WORLD, &status );
//printf("RECEBIDO---\n");
//MPI_Send( start_global_copy, (size_t) end_global_copy-start_global_copy, MPI_BYTE, worker_p, 5, MPI_COMM_WORLD );

MPI_Unpack( buff, Yap_worker_area_size, &pos, start_global_copy, (size_t) end_global_copy-start_global_copy, MPI_BYTE, MPI_COMM_WORLD);


//MPI_Recv( start_local_copy, (size_t) end_local_copy-start_local_copy, MPI_BYTE, worker_p, 6, MPI_COMM_WORLD, &status );
//MPI_Send( start_local_copy,(size_t) end_local_copy-start_local_copy, MPI_BYTE, worker_p, 6, MPI_COMM_WORLD );

MPI_Unpack( buff, Yap_worker_area_size, &pos, start_local_copy, (size_t) end_local_copy-start_local_copy, MPI_BYTE, MPI_COMM_WORLD);

//MPI_Recv(  start_trail_copy, (size_t) end_trail_copy-start_trail_copy, MPI_BYTE, worker_p, 7, MPI_COMM_WORLD, &status );
//MPI_Send( start_trail_copy,(size_t) end_trail_copy-start_trail_copy, MPI_BYTE, worker_p, 7, MPI_COMM_WORLD );

MPI_Unpack( buff, Yap_worker_area_size, &pos, start_trail_copy, (size_t) end_trail_copy-start_trail_copy, MPI_BYTE, MPI_COMM_WORLD);

//mpi_install_array(start_local_copy, GLOBAL_root_cp, team_array);
 printf("Q começar a copia aqui %p  %p\n",start_local_copy,end_local_copy);

int load = mpi_install_array(start_local_copy, 1, team_array);
LOCAL_load = load; 

*load_p = team_array[0];
//printf("load_p %d --- %d\n",*load_p,LOCAL_load);

//sleep(100);
  //choiceptr a = (choiceptr) LOCAL_start_local_copy;
 // printf("RECEBIDO--- %p %p\n",a, a->cp_tr);
  //install_array(LOCAL_start_local_copy,LOCAL_end_local_copy);
 GLOBAL_root_cp = end_local_copy;
 
 LOCAL_top_or_fr = GLOBAL_root_or_fr;
 TR = (tr_fr_ptr) end_trail_copy;
 B = start_local_copy;
 LOCAL_is_team_share = 1;

 //printf("FIM-----------TEAM-MENSAGEM QQQ ----  %p   %d\n",comm_rank,LOCAL_is_team_share);  
//free(buff);
//free(team_array);
  return TRUE;

}


int mpi_team_p_share_work(int worker_q){

//printf("%d ---------------MENSAGEM PPP ----  %d  %p %p\n",getpid(),worker_q,GLOBAL_buff,GLOBAL_team_array2);  

//CELL *buff = malloc(Yap_worker_area_size);
//int *team_array = malloc(1000*sizeof(int));


CELL *buff = GLOBAL_buff;
int *team_array = GLOBAL_team_array2;

MPI_Request request;

  int copy_info[6];
  MPI_Status status;
  int pos = 0;
  /* sharing request accepted */

  CELL start_global_copy = copy_info[0] = (CELL) (H0);                       
  CELL end_global_copy   = copy_info[1] = (CELL) (HR);                        
  CELL start_local_copy  = copy_info[2] = (CELL) (B);                        
  CELL end_local_copy    = copy_info[3] = (CELL) (GLOBAL_root_cp);           
  CELL start_trail_copy  = copy_info[4] = (CELL) (GLOBAL_root_cp->cp_tr);    
  CELL end_trail_copy    = copy_info[5] = (CELL) (TR);

  printf("start %p end %p\n",start_local_copy, end_local_copy);

/*
int i;
while( i < 6){
 printf("Q | %p|\n",copy_info[i]);
 i++;
}
*/

MPI_Pack(copy_info, 6, MPI_INT, buff, Yap_worker_area_size, &pos, MPI_COMM_WORLD);
//MPI_Send( &copy_info, 6, MPI_INT, worker_q, 44, MPI_COMM_WORLD );

//printf("%d  %d  pos %d \n",getpid(),worker_q,pos);  

//enviar 




int load = mpi_invalidar_alternativas(B, 0, 1, team_array);
//printf("LOAD LOAD LOAD  %d %d\n",load,LOCAL_load);
team_array[0]=LOCAL_load =load;
//mpi_invalidar_alternativas(B, GLOBAL_root_cp,team_array);

MPI_Pack(team_array, 1000, MPI_INT, buff, Yap_worker_area_size, &pos, MPI_COMM_WORLD);

/*
CELL *buff = malloc((size_t) (end_global_copy - start_global_copy));
CELL *buff2 = malloc((size_t) (end_local_copy - start_local_copy));
CELL *buff3 = malloc((size_t) (end_trail_copy - start_trail_copy));
*/

//MPI_Send(team_array, 100, MPI_INT, worker_q, 10, MPI_COMM_WORLD );

//MPI_Send( start_global_copy, (size_t) (end_global_copy - start_global_copy), MPI_BYTE, worker_q, 5, MPI_COMM_WORLD );
MPI_Pack(start_global_copy, (size_t) (end_global_copy - start_global_copy), MPI_BYTE, buff, Yap_worker_area_size, &pos, MPI_COMM_WORLD);

//MPI_Send( start_local_copy, (size_t) (end_local_copy - start_local_copy), MPI_BYTE, worker_q, 6, MPI_COMM_WORLD );
MPI_Pack(start_local_copy, (size_t) (end_local_copy - start_local_copy), MPI_BYTE, buff, Yap_worker_area_size, &pos, MPI_COMM_WORLD);

//MPI_Send( start_trail_copy, (size_t) (end_trail_copy - start_trail_copy), MPI_BYTE, worker_q, 7, MPI_COMM_WORLD );
MPI_Pack(start_trail_copy, (size_t) (end_trail_copy - start_trail_copy), MPI_BYTE, buff, Yap_worker_area_size, &pos, MPI_COMM_WORLD);

//printf("P  1%p  %p\n",GLOBAL_buff,GLOBAL_team_array2);
MPI_Send(buff, pos, MPI_PACKED, worker_q, 7, MPI_COMM_WORLD );
//MPI_Isend(buff, pos, MPI_PACKED, worker_q, 7, MPI_COMM_WORLD, &request);
//printf("P  2\n");
//free(buff);
//free(team_array);
  return 1;
}

#ifdef YAPOR_SPLIT
static inline
yamop *SCH_retrieve_alternative(yamop *ap, int offset){

  if(ap == NULL) return INVALIDWORK;
//printf("AQUI %p\n",ap);
//printf("A %p LTT %d  -> ->%d                 %p   %p\n",ap,3,offset,INVALIDWORK,GETWORK);

  int ap_left = YAMOP_LTT(ap);


  //printf("B %p LTT %d  -> ->%d                 %p   %p\n",ap,YAMOP_LTT(ap),offset,INVALIDWORK,GETWORK);

  if(offset >= ap_left) return INVALIDWORK;
    while(offset > 0){
      ap = NEXTOP(ap,Otapl);
      offset--;
    }

    if (YAMOP_LTT(ap)> 200)
      return INVALIDWORK; 


  return ap;
}

 void mpi_invalidar_alternativas2(choiceptr b, choiceptr base, int worker, int* b_lub, int* aux_lub){



   if(b == GLOBAL_root_cp){
     *b_lub = 0;
     *aux_lub = 0;
     return;
   }

   int ltt=0;
   //choiceptr aux = base + ( b - B);

   choiceptr aux = (size_t)base + ((size_t) b - (size_t) B);

   //printf("INVALIDAR  %p -- %p   ap  %p ---- %p\n",b ,GLOBAL_root_cp,b->cp_ap,aux->cp_ap);
   //printf("____  %p -- %p   \n",b->cp_b,aux->cp_b);


   //printf("INVALIDAR --------------\n");
   //printf("INVALIDAR  %p -- %p   \n",b ,b->cp_ap);
   yamop* alternative;

   // printf("                                                                     %p  -------------- %p\n",b->cp_ap,aux->cp_ap);

   if( b->cp_ap == GETWORK){
     
     LOCK_OR_FRAME(b->cp_or_fr);
     if(OrFr_alternative(b->cp_or_fr) != NULL){
       ltt = YAMOP_LTT(OrFr_alternative(b->cp_or_fr));
       alternative =  SCH_retrieve_alternative(OrFr_alternative(b->cp_or_fr),OrFr_so(b->cp_or_fr));
       //       if (alternative != INVALIDWORK){
	 aux->cp_so = OrFr_so(b->cp_or_fr)*2;
	 OrFr_so(b->cp_or_fr) =  OrFr_so(b->cp_or_fr) * 2;
	 //}
       
       if (worker){
	 aux->cp_ap = OrFr_alternative(b->cp_or_fr);
	 if (alternative == INVALIDWORK)
	   OrFr_alternative(b->cp_or_fr) = NULL;
	 else
	   OrFr_alternative(b->cp_or_fr) = alternative;
	 worker = 0;
       }else {
	 aux->cp_ap = alternative;
	 worker = 1;
       }
     } else
       aux->cp_ap = INVALIDWORK;
     UNLOCK_OR_FRAME(b->cp_or_fr);
   } else {
     if(b->cp_ap != INVALIDWORK){
       ltt = YAMOP_LTT(b->cp_ap);
       //printf("A INVALIDAR --------------\n");
       alternative = SCH_retrieve_alternative(aux->cp_ap,aux->cp_so);
       //printf("%p  -------------- %p      %p\n",b->cp_ap,aux->cp_ap,alternative);
       if (alternative != INVALIDWORK){
	 aux->cp_so = b->cp_so*2;
	 b->cp_so = b->cp_so * 2;
       }
       if (worker){
	 b->cp_ap = alternative;
	 worker = 0;
       }else {
	 aux->cp_ap = alternative;
	 worker = 1;
       }//printf("%p  -------------- %p\n",b->cp_ap,aux->cp_ap);
       
     }
   }

   int b_lub_next;
   int aux_lub_next;
   int lub = 0;






   mpi_invalidar_alternativas2(b->cp_b, base, worker, &b_lub_next, &aux_lub_next);

   //printf("(%p)   %d\n",b->cp_ap,worker);
   if(b->cp_ap != GETWORK){
     b->cp_or_fr = b_lub_next;
     //printf("(%p) A \n",b->cp_ap);
     if(b->cp_ap != INVALIDWORK) {
       //printf("(%p) B \n",b->cp_ap);
       lub = ltt/b->cp_so;
       if(!worker)
	 lub =  lub + ltt%b->cp_so; 
     }
   }



   *b_lub = b_lub_next + lub;
   //   printf("A %d\n",*b_lub);
   lub = 0;

   //printf("(%p)\n",aux->cp_ap);
   if(aux->cp_ap != GETWORK){
     //printf("(%p) A \n",aux->cp_ap);
     aux->cp_or_fr = aux_lub_next;
     if(aux->cp_ap != INVALIDWORK){
       //printf("(%p) B \n",aux->cp_ap);
       lub = ltt/b->cp_so;
       if(worker)
	 lub =  lub + ltt%b->cp_so;
     }
   }

   *aux_lub = aux_lub_next + lub;
   //printf("B %d\n",*aux_lub);
  

   return;
 }


#else

void mpi_invalidar_alternativas2(choiceptr b, choiceptr base, int share, int* b_lub, int* aux_lub){



if(b == GLOBAL_root_cp){
 *b_lub = 0;
 *aux_lub = 0;
//if(comm_rank == 0)
 //printf("##############################\n");
 return;
}


//choiceptr aux = (size_t)base + ((size_t) b - (size_t) B);

choiceptr aux = base + ( b - B);

//printf(" %d --------  %d       %d\n", b, aux , offset);

//printf(" %p --------  %p       %p ----- %p\n", b->cp_ap, aux->cp_ap,b->cp_b, aux->cp_b);

//printf("(%d,%d) A %p -------- %p\n", comm_rank,worker_id, b, GLOBAL_root_cp);


if(share){
    if(b->cp_ap != INVALIDWORK){
        if(b->cp_ap == GETWORK){
	  if(OrFr_alternative(b->cp_or_fr) == NULL){
            LOCK_OR_FRAME(b->cp_or_fr);
            if(OrFr_alternative(b->cp_or_fr) == NULL){
                aux->cp_ap = INVALIDWORK;
            } else {
                aux->cp_ap = OrFr_alternative(b->cp_or_fr);
                OrFr_alternative(b->cp_or_fr) = NULL;
                share = 0;
                //count++;
            }
            UNLOCK_OR_FRAME(b->cp_or_fr);
	  }
        }else {
            aux->cp_ap = b->cp_ap;
            b->cp_ap = INVALIDWORK;
            share = 0;
            //count++;
        }
    } else {
        aux->cp_ap = INVALIDWORK;
    }
} else{
    if(b->cp_ap != INVALIDWORK || (b->cp_ap == GETWORK && OrFr_alternative(b->cp_or_fr) != NULL))
    share = 1;
    aux->cp_ap = INVALIDWORK;
}
//printf("%p   (%d) %p     %p -------- %p   %d\n",B, comm_rank, b,aux->cp_ap, b->cp_ap,YAMOP_LTT(b->cp_ap));
/*
if(comm_rank == 0)
if(b->cp_ap == GETWORK)
printf("   (%d) %p  %p -------- %p (%p)  %p\n", comm_rank, b,aux->cp_ap, b->cp_ap,OrFr_alternative(b->cp_or_fr),GLOBAL_root_cp);

if(share){
    if(b->cp_ap != INVALIDWORK){
        if(b->cp_ap == GETWORK){
                aux->cp_ap = INVALIDWORK;
        }else {
            aux->cp_ap = b->cp_ap;
            b->cp_ap = INVALIDWORK;
            share = 0;
            //count++;
        }
    } else {
        aux->cp_ap = INVALIDWORK;
    }
} else{
    if(b->cp_ap != INVALIDWORK || (b->cp_ap == GETWORK && OrFr_alternative(b->cp_or_fr) != NULL))
    share = 1;
    aux->cp_ap = INVALIDWORK;
}
*/

/*
if(comm_rank == 0){
if(b->cp_ap == GETWORK)
printf("***(%d) %p  %p -------- %p (%p)  %p\n", comm_rank, b,aux->cp_ap, b->cp_ap,OrFr_alternative(b->cp_or_fr),GLOBAL_root_cp);
else
printf("(%d) B %p   %p-------- %p\n", comm_rank, b, aux->cp_ap, b->cp_ap);
}
*/

 int b_lub_next;
 int aux_lub_next;
 int lub = 0;

 mpi_invalidar_alternativas2(b->cp_b, base, share, &b_lub_next, &aux_lub_next);

 
  if(b->cp_ap != GETWORK){
    b->cp_or_fr = b_lub_next;
    if(b->cp_ap != INVALIDWORK)
    lub = YAMOP_LTT(b->cp_ap);
  }

  *b_lub = b_lub_next + lub;
  lub = 0;

  if(aux->cp_ap != GETWORK){
    aux->cp_or_fr = aux_lub_next;
    if(aux->cp_ap != INVALIDWORK)
    lub = YAMOP_LTT(b->cp_ap);
  }

  *aux_lub = aux_lub_next + lub;

  //printf(" %p -------- %p \n", b->cp_ap, aux->cp_ap);
 // printf("(%d,%d)  LTT    %d  	%d\n",comm_rank,worker_id,b_lub_next,aux_lub_next);

  return;
}

#endif

 int mpi_delegate_team_p_share_work(){

   //printf("%d INICO (%d,%d) P DELEGADO  %d   %p\n",getpid(),comm_rank,worker_id,LOCAL_delegate_share_area,GLOBAL_mpi_delegate_arena_start(LOCAL_delegate_share_area));

 // printf("%d ---------------MENSAGEM PPP ----  %d  \n",getpid(),worker_id);  

   int copy_info[6];
   int area_to_copy = LOCAL_delegate_share_area;
   //LOCAL_delegate_share_area;


   CELL start_global_copy = copy_info[0] = (CELL) (H0);                       
   CELL end_global_copy   = copy_info[1] = (CELL) (HR);                        
   CELL start_local_copy  = copy_info[2] = (CELL) (B);                        
   CELL end_local_copy    = copy_info[3] = (CELL) (GLOBAL_root_cp);           
   CELL start_trail_copy  = copy_info[4] = (CELL) (GLOBAL_root_cp->cp_tr);    
   CELL end_trail_copy    = copy_info[5] = (CELL) (TR);

/*int i;
 for(i=0;i<6;i++)
  printf("A [%d] --- %d\n",i,copy_info[i]);
*/
   //MPI_Pack(copy_info, 6, MPI_INT, buff, Yap_worker_area_size, &pos, MPI_COMM_WORLD);
   


   //team_array[0]=LOCAL_load =load;
  
   long pos = GLOBAL_mpi_delegate_arena_start(area_to_copy);



int* worker_q_load = (size_t) pos + (size_t) (6*sizeof(int));

//printf("1 ---------------MENSAGEM PPP ----  %d    %p\n",getpid(),pos);  

        memcpy((void *) pos,           
               (void *) copy_info,                                
               (size_t) 7*sizeof(int));

pos = pos + (9*sizeof(int))+((GLOBAL_mpi_n_teams*2)*sizeof(int))+sizeof(int);
                                            
//printf("2 ---------------MENSAGEM PPP ----  %d   %p \n",getpid(),pos);  
          
        memcpy((void *) pos,           
               (void *) start_global_copy,                                
               (size_t) (end_global_copy - start_global_copy));

pos = pos + (end_global_copy - start_global_copy) +1;
                                       
//printf("3 ---------------MENSAGEM PPP ----  %d  %p\n",getpid(),pos);                  
        memcpy((void *) pos,           
               (void *) start_trail_copy,                                
               (size_t) (end_trail_copy - start_trail_copy));

pos = pos + (end_trail_copy - start_trail_copy) +1;
long local_base = pos;
                                                  
        memcpy((void *) pos,           
               (void *) start_local_copy,                                
               (size_t) (end_local_copy - start_local_copy));

//printf("4 ---------------MENSAGEM PPP ----  %d  %p\n",getpid(),pos);  

pos = pos + (end_local_copy - start_local_copy);





int b_lub;
int aux_lub;

mpi_invalidar_alternativas2(B, local_base, 1, &b_lub, &aux_lub);

   GLOBAL_mpi_delegate_len(area_to_copy) = pos - GLOBAL_mpi_delegate_arena_start(area_to_copy);


LOCAL_load = b_lub;
GLOBAL_mpi_delegate_new_load(area_to_copy) = *worker_q_load = aux_lub;
//printf("%d (%d) ENVIEI PARA O ( %d LOAD %d TIME %d )---- MY_LOAD %d   %p\n",GLOBAL_time_stamp,comm_rank,GLOBAL_mpi_delegate_worker_q(area_to_copy),aux_lub,GLOBAL_mpi_load_time(GLOBAL_mpi_delegate_worker_q(area_to_copy)),b_lub,B->cp_ap);
//printf("\n\n P %d LOADS %d %d \n %d %d (%d)\n",comm_rank,*my_load,*worker_q_load,GLOBAL_mpi_load(0),GLOBAL_mpi_load(1),GLOBAL_mpi_delegate_worker_q(area_to_copy));


   //printf("FIM (%d,%d) P DELEGADO  %d  \n",comm_rank,worker_id,area_to_copy);
   //MPI_Send(buff, pos, MPI_PACKED, worker_q, 7, MPI_COMM_WORLD );
   //MPI_Isend(buff, pos, MPI_PACKED, worker_q, 7, MPI_COMM_WORLD, &request);
   //printf("P  2\n");
   //free(buff);
   //free(team_array);
   return 1;
 }


int mpi_delegate_team_q_share_work(int worker_p){

  //printf("%d ---------------MENSAGEM QQQ ----  %d   \n",getpid(),worker_p);  


int msg[9];
MPI_Status status;
long pos;

CELL* buffer = GLOBAL_mpi_delegate_arena_start(LOCAL_delegate_share_area); 

MPI_Recv( buffer, Yap_worker_area_size, MPI_BYTE, worker_p, 7, MPI_COMM_WORLD, &status );

pos = buffer;

//printf("4 ---------------MENSAGEM QQQ ----  %d  \n",getpid());  

        memcpy((void *) msg, 
               (void *) pos,                                          
               (size_t) 9*sizeof(int));

pos = pos + (9*sizeof(int));


//int h;
// for(h=0;h<8;h++)
//  printf("B [%d] --- %p\n",h,msg[h]);


 CELL start_global_copy = msg[0];                    
 CELL end_global_copy   = msg[1];                      
 CELL start_local_copy  = msg[2];                      
 CELL end_local_copy    = msg[3];           
 CELL start_trail_copy  = msg[4];    
 CELL end_trail_copy    = msg[5]; 

// GLOBAL_mpi_load(worker_p) = msg[6];
 LOCAL_load = msg[6];

//printf(" (%d) <- (%d) LOADS %d  \n",comm_rank,worker_p,msg[6]);




int* load_time = pos + (GLOBAL_mpi_n_teams*sizeof(int))+(1*sizeof(int));
int* load = pos;

//printf("LOAD %d  LOAD_TIME %d\n",*load,*load_time);

/*
int i=0;
while(i < GLOBAL_mpi_n_teams){
printf("** %d  %d\n",load[i],load_time[i]);

i++;
}*/

int i;
        for(i=0; i< GLOBAL_mpi_n_teams; i++){
            //printf("(%d) %d -- %d\n",comm_rank,load_time[i],load[i]);
            if(comm_rank != i && load_time[i] > GLOBAL_mpi_load_time(i)){
              GLOBAL_mpi_load_time(i) = load_time[i];
              GLOBAL_mpi_load(i) = load[i]; 
            }
        }

//printf("%d A LOAD %d  LOAD_TIME %d\n",comm_rank,GLOBAL_mpi_load(worker_p),GLOBAL_mpi_load_time(worker_p));

GLOBAL_mpi_load(worker_p) = msg[7];
GLOBAL_mpi_load_time(worker_p) = msg[8];

//printf("%d B LOCAL %d LOAD %d  LOAD_TIME %d\n",comm_rank,msg[6],msg[7],msg[8]);

pos = pos + ((GLOBAL_mpi_n_teams*2)*sizeof(int))+sizeof(int);
//printf("1 ---------------MENSAGEM QQQ ----  %d  \n",getpid());  
          
        memcpy((void *) start_global_copy,
               (void *) pos,                                
               (size_t) (end_global_copy - start_global_copy));

pos = pos + (end_global_copy - start_global_copy) +1;
                                       
//printf("3 ---------------MENSAGEM QQQ ----  %d  \n",getpid());                  
        memcpy((void *) start_trail_copy,   
               (void *) pos,                                        
               (size_t) (end_trail_copy - start_trail_copy));

pos = pos + (end_trail_copy - start_trail_copy) +1;
//int local_base = pos;
                                                  
        memcpy((void *) start_local_copy,  
               (void *) pos,                                         
               (size_t) (end_local_copy - start_local_copy));

//printf("2 ---------------MENSAGEM QQQ ----  %d   %d\n",getpid(),LOCAL_load);  

//pos = pos + (end_local_copy - start_local_copy);




 GLOBAL_root_cp = end_local_copy;
 LOCAL_top_or_fr = GLOBAL_root_or_fr;
 TR = (tr_fr_ptr) end_trail_copy;
 B = start_local_copy;
 LOCAL_is_team_share = 1;
 //printf("%d                                                                                                  RECEBI (%d) <- (%d) \n",msg[8],comm_rank,worker_p);  
  return TRUE;

}


#endif


