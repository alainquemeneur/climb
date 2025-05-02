#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "climb.h"

int main()
{
CLIMB_MYSQL_DBHANDLER handler; // this variable will be the MySQL database access handler
CLIMB_MYSQL_DBRESULT result; // this variable will store MySQL queries responses
char *request_body; // this variable will store the http(s) request body, sent by the calling client
char *request_method; // this variable will store the http(s) method used by the calling client
char *request_uri; // this variable will store the uri targeted by the calling client
char query[1000],res[1000];
struct json_response *jr; // this variable will be the final JSON response we are going to build
int number_of_items_in_body,semaphore,index;

char *first_name, *second_name; // these variables will store the text entered by the user in the calling client form
char *param1,*param2; // these variables will store the names of the fields in the calling client form

request_method=climb_http_request_method(); // first we check the calling method of the calling client form (GET or POST)
if(strcmp(request_method,"POST")==0) request_body=climb_http_post_request_body(); // if POST, we get the sent body the POST way
else request_body=climb_http_get_request_body(); // if GET, we get the sent body the GET way
first_name=(char *)malloc(strlen(request_body)); // now we allocate room for the 4 variables based on the body content. Be aware that this kind of malloc is the proper way to avoid buffer overflow attacks in the body
second_name=(char *)malloc(strlen(request_body));
param1=(char *)malloc(strlen(request_body));
param2=(char *)malloc(strlen(request_body));
request_uri= climb_http_request_uri(); // we get the target URI
number_of_items_in_body=climb_http_request_body_items_number(request_body); // we get the number of fields in the body, if needed (it will be 2 here)
climb_http_request_body_item(request_body,1,second_name); // we get the last field. Please note that the last field is number 1
climb_http_translate(second_name); // we translate the field to correct all the html strange caracters
climb_http_request_body_item(request_body,2,first_name); // we get the first field
climb_http_translate(first_name); // and correct it as well
climb_http_request_body_item_name(request_body,1,param2); // this is how we get the last parameter name (often useless). The last one is also number 1
climb_http_translate(param2); // correction
climb_http_request_body_item_name(request_body,2,param1); // we get the first parameter name
climb_http_translate(param1); // correction

// Now we are doing the MySQL part
// This is how we are connecting to the local MySQL DB. In the present case, this call will fail since the DB have not been set up
if((handler=climb_mysql_opendatabase("database_name","localhost","database_user","database_password"))!=NULL)
	{
	strcpy(query,"select table_field from database_table;"); // you can put here any kind of SQL query
	result=climb_mysql_query(handler,query); // we send the query to MySQL
	for(int i=0;i<climb_mysql_ntuples(result);i++) // we get the number of tuples in the SQL response
		{
		climb_mysql_getvalue(result,i,0,res,1000); // for each of the tuple, we get the value of column 0
		// in this example, we are not doing anything with these tuples
		}
	climb_mysql_clear_result(result); // every SQL request must be cleared before being able to query another one with climb_mysql_query
	climb_mysql_closedatabase(handler); // one all SQL requests are done, we close the MySQL access
	}

// Now for the example, we create a critical code section. This is optional. Only interesting if there is a risk another C backend accesses the same ressources at the same time, which is not the case here
// First we check if a semaphore with pack=1 already exists. If not it is created
// Different packs creates different semaphores. Please be aware that all the backends using the same semaphore token use the same pack number.
// Here we use pack number 1
if(climb_semaphore_get(&semaphore, 1)==-1) climb_semaphore_init(&semaphore, 1);
// Then we claim for the token. Be aware that this call will be blocking until the token is available (meaning, released by the backend using it, if any)
climb_semaphore_take_token(&semaphore);
// If we arrive here, it means that the token has been granted by the Linux kernel to this program
// we can now begin to build the JSON response

// Ok, this is the JSON response sequence
// First, we initialize our JSON response
// The JSON response will content 2 zones, "status" zone and "data" zone
// "status" zone is to inform the client of the status of the response (true if ok, false if nok)
// "data" zone is to send to the client the response itself (x key/value pairs)
jr=climb_json_init(); // this creates the "status" and the "data" zone
// If you want to trigger error status to "on" with a status code string you can use :
// climb_json_status_seterror(jr, "my_error_status");
// Now we add a first record in the "data" zone
climb_json_data_add_record_(jr);
// In the first record of the "data" zone, we add 4 key/value pairs
climb_json_data_record_add_keyvalue_(jr, "firstname", first_name);
climb_json_data_record_add_keyvalue_(jr, "secondname", second_name);
climb_json_data_record_add_keyvalue_(jr, "first param name", param1);
climb_json_data_record_add_keyvalue_(jr, "second param name", param2);
// Then we add a second record in the "data" zone
climb_json_data_add_record_(jr);
// In this record, we add 3 key/value pairs
climb_json_data_record_add_keyvalue_(jr, "uri", request_uri);
climb_json_data_record_add_keyvalue_(jr, "method", request_method);
climb_json_data_record_add_keyvalue_(jr, "body", request_body);
// Now the JSON response is built and ready
// The only remaining task to be performed is to send it back to the calling client

// Let's send the JSON response to the calling client
climb_http_response_init(); // this initialize the http(s) response
climb_json_response_(jr); // this sends the JSON response to the calling client
climb_semaphore_release_token(&semaphore); // then we can release the token, to free potential other backends blocked waiting for it
// then the backend program is over and stop. If the user click again on the button, this program will be launched by NginX again
}