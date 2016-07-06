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
#include <time.h>

Term
  Yap_CallMetaCall(Term t, Term mod);

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
int my_comm_rank;

/*
REGSTORE * old_regs = NULL;

if(old_regs == NULL){
printf("NULL NULL\n");
old_regs = malloc(sizeof(REGSTORE));
memcpy(old_regs, Yap_regp, sizeof(REGSTORE));
}
*/

if(GLOBAL_regs==NULL && worker_id==0){
GLOBAL_regs = malloc(sizeof(REGSTORE));
memcpy(GLOBAL_regs, Yap_regp, sizeof(REGSTORE));
//printf("                                                         REGS REGS %d\n",getpid());
//char cwd[1024];
//getcwd(cwd, sizeof(cwd));
// printf("---------------------------------------%s---\n",cwd);
}

/*
if(worker_id == 0){
GLOBAL_buff = malloc(Yap_worker_area_size);
GLOBAL_team_array2 = malloc(100*sizeof(int)); 
printf("maloocs %p %p\n",GLOBAL_team_array2, GLOBAL_buff);   
}
*/
    /* wait for a new parallel goal */
//printf( "FIRST TIME %d ####################################### (%d,%d) ####################################### %d  --- load %d\n",getpid(),comm_rank,worker_id,GLOBAL_execution_counter);
LOCK(GLOBAL_locks_bm_present_team_workers);
BITMAP_insert(GLOBAL_bm_present_team_workers, worker_id);
UNLOCK(GLOBAL_locks_bm_present_team_workers);
    //running -------------------------------------
