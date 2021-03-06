/*
 * Copyright Dmitrii Naidolinskii
 */
#include <mysql/mysql.h>
#include <stdio.h>
#include <json-c/json.h>
#include <string.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <sys/stat.h>
#include "db.h"

static MYSQL *mysql;
static MYSQL *mysql_second;

static int get_status_by_name (const char *name) {
	char query[512];
	snprintf (query, 512, "select * from online where name = '%s';",
			name
		 );
	mysql_query (mysql_second, query);
	MYSQL_RES *res = mysql_store_result (mysql_second);
	unsigned long long int num_fields = mysql_num_rows (res);
	mysql_free_result (res);

	if (num_fields > 0) return 1;
	return 0;
}

void mysql_get_list_users (const char *ptr) {
	const int NAME = 2;
	char *name;
	char query[512];
	snprintf (query, 512, "select * from online where ssl_ptr = %s;",
			ptr
		 );
	mysql_query (mysql, query);
	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	if (num_fields) {
		MYSQL_ROW row = mysql_fetch_row (res);
		snprintf (query, 512, "select * from acc where id != %s;",
				row[1]
			 );
		mysql_free_result (res);
		mysql_query (mysql, query);
	}



	res = mysql_store_result (mysql);
	num_fields = mysql_num_rows (res);

	SSL *ssl = (SSL *) atol (ptr);

	if (num_fields > 0) {
		for (int i = 0; i < num_fields; i++) {
			MYSQL_ROW row = mysql_fetch_row (res);
			json_object *jb = json_object_new_object ();
			json_object *jtype = json_object_new_string ("status_online");
			json_object_object_add (jb, "type", jtype);

			json_object *jname = json_object_new_string (row[1]);
			int status = get_status_by_name (row[1]);
			json_object *jstatus = json_object_new_int (status);
			json_object_object_add (jb, "name", jname);
			json_object_object_add (jb, "status", jstatus);
			const char *data = json_object_to_json_string_ext (jb, JSON_C_TO_STRING_PRETTY);
			SSL_write (ssl, data, strlen (data));
			json_object_put (jb);
		}
		mysql_free_result (res);
		return;
	}

	mysql_free_result (res);
	return;
}

