/* nuklear - 1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

static char *LOG_STR=NULL;
static size_t LOG_STR_LEN=0;
FILE *LOG_STR_FP=NULL;

char user_name[10]={0};
int user_name_len=0;
static char msgtext[90]={0};
static int msgtext_len=0;
char *input=NULL;
int input_len=0;
int RUN = 1;

/* ===============================================================
 *
 *						  DEMO
 *
 * ===============================================================*/
void *u_loop(void *user_input)
{
	/* Platform */
	SDL_Window *win;
	SDL_GLContext glContext;
	int win_width, win_height;

	/* GUI */
	struct nk_context *ctx;
	struct nk_colorf bg;
	
	LOG_STR_FP=open_memstream(&LOG_STR,&LOG_STR_LEN);
	
	/* SDL setup */
	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	win = SDL_CreateWindow("Demo",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
	glContext = SDL_GL_CreateContext(win);
	SDL_GetWindowSize(win, &win_width, &win_height);

	/* OpenGL setup */
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glewExperimental = 1;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to setup GLEW\n");
		exit(1);
	}

	ctx = nk_sdl_init(win);
	/* Load Fonts: if none of these are loaded a default font will be used  */
	/* Load Cursor: if you uncomment cursor loading please hide the cursor */
	struct nk_font_atlas *atlas;
	nk_sdl_font_stash_begin(&atlas);

	nk_sdl_font_stash_end();

	bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
	while (RUN)
	{
		/* Input */
		SDL_Event evt;
		nk_input_begin(ctx);
		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_QUIT)
			{
				RUN=0;
				goto cleanup;
			}
			nk_sdl_handle_event(&evt);
		} nk_input_end(ctx);

		/* GUI */
		if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 500),
			NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
			NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
		{
			enum {EASY, HARD};
			static int op = EASY;
			static int property = 20;

			nk_layout_row_static(ctx, 30, 100, 1);
			nk_label(ctx, "user_name", NK_TEXT_LEFT);
			nk_edit_string(ctx, NK_EDIT_FIELD, user_name, &user_name_len, 10, nk_filter_default);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_label(ctx, "Input", NK_TEXT_LEFT);
			nk_edit_string(ctx, NK_EDIT_FIELD, msgtext, &msgtext_len, 90, nk_filter_default);
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_button_label(ctx, "Send"))
			{
				printf("button pressed! %s %s\n",user_name,msgtext);
/*				fprintf(LOG_STR_FP,"%s: %s\n",user_name,msgtext);*/
/*				fflush(LOG_STR_FP);*/
				input_len=msgtext_len;
				input=strdup(msgtext);
				
				memset(msgtext,'\0',sizeof(msgtext));
				msgtext_len=0;
			}
			nk_layout_row_dynamic(ctx, 25, 1);
			
			nk_label(ctx, "Set a box", NK_TEXT_LEFT);
			for(int i=1;i<=3;i++)
			{
				char label[20]={0};
			
				snprintf(label,20,"SET B%d",i);
			
				if (nk_button_label(ctx, label))
				{
					printf("Set button %d pressed!  %s %s\n",i);
					input_len=asprintf(&input,"S%d",i);
				}
			}
			
			nk_label(ctx, "Guess a box", NK_TEXT_LEFT);
			for(int i=1;i<=3;i++)
			{
				char label[20]={0};
			
				snprintf(label,20,"GUESS B%d",i);
			
				if (nk_button_label(ctx, label))
				{
					printf("Guess button %d pressed!\n",i);
					input_len=asprintf(&input,"G%d",i);
				}
			}
			
			nk_label(ctx, "Output", NK_TEXT_LEFT);
			
			char *tmp_start=LOG_STR;
			char *tmp=LOG_STR;
			
			if(tmp && *tmp)
			{
				do
				{
					if((*tmp=='\n' || *tmp=='\0') && tmp-tmp_start>0)
					{
						char tmp_storage[128]={0};
						strncpy(tmp_storage,tmp_start,tmp-tmp_start);
						nk_label(ctx, tmp_storage, NK_TEXT_LEFT);
						
						tmp_start=tmp+1;
					}
					else
					{
						tmp++;
					}
				}while(*tmp);
			}
		}
		nk_end(ctx);
		
		/* ----------------------------------------- */

		/* Draw */
		SDL_GetWindowSize(win, &win_width, &win_height);
		glViewport(0, 0, win_width, win_height);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(bg.r, bg.g, bg.b, bg.a);
		/* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
		 * with blending, scissor, face culling, depth test and viewport and
		 * defaults everything back into a default state.
		 * Make sure to either a.) save and restore or b.) reset your own state after
		 * rendering the UI. */
		nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
		SDL_GL_SwapWindow(win);
	}

cleanup:
	nk_sdl_shutdown();
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}