if(worker_id == 0){
  while (!BITMAP_same(GLOBAL_bm_present_workers,GLOBAL_bm_present_team_workers));
  if(GLOBAL_execution_counter == 10)
    GLOBAL_execution_counter = 0;
  else
    GLOBAL_execution_counter = 10;
    
  GLOBAL_delay_msg = 2;
  GLOBAL_delay_msg_count = 1;
  GLOBAL_delay_count = 0;
  GLOBAL_msg_count = 0;
  GLOBAL_share_count = 0;
  GLOBAL_time_stamp =1;

 int number_teams;
 MPI_Comm_size(MPI_COMM_WORLD,&number_teams);
  int i;
  for(i=0;i<number_teams;i++){
   GLOBAL_mpi_load(i)=0;
   GLOBAL_mpi_load_time(i)=0;
  }
  int flag = 0;
 MPI_Status status;
 int msg[100];

 MPI_Barrier(MPI_COMM_WORLD);

 MPI_Iprobe(MPI_ANY_SOURCE, 44, MPI_COMM_WORLD, &flag, &status );
 while(flag){
   MPI_Recv( &msg, 100, MPI_INT, MPI_ANY_SOURCE, 44, MPI_COMM_WORLD, &status );
   //printf("----------------------------------------------------------------------------------------- (%d) %d MENSAGEM: %d %d \n",comm_rank,getpid(),msg[0],msg[1]);
   MPI_Iprobe(MPI_ANY_SOURCE, 44, MPI_COMM_WORLD, &flag, &status );
 }

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

      if(s[0] == '\0'){
         int cp_tr[3];
         cp_tr[0] = NULL;
         MPI_Bcast(&cp_tr, 3, MPI_INT, 0, MPI_COMM_WORLD);
         Yap_exit(0);
      }
      //int a;
      //MPI_Recv(&a,1,MPI_INT,0,0,parent,MPI_STATUS_IGNORE);
      int i;
//  Term bb = YAP_ReadBuffer("queens(S)",NULL);
//  YAP_RunGoal(bb);
//Yap_StartSlots( PASS_REGS1 );
  
      //goal = YAP_ImportTerm(s);
      goal = YAP_ReadBuffer(s,NULL);
//  Term bb = YAP_ReadBuffer("queens(S)",NULL);
//  YAP_RunGoal(bb);
  
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
    //printf("BEFORE RUN GOAL  %p   %p\n",PREG,P);
 //Yap_do_low_level_trace = TRUE;
 saveregs();
 // goal = YAP_ReadBuffer("queens(X)", NULL);
 //printf("BEFORE ASP=%p (%ld) ENV=%p B=%p H=%p\n", ASP, LCL0-ASP, ENV, B, HR);
 Yap_CallMetaCall(goal, CurrentModule);
 //Yap_do_low_level_trace = FALSE;
 //Yap_DebugPlWrite(goal);
 setregs();
 //Yap_DebugPlWrite(goal);
 //printf("AFTER call_meta_call  %p   %p\n",PREG,P);  
 //printf("       ASP=%p (%ld) ENV=%p B=%p H=%p\n", ASP, LCL0-ASP, ENV, B, HR);
 JMPNext();    //YAP_RunGoal(goal);
  }
  else {
    if(GLOBAL_mpi_active){
      //sleep(100);
      //printf(" ANTES DO BCAST DA TRILHA\n");
      int cp_tr[3];
      MPI_Bcast(&cp_tr, 3, MPI_INT, 0, MPI_COMM_WORLD);
      if(cp_tr[0] == NULL)
         Yap_exit(0);
      if(comm_rank != 0)
      GLOBAL_mpi_load(0) = 1;
      //printf("------------------------ DEPOIS DO BCAST DA TRILHA\n");
      GLOBAL_root_cp = cp_tr[0];
      GLOBAL_root_cp->cp_tr = cp_tr[1];
      R_COMM_query_has_results(comm_rank) = cp_tr[2];
      make_root_choice_point();
//printf( "AAAA %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %p\n",getpid(),comm_rank,team_id,GLOBAL_root_cp->cp_tr);
       //Term bb = YAP_ReadBuffer("queens(S)",NULL);
       //YAP_RunGoal(bb);
//printf( "AAAA %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %p\n",getpid(),comm_rank,team_id,GLOBAL_root_cp->cp_tr);
      if(mpi_team_get_work()){
        //printf("SHARED FAIL %d\n",getpid());
        goto shared_fail;
      }
      else{
        goto shared_end;      
      }
//printf( "SAHRED FAIL %d $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ (%d,%d) $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ %d\n",getpid(),comm_rank,team_id,worker_id);
      
      //sleep(100);
    } else {
    while (BITMAP_same(COMM_bm_present_teams,COMM_bm_finished_teams));
    printf("BITMAP_same %lx\n", COMM_bm_present_teams);
    }
    make_root_choice_point();
    if (team_get_work())
     goto shared_fail;
 }
} else {
  //printf( "A FREE FREE FREE FREE  %d\n",GLOBAL_bm_free_workers);
  while(BITMAP_same(GLOBAL_bm_free_workers,GLOBAL_bm_present_workers));
 // printf( "FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FR (%d,%d) %pFREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FREE FR\n",comm_rank,worker_id,GLOBAL_root_cp);
  //printf( "B FREE FREE FREE FREE  %d\n",GLOBAL_bm_free_workers);
  //sleep(100);
  PUT_OUT_FINISHED(worker_id);          
  LOCK(GLOBAL_lock_free_workers);
  BITMAP_insert(GLOBAL_bm_free_workers, worker_id);
  UNLOCK(GLOBAL_lock_free_workers);
  //if(!LOCAL_top_cp)
  make_root_choice_point();
  SCHEDULER_GET_WORK();
}
shared_end:
//my_comm_rank = comm_rank;
//Yap_InitYaamRegs2( worker_id, team_id );
//comm_rank = my_comm_rank;
//printf( "SHARED END SHARED END SHARED END SHARED END SHARED END SHARED END   (%d,%d) SHARED END SHARED END SHARED END SHARED END SHARED END SHARED END \n",comm_rank,worker_id);
//printf("%d/%d/%d     ASP=%p (%ld) ENV=%p B=%p H=%p\n", worker_id, team_id, comm_rank,ASP, LCL0-ASP, ENV, B, HR);
PUT_IN_FINISHED(worker_id);
if (worker_id == 0) {
//printf("FIM FIMM %d\n",getpid());
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
//printf("2 FIM FIMM %d\n",getpid());
if( R_COMM_query_has_results(comm_rank)){
if(comm_rank == 0)
  MPI_Send(&a, 1, MPI_INT, 0, 34, parent);
// printf("%d %d Resultados %d           .-----%d\n",getpid(),comm_rank,comm_rank, R_COMM_query_has_results(comm_rank));

MPI_Recv( &a, 1, MPI_INT, MPI_ANY_SOURCE, 34, parent, MPI_STATUS_IGNORE);
char buf [5000] [500];
int count = 0;
int size;
    if (OrFr_qg_solutions(LOCAL_top_or_fr)) {
    qg_ans_fr_ptr aux_answer1, aux_answer2;
    aux_answer1 = SolFr_first(OrFr_qg_solutions(LOCAL_top_or_fr));
    while (aux_answer1) {
     //printf("1--%d   %d  -- %d\n",getpid(), count,((count+1)%5000));
      Term aux = Yap_PopTermFromDB(AnsFr_answer(aux_answer1));
      //printf("2--%d    %p    %p   %p\n",getpid(),AnsFr_answer(aux_answer1),ASP,HR);
      //Yap_DebugPlWrite(aux);
      YAP_WriteBuffer(aux,buf[count], 500, 0);
      //printf("3--%d    %p    %p   %p\n",getpid(),AnsFr_answer(aux_answer1),ASP,HR);
      
      aux_answer2 = aux_answer1;
      aux_answer1 = AnsFr_next(aux_answer1);
      FREE_QG_ANSWER_FRAME(aux_answer2);
     // if(count % 100 == 0)
      //printf("--%d  %d\n",count, getpid());
      count++;
      if(((count)%5000)==0){
	//printf("cont %d |%c|\n",count,buf[4999][0]);
      MPI_Send(&buf, 5000*500, MPI_CHAR, 0, 34, parent);
      count = 0;
      }     
    }
    FREE_QG_SOLUTION_FRAME(OrFr_qg_solutions(LOCAL_top_or_fr));
    OrFr_qg_solutions(LOCAL_top_or_fr) = NULL;
  }
  buf[count][0] = '\0';

  MPI_Send(&buf, 5000*500, MPI_CHAR, 0, 34, parent);
}
//resultados
//printf( "2 SHARED END SHARED END SHARED END SHARED END SHARED END SHARED END   (%d,%d)  %pSHARED END SHARED END SHARED END SHARED END SHARED END SHARED END \n",comm_rank,worker_id);
//printf(" REGS REGS %p\n",GLOBAL_regs);
//free(old_regs);
  finish_yapor();
  free_root_choice_point();
memcpy(Yap_regp, GLOBAL_regs, sizeof(REGSTORE));
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
  //printf("----------MPI START----------(%d)   (%d,%d)\n",getpid(),team_id,worker_id);
#ifdef YAPOR_MPI


   //a = YAP_ReadBuffer("clause(solve(_,_,_),C,R)",NULL);
   //YAP_RunGoal(a);
   //Yap_DebugPlWrite(a);
  #include "mpi.h" 
        int rank = 100;
        int size,i;
    int number_teams;
    int workers_per_team[100];

    MPI_Comm parent; 
    MPI_Comm_get_parent(&parent); 
    MPI_Comm_rank(parent, &rank); 
    MPI_Comm_size(parent, &size);
    //printf("(%d) MPI START--- size %d ---- rank %d \n",getpid(),size,rank);
    MPI_Bcast(&number_teams, 1, MPI_INT, 0, parent);
    GLOBAL_mpi_n_teams = number_teams;
    printf("(%d) MPI START---    %d \n",getpid(),GLOBAL_mpi_n_teams);
    MPI_Bcast(workers_per_team,number_teams,MPI_INT,0,parent);
   
    //for(i=0;i<number_teams;i++)
      //printf("(%d) %d\n",i,workers_per_team[i]);


  char st_term[500];
  char st_consult[1000]={""};
  Term in;
  //printf("\n -----------------------------%d ---------------------------------------------------- \n",getpid());
  MPI_Bcast(st_term,500,MPI_CHAR,0,parent);
  strcat(st_consult,"consult('");
  strcat(st_consult,st_term);
  strcat(st_consult,"')");
  //printf("||||%s||||\n",st_consult);
   in = YAP_ReadBuffer(st_consult,NULL);
  //Yap_DebugPlWrite(in);
  YAP_RunGoal(in);


  GLOBAL_regs = NULL;
//------
//GLOBAL_regs = malloc(sizeof(REGSTORE));
//memcpy(GLOBAL_regs, Yap_regp, sizeof(REGSTORE));
//------
GLOBAL_buff = malloc(Yap_worker_area_size);
GLOBAL_team_array2 = malloc(1000*sizeof(int)); 
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
