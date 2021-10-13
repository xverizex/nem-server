/*
 * Copyright Dmitrii Naidolinskii
 */ 
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <json-c/json.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "db.h"

#define EPOLL_SIZE         1024
#define FOREVER            1
#define DT_SIZE            4096

static int ep;
static struct epoll_event ev, events[EPOLL_SIZE];
static pthread_t p1;
static int nfds;

struct data_client {
	SSL *ssl;
	SSL_CTX *ctx;
	int client;
};

static int parse (char *dt, int *id) {
	json_object *jobj = NULL;
	int stringlen = 0;
	enum json_tokener_error jerr;
	json_tokener *tok = json_tokener_new ();
	stringlen = strlen (dt);
	jobj = json_tokener_parse_ex (tok, dt, stringlen);
	jerr = json_tokener_get_error (tok);

	if (jerr != json_tokener_success) {
		fprintf (stderr, "Error tok: %s\n", json_tokener_error_desc (jerr));
		return -1;
	}

	json_object *jtype = json_object_object_get (jobj, "type");

	if (!json_object_is_type (jtype, json_type_string)) {
		json_object_put (jobj);
		fprintf (stderr, "damn. type is not string.\n");
		return -1;
	}

	const char *type = json_object_get_string (jtype);
	if (!strncmp (type, "register", 9)) {
		int ret;
		if ((ret = mysql_registration_server (jobj)) == 0) {
			json_object_put (jobj);
			return -1;
		}
		*id = mysql_get_person_id (jobj);
		json_object_put (jobj);
		return ret;
	}

	if (!strncmp (type, "login", 6)) {
		int ret;
		if ((ret = mysql_login_server (jobj)) == 0) {
			json_object_put (jobj);
			return -1;
		}
		*id = mysql_get_person_id (jobj);
		json_object_put (jobj);
		return ret;
	}

	if (!strncmp (type, "get_list", 9)) {
		return STATUS_GET_LIST;
	}

	if (!strncmp (type, "handshake", 10)) {
		int ret;
		if ((ret = mysql_handshake (jobj)) == 0) {
			json_object_put (jobj);
			return -1;
		}
		json_object_put (jobj);
		return ret;
	}
	if (!strncmp (type, "message", 8)) {
		int ret;
		if ((ret = mysql_check_message (jobj)) == 0) {
			json_object_put (jobj);
			return -1;
		}
		json_object_put (jobj);
		return ret;
	}
	if (!strncmp (type, "feed", 5)) return STATUS_FEED;

	if (!strncmp (type, "file_add", 9)) {
		int ret;
		if ((ret = mysql_check_file_add (jobj)) == 0) {
			json_object_put (jobj);
			return -1;
		}
		json_object_put (jobj);
		return ret;
	}
	if (!strncmp (type, "storage_files", 14)) {
		int ret;
		if ((ret = mysql_check_storage_files (jobj)) == 0) {
			json_object_put (jobj);
			return -1;
		}
		json_object_put (jobj);
		return ret;
	}
	if (!strncmp (type, "get_file", 9)) {
		int ret;
		if ((ret = mysql_check_get_file (jobj)) == 0) {
			json_object_put (jobj);
			return -1;
		}
		json_object_put (jobj);
		return ret;
	}

	return -1;
}

static json_object *get_json_buf_ok () {
	json_object *jo = json_object_new_object ();
	json_object *jstatus = json_object_new_string ("ok");
	json_object_object_add (jo, "status", jstatus);
	json_object *jtype = json_object_new_string ("result");
	json_object_object_add (jo, "type", jtype);
	return jo;
}

static json_object *get_json_buf_false () {
	json_object *jo = json_object_new_object ();
	json_object *jstatus = json_object_new_string ("false");
	json_object_object_add (jo, "status", jstatus);
	json_object *jtype = json_object_new_string ("result");
	json_object_object_add (jo, "type", jtype);
	return jo;
}

