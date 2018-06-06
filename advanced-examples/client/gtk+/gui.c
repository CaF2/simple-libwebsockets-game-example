#include <gtk/gtk.h>

static GtkWidget *entry_username=NULL;
GtkWidget *textview=NULL;

char user_name[10]={0};
int user_name_len=0;
static char msgtext[90]={0};
static int msgtext_len=0;
char *input=NULL;
int input_len=0;
int RUN = 1;

void guess_click(GtkButton *button, gpointer user_data)
{
	gint i=(intptr_t)(void*)user_data;

	input_len=asprintf(&input,"G%d",i);
}

void set_click(GtkButton *button, gpointer user_data)
{
	gint i=(intptr_t)(void*)user_data;

	input_len=asprintf(&input,"S%d",i);
}

void send_click(GtkButton *button, gpointer user_data)
{
	GtkWidget *label_input=user_data;
	
	//define the username first
	char *usertxt=gtk_entry_get_text(GTK_ENTRY(entry_username));
	
	if(usertxt)
	{
		user_name_len=gtk_entry_get_text_length(GTK_ENTRY(entry_username));
		
		strncpy(user_name,usertxt,sizeof(user_name));
	}
	
	//set the text afterwards
	char *text=gtk_entry_get_text(GTK_ENTRY(label_input));
	
	if(text)
	{
		input_len=gtk_entry_get_text_length(GTK_ENTRY(label_input));
		
		input=strdup(text);
	}
}

void *u_loop(void *user_input)
{
	gtk_init(NULL, NULL);
	
	GtkWidget *window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_title(GTK_WINDOW(window), "Demo");
	
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 100);
	
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
	GtkWidget *main_grid=gtk_grid_new();
	gtk_grid_set_column_homogeneous(main_grid,TRUE);
	gtk_widget_set_hexpand(main_grid,TRUE);
		
		GtkWidget *label_username=gtk_label_new("Username");
		gtk_grid_attach(GTK_GRID(main_grid), label_username,1,1,1,1);
		
		entry_username=gtk_entry_new();
		gtk_grid_attach(GTK_GRID(main_grid), entry_username,1,2,1,1);
		
		GtkWidget *scrolled=gtk_scrolled_window_new(NULL,NULL);
		gtk_widget_set_size_request(scrolled,-1,200);
			textview=gtk_text_view_new();
			gtk_text_view_set_editable(GTK_TEXT_VIEW(textview),FALSE);
			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (scrolled), textview);
		gtk_grid_attach(GTK_GRID(main_grid), scrolled,1,3,1,1);
		
		GtkWidget *label_input=gtk_label_new("Input");
		gtk_grid_attach(GTK_GRID(main_grid), label_input,1,4,1,1);
		
		GtkWidget *entry_input=gtk_entry_new();
		gtk_grid_attach(GTK_GRID(main_grid), entry_input,1,5,1,1);
		
		GtkWidget *button_send=gtk_button_new_with_label("Send");
		g_signal_connect (button_send, "clicked", G_CALLBACK (send_click), entry_input);
		gtk_grid_attach(GTK_GRID(main_grid), button_send,1,6,1,1);
		
		gsize set_amount=3;
		gsize guess_amount=3;
		
		for(gint i=1;i<=set_amount;i++)
		{
			g_autofree gchar *str_guess=g_strdup_printf("Set %d",i);
		
			GtkWidget *button_guess=gtk_button_new_with_label(str_guess);
			g_signal_connect (button_guess, "clicked", G_CALLBACK (set_click), (void*)(intptr_t)i);
			gtk_grid_attach(GTK_GRID(main_grid), button_guess,1,7+i,1,1);
		}
		
		for(gint i=1;i<=guess_amount;i++)
		{
			g_autofree gchar *str_guess=g_strdup_printf("Guess %d",i);
		
			GtkWidget *button_guess=gtk_button_new_with_label(str_guess);
			g_signal_connect (button_guess, "clicked", G_CALLBACK (guess_click), (void*)(intptr_t)i);
			gtk_grid_attach(GTK_GRID(main_grid), button_guess,1,8+set_amount+i,1,1);
		}
	
	gtk_container_add(GTK_CONTAINER(window), main_grid);
	
	gtk_widget_show_all(window);
	gtk_main();
	
	RUN=0;
	
	return 0;
}
