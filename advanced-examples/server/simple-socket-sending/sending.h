
/**
	Header file for Sending
*/
#pragma once

#ifndef SENDING
#define SENDING

#include <stdarg.h>
#include <libwebsockets.h>
#include <glib.h>
#include <time.h>

typedef enum Message_type
{
	SEND_STORE,
	SEND_ERASE,
	SEND_HAVE_ONE
}Message_type;

int send_global_data(struct lws_context *context,struct lws_protocols *protocol,const char *memory,Message_type mtype,const char *data,size_t len);
int send_global_message_v(struct lws_context *context,struct lws_protocols *protocol,const char *memory,Message_type mtype,const char *data,va_list args);
int send_global_message(struct lws_context *context,struct lws_protocols *protocol,const char *memory,Message_type mtype,const char *data,...);
int init_user(struct lws *wsi,void *user_id,void *user);
int deinit_user(struct lws *wsi);
ssize_t number_of_connected_users();
int send_user_data(struct lws *wsi,const char *memory,Message_type mtype,const char *data,size_t len);
int send_user_message_v(struct lws *wsi,const char *memory,Message_type mtype,const char *data,va_list args);
int send_user_message(struct lws *wsi,const char *memory,Message_type mtype,const char *data,...);
int send_global_data_from_id(void *user_id,const char *memory,Message_type mtype,const char *data,size_t len);
int send_global_message_from_id_v(void *user_id,const char *memory,Message_type mtype,const char *data,va_list args);
int send_global_message_from_id(void *user_id,const char *memory,Message_type mtype,const char *data,...);
int send_messages(struct lws *wsi,enum lws_callback_reasons reason,void *user,void *in,size_t len);

#endif
