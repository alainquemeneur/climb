#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#define __USE_XOPEN
#include <time.h>
#include <mysql/mysql.h>
#include "climb.h"

union type_arg arg;

// MySQL functions
CLIMB_MYSQL_DBHANDLER climb_mysql_opendatabase(char *databasename, char *host, char *login, char *password)
{
CLIMB_MYSQL_DBHANDLER handler;

handler=(MYSQL *)malloc(sizeof(MYSQL));
mysql_init(handler);
return(mysql_real_connect(handler, host, login, password, databasename, 0, NULL, 0));
}

CLIMB_MYSQL_DBRESULT climb_mysql_query(CLIMB_MYSQL_DBHANDLER handler, char *query)
{
CLIMB_MYSQL_DBRESULT result;
char *buf;

result=(MYSQL_RES *)malloc(sizeof(MYSQL_RES));
buf=(char *)malloc(strlen(query)+10);
strcpy(buf, query);
buf[strlen(buf)-1]=0;
mysql_query(handler, buf);
result=mysql_store_result(handler);
free(buf);
return(result);
}

int climb_mysql_ntuples(CLIMB_MYSQL_DBRESULT result)
{
return((int)mysql_num_rows(result));
}

void climb_mysql_clear_result(CLIMB_MYSQL_DBRESULT result)
{
mysql_free_result(result);
}

void climb_mysql_closedatabase(CLIMB_MYSQL_DBHANDLER handler)
{
mysql_close(handler);
free(handler);
}

void climb_mysql_getvalue(CLIMB_MYSQL_DBRESULT result, int row, int col, char *value, int max)
{
MYSQL_ROW rrow;

mysql_data_seek(result, row);
rrow=mysql_fetch_row(result);
strncpy(value, rrow[col], max-1);
value[max]=0;
}

// Hash function
void climb_sha56_hash(char *string)
{
char *buffer;
int fd,nb,i,start;

buffer=(char *)malloc(1100);
sprintf(buffer,"/tmp/climb_hash_in_%d",getpid());
fd=open(buffer,O_CREAT | O_TRUNC | O_WRONLY, 0644);
write(fd,string,strlen(string));
close(fd);
sprintf(buffer,"/usr/bin/openssl dgst -sha256 \'/tmp/climb_hash_in_%d\' > \'/tmp/climb_hash_out_%d\'",getpid(),getpid());
system(buffer);
sprintf(buffer,"/tmp/climb_hash_out_%d",getpid());
fd=open(buffer,O_RDONLY);
nb=read(fd,buffer,1000);
buffer[nb]=0;
close(fd);
i=0;
while(buffer[i]!='=') i++;
i+=2;
start=i;
while(buffer[i]!=0x0A)
	{
	buffer[i-start]=buffer[i];
	i++;
	}
buffer[i-start]=0;
strcpy(string,buffer);
sprintf(buffer,"/tmp/climb_hash_in_%d",getpid());
unlink(buffer);
sprintf(buffer,"/tmp/climb_hash_out_%d",getpid());
unlink(buffer);
free(buffer);
}

// Http/s functions
char* climb_http_post_request_body()
{
int query_size;

query_size=atoi(getenv("CONTENT_LENGTH"));
char* query_string = (char*) malloc(query_size+10);
if (query_string != NULL) fread(query_string,query_size,1,stdin);
return query_string;
}

char *climb_http_get_request_body()
{
char *query_string;

query_string=(char *)malloc(strlen(getenv("QUERY_STRING"))+10);
strcpy(query_string,getenv("QUERY_STRING"));
return(query_string);
}

char *climb_http_request_method()
{
char *method;

method=(char *)malloc(1000);
strcpy(method,getenv("REQUEST_METHOD"));
return(method);
}

char* climb_http_request_uri()
{
char *url;

url=(char *)malloc(1000);
strcpy(url,getenv("REQUEST_URI"));
return(url);
}

