/* MYDDAS */

#if defined MYDDAS_MYSQL || defined MYDDAS_ODBC

/* myddas_initialization.c */
MYDDAS_GLOBAL          myddas_init_initialize_myddas(void);
MYDDAS_UTIL_CONNECTION myddas_init_initialize_connection(void *,void *,MYDDAS_UTIL_CONNECTION);
MYDDAS_UTIL_PREDICATE  myddas_init_initialize_predicate(char *, int, char *,MYDDAS_UTIL_PREDICATE);

#ifdef MYDDAS_STATS
/* myddas_statistics.c */
MYDDAS_GLOBAL          myddas_stats_initialize_global_stats(MYDDAS_GLOBAL);
MYDDAS_STATS_STRUCT    myddas_stats_initialize_connection_stats(void);
void                   myddas_stats_delete_stats_list(MYDDAS_STATS_STRUCT);
#endif /* MYDDAS_STATS */

#ifdef MYDDAS_MYSQL
/* myddas_util.c */
void                   myddas_util_table_write(MYSQL_RES *);
#endif
Short                  myddas_util_connection_type(void *);
MYDDAS_UTIL_CONNECTION myddas_util_add_connection(void *,void *);
MYDDAS_UTIL_CONNECTION myddas_util_search_connection(void *);
void                   myddas_util_delete_connection(void *);
MYDDAS_UTIL_CONNECTION myddas_util_add_predicate(char * ,Int , char *,void *);
MYDDAS_UTIL_PREDICATE  myddas_util_search_predicate(char * ,Int , char *);
void                   myddas_util_delete_predicate(MYDDAS_UTIL_PREDICATE);

/* Get's the number of queries to save */
UInt                   myddas_util_get_total_multi_queries_number(MYDDAS_UTIL_CONNECTION);
void                   myddas_util_set_total_multi_queries_number(MYDDAS_UTIL_CONNECTION,UInt);
#ifdef MYDDAS_ODBC
/* Return enviromment identifier*/
SQLHENV                myddas_util_get_odbc_enviromment(SQLHDBC);
#endif

void *                 myddas_util_get_list_pred(MYDDAS_UTIL_CONNECTION);
void *                 myddas_util_get_pred_next(void *);
char *                 myddas_util_get_pred_module(void *);
char *                 myddas_util_get_pred_name(void *);
MyddasInt              myddas_util_get_pred_arity(void *);
//DELETE THIS WHEN DB_STATS  IS COMPLETED
MyddasInt              get_myddas_top(void);

#ifdef DEBUG
void check_int(void);
#endif

#endif /* MYDDAS_MYSQL || MYDDAS_ODBC */

/* myddas_mysql.c */
#if defined MYDDAS_MYSQL
void    Yap_InitMYDDAS_MySQLPreds(void);
void    Yap_InitBackMYDDAS_MySQLPreds(void);
#endif

/* myddas_odbc.c */
#if defined MYDDAS_ODBC
void    Yap_InitMYDDAS_ODBCPreds(void);
void    Yap_InitBackMYDDAS_ODBCPreds(void);
#endif

/* myddas_shared.c */
#if defined MYDDAS_ODBC || defined MYDDAS_MYSQL
void    Yap_MYDDAS_delete_all_myddas_structs(void);
void    Yap_InitMYDDAS_SharedPreds(void);
void    Yap_InitBackMYDDAS_SharedPreds(void);
#endif

/* myddas_top_level.c */
#if defined MYDDAS_TOP_LEVEL && defined MYDDAS_MYSQL //&& defined HAVE_LIBREADLINE
void    Yap_InitMYDDAS_TopLevelPreds(void);
#endif

