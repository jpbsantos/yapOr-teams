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
 #include "mpi.h" 

char* 
term2string(char** ptr, size_t* size, const YAP_Term t) {
  int missing = YAP_ExportTerm(t, *ptr, *size);
  while(missing < 1){
   // expand_buffer(ptr, size);
    missing = YAP_ExportTerm(t, *ptr, *size);
  }
  return *ptr;
}

/* -------------------------------- **
**      Scheduler instructions      **
** -------------------------------- */

PBOp(getwork_first_time,e)
    /* wait for a new parallel goal */
printf( "FIRST TIME %d ####################################### (%d,%d) ####################################### %p\n",getpid(),comm_rank,worker_id,B);
LOCK(GLOBAL_locks_bm_present_team_workers);
BITMAP_insert(GLOBAL_bm_present_team_workers, worker_id);
UNLOCK(GLOBAL_locks_bm_present_team_workers);
    //running -------------------------------------
if(worker_id == 0){
  while (!BITMAP_same(GLOBAL_bm_present_workers,GLOBAL_bm_present_team_workers));
  MPI_Barrier(MPI_COMM_WORLD);
  if( comm_rank == 0) {
    Term goal;
    if(GLOBAL_mpi_active){
      //char* s;
      char s[300];
      //s = malloc(300*sizeof(char));
      MPI_Comm parent; 
      MPI_Comm_get_parent(&parent); 
      int a = 0;
      MPI_Send(&a, 1, MPI_INT, 0, 21, parent);

      MPI_Recv(s,300,MPI_CHAR,0,0,parent,MPI_STATUS_IGNORE);
      //int a;
      //MPI_Recv(&a,1,MPI_INT,0,0,parent,MPI_STATUS_IGNORE);
      int i;
      for(i=0; i<300; i++)
         printf("%c",s[i]);
      printf("\n---dePOIs\n");
//  Term bb = YAP_ReadBuffer("queens(S)",NULL);
//  YAP_RunGoal(bb);
//Yap_StartSlots( PASS_REGS1 );
  
      goal = YAP_ImportTerm(s);
//  Term bb = YAP_ReadBuffer("queens(S)",NULL);
//  YAP_RunGoal(bb);
  
      Yap_DebugPlWrite(goal);
      //while(!BITMAP_same(COMM_bm_present_teams,COMM_bm_running_teams));
     // MPI_Barrier(MPI_COMM_WORLD);
    }
    else{
      LOCK(COMM_locks_bm_running_teams);
      BITMAP_insert(COMM_bm_running_teams, comm_rank);
      UNLOCK(COMM_locks_bm_running_teams); 
      while (!GLOBAL_flag(team_id));
      GLOBAL_flag(team_id) = 0;
      Term t = GLOBAL_data_base_key(team_id);
      goal = YAP_Recorded(t);
      GLOBAL_data_base_key(team_id) = NULL;
      while(!BITMAP_same(COMM_bm_present_teams,COMM_bm_running_teams));
    }  
    printf("BEFORE RUN GOAL  %p\n",B);    
    YAP_RunGoal(goal);
  }
  else {
    if(GLOBAL_mpi_active){
      //sleep(100);
      printf(" ANTES DO BCAST DA TRILHA\n");
      int cp_tr[3];
      MPI_Bcast(&cp_tr, 3, MPI_INT, 0, MPI_COMM_WORLD);
      GLOBAL_mpi_load(0) = 1;
      printf("------------------------ DEPOIS DO BCAST DA TRILHA\n");
      GLOBAL_root_cp = cp_tr[0];
      GLOBAL_root_cp->cp_tr = cp_tr[1];
      R_COMM_query_has_results(comm_rank) = cp_tr[2];
      make_root_choice_point();
//printf( "AAAA %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %p\n",getpid(),comm_rank,team_id,GLOBAL_root_cp->cp_tr);
       //Term bb = YAP_ReadBuffer("queens(S)",NULL);
       //YAP_RunGoal(bb);
//printf( "AAAA %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %p\n",getpid(),comm_rank,team_id,GLOBAL_root_cp->cp_tr);
      if(mpi_team_get_work()){
        printf("SHARED FAIL %d\n",getpid());
        goto shared_fail;
      }
      else{
        goto shared_end;      
      }
//printf( "SAHRED FAIL %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %d\n",getpid(),comm_rank,team_id,worker_id);
      
      //sleep(100);
    } else {
    while (BITMAP_same(COMM_bm_present_teams,COMM_bm_finished_teams));
    }
    make_root_choice_point();
    if (team_get_work())
     goto shared_fail;
 }
} else {
  while(BITMAP_same(GLOBAL_bm_free_workers,GLOBAL_bm_present_workers));
  printf( "FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FR (%d,%d) FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FR\n",comm_rank,worker_id);
  //sleep(100);
  PUT_OUT_FINISHED(worker_id);          
  LOCK(GLOBAL_lock_free_workers);
  BITMAP_insert(GLOBAL_bm_free_workers, worker_id);
  UNLOCK(GLOBAL_lock_free_workers);
  make_root_choice_point();
  SCHEDULER_GET_WORK();
}
shared_end:
printf( "SHARED END SHARED END SHARED END SHARED END SHARED END SHARED END   (%d,%d) SHARED END SHARED END SHARED END SHARED END SHARED END SHARED END \n",comm_rank,worker_id);
PUT_IN_FINISHED(worker_id);
if (worker_id == 0) {
  while (! BITMAP_same(GLOBAL_bm_present_workers,GLOBAL_bm_finished_workers));
      //PUT_OUT_ROOT_NODE(worker_id);
      //finished teams
  LOCK(COMM_locks_bm_finished_teams);
  BITMAP_insert(COMM_bm_finished_teams, comm_rank);
  UNLOCK(COMM_locks_bm_finished_teams);   
      //finished teams
      //running
  LOCK(COMM_locks_bm_running_teams);
  BITMAP_delete(COMM_bm_running_teams, comm_rank);
  UNLOCK(COMM_locks_bm_running_teams);  
      //running
  while(!BITMAP_empty(COMM_bm_running_teams));
  if(comm_rank == 0) 
    COMM_finished = 1;

//resultados
MPI_Comm parent; 
MPI_Comm_get_parent(&parent);
int a;
if( R_COMM_query_has_results(comm_rank)){
if(comm_rank == 0)
  MPI_Send(&a, 1, MPI_INT, 0, 34, parent);
printf("%d Resultados %d           .-----%d\n",comm_rank,comm_rank, R_COMM_query_has_results(comm_rank));

MPI_Recv( &a, 1, MPI_INT, MPI_ANY_SOURCE, 34, parent, MPI_STATUS_IGNORE);
char buf [5000] [256];
int count = 0;
int size;
    if (OrFr_qg_solutions(LOCAL_top_or_fr)) {
    qg_ans_fr_ptr aux_answer1, aux_answer2;
    aux_answer1 = SolFr_first(OrFr_qg_solutions(LOCAL_top_or_fr));
    while (aux_answer1) {
      Term aux = Yap_PopTermFromDB(AnsFr_answer(aux_answer1));
      YAP_WriteBuffer(aux,buf[count], 256, 0);
      count++;
      aux_answer2 = aux_answer1;
      aux_answer1 = AnsFr_next(aux_answer1);
      FREE_QG_ANSWER_FRAME(aux_answer2);
      //printf("--%d  %d\n",count, getpid());
    }
    FREE_QG_SOLUTION_FRAME(OrFr_qg_solutions(LOCAL_top_or_fr));
    OrFr_qg_solutions(LOCAL_top_or_fr) = NULL;
  }
  buf[count][0] = '\0';
if( count != 0)
  MPI_Send(&buf, 5000*256, MPI_CHAR, 0, 34, parent);
}
//resultados
printf( "2 SHARED END SHARED END SHARED END SHARED END SHARED END SHARED END   (%d,%d) SHARED END SHARED END SHARED END SHARED END SHARED END SHARED END \n",comm_rank,worker_id);
  finish_yapor();
  free_root_choice_point();
  PREG = GETWORK_FIRST_TIME;
  PREFETCH_OP(PREG);
  GONext();
} else {
  PREG = GETWORK_FIRST_TIME;
  PREFETCH_OP(PREG);
  GONext();
}
ENDPBOp();




  PBOp(getwork,Otapl)
    //printf("or inst get_work\n");
