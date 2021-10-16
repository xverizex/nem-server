#pragma once

#include <json-c/json.h>

#define STATUS_REGISTER           1
#define STATUS_GET_LIST           2
#define STATUS_GET_ONLINE_LIST    3
#define STATUS_LOGIN              4
#define STATUS_HANDSHAKE          5
#define STATUS_MESSAGE            6
#define STATUS_FEED               7
#define STATUS_FILE_ADD           8
#define STATUS_STORAGE_FILES      9
#define STATUS_GET_FILE          10
void init_mysql ();
int mysql_registration_server (json_object *j);
void set_to_online_table (const char *ssl_ptr, const int id);
void unset_to_online_table (const char *ssl_ptr);
int mysql_get_person_id (json_object *obj);
void mysql_show_online_status (const int id, const int status);
void mysql_get_list_users (const char *ptr);
json_object *mysql_get_list_online_users (const char *ptr);
void mysql_show_online_status_ptr (const char *ptr, const int status);
int mysql_login_server (json_object *j);
int mysql_handshake (json_object *j);
json_object *mysql_handshake_to_user (const char *ptr, char *dt);
int mysql_check_message (json_object *j);
void mysql_send_message (char *ptr, char *dt);
void mysql_feed (const char *ptr);
int mysql_check_file_add (json_object *j);
void mysql_file_add (const char *ptr, const char *dt);
int mysql_check_storage_files (json_object *j);
void mysql_storage_files (const char *ptr, const char *dt);
int mysql_check_get_file (json_object *j);
void mysql_get_file (const char *ptr, const char *dt, int *is_closed, int *is_send_files);