static void *handler_clients_cb (void *data) {
	char *dt = malloc (DT_SIZE);
	while (FOREVER) {
		nfds = epoll_wait (ep, events, EPOLL_SIZE, -1);
		if (nfds == -1) {
			perror ("epoll_wait");
			exit (EXIT_FAILURE);
		}

		for (int i = 0; i < nfds; i++) {
			struct data_client *dc = events[i].data.ptr;
			int size = SSL_read (dc->ssl, dt, DT_SIZE);
			printf ("readed: %d\n", size);
			if (size <= 0) {
				char ptr[64];
				snprintf (ptr, 64, "%lld", dc->ssl);
				mysql_show_online_status_ptr (ptr, 0);
				unset_to_online_table (ptr);
				SSL_free (dc->ssl);
				int ret = epoll_ctl (ep, EPOLL_CTL_DEL, dc->client, &ev);
				if (ret == -1) {
					perror ("epoll_ctl del client");
				}
				close (dc->client);
				free (dc);
				continue;
			}
			dt[size] = 0;
			printf ("%s\n", dt);
			int ret;
			int id = 0;
			if ((ret = parse (dt, &id)) == -1) {
				printf ("error parse\n");
				json_object *buf_false = get_json_buf_false ();
				const char *buf = json_object_to_json_string_ext (buf_false, JSON_C_TO_STRING_PRETTY);
				SSL_write (dc->ssl, buf, strlen (buf));
				json_object_put (buf_false);
				if (ret != STATUS_REGISTER) {
					char ptr[64];
					snprintf (ptr, 64, "%lld", dc->ssl);
					mysql_show_online_status_ptr (ptr, 0);
					unset_to_online_table (ptr);
				}
				SSL_free (dc->ssl);
				int ret = epoll_ctl (ep, EPOLL_CTL_DEL, dc->client, &ev);
				if (ret == -1) {
					perror ("epoll_ctl del client");
				}
				close (dc->client);
				free (dc);
				continue;
			}
			printf ("status: %d\n", ret);
			switch (ret) {
				case STATUS_REGISTER: 
				case STATUS_LOGIN:
					{
						if (id < 0) {
							json_object *buf_false = get_json_buf_false ();
							const char *buf = json_object_to_json_string_ext (buf_false, JSON_C_TO_STRING_PRETTY);
							SSL_write (dc->ssl, buf, strlen (buf));
							json_object_put (buf_false);
							SSL_free (dc->ssl);
							int ret = epoll_ctl (ep, EPOLL_CTL_DEL, dc->client, &ev);
							if (ret == -1) {
								perror ("epoll_ctl del client");
							}
							close (dc->client);
							free (dc);
							continue;
						}
						json_object *buf_ok = get_json_buf_ok ();
						const char *buf = json_object_to_json_string_ext (buf_ok, JSON_C_TO_STRING_PRETTY);
						SSL_write (dc->ssl, buf, strlen (buf));
						json_object_put (buf_ok);

						/* get ssl ptr */
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						set_to_online_table (ptr, id);
						mysql_show_online_status (id, 1);
					}
					break;
				case STATUS_GET_LIST:
					{
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						json_object *jb = mysql_get_list_users (ptr);
						const char *buffer = json_object_to_json_string_ext (jb, JSON_C_TO_STRING_PRETTY);
						SSL_write (dc->ssl, buffer, strlen (buffer));
						json_object_put (jb);
					}
					break;
				case STATUS_HANDSHAKE:
					{
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						json_object *jb = mysql_handshake_to_user (ptr, dt);
						const char *buffer = json_object_to_json_string_ext (jb, JSON_C_TO_STRING_PRETTY);
						SSL_write (dc->ssl, buffer, strlen (buffer));
						json_object_put (jb);
					}
					break;
				case STATUS_MESSAGE:
					{
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						mysql_send_message (ptr, dt);
					}
					break;
				case STATUS_FEED:
					{
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						mysql_feed (ptr);
					}
					break;
				case STATUS_FILE_ADD:
					{
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						mysql_file_add (ptr, dt);
					}
					break;
				case STATUS_STORAGE_FILES:
					{
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						mysql_storage_files (ptr, dt);
					}
					break;
				case STATUS_GET_FILE:
					{
						char ptr[64];
						snprintf (ptr, 64, "%lld", dc->ssl);
						mysql_get_file (ptr, dt);
					}
					break;
			}
		}
	}
}