#ifdef TABLING
    if (DepFr_leader_cp(LOCAL_top_dep_fr) == Get_LOCAL_top_cp()) {
      /* the current top node is a leader node with consumer nodes below */
      if (DepFr_leader_dep_is_on_stack(LOCAL_top_dep_fr)) {
        /*    the frozen branch depends on the current top node     **
	** this means that the current top node is a generator node */
        LOCK_OR_FRAME(LOCAL_top_or_fr);
        if (OrFr_alternative(LOCAL_top_or_fr) == NULL ||
           (OrFr_alternative(LOCAL_top_or_fr) == ANSWER_RESOLUTION && B_FZ != Get_LOCAL_top_cp())) {
          /*                 there are no unexploited alternatives                 **
          ** (NULL if batched scheduling OR ANSWER_RESOLUTION if local scheduling) */
          UNLOCK_OR_FRAME(LOCAL_top_or_fr);
	  goto completion;
        } else {
          /*                     there are unexploited alternatives                     **
	  ** we should exploit all the available alternatives before execute completion */
          PREG = OrFr_alternative(LOCAL_top_or_fr);
          PREFETCH_OP(PREG);
          GONext();
        }
/* ricroc - obsolete
#ifdef  batched scheduling
        if (OrFr_alternative(LOCAL_top_or_fr) != NULL) {
#else   local scheduling
        if (OrFr_alternative(LOCAL_top_or_fr) != ANSWER_RESOLUTION || B_FZ == Get_LOCAL_top_cp()) {
#endif
          PREG = OrFr_alternative(LOCAL_top_or_fr);
          PREFETCH_OP(PREG);
          GONext();
        }
        UNLOCK_OR_FRAME(LOCAL_top_or_fr);
*/
      }
      goto completion;
    }
