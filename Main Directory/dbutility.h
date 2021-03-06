#ifndef db_utility_H
#define db_utility_H
#include <mysql.h>

#define PAGE_SIZE 4096

//DB config
#define REMOTE_LOC "172.24.18.7"
#define REMOTE_USER "barani008"
#define SERVER_LOC "localhost"
#define USER "root"
#define PASS "criminalminds"
#define DATABASE "cse509"

//update flags
#define UPDATE_ALL              0
#define UPDATE_WHITELIST        1
#define UPDATE_BLACKLIST        2

typedef struct signatures{
unsigned int sig_count;
char * signatures;
}blacklist_from_db;

//Method to verify tables exist
extern int verify_tables(MYSQL* conn);

//Method to retrieve the sha256 hash of the file given the path
extern char* getsha256(char * file_path);

//method to retrieve the list of virus signatures
extern struct signatures* getstructures();

//method to update the virus signatures
extern int update_structures(unsigned int flags);

//method to compare the whitelist hashes
extern int isWhitelisted(char * file_path);

//method to check for (and create) antivirus tables
extern int verify_tables(MYSQL *conn);
#endif
