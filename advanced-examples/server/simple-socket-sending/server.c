#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>

#include "sending.h"

#define EXAMPLE_RX_BUFFER_BYTES (100)
const char *GLOBAL_USERNAME="ADMIN";
char GLOBAL_GUESS_MEMORY='\0';

static int callback_http( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_HTTP:
		{
			int s=lws_serve_http_file( wsi, "../../../example.html", "text/html", NULL, 0 );
			
			if (s < 0 || ((s > 0) && lws_http_transaction_completed(wsi)))
			{
				return -1;
			}
			break;
		}
		case LWS_CALLBACK_HTTP_FILE_COMPLETION:
			if (lws_http_transaction_completed(wsi))
			{
				return -1;
			}
		default:
			break;
	}

	return 0;
}

static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	printf("USER:: %p WSI:: %p REASON:: %d\n",user,wsi,reason);

	switch( reason )
	{
		case LWS_CALLBACK_ESTABLISHED:
			init_user(wsi,NULL,wsi);
			send_user_message(wsi, "C%-10s%s",GLOBAL_USERNAME,"Hello and welcome!");
		break;
		case LWS_CALLBACK_CLOSED:
			deinit_user(wsi);
		break;
		case LWS_CALLBACK_RECEIVE:
		{
			char *input=(char*)in;
			
			//for set and get
			if(len==2)
			{
				if(input[0]=='G')
				{
					//compare the box character (num)
					if(input[1]==GLOBAL_GUESS_MEMORY)
					{
						send_global_message(wsi, "C%-10s%s",GLOBAL_USERNAME,"CORRECT GUESS!");
					}
					else
					{
						send_global_message(wsi, "C%-10s%s",GLOBAL_USERNAME,"WRONG GUESS!");
					}
					
					return 0;
				}
				else if(input[0]=='S')
				{
					GLOBAL_GUESS_MEMORY=input[1];
					
					send_global_message(wsi, "C%-10s%s",GLOBAL_USERNAME,"BOX IS SET!");
					
					return 0;
				}
			}
			else if(input[0]=='C')
			{
				send_global_data(wsi, strndup(input,len),len,TRUE);
			}
			break;
		}
		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			send_messages(wsi,reason,user,in,len);
			break;
		}
		default:
			break;
	}

	return 0;
}

static struct lws_protocols protocols[] =
{
	/* The first protocol must always be the HTTP handler */
	{
		"http-only",   /* name */
		callback_http, /* callback */
		0,             /* No per session data. */
		0,             /* max frame size / rx buffer */
	},
	{
		"example-protocol",
		callback_example,
		0,
		EXAMPLE_RX_BUFFER_BYTES,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

int main( int argc, char *argv[] )
{
	struct lws_context_creation_info info;
	memset( &info, 0, sizeof(info) );

	info.port = 8000;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	struct lws_context *context = lws_create_context( &info );

	while( 1 )
	{
		lws_service( context, /* timeout_ms = */ 1000000 );
	}

	lws_context_destroy( context );

	return 0;
}