static int init_socket () {
	int ret, sock;
	ret = sock = socket (AF_INET, SOCK_STREAM, 0);
	if (ret == -1) {
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	int opt_val = 1;
	ret = setsockopt (sock, SOL_SOCKET, SO_REUSEPORT, &opt_val, sizeof (opt_val));
	if (ret == -1) {
		perror ("setsockopt reuseport");
		exit (EXIT_FAILURE);
	}

	struct sockaddr_in s;
	s.sin_family = AF_INET;
	s.sin_port = htons (12345);
	inet_aton ("0.0.0.0", &s.sin_addr);

	ret = bind (sock, (const struct sockaddr *) &s, sizeof (s));
	if (ret == -1) {
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	listen (sock, 0);

	return sock;
}


static void create_epoll_and_thread (const int sock) {
	ep = epoll_create1 (0);
	pthread_create (&p1, NULL, handler_clients_cb, NULL);
}


static void init_ssl (struct data_client *dc, const int sock) {
	dc->ssl = SSL_new (dc->ctx);
	int ret = SSL_set_fd (dc->ssl, sock);
}

static void clear_ssl_dc (struct data_client *dc) {
	SSL_free (dc->ssl);
}

static SSL_CTX *get_ctx () {
	SSL_CTX *ctx = SSL_CTX_new (SSLv23_server_method ());

	if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	return ctx;
}

static void loop_handler (const int sock) {

	SSL_CTX *ctx = get_ctx ();

	while (FOREVER) {
		struct sockaddr_in cl;
		int ret, client;
		socklen_t sl = sizeof (cl);
		ret = client = accept (sock, (struct sockaddr *) &cl, &sl);
		if (ret == -1) {
			perror ("accept");
			sleep (1);
			continue;
		}
		struct data_client *dc = calloc (1, sizeof (struct data_client));
		if (!dc) {
			close (client);
			continue;
		}
		dc->ctx = ctx;
		dc->client = client;
		fd_set rfds;
		struct timeval tv;
		int retval;
		init_ssl (dc, client);
		FD_ZERO (&rfds);
		FD_SET (client, &rfds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		char buf[10];
		retval = select (client + 1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			perror ("select");
			clear_ssl_dc (dc);
			close (client);
			free (dc);
			continue;
		} else if (retval) {
			ssize_t rl = recv (client, buf, 10, MSG_PEEK);
			if (rl < 6) {
				clear_ssl_dc (dc);
				close (client);
				free (dc);
				continue;
			}
			FD_ISSET (0, &rfds);
			ret = SSL_accept (dc->ssl);
			if (ret <= 0) {
				fprintf (stderr, "ssl accept error: %d\n", SSL_get_error (dc->ssl, ret));
				clear_ssl_dc (dc);
				close (client);
				free (dc);
				continue;
			}
		} else {
			clear_ssl_dc (dc);
			close (client);
			free (dc);
			continue;
		}

		ev.events = EPOLLIN;
		ev.data.ptr = dc;

		ret = epoll_ctl (ep, EPOLL_CTL_ADD, client, &ev);
		if (ret == -1) {
			perror ("epoll_ctl add client");
			clear_ssl_dc (dc);
			close (client);
			free (dc);
		}
	}
}

static void init_ssl_library () {
	SSL_load_error_strings();
    	OpenSSL_add_ssl_algorithms();
}

void handler_signal (int sig) {
}

int main (int argc, char **argv) {
	signal (SIGPIPE, SIG_IGN);

	init_mysql ();
	init_ssl_library ();
	int sock = init_socket ();
	create_epoll_and_thread (sock);
	loop_handler (sock);
}
