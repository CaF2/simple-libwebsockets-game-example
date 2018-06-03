#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

//from the gui
void *u_loop(void *user_input);

extern FILE *LOG_STR_FP;
extern char *input;
extern int input_len;
extern char user_name[10];
extern int user_name_len;
extern int RUN;

static struct lws *web_socket = NULL;

#define EXAMPLE_RX_BUFFER_BYTES (100)

static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
	switch( reason )
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
		break;

		case LWS_CALLBACK_CLIENT_RECEIVE:
		{
			/* Handle incomming messages here. */
			char *instr=in;
			if(len>0)
			{
				if(instr[0]=='C')
				{
					fprintf(LOG_STR_FP,"%s\n",instr+1);
					fflush(LOG_STR_FP);
				}
			}
		}
		break;

		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{
			if(input)
			{			
				unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
				unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
				
				if(strcmp(input,"q")==0)
				{
					RUN=0;
					break;
				}
				else if(input_len>0)
				{
					if(*input=='S' || *input=='s')
					{
						strncpy((char*)p,input,EXAMPLE_RX_BUFFER_BYTES);
						p[0]='S';
						
						lws_write( wsi, p, input_len, LWS_WRITE_TEXT );
					}
					else if(*input=='G' || *input=='g')
					{
						strncpy((char*)p,input,EXAMPLE_RX_BUFFER_BYTES);
						p[0]='G';
						
						lws_write( wsi, p, input_len, LWS_WRITE_TEXT );
					}
					else
					{
						printf("RECV:: %s %s\n",user_name,input);
						//chat
						p[0]='C';
						memset((char*)p+1,' ',10);
						memcpy((char*)p+1,user_name,user_name_len);
						strncpy((char*)p+1+10,input,EXAMPLE_RX_BUFFER_BYTES-1-10);
						
						lws_write( wsi, p, input_len+1+10, LWS_WRITE_TEXT );
					}
				}
				
				free(input);
				input=NULL;
				input_len=0;
				
				break;
			}
		}

		case LWS_CALLBACK_CLOSED:
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			web_socket = NULL;
			break;

		default:
			break;
	}

	return 0;
}

enum protocols
{
	PROTOCOL_EXAMPLE = 0,
	PROTOCOL_COUNT
};

static struct lws_protocols protocols[] =
{
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

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;

	struct lws_context *context = lws_create_context( &info );
	
	pthread_t u_loop_thread;

	pthread_create(&u_loop_thread, NULL, u_loop,NULL);

	while(RUN==1)
	{
		/* Connect if we are not connected to the server. */
		if( !web_socket)
		{
			struct lws_client_connect_info ccinfo = {0};
			ccinfo.context = context;
			ccinfo.address = argc>1?argv[1]:"localhost";
			ccinfo.port = 8000;
			ccinfo.path = "/";
			ccinfo.host = lws_canonical_hostname( context );
			ccinfo.origin = "origin";
			ccinfo.protocol = protocols[PROTOCOL_EXAMPLE].name;
			web_socket = lws_client_connect_via_info(&ccinfo);
		}
		
		if(input)
		{
			lws_callback_on_writable( web_socket );
		}

		lws_service( context, /* timeout_ms = */ 250 );
	}

	lws_context_destroy( context );

	return 0;
}
