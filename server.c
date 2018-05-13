#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>

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

#define EXAMPLE_RX_BUFFER_BYTES (100)
struct payload
{
	unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
	size_t len;
} received_payload;

static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_RECEIVE:
		{
			char *input=(char*)in;
			int dont_send=0;
			
			//for set and get
			if(strlen(input)==2)
			{
				if(input[0]=='G')
				{
					//compare the box character (num)
					if(input[1]==received_payload.data[LWS_SEND_BUFFER_PRE_PADDING+1])
					{
						const char *msg="CADMIN     CORRECT GUESS!";
						size_t msg_len=strlen(msg);
						
						memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], msg, msg_len );
						received_payload.len = msg_len;
					}
					else
					{
						const char *msg="CADMIN     WRONG GUESS!";
						size_t msg_len=strlen(msg);
						
						memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], msg, msg_len );
						received_payload.len = msg_len;
					}
					
					lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
					
					return 0;
				}
			}
			
			printf("recieve %s\n",input);
			memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
			received_payload.len = len;
			
			if(dont_send==0)
			{
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			break;
		}
		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			char fun=received_payload.data[LWS_SEND_BUFFER_PRE_PADDING];
			//we wont send Gs and Ss
			if(fun=='G' || fun=='S')
			{
				return 0;
			}
		
			lws_write( wsi, &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], received_payload.len, LWS_WRITE_TEXT );
			break;
		}
		default:
			break;
	}

	return 0;
}

enum protocols
{
	PROTOCOL_HTTP = 0,
	PROTOCOL_EXAMPLE,
	PROTOCOL_COUNT
};

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