static char *get_name_from_id (const int id) {
	const int NAME = 1;
	char *name;
	char query[512];
	snprintf (query, 512, "select * from acc where id = %d;",
			id
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	if (num_fields > 0) {
		MYSQL_ROW row = mysql_fetch_row (res);
		name = strdup (row[NAME]);
		mysql_free_result (res);
		return name;
	}

	mysql_free_result (res);

	return NULL;
}

static int is_online (const char *name) {
	char query[512];
	char t_name[68];
	mysql_escape_string (t_name, name, strlen (name));

	snprintf (query, 512, "select * from online where name = '%s';",
			t_name
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	mysql_free_result (res);

	if (num_fields > 0) {
		return 1; 
	}

	return 0;
}

static char *get_our_name (const char *ptr);

static json_object *build_json_message (const char *from, const char *to, const char *data) {
	json_object *obj = json_object_new_object ();
	json_object *jtype = json_object_new_string ("message");
	json_object *jfrom = json_object_new_string (from);
	json_object *jto = json_object_new_string (to);
	json_object *jdata = json_object_new_string (data);
	json_object_object_add (obj, "type", jtype);
	json_object_object_add (obj, "from", jfrom);
	json_object_object_add (obj, "to", jto);
	json_object_object_add (obj, "data", jdata);
	return obj;
}

void mysql_feed (const char *ptr) {
	const int NAME_TO = 1;
	const int NAME_FROM = 2;
	const int DATA = 3;
	char *our_name = get_our_name (ptr);
	char query[512];
	snprintf (query, 512, "select * from waitness where name_to = '%s';",
			our_name
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	SSL *ssl = (SSL *) atol (ptr);
	if (num_fields > 0) {
		for (int i = 0; i < num_fields; i++) {
			MYSQL_ROW row = mysql_fetch_row (res);
			json_object *jb = build_json_message (row[NAME_FROM], row[NAME_TO], row[DATA]);
			const char *data = json_object_to_json_string_ext (jb, JSON_C_TO_STRING_PRETTY);
			SSL_write (ssl, data, strlen (data));
			json_object_put (jb);
		}
	}

	mysql_free_result (res);

	snprintf (query, 512, "delete from waitness where name_to = '%s';",
			our_name
		 );
	mysql_query (mysql, query);
}

static void save_to_mysql (const char *our_name, const char *to_name, const char *data) {
	int len = 16384;
	char *query = malloc (len);
	int len_to_name = strlen (to_name);
	int len_data = strlen (data);
	char *cdata = malloc (len_data + 1);
	char *cto_name = malloc (len_to_name + 1);

	mysql_escape_string (cto_name, to_name, strlen (to_name));
	mysql_escape_string (cdata, data, strlen (data));

	snprintf (query, len, "insert into waitness (name_from, name_to, data) VALUES ('%s', '%s', '%s');",
			our_name,
			cto_name,
			cdata
		 );
	mysql_query (mysql, query);
	free (cdata);
	free (query);
	free (cto_name);
}

static char *get_ptr_from_name (const char *name) {
	char query[512];
	snprintf (query, 512, "select * from online where name = '%s';",
			name
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	if (num_fields > 0) {
		MYSQL_ROW row = mysql_fetch_row (res);
		char *ptr = strdup (row[3]);
		mysql_free_result (res);
		return ptr; 
	}

	mysql_free_result (res);
	return NULL;
}

static int mysql_add_file_to_table (const char *from, 
		const char *to, 
		const char *filename, 
		const char *data,
		const char *ckey,
		const char *ivec,
		const int is_start,
		const char *path_file)
{
	int len;
	int ll;
	len = strlen (to);
	len *= 2;
	char *cto = malloc (len + 1);
	if (!cto) return -1;
	ll = mysql_escape_string (cto, to, strlen (to));
	cto[ll] = 0;
	len = strlen (filename);
	len *= 2;
	char *cfilename = malloc (len + 1);
	if (!cfilename) { free (cto); return -1; }
	ll = mysql_escape_string (cfilename, filename, strlen (filename));
	cfilename[ll] = 0;
	len = strlen (data);
	len *= 2;
	char *cdata = malloc (len + 1);
	if (!cdata) {
		free (cto);
		free (cfilename);
		return -1;
	}
	ll = mysql_escape_string (cdata, data, strlen (data));
	cdata[ll] = 0;
	len = strlen (ckey);
	len *= 2;
	char *cckey = malloc (len + 1);
	if (!cckey) {
		free (cto);
		free (cfilename);
		free (cdata);
		return -1;
	}
	ll = mysql_escape_string (cckey, ckey, strlen (ckey));
	cckey[ll] = 0;
	len = strlen (ivec);
	len *= 2;
	char *civec = malloc (len + 1);
	if (!civec) {
		free (cto);
		free (cfilename);
		free (cdata);
		free (cckey);
		return -1;
	}
	ll = mysql_escape_string (civec, ivec, strlen (ivec));
	civec[ll] = 0;
	
	char query[16384];
	snprintf (query, 16384, "select * from storage where name_from = '%s' and name_to = '%s' and filename = '%s';",
			from,
			cto,
			cfilename
		 );
	mysql_query (mysql, query);
	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	mysql_free_result (res);
	int ret = 0;
	if (num_fields > 0) {
		ret = -1;
		if (is_start) {
			ret = 0;
			FILE *fp = fopen (path_file, "a");
			fwrite (cdata, 1, strlen (cdata), fp);
			fclose (fp);
		}
	} else {
		snprintf (query, 16384, "insert into storage (name_from, name_to, filename, data_path, ckey, ivec) "
				"values ('%s', '%s', '%s', '%s', '%s', '%s');",
				from,
				cto,
				cfilename,
				path_file,
				cckey,
				civec);
		mysql_query (mysql, query);
		FILE *fp = fopen (path_file, "w");
		fwrite (cdata, 1, strlen (cdata), fp);
		fclose (fp);
		ret = 0;
	}

	free (cdata);
	free (cfilename);
	free (cto);
	free (cckey);
	free (civec);

	return ret;
}

static json_object *json_build_storage_message (const char *from, 
		const char *filename,
		const char *data,
		const char *ckey,
		const char *ivec,
		size_t pos)
{
	char dt[16];
	snprintf (dt, 16, "%s", &data[pos]);
	json_object *obj = json_object_new_object ();
	json_object *jtype = json_object_new_string ("storage_files");
	json_object *jfrom = json_object_new_string (from);
	json_object *jfilename = json_object_new_string (filename);
	json_object *jckey = json_object_new_string (ckey);
	json_object *jivec = json_object_new_string (ivec);
	json_object_object_add (obj, "type", jtype);
	json_object_object_add (obj, "from", jfrom);
	json_object_object_add (obj, "filename", jfilename);
	json_object_object_add (obj, "ckey", jckey);
	json_object_object_add (obj, "ivec", jivec);
	return obj;
}

static int mysql_get_list_files (const char *to, 
		const char *from,
		const char *ptr)
{
	int len;
	int ll;
	len = strlen (to);
	len *= 2;
	char *cto = malloc (len + 1);
	if (!cto) return -1;
	ll = mysql_escape_string (cto, to, strlen (to));
	cto[ll] = 0;
	
	char query[16384];
	snprintf (query, 16384, "select * from storage where name_from = '%s' and name_to = '%s';",
			from,
			cto
		 );
	mysql_query (mysql, query);
	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	int ret = 0;
	if (num_fields > 0) {
		const int ID = 0;
		const int CKEY = 1;
		const int IVEC = 2;
		const int FROM = 3;
		const int TO = 4;
		const int FILENAME = 5;
		const int DATA = 6;
		SSL *ssl = (SSL *) atol (ptr);
		size_t pos = 0;
		for (int i = 0; i < num_fields; i++)
		{
			MYSQL_ROW row = mysql_fetch_row (res);
			json_object *jb = json_build_storage_message (row[FROM], row[FILENAME], row[DATA], row[CKEY], row[IVEC], pos);
			const char *data = json_object_to_json_string_ext (jb, JSON_C_TO_STRING_PRETTY);
			SSL_write (ssl, data, strlen (data));
			json_object_put (jb);
			pos += 16;
		}
	}
	
	mysql_free_result (res);

	free (cto);

	return ret;
}

static char *get_our_name (const char *ptr);

void mysql_storage_files (const char *ptr, const char *dt) {
	json_object *jb = json_tokener_parse (dt);
	json_object *jfrom = json_object_object_get (jb, "from");

	const char *from = json_object_get_string (jfrom);

	char *our_name = get_our_name (ptr);

	int res = mysql_get_list_files (our_name, from, ptr);

	free (our_name);
	json_object_put (jb);
}
void mysql_file_add (const char *ptr, const char *dt) {
	json_object *jb = json_tokener_parse (dt);
	json_object *jto = json_object_object_get (jb, "to");
	json_object *jdata = json_object_object_get (jb, "data");
	json_object *jfilename = json_object_object_get (jb, "filename");
	json_object *jis_start = json_object_object_get (jb, "is_start");
	json_object *jckey = json_object_object_get (jb, "ckey");
	json_object *jivec = json_object_object_get (jb, "ivec");

	const char *to_name = json_object_get_string (jto);
	const char *data = json_object_get_string (jdata);
	const char *filename = json_object_get_string (jfilename);
	const char *ckey = json_object_get_string (jckey);
	const char *ivec = json_object_get_string (jivec);
	const int is_start = json_object_get_int (jis_start);

	char *our_name = get_our_name (ptr);
	char *path_file = calloc (255, 1);

	if (strstr (to_name, "..")) goto end;
	if (strstr (our_name, "..")) goto end;
	if (strstr (filename, "..")) goto end;

	snprintf (path_file, 255, "files");
	mkdir (path_file, 0755);
	snprintf (path_file, 255, "files/%s", our_name);
	mkdir (path_file, 0755);
	snprintf (path_file, 255, "files/%s/%s", our_name, to_name);
	mkdir (path_file, 0755);
	snprintf (path_file, 255, "files/%s/%s/%s", our_name, to_name, filename);


	int res = mysql_add_file_to_table (our_name, to_name, filename, data, ckey, ivec, is_start, path_file);

end:
	free (path_file);
	free (our_name);
	json_object_put (jb);
}

struct dtf {
	SSL *ssl;
	char *data;
	char *filename;
	char *from;
	char *ckey;
	char *ivec;
	size_t pos;
	size_t size;
	int *is_closed;
	int *is_send_file;
	int index;
	char *dt;
};

static void build_and_send_json_file (struct dtf *dtf, int size) {
	json_object *jb = json_object_new_object ();
	json_object *jtype = json_object_new_string ("getting_file");
	json_object *jfrom = json_object_new_string (dtf->from);
	json_object *jpos = json_object_new_int64 (dtf->pos);
	json_object *jsize = json_object_new_int64 (dtf->size);
	json_object *jfilename = json_object_new_string (dtf->filename);
	json_object *jckey = json_object_new_string (dtf->ckey);
	json_object *jivec = json_object_new_string (dtf->ivec);
	json_object *jindex = json_object_new_int (dtf->index);
	json_object_object_add (jb, "type", jtype);
	json_object_object_add (jb, "from", jfrom);
	json_object_object_add (jb, "filename", jfilename);
	json_object_object_add (jb, "pos", jpos);
	json_object_object_add (jb, "size", jsize);
	json_object_object_add (jb, "ckey", jckey);
	json_object_object_add (jb, "ivec", jivec);
	json_object *jdata = json_object_new_string (dtf->dt);
	json_object_object_add (jb, "data", jdata);
	json_object_object_add (jb, "index", jindex);

	const char *dt = json_object_to_json_string_ext (jb, JSON_C_TO_STRING_PRETTY);
	SSL_write (dtf->ssl, dt, strlen (dt));
	json_object_put (jb);
}

static void *process_sending_file (void *data) {
	struct dtf *dtf = (struct dtf *) data;

	struct stat st;
	stat (dtf->data, &st);
	dtf->size = st.st_size;
	FILE *fp = fopen (dtf->data, "r");
	while (dtf->pos < dtf->size) {
		if (*(dtf->is_closed) == 1) break;
		int size = fread (dtf->dt, 1, 16 * 900, fp);
		if (size <= 0) break;
		build_and_send_json_file (dtf, size);
		dtf->pos += size;
	}
	fclose (fp);
	*(dtf->is_send_file) = 0;
	free (dtf->data);
	free (dtf->filename);
	free (dtf->from);
	free (dtf->dt);
	if (*(dtf->is_closed)) {
		free (dtf->is_closed);
		free (dtf->is_send_file);
	}
	free (dtf);
	return NULL;
}

void mysql_get_file (const char *ptr, const char *dt, int *is_closed, int *is_send_file) {
	json_object *jb = json_tokener_parse (dt);
	json_object *jfrom = json_object_object_get (jb, "from");
	json_object *jfilename = json_object_object_get (jb, "filename");
	json_object *jindex = json_object_object_get (jb, "index");
	const char *to_name = json_object_get_string (jfrom);
	const char *filename = json_object_get_string (jfilename);
	int index = json_object_get_int (jindex);
	char *our_name = get_our_name (ptr);

	SSL *ssl = (SSL *) atol (ptr);

	int len;
	int ll;
	len = strlen (to_name);
	len *= 2;
	char *cfrom = malloc (len + 1);
	if (!cfrom) return;
	ll = mysql_escape_string (cfrom, to_name, strlen (to_name));
	cfrom[ll] = 0;
	char *cfilename = malloc (len + 1);
	if (!cfilename) return;
	ll = mysql_escape_string (cfilename, filename, strlen (filename));
	cfilename[ll] = 0;
	
	char query[16384];
	snprintf (query, 16384, "select data_path,ckey,ivec from storage where name_from = '%s' and name_to = '%s' and filename = '%s';",
			cfrom,
			our_name,
			cfilename
		 );
	mysql_query (mysql, query);
	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);

	if (num_fields) {
		MYSQL_ROW row = mysql_fetch_row (res);
		struct dtf *dtf = calloc (1, sizeof (struct dtf));
		dtf->dt = calloc (16 * 900 + 1, 1);
		dtf->ssl = ssl;
		dtf->pos = 0L;
		dtf->size = strlen (row[0]);
		dtf->data = strdup (row[0]);
		dtf->filename = strdup (cfilename);
		dtf->from = strdup (cfrom);
		dtf->ckey = strdup (row[1]);
		dtf->ivec = strdup (row[2]);
		dtf->is_closed = is_closed;
		dtf->is_send_file = is_send_file;
		dtf->index = index;
		*(dtf->is_send_file) = 1;
		pthread_t t;
		pthread_create (&t, NULL, process_sending_file, dtf);
	}

	mysql_free_result (res);
	free (our_name);
	json_object_put (jb);
}
void mysql_send_message (char *ptr, char *dt) {
	json_object *jb = json_tokener_parse (dt);
	json_object *jto = json_object_object_get (jb, "to");
	json_object *jdata = json_object_object_get (jb, "data");
	const char *to_name = json_object_get_string (jto);
	const char *data = json_object_get_string (jdata);
	char *our_name = get_our_name (ptr);
	if (!is_online (to_name)) {
		save_to_mysql (our_name, to_name, data);
		free (our_name);
		return;
	}
	json_object *jour_name = json_object_new_string (our_name);
	json_object_object_del (jb, "to");
	json_object_object_add (jb, "from", jour_name);

	char *to_ptr = get_ptr_from_name (to_name);
	if (!to_ptr) {
		free (our_name);
		fprintf (stderr, "error mysql_send_message > get_ptr_from_name\n");
		return;
	}

	SSL *ssl = (SSL *) atol (to_ptr);
	data = json_object_to_json_string_ext (jb, JSON_C_TO_STRING_PRETTY);
	SSL_write (ssl, data, strlen (data));

	free (our_name);
	json_object_put (jb);
}

int mysql_check_get_file (json_object *jb) {
	json_object *jto = json_object_object_get (jb, "from");
	json_object *jfilename = json_object_object_get (jb, "filename");
	if (!jto || !jfilename) {
		return 0;
	}

	const char *to = json_object_get_string (jto);
	const char *filename = json_object_get_string (jfilename);

	if (strlen (to) > 16) {
		return 0;
	}
	if (strlen (filename) > 256) {
		return 0;
	}

	return STATUS_GET_FILE;
}
int mysql_check_file_add (json_object *jb) {
	json_object *jto = json_object_object_get (jb, "to");
	json_object *jfilename = json_object_object_get (jb, "filename");
	json_object *jdata = json_object_object_get (jb, "data");
	json_object *jckey = json_object_object_get (jb, "ckey");
	json_object *jivec = json_object_object_get (jb, "ivec");
	json_object *jis_start = json_object_object_get (jb, "is_start");
	if (!jto || !jfilename || !jdata || !jckey || !jivec || !jis_start) {
		return 0;
	}

	const char *to = json_object_get_string (jto);
	const char *filename = json_object_get_string (jfilename);
	const char *data = json_object_get_string (jdata);

	if (strlen (to) > 16) {
		return 0;
	}
	if (strlen (filename) > 256) {
		return 0;
	}

	return STATUS_FILE_ADD;
}

int mysql_check_storage_files (json_object *jb) {
	json_object *jfrom = json_object_object_get (jb, "from");
	if (!jfrom) {
		return 0;
	}

	const char *from = json_object_get_string (jfrom);

	if (strlen (from) > 16) {
		return 0;
	}

	return STATUS_STORAGE_FILES;
}

int mysql_check_message (json_object *jb) {
	json_object *jto = json_object_object_get (jb, "to");
	json_object *jdata = json_object_object_get (jb, "data");

	const char *to = json_object_get_string (jto);
	const char *data = json_object_get_string (jdata);

	if (!to || !data) return 0;

	if (strlen (to) > 16) return 0;
	if (strlen (data) <= 0) return 0;

	return STATUS_MESSAGE;
}

void unset_to_online_table (const char *ptr) {
	char query[512];
	snprintf (query, 512, "delete from online where ssl_ptr = %s;",
			ptr
		 );
	mysql_query (mysql, query);
	snprintf (query, 512, "delete from handshake where ssl_ptr = %s;",
			ptr
		 );
	mysql_query (mysql, query);
}

void set_to_online_table (const char *ptr, const int id) {
	char *name = get_name_from_id (id);
	char query[512];
	snprintf (query, 512, "insert into online (id_person, name, ssl_ptr) VALUES (%d, '%s', %s);",
			id,
			name,
			ptr
		 );
	mysql_query (mysql, query);
	free (name);
}

static json_object *get_json_status (char *name, const int status) {
	json_object *jb = json_object_new_object ();
	json_object *jtype = json_object_new_string ("status_online");
	json_object *jstatus = json_object_new_int (status);
	json_object *jname = json_object_new_string (name);

	json_object_object_add (jb, "type", jtype);
	json_object_object_add (jb, "status", jstatus);
	json_object_object_add (jb, "name", jname);

	return jb;
}

void mysql_show_online_status_ptr (const char *ptr, const int status) {
	const int SSL_PTR = 3;
	const int NAME = 2;
	char query[512];
	snprintf (query, 512, "select * from online where ssl_ptr = %s;",
			ptr
		 );
	mysql_query (mysql, query);
	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	if (num_fields == 0) {
		mysql_free_result (res);
		return;
	}
	MYSQL_ROW row = mysql_fetch_row (res);
	json_object *jbuf = get_json_status (row[NAME], status);
	mysql_free_result (res);

	snprintf (query, 512, "select * from online where ssl_ptr != %s;",
			ptr
		 );
	mysql_query (mysql, query);
	res = mysql_store_result (mysql);

	num_fields = mysql_num_rows (res);
	if (num_fields > 0) {
		const char *buf = json_object_to_json_string_ext (jbuf, JSON_C_TO_STRING_PRETTY);
		for (int i = 0; i < num_fields; i++) {
			row = mysql_fetch_row (res);
			SSL *ssl = (SSL *) atol (row[SSL_PTR]);
			int ret = SSL_write (ssl, buf, strlen (buf));
		}
		json_object_put (jbuf);
	}

	mysql_free_result (res);
}
void mysql_show_online_status (const int id, const int status) {
	const int SSL_PTR = 3;
	char query[512];
	snprintf (query, 512, "select * from online where id_person != %d;",
			id
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);
	if (num_fields > 0) {
		char *name = get_name_from_id (id);
		json_object *jbuf = get_json_status (name, status);
		free (name);
		MYSQL_ROW row = mysql_fetch_row (res);
		const char *buf = json_object_to_json_string_ext (jbuf, JSON_C_TO_STRING_PRETTY);
		for (int i = 0; i < num_fields; i++) {
			SSL *ssl = (SSL *) atol (row[SSL_PTR]);
			int ret = SSL_write (ssl, buf, strlen (buf));
		}
		json_object_put (jbuf);
	}

	mysql_free_result (res);
}

int mysql_get_person_id (json_object *j) {
	json_object *jlogin = json_object_object_get (j, "login");
	const char *login = json_object_get_string (jlogin);

	char to_login[68];

	char query[512];
	mysql_escape_string (to_login, login, strlen (login));

	snprintf (query, 512, "select id from acc where name = '%s' limit 1;", to_login);
	int ret = mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);

	unsigned long long int num_fields = mysql_num_rows (res);
	if (num_fields <= 0) {
		return -1;
		mysql_free_result (res);
	}

	MYSQL_ROW row = mysql_fetch_row (res);

	int id = atoi (row[0]);

	mysql_free_result (res);

	return id;
}

int mysql_handshake (json_object *j) {
	json_object *jto_name = json_object_object_get (j, "to_name");
	json_object *jstatus = json_object_object_get (j, "status");
	json_object *jkey = json_object_object_get (j, "key");

	if (!jto_name || !jstatus) return -1;

	if (!json_object_is_type (jto_name, json_type_string)) {
		return -1;
	}
	if (!json_object_is_type (jstatus, json_type_int)) {
		return -1;
	}

	const char *to_name = json_object_get_string (jto_name);
	const int status = json_object_get_int (jstatus);

	if (status) {
		if (jkey == NULL) return -1;
		if (!json_object_is_type (jkey, json_type_string)) {
			return -1;
		}
		const char *key = json_object_get_string (jkey);
		if (strlen (key) >= 2048) return -1;
	}

	if (strlen (to_name) > 16) return -1;
	if (status < 0) return -1;

	return STATUS_HANDSHAKE;
}

static void get_json_handshake_data_status (char *dt, char to_name[17], int *status) {
	json_object *jd = json_tokener_parse (dt);
	json_object *jto_name = json_object_object_get (jd, "to_name");
	json_object *jstatus = json_object_object_get (jd, "status");
	const char *to_name_c = json_object_get_string (jto_name);
	*status = json_object_get_int (jstatus);
	strncpy (to_name, to_name_c, strlen (to_name_c) + 1);
	json_object_put (jd);
}

static int found_this_same_handshake_request (const char *ptr, char to_name[17], int status) {
	char to_name_c[68];

	char query[512];
	mysql_escape_string (to_name_c, to_name, strlen (to_name));

	snprintf (query, 512, "select * from handshake where to_name = '%s' and ssl_ptr = %s;", to_name_c, ptr);
	mysql_query (mysql, query);
	MYSQL_RES *res = mysql_store_result (mysql);

	unsigned long long int num_fields = mysql_num_rows (res);

	mysql_free_result (res);

	if (num_fields) return 1;
	return 0;
}

json_object *json_build_handshake_string (char *to_name, const int status) {
	json_object *jb = json_object_new_object ();
	json_object *jstatus = json_object_new_int (status);
	json_object *jname = json_object_new_string (to_name);
	json_object *jtype = json_object_new_string ("handshake_answer");
	json_object_object_add (jb, "type", jtype);
	json_object_object_add (jb, "status_handshake", jstatus);
	json_object_object_add (jb, "to_name", jname);
	return jb;
}

static void delete_record_from_handshake (const char *ptr, 
		char *to_name)
{
	char query[512];
	printf ("delete record to_name: %s\n", to_name);
	snprintf (query, 512, "delete from handshake where ssl_ptr = %s and to_name = '%s';",
			ptr,
			to_name
		 );
	mysql_query (mysql, query);
}

static void delete_record_handshake (const char *ptr, 
		char *our_name,
		char *to_name_ptr,
		char *to_name) {
	char query[512];
	snprintf (query, 512, "delete from handshake where ssl_ptr = %s and to_name = '%s';",
			ptr,
			to_name
		 );
	mysql_query (mysql, query);
	snprintf (query, 512, "delete from handshake where ssl_ptr = %s and to_name = '%s';",
			to_name_ptr,
			our_name
		 );
	mysql_query (mysql, query);
}

static int made_record_handshake (const char *ptr, char *dt) {
	json_object *jb = json_tokener_parse (dt);
	json_object *jto_name = json_object_object_get (jb, "to_name");
	json_object *jkey_pem = json_object_object_get (jb, "key");
	char key_pem[2048];
	char name[68];

	const char *to_name = json_object_get_string (jto_name);
	const char *key = json_object_get_string (jkey_pem);

	mysql_escape_string (name, to_name, strlen (to_name));
	mysql_escape_string (key_pem, key, strlen (key));

	const int len = 2048 + 68 + 512;
	char query[len];
	snprintf (query, len, "insert into handshake (ssl_ptr, to_name, key_pem) VALUES (%s, '%s', '%s');",
			ptr,
			name,
			key_pem);
	mysql_query (mysql, query);
	return 1;
}

static char *get_our_name (const char *ptr) {
	char query[512];
	snprintf (query, 512, "select * from online where ssl_ptr = %s;",
			ptr
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num = mysql_num_rows (res);
	if (num == 0) {
		mysql_free_result (res);
		return NULL;
	}

	MYSQL_ROW row = mysql_fetch_row (res);

	const int NAME = 2;

	char *name = calloc (17, 1);
	strncpy (name, row[NAME], strlen (row[NAME]) + 1);

	mysql_free_result (res);

	return name;
}

static int to_name_to_same_req_handshake (char *to_name, const char *ptr, char *to_name_ptr, char **our_name) {
	char query[512];
	snprintf (query, 512, "select * from online where name = '%s';",
			to_name
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num = mysql_num_rows (res);
	if (num == 0) {
		mysql_free_result (res);
		return 0;
	}

	const int SSL_PTR = 3;

	MYSQL_ROW row = mysql_fetch_row (res);

	strncpy (to_name_ptr, row[SSL_PTR], strlen (row[SSL_PTR]) + 1);

	mysql_free_result (res);

	*our_name = get_our_name (ptr);
	if (!*our_name) return 0;

	snprintf (query, 512, "select * from handshake where ssl_ptr = %s and to_name = '%s';",
			to_name_ptr,
			*our_name
		 );
	mysql_query (mysql, query);

	res = mysql_store_result (mysql);
	num = mysql_num_rows (res);
	if (num == 0) {
		mysql_free_result (res);
		return 0;
	}

	mysql_free_result (res);

	return 1;
}

static char *get_key_of (const char *ssl_ptr, char *name) {
	char query[512];
	snprintf (query, 512, "select * from handshake where ssl_ptr = %s and to_name = '%s';",
			ssl_ptr,
			name
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num = mysql_num_rows (res);
	if (num == 0) {
		mysql_free_result (res);
		return NULL;
	}

	MYSQL_ROW row = mysql_fetch_row (res);
	const int KEY = 3;
	char *key = strdup (row[KEY]);
	mysql_free_result (res);

	return key;
}

static json_object *build_json_exchange_key (char *name, char *key) {
	json_object *jb = json_object_new_object ();
	json_object *jtype = json_object_new_string ("handshake_key");
	json_object *jfrom = json_object_new_string (name);
	json_object *jkey = json_object_new_string (key);

	json_object_object_add (jb, "type", jtype);
	json_object_object_add (jb, "from", jfrom);
	json_object_object_add (jb, "key", jkey);
	return jb;
}

static int exchange_keys_pem (char *our_name, char *to_name, const char *ptr, char *to_name_ptr) {
	char *key_ptr = get_key_of (ptr, to_name);
	char *key_to_name_ptr = get_key_of (to_name_ptr, our_name);

	if (!key_ptr) {
		free (key_to_name_ptr);
		return -1;
	}
	if (!key_to_name_ptr) {
		free (key_ptr);
		return -1;
	}

	json_object *jkey_ptr_packet = build_json_exchange_key (our_name, key_ptr);
	json_object *jkey_to_name_ptr_packet = build_json_exchange_key (to_name, key_to_name_ptr);

	const char *buffer_key_ptr = json_object_to_json_string_ext (jkey_ptr_packet, JSON_C_TO_STRING_PRETTY);
	const char *buffer_key_to_name_ptr = json_object_to_json_string_ext (jkey_to_name_ptr_packet, JSON_C_TO_STRING_PRETTY);

	SSL *ssl_ptr = (SSL *) atol (ptr);
	SSL *ssl_to_name_ptr = (SSL *) atol (to_name_ptr);

	SSL_write (ssl_ptr, buffer_key_to_name_ptr, strlen (buffer_key_to_name_ptr));
	SSL_write (ssl_to_name_ptr, buffer_key_ptr, strlen (buffer_key_ptr));

	free (key_ptr);
	free (key_to_name_ptr);
	json_object_put (jkey_ptr_packet);
	json_object_put (jkey_to_name_ptr_packet);
}

static json_object *build_json_handshake_notice (const char *our_name, const int status) {
	json_object *ob = json_object_new_object ();
	json_object *type = json_object_new_string ("handshake_notice");
	json_object *name = json_object_new_string (our_name);
	json_object *jstatus = json_object_new_int (status);
	json_object_object_add (ob, "type", type);
	json_object_object_add (ob, "from", name);
	json_object_object_add (ob, "status", jstatus);
	return ob;
}

static void notice_to_name_ptr_about_handshake (const char *to_name_ptr, const char *our_name, const int status) {
	json_object *obj = build_json_handshake_notice (our_name, status);
	const char *data = json_object_to_json_string_ext (obj, JSON_C_TO_STRING_PRETTY);
	SSL *ssl = (SSL *) atol (to_name_ptr);
	SSL_write (ssl, data, strlen (data));
	json_object_put (obj);
}

static char *get_to_name_ptr (const char *to_name) {
	char query[512];
	snprintf (query, 512, "select * from online where name = '%s';",
			to_name
		 );
	mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num = mysql_num_rows (res);
	if (num == 0) {
		mysql_free_result (res);
		return NULL;
	}

	MYSQL_ROW row = mysql_fetch_row (res);
	const int SSL_PTR = 3;
	char *name = strdup (row[SSL_PTR]);
	mysql_free_result (res);

	return name;
}

json_object *mysql_handshake_to_user (const char *ptr, char *dt) {
	char to_name[17];
	int status = 0;
	get_json_handshake_data_status (dt, to_name, &status);
	if (status == 0) {
		delete_record_from_handshake (ptr, to_name);
		char *our_name = get_our_name (ptr);
		char *to_name_ptr = get_to_name_ptr (to_name);
		if (to_name_ptr) {
			notice_to_name_ptr_about_handshake (to_name_ptr, our_name, status);
			free (to_name_ptr);
		} else {
			free (our_name);
			return json_build_handshake_string (to_name, 0);
		}
		free (our_name);
		return json_build_handshake_string (to_name, 1);
	}
	if (found_this_same_handshake_request (ptr, to_name, status))
		return json_build_handshake_string (to_name, 0);

	if (made_record_handshake (ptr, dt)) {
//		return json_build_handshake_string (to_name, 1);

		char to_name_ptr[64];
		char *our_name = NULL;
		if (to_name_to_same_req_handshake (to_name, ptr, to_name_ptr, &our_name)) {
			int ret = exchange_keys_pem (our_name, to_name, ptr, to_name_ptr);
			delete_record_handshake (ptr, our_name, to_name_ptr, to_name);
			free (our_name);
			if (ret == -1) return json_build_handshake_string (to_name, 0);
			return json_build_handshake_string (to_name, 1);
		}
		if (our_name) {
			notice_to_name_ptr_about_handshake (to_name_ptr, our_name, status);
			free (our_name);
		}

	}

	return json_build_handshake_string (to_name, 0);
}

int mysql_login_server (json_object *j) {
	json_object *jlogin = json_object_object_get (j, "login");
	json_object *jpassword = json_object_object_get (j, "password");

	if (!jlogin || !jpassword) return -1;

	if (!json_object_is_type (jlogin, json_type_string)) {
		return -1;
	}
	if (!json_object_is_type (jpassword, json_type_string)) {
		return -1;
	}

	const char *login = json_object_get_string (jlogin);
	const char *password = json_object_get_string (jpassword);

	if (strlen (login) > 16) return -1;
	if (strlen (password) > 16) return -1;

	char to_login[68];
	char to_password[68];

	char query[512];
	mysql_escape_string (to_login, login, strlen (login));
	mysql_escape_string (to_password, password, strlen (password));

	snprintf (query, 512, "select * from acc where name = '%s' and password = '%s';", to_login, to_password);
	int ret = mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);

	mysql_free_result (res);

	if (num_fields == 0) return -1;
	return STATUS_LOGIN;
}
int mysql_registration_server (json_object *j) {
	json_object *jlogin = json_object_object_get (j, "login");
	json_object *jpassword = json_object_object_get (j, "password");

	if (!jlogin || !jpassword) return -1;

	if (!json_object_is_type (jlogin, json_type_string)) {
		return -1;
	}
	if (!json_object_is_type (jpassword, json_type_string)) {
		return -1;
	}

	const char *login = json_object_get_string (jlogin);
	const char *password = json_object_get_string (jpassword);

	if (strlen (login) > 16) return -1;
	if (strlen (password) > 16) return -1;

	char to_login[68];
	char to_password[68];

	char query[512];
	mysql_escape_string (to_login, login, strlen (login));
	mysql_escape_string (to_password, password, strlen (password));

	snprintf (query, 512, "select * from acc where name = '%s';", to_login);
	int ret = mysql_query (mysql, query);

	MYSQL_RES *res = mysql_store_result (mysql);
	unsigned long long int num_fields = mysql_num_rows (res);

	mysql_free_result (res);

	if (num_fields == 0) {
		snprintf (query, 512, "insert into acc (name, password) VALUES ('%s', '%s');",
				to_login,
				to_password
			 );
		mysql_query (mysql, query);
		return STATUS_REGISTER;
	} else {
		return -1;
	}	
}

void init_mysql () {
	mysql = mysql_init (NULL);
	mysql_second = mysql_init (NULL);

	mysql_real_connect (mysql,
			"localhost",
			"chat",
			"chat",
			"chat",
			0,
			NULL,
			0);

	mysql_real_connect (mysql_second,
			"localhost",
			"chat",
			"chat",
			"chat",
			0,
			NULL,
			0);

	char buf[512];
	snprintf (buf, 512, "delete from online;");
	mysql_query (mysql, buf);
	snprintf (buf, 512, "delete from handshake;");
	mysql_query (mysql, buf);
}


