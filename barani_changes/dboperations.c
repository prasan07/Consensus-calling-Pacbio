/*
  program to connect to mysql and access blacklist and whitelist tables
  and also updating the current database from remote server. 
  The methods create their connection and destroy them before returning to
  the calling function, this is a preferred design choice for programs that does not
  use the db often. This prevents having an open db connection all the time.
*/
#include <mysql.h>
#include <stdio.h>
#include "dbutility.h"
#include <stdlib.h>
#include <string.h>

/*MYSQL *conn = NULL;
	
int db_init(){
	conn = mysql_init(NULL);
        if (!mysql_real_connect(conn, SERVER_LOC,
                                USER, PASS, DATABASE, 0, NULL, 0)) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }	
}

void db_close(){
        if(conn!=NULL)
                mysql_close(conn);
}*/

/*
function 	: method to retrieve the list of virus signatures
return value 	: returns a struct signatures list which contains all the blacklisted signatures in the db.
parameters 	: none
*/
struct signatures *getstructures(){
	int row_count = 0;
	int total_length = 0;
	int current_loc = 0;
	struct signatures* result = (struct signatures *) malloc(sizeof(struct signatures));	
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;

	conn = mysql_init(NULL);
        /* Connect to the local database */
        if (!mysql_real_connect(conn, SERVER_LOC,
                                USER, PASS, DATABASE, 0, NULL, 0)) {
                fprintf(stderr, "%s\n", mysql_error(conn));
		free(result);
                return NULL;
        }
	
	/* execute SQL query */
        if (mysql_query(conn, "select * from blacklist")) {
                fprintf(stderr, "%s\n", mysql_error(conn));
		free(result);
                return NULL;
	}
	res = mysql_store_result(conn);
	if (res == NULL)
	{	
		free(result);
		return NULL;
	}
	/* find the total size of the strings in the table */
	while ((row = mysql_fetch_row(res)))
	{
		total_length+=strlen(row[1])+1;
                row_count++;
	}
	result->signatures = (char *) malloc(total_length);
	result->sig_count = row_count;
	mysql_data_seek(res, 0);
	/*copy the strings in the structure one by one*/
	while ((row = mysql_fetch_row(res)))
	{
		memcpy(result->signatures + current_loc, row[1], strlen(row[1]));  
                current_loc += strlen(row[1]);
                result->signatures[current_loc] = '\0';
                current_loc++;
	}
	/*perform clean up and exit*/
	mysql_free_result(res);
	mysql_close(conn);
	return result;
}

/*
function 	: method to update the virus signatures from remote DB server
return value 	: 1 on success, -1 on failure
parameters 	: none
*/
int update_structures(){
	unsigned int max_hash_id = 0, max_signature_id = 0;
	char query[300], insert_query[65100];
        MYSQL_RES *res;
        MYSQL_ROW row;
	MYSQL *conn;
        MYSQL *remote_conn;

        conn = mysql_init(NULL);
        /* Connect to local database */
        if (!mysql_real_connect(conn, SERVER_LOC,
                                USER, PASS, DATABASE, 0, NULL, 0)) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
	/*//retrieve max id from whitelist
        if (mysql_query(conn, "select max(hash_id) from whitelist")) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
        if((row = mysql_fetch_row(res)) != NULL){
                max_hash_id = atoi(row[0]);
        }else{
                max_hash_id = 0;
        }
	//retrieve max id from blacklist
	if (mysql_query(conn, "select max(signature_id) from blacklist")) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
	res = mysql_use_result(conn);
        if((row = mysql_fetch_row(res)) != NULL){
                max_signature_id = atoi(row[0]);
        }else{
                max_signature_id = 0;
        }
	// Connect to remote database */
	remote_conn = mysql_init(NULL);
        if (!mysql_real_connect(remote_conn, REMOTE_LOC,
                                REMOTE_USER, PASS, DATABASE, 3306, NULL, 0)) {
                //fprintf(stderr, "%s\n", mysql_error(conn));
		perror("error!!");
		printf("connection failed!!!");
                return -1;
        }
	/*retrieve update data for whitelist*/
	//sprintf(query, "select * from whitelist");
        if (mysql_query(remote_conn, "select * from `whitelist`")) {
                fprintf(stderr, "%s\n", mysql_error(conn));
		printf("exit");
                return -1;
        }
	res = mysql_store_result(remote_conn);
	int num = mysql_num_rows(res);
	/* insert new data into whitelist*/
        while((row = mysql_fetch_row(res)) != NULL){
                sprintf(insert_query, "insert into whitelist values (%d , '%s')", atoi(row[0]), row[1]);
		if (mysql_query(conn, insert_query)) {
                	fprintf(stderr, "%s\n", mysql_error(conn));
                	return -1;
        	}
        }
	mysql_free_result(res);
	/*retrieve update data for blacklist*/
	sprintf(query, "select * from blacklist");
        if (mysql_query(remote_conn, query)) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(remote_conn);
	/* insert new data into blacklist*/
        while((row = mysql_fetch_row(res)) != NULL){
                sprintf(insert_query, "insert into blacklist values (%d , '%s')", atoi(row[0]), row[1]);
                if (mysql_query(conn, insert_query)) {
                        fprintf(stderr, "%s\n", mysql_error(conn));
                        return -1;
                }
        }
	/*perform clean up and exit*/
        mysql_free_result(res);
        mysql_close(conn);
	return 0;	
}

