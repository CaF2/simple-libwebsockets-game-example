#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>

#define EXAMPLE_RX_BUFFER_BYTES (100)
typedef struct Chat_memory
{
	unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
	size_t len;
}Chat_memory;

Chat_memory GLOBAL_CHAT_MEMORY={0};

char GLOBAL_GUESS_MEMORY=0;

static int callback_http( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_HTTP:
			lws_serve_http_file( wsi, "example.html", "text/html", NULL, 0 );
			break;
		default:
			break;
	}

	return 0;
}

static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_RECEIVE:
		{
			char *input=(char*)in;
			int dont_send=0;
			
			//for set and get
			if(len==2)
			{
				if(input[0]=='G')
				{
					//compare the box character (num)
					if(input[1]==GLOBAL_GUESS_MEMORY)
					{
						const char *msg="CADMIN     CORRECT GUESS!";
						size_t msg_len=strlen(msg);
						
						memcpy( &GLOBAL_CHAT_MEMORY.data[LWS_SEND_BUFFER_PRE_PADDING], msg, msg_len );
						GLOBAL_CHAT_MEMORY.len = msg_len;
					}
					else
					{
						const char *msg="CADMIN     WRONG GUESS!";
						size_t msg_len=strlen(msg);
						
						memcpy( &GLOBAL_CHAT_MEMORY.data[LWS_SEND_BUFFER_PRE_PADDING], msg, msg_len );
						GLOBAL_CHAT_MEMORY.len = msg_len;
					}
					
					lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
					
					return 0;
				}
				else if(input[0]=='S')
				{
					GLOBAL_GUESS_MEMORY=input[1];
					
					const char *msg="CADMIN     BOX IS SET!";
					size_t msg_len=strlen(msg);
					
					memcpy( &GLOBAL_CHAT_MEMORY.data[LWS_SEND_BUFFER_PRE_PADDING], msg, msg_len );
					GLOBAL_CHAT_MEMORY.len = msg_len;
					
					lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
					
					return 0;
				}
			}
			
			printf("recieve [%s]\n",input);
			memcpy( &GLOBAL_CHAT_MEMORY.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
			GLOBAL_CHAT_MEMORY.len = len;
			
			if(dont_send==0)
			{
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			break;
		}
		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			char fun=GLOBAL_CHAT_MEMORY.data[LWS_SEND_BUFFER_PRE_PADDING];
			//we wont send Gs and Ss
			if(fun=='G' || fun=='S')
			{
				return 0;
			}
		
			lws_write( wsi, &GLOBAL_CHAT_MEMORY.data[LWS_SEND_BUFFER_PRE_PADDING], GLOBAL_CHAT_MEMORY.len, LWS_WRITE_TEXT );
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