void climb_internal_http_urldecode(char *string)
{
int i,start;
char *res,buf[50];

start=i=0;
res=(char *)malloc(10*strlen(string));
while(i<=strlen(string))
	{
	if(string[i]!='%') 
		{
		res[start]=string[i];
		i++;
		start++;
		}
	else
		{
		if(string[i+1]=='0' && string[i+2]=='A')
			{
			res[start]='\\';
			res[start+1]='\\';
			res[start+2]='n';
			start = start+3;
			i = i+3;
			}
		else
			{
			if(string[i+1]!='3' || string[i+2]!='C')
				{
				sprintf(buf,"0x%c%c",string[i+1],string[i+2]);
				res[start]=strtol(buf,NULL,16);
				start++;
				i = i+3;
				}
			else
				{
				res[start]='&';
				res[start+1]='l';
				res[start+2]='t';
				res[start+3]=';';
				start = start+4;
				i = i+3;
				}
			}
		}
	}
strcpy(string,res);
free(res);
}

void climb_internal_http_prepare(char *string)
{
int i,start;
char *res;

res=(char *)malloc(strlen(string)*2);
i=start=0;
do
	{
	if(string[i]!=';' && string[i]!='\'' && string[i]!='"') res[start]=string[i];
	else res[start]=' ';
	start++;
	i++;
	}
while(i<=strlen(string));
strcpy(string,res);
free(res);
}

void climb_http_translate(char *in_string)
{
int i;

for(i=0;i<strlen(in_string);i++)
	{
	if(in_string[i]=='+') in_string[i]=' ';
	}
climb_internal_http_urldecode(in_string);
climb_internal_http_prepare(in_string);
}

int climb_http_request_body_items_number(char *string)
{
int i,sum;

sum=0;
for(i=0;i<strlen(string);i++)
	{
	if(string[i]=='=') sum++;
	}
return(sum);
}

int climb_http_request_body_item(char *in_string,int pos,char *out_string)
{
int i,n,start;

i=strlen(in_string);
for(n=0; n<pos;n++) 
	{
	while(in_string[i]!='=') i--;
	i--;
	}
i+=2;
start=i;
do
	{
	out_string[i-start]=in_string[i];
	i++;
	}
while(in_string[i-1]!=0 && in_string[i-1]!='&');
out_string[i-start-1]=0;
}

int climb_http_request_body_item_name(char *in_string,int pos,char *out_string)
{
int i,n,start;

i=strlen(in_string);
for(n=0; n<pos;n++) 
	{
	while(in_string[i]!='&' && i>=0) i--;
	i--;
	}
i+=2;
start=i;
do
	{
	out_string[i-start]=in_string[i];
	i++;
	}
while(in_string[i-1]!=0 && in_string[i-1]!='=');
out_string[i-start-1]=0;
}

char *climb_http_request_authorization_header()
{
char *res;

res=(char *)malloc(1000);
strcpy(res,getenv("HTTP_AUTHORIZATION"));
return(res);
}

void climb_http_response_init()
{
printf("Access-Control-Allow-Origin: *\nContent-Type: application/json;charset=UTF-8\n\n");
}

// Time functions
int climb_time_dayoftheweek(char * date_string)
{
// date_string must be in format year-month-day
// returns 0 for sunday, 1 for monday, ...
struct tm tm;
time_t t;

memset((void *) &tm, 0, sizeof(tm));
if(strptime(date_string,"%Y-%m-%d",&tm)!=NULL) 
	{
    t=mktime(&tm);
    if(t>=0) 
    	{
      	return (localtime(&t)->tm_wday);
    	}
  	}
return (-1);
}

void climb_time_zeroize(int num, char *out_string)
{
if(num<10) sprintf(out_string,"0%d",num);
else sprintf(out_string,"%d",num);
}

// JSON functions
struct json_response *climb_json_init()
{
struct json_response *jr;

jr=(struct json_response *)malloc(sizeof(struct json_response));
jr->zones=(struct json_zone *)malloc(sizeof(struct json_zone)*2);
strcpy(jr->zones[0].name,"status");
strcpy(jr->zones[1].name,"data");
jr->zones[0].records=(struct json_record *)malloc(sizeof(struct json_record));
jr->zones[0].records_number=1;
jr->zones[0].records[0].keyvalues=(struct json_keyvalue *)malloc(2*sizeof(struct json_keyvalue));
jr->zones[0].records[0].keyvalues_number=2;
strcpy(jr->zones[0].records[0].keyvalues[0].key,"error");
strcpy(jr->zones[0].records[0].keyvalues[0].value,"false");
strcpy(jr->zones[0].records[0].keyvalues[1].key,"errorstatus");
strcpy(jr->zones[0].records[0].keyvalues[1].value,"");
jr->zones[0].records[0].keyvalues_number=2;
jr->zones[1].records=(struct json_record *)malloc(1000000*sizeof(struct json_record));
jr->zones[1].records_number=0;
return(jr);
}

