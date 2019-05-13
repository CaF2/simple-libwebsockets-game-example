#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>

static int callback_http( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len );
static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len );
static void foreign_timer_service(void *foreign_loop);
static void signal_cb(int signum);

#define EXAMPLE_RX_BUFFER_BYTES (100)
typedef struct Chat_memory
{
	unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
	size_t len;
}Chat_memory;

Chat_memory GLOBAL_CHAT_MEMORY={0};

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

char GLOBAL_GUESS_MEMORY=0;

struct lws_context_creation_info info;
static struct lws_context *context;

static uv_loop_t loop_uv;
static uv_timer_t timer_outer_uv;
static uv_signal_t sighandler_uv;

static int sequence=0;

static void timer_cb_uv(uv_timer_t *t)
{
	foreign_timer_service(&loop_uv);
}

static void signal_cb_uv(uv_signal_t *watcher, int signum)
{
	signal_cb(signum);
}

static void foreign_event_loop_init_and_run_libuv(void)
{
	/* we create and start our "foreign loop" */

	uv_loop_init(&loop_uv);
	uv_signal_init(&loop_uv, &sighandler_uv);
	uv_signal_start(&sighandler_uv, signal_cb_uv, SIGINT);

	uv_timer_init(&loop_uv, &timer_outer_uv);
	uv_timer_start(&timer_outer_uv, timer_cb_uv, 0, 1000);

	/* this only has to exist for the duration of create context */
	void *foreign_loops[1];
	
	foreign_loops[0] = &loop_uv;
	info.foreign_loops = foreign_loops;

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return;
	}

	uv_run(&loop_uv, UV_RUN_DEFAULT);
}

static void foreign_event_loop_stop_libuv(void)
{
	uv_stop(&loop_uv);
}

static void foreign_event_loop_cleanup_libuv(void)
{
	/* cleanup the foreign loop assets */

	uv_timer_stop(&timer_outer_uv);
	uv_close((uv_handle_t*)&timer_outer_uv, NULL);
	uv_signal_stop(&sighandler_uv);
	uv_close((uv_handle_t *)&sighandler_uv, NULL);

	uv_run(&loop_uv, UV_RUN_DEFAULT);
	uv_loop_close(&loop_uv);
}

/* this is called at 1Hz using a foreign loop timer */

static void foreign_timer_service(void *foreign_loop)
{
	puts("Foreign 1Hz timer");

	sequence++;

	if((sequence%10)==0)
	{
		const char *msg="CADMIN     HELLO FROM TIMER!";
		size_t msg_len=strlen(msg);
		
		memcpy( &GLOBAL_CHAT_MEMORY.data[LWS_SEND_BUFFER_PRE_PADDING], msg, msg_len );
		GLOBAL_CHAT_MEMORY.len = msg_len;
		
		lws_callback_on_writable_all_protocol(context, &protocols[1]);
		
		puts("TIMER AROUND");
	}
}


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
		}
		break;
		case LWS_CALLBACK_HTTP_FILE_COMPLETION:
			if (lws_http_transaction_completed(wsi))
			{
				return -1;
			}
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

static void signal_cb(int signum)
{
	lwsl_notice("Signal %d caught, exiting...\n", signum);

	switch (signum) {
	case SIGTERM:
	case SIGINT:
		break;
	default:
		break;
	}
	
	lws_context_destroy(context);
	foreign_event_loop_stop_libuv();
}


int main( int argc, char *argv[] )
{
	memset( &info, 0, sizeof(info) );

	info.port = 8000;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.options = LWS_SERVER_OPTION_LIBUV;
	
	if (info.options & LWS_SERVER_OPTION_LIBUV)
		foreign_event_loop_init_and_run_libuv();

	lws_context_destroy(context);

	/* foreign loop specific cleanup and exit */

	if (info.options & LWS_SERVER_OPTION_LIBUV)
		foreign_event_loop_cleanup_libuv();

	return 0;
}