/*
function	: method to compare the whitelist hashes
return value 	: 1 on success (hash is present)
		  0 on success (hash is not present)
		 -1 on error
parameters	: path of the file whose hash is to be compared
*/
int isWhitelisted(char * file_path){
	char* hash_val = NULL;
	char query[300];
	int result = 0;
	MYSQL_RES *res;
        MYSQL_ROW row;
	MYSQL *conn;

        conn = mysql_init(NULL);
        /* Connect to local database */
        if (!mysql_real_connect(conn, SERVER_LOC,
                                USER, PASS, DATABASE, 0, NULL, 0)) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
	/*retrieve hash of the given file*/
	hash_val = getsha256(file_path);
	printf("%s \n", hash_val);	
	sprintf(query, "select * from whitelist where hash = '%s'", hash_val);
	printf("%s \n", query);
	/*check for the hash in the whitelist table*/
	if (mysql_query(conn, query)) {
                fprintf(stderr, "%s\n", mysql_error(conn));
                return -1;
        }
        res = mysql_use_result(conn);
	if((row = mysql_fetch_row(res)) != NULL){
                result = 1;
        }else{
		result = 0;
	}
	/*perform clean up and exit*/
	mysql_free_result(res);
   	mysql_close(conn);
	return result;
}
/*
int main() {
        struct signatures* data = getstructures();
        int count = data->sig_count;
        int i = 0;
        int next_loc = 0;
        for(i = 0;i<count; i++){
                printf("%s \n", data->signatures+next_loc);
                next_loc+= strlen(data->signatures+next_loc)+1;
        }
        printf("is the file white listed: %d\n", isWhitelisted("/bin/cat"));
	printf("is the file white listed: %d\n", isWhitelisted("/bin/ls"));
	data = getstructures();
        count = data->sig_count;
        i = 0;
        next_loc = 0;
        for(i = 0;i<count; i++){
                printf("%s \n", data->signatures+next_loc);
                next_loc+= strlen(data->signatures+next_loc)+1;
        }
	free(data);
	data = getstructures();
        count = data->sig_count;
        i = 0;
        next_loc = 0;
        for(i = 0;i<count; i++){
                printf("%s \n", data->signatures+next_loc);
                next_loc+= strlen(data->signatures+next_loc)+1;
        }
	free(data);
	data = getstructures();
        count = data->sig_count;
        i = 0;
        next_loc = 0;
        for(i = 0;i<count; i++){
                printf("%s \n", data->signatures+next_loc);
                next_loc+= strlen(data->signatures+next_loc)+1;
        }
	//printf("%d \n", update_structures());	
}*/