void climb_json_status_seterror(struct json_response *jr, char *errorstatus)
{
strcpy(jr->zones[0].records[0].keyvalues[0].value,"true");
strncpy(jr->zones[0].records[0].keyvalues[1].value,errorstatus,1000);
}

void climb_json_status_unseterror(struct json_response *jr)
{
strcpy(jr->zones[0].records[0].keyvalues[0].value,"false");
strcpy(jr->zones[0].records[0].keyvalues[1].value,"");
}

void climb_json_data_add_record_(struct json_response *jr)
{
int i;

i=jr->zones[1].records_number;
i++;
jr->zones[1].records_number=i;
jr->zones[1].records[i-1].keyvalues=(struct json_keyvalue *)malloc(100*sizeof(struct json_keyvalue));
jr->zones[1].records[i-1].keyvalues_number=0;
}

void climb_json_data_record_add_keyvalue_(struct json_response *jr, char *key, char *value)
{
int i,j;

i=jr->zones[1].records_number-1;
j=jr->zones[1].records[i].keyvalues_number;
jr->zones[1].records[i].keyvalues_number=j+1;
if(j<100)
	{
	strncpy(jr->zones[1].records[i].keyvalues[j].key,key,1000);
	strncpy(jr->zones[1].records[i].keyvalues[j].value,value,1000);
	}
}

char *climb_internal_json_manage_boolean(char *in_string)
{
char *out_string;

out_string=(char *)malloc(1000);
if(strcmp(in_string,"true")==0 || strcmp(in_string,"false")==0) strcpy(out_string,in_string);
else sprintf(out_string,"\"%s\"",in_string);
return(out_string);
}

void climb_json_response_(struct json_response *jr)
{
int i,j;

printf("{\n\
  \"%s\":[\n\
    {\n\
    \"%s\":%s,\n\
    \"%s\":%s\n\
    }\n\
  ],\n\
  \"data\":[\n",jr->zones[0].name, jr->zones[0].records[0].keyvalues[0].key,climb_internal_json_manage_boolean(jr->zones[0].records[0].keyvalues[0].value),jr->zones[0].records[0].keyvalues[1].key,climb_internal_json_manage_boolean(jr->zones[0].records[0].keyvalues[1].value));
for(i=0;i<jr->zones[1].records_number;i++)
	{
    printf("   {\n");
    for(j=0;j<jr->zones[1].records[i].keyvalues_number;j++)
    	{
    	printf("   \"%s\":%s",jr->zones[1].records[i].keyvalues[j].key,climb_internal_json_manage_boolean(jr->zones[1].records[i].keyvalues[j].value));
    	if(j==jr->zones[1].records[i].keyvalues_number-1) printf("\n");
    	else printf(",\n");
    	}
    printf("   }");
    if(i==jr->zones[1].records_number-1) printf("\n");
    else printf(",\n");
    }
printf("  ]\n\
}\n");
}

// Semaphores functions
void climb_semaphore_init(int *sem, int pack)
{
if((sem[0]=semget(ftok(getenv("HOME"),pack),1,IPC_CREAT | 0644))!=-1)
	{
	arg.value=1;
	semctl(sem[0],0,SETVAL,arg);
	}
}

int climb_semaphore_get(int *sem, int pack)
{
if((sem[0]=semget(ftok(getenv("HOME"),pack),1,0644))==-1) return(-1);
return(0);
}

void climb_semaphore_take_token(int *sem)
{
struct sembuf s;

s.sem_num=0;
s.sem_op=-1;
s.sem_flg=0;
semop(sem[0],&s,1);
}

void climb_semaphore_release_token(int *sem)
{
struct sembuf s;

s.sem_num=0;
s.sem_op=1;
s.sem_flg=0;
semop(sem[0],&s,1);
}