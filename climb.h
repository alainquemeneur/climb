#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> 
#include <string.h>
#include <fcntl.h>
#define __USE_XOPEN
#include <time.h>
#include <mysql/mysql.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <sys/sem.h>

// Necessary structures and union
struct json_keyvalue
{
char key[1000],value[1000];
};

struct json_record
{
struct json_keyvalue *keyvalues;
int keyvalues_number;
};

struct json_zone
{
char name[1000];
struct json_record *records;
int records_number;
};

struct json_response
{
struct json_zone *zones;
};

union type_arg
{
int value;
struct semid_ds *buffer;
unsigned short *tab_value;
};

// MySQL functions
#define _MYSQL

#define CLIMB_MYSQL_DBHANDLER MYSQL *
#define CLIMB_MYSQL_DBRESULT MYSQL_RES *

CLIMB_MYSQL_DBHANDLER climb_mysql_opendatabase(char *databasename, char *host, char *login, char *password);
CLIMB_MYSQL_DBRESULT climb_mysql_query(CLIMB_MYSQL_DBHANDLER handler, char *query);
int climb_mysql_ntuples(CLIMB_MYSQL_DBRESULT result);
void climb_mysql_clear_result(CLIMB_MYSQL_DBRESULT result);
void climb_mysql_closedatabase(CLIMB_MYSQL_DBHANDLER handler);
void climb_mysql_getvalue(CLIMB_MYSQL_DBRESULT result, int row, int col, char *value, int max);

// Hash function
void climb_sha56_hash(char *string);

// Http/s functions
char *climb_http_get_request_body();
char *climb_http_post_request_body();
char *climb_http_request_method();
char* climb_http_request_uri();
void climb_http_translate(char *in_string);
int climb_http_request_body_items_number(char *string);
int climb_http_request_body_item(char *in_string,int pos,char *out_string);
int climb_http_request_body_item_name(char *in_string,int pos,char *out_string);
void climb_http_response_init();

// Time functions
int climb_time_dayoftheweek(char * date_string);
void climb_time_zeroize(int num, char *out_string);

// JSON functions
struct json_response *climb_json_init();
void climb_json_status_seterror(struct json_response *jr, char *errorstatus);
void climb_json_status_unseterror(struct json_response *jr);
void climb_json_data_record_add_keyvalue_(struct json_response *jr, char *key, char *value);
void climb_json_data_add_record_(struct json_response *jr);
void climb_json_response_(struct json_response *jr);

// Semaphore functions
void climb_semaphore_init(int *sem, int pack);
int climb_semaphore_get(int *sem, int pack);
void climb_semaphore_take_token(int *sem);
void climb_semaphore_release_token(int *sem);