#endif /* TABLING */ 
    LOCK_OR_FRAME(LOCAL_top_or_fr);
#ifdef YAPOR_TEAMS
    if (OrFr_alternative(LOCAL_top_or_fr) && OrFr_alternative(LOCAL_top_or_fr) != INVALIDWORK) {
#else
    if (OrFr_alternative(LOCAL_top_or_fr)) {
#endif
      PREG = OrFr_alternative(LOCAL_top_or_fr);
      //printf(" (%d,%d) GET_WORK B %p  alt %p  or_B %p\n",team_id,worker_id,B,PREG,OrFr_node(LOCAL_top_or_fr));
     /* if (PREG == 0x10330798 && worker_id == 1){
        printf("sleep\n");
        sleep(60);
        printf(" (%d,%d) GET_WORK B %p  alt %p  or_B %p\n",team_id,worker_id,B,PREG,OrFr_node(LOCAL_top_or_fr));
      }*/
      PREFETCH_OP(PREG);
      GONext();
    } else {
      UNLOCK_OR_FRAME(LOCAL_top_or_fr);
      SCHEDULER_GET_WORK();
    }
  ENDPBOp();



  /* The idea is to check whether we are the last worker in the node.
     If we are, we can go ahead, otherwise we should call the scheduler. */
  PBOp(getwork_seq,Otapl)
    LOCK_OR_FRAME(LOCAL_top_or_fr);
    if (OrFr_alternative(LOCAL_top_or_fr) &&
        BITMAP_alone(OrFr_members(LOCAL_top_or_fr), worker_id)) {
      PREG = OrFr_alternative(LOCAL_top_or_fr);
      PREFETCH_OP(PREG);
      GONext();
    } else {
      UNLOCK_OR_FRAME(LOCAL_top_or_fr);
      SCHEDULER_GET_WORK();
    }
  ENDPBOp();


  PBOp(invalidwork,Otapl)
    //printf("INVALID (%d,%d)\n",team_id,worker_id);
    B = B->cp_b;
   goto fail;
  ENDPBOp();


  PBOp(mpistart,Otapl)
  printf("----------MPI START----------(%d)   (%d,%d)\n",getpid(),team_id,worker_id);
#ifdef YAPOR_MPI
  Term a = YAP_ReadBuffer("consult('team.pl')",NULL);
  //Yap_DebugPlWrite(a);
   YAP_RunGoal(a);

  // a = YAP_ReadBuffer("clause(queens(_),C,R)",NULL);
   //YAP_RunGoal(a);
   //Yap_DebugPlWrite(a);

  a = YAP_ReadBuffer("queens(S)",NULL);
  YAP_RunGoal(a);
  //sleep(100);

   //a = YAP_ReadBuffer("clause(solve(_,_,_),C,R)",NULL);
   //YAP_RunGoal(a);
   //Yap_DebugPlWrite(a);
  #include "mpi.h" 
        int rank = 100;
        int size,i;
    int number_teams;
    int workers_per_team[100];
    Term out;
    MPI_Comm parent; 
    MPI_Comm_get_parent(&parent); 
    MPI_Comm_rank(parent, &rank); 
    MPI_Comm_size(parent, &size);
    //printf("(%d) MPI START--- size %d ---- rank %d \n",getpid(),size,rank);
    MPI_Bcast(&number_teams, 1, MPI_INT, 0, parent);
     printf("(%d) MPI START---    %d \n",getpid(),number_teams);
    MPI_Bcast(workers_per_team,number_teams,MPI_INT,0,parent);
   
    for(i=0;i<number_teams;i++)
      printf("(%d) %d\n",i,workers_per_team[i]);
    
   //a = YAP_ReadBuffer("queens(S)",NULL);
   //YAP_RunGoal(a);

    Yap_init_mpi_yapor_teams_workers(workers_per_team[rank], 0, 0, rank);
    exit(0);
#endif

  ENDPBOp();


  PBOp(sync,Otapl)
    CUT_wait_leftmost();
    PREG = NEXTOP(PREG,Otapl);
    PREFETCH_OP(PREG);
    GONext();
  ENDPBOp();
