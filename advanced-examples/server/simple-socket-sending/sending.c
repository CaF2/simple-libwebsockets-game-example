#include "sending.h"

typedef struct Memory_holder
{
	GArray *mhistory;
	
	char *name;
}Memory_holder;

typedef struct Chat_memory
{
	char *data;
	size_t len;
	time_t time;
	
	uint8_t free_data : 1;
	uint8_t history : 1;
}Chat_memory;

typedef struct User_info
{
	const void *user_id;
	
	const void *user;
	
	GArray *send_array; ///< Chat_memory
	
	time_t curr_message_time;
	//to avoid_conflicts
	guint curr_message_i;
	
	uint8_t send_data : 1;
}User_info;

static GArray *GLOBAL_SEND_TO_ALL_MEMORY=NULL;
static pthread_mutex_t GLOBAL_SEND_TO_ALL_MEMORY_MUTEX = PTHREAD_MUTEX_INITIALIZER;

static GHashTable *GLOBAL_USER_SEND_TABLE=NULL;
static pthread_mutex_t GLOBAL_USER_SEND_TABLE_MUTEX = PTHREAD_MUTEX_INITIALIZER;

static void global_user_send_table_set(GHashTable *value)
{
	pthread_mutex_lock(&GLOBAL_USER_SEND_TABLE_MUTEX);
	GLOBAL_USER_SEND_TABLE=value;
	pthread_mutex_unlock(&GLOBAL_USER_SEND_TABLE_MUTEX);
}

static GHashTable *global_user_send_table_get(void)
{
	pthread_mutex_lock(&GLOBAL_USER_SEND_TABLE_MUTEX);
	GHashTable *value=GLOBAL_USER_SEND_TABLE;
	pthread_mutex_unlock(&GLOBAL_USER_SEND_TABLE_MUTEX);
	
	return value;
}

static void __auto_mutex(pthread_mutex_t **auto_mutex)
{
	pthread_mutex_unlock(*auto_mutex);
}

#define auto_mutex(mutex) __attribute__((cleanup(__auto_mutex))) pthread_mutex_t *auto_ ## mutex = &mutex; pthread_mutex_lock(auto_ ## mutex)

/**
	Code for handeling the User_info struct.
*/

/**
	Constructor for User_info
	
	@param self
		reset or initialize the User_info struct.
	@returns
		initialized User_info struct.
*/
static User_info *user_info_init(User_info *self,const void *user_id,const void *user)
{
	memset(self,'\0',sizeof(User_info));

	self->user_id=user_id;
	self->user=user;
	
	return self;
}

/**
	Constructor and allocator for the User_info struct
	
	@returns
		newly allocated User_info struct.
*/
static User_info *user_info_new(const void *user_id,const void *user)
{
	User_info *self=malloc(sizeof(User_info));
	if(!self)
	{
		return NULL;
	}

	return user_info_init(self,user_id,user);
}

/**
	De-initialize the struct but does not free the input.
	
	@param self
		Struct to handle
*/
static void user_info_finalize(User_info *self)
{
	
}

/**
	Destructor of the struct, will call user_info_finalize and free. 
	
	@param self
		Struct to handle. Note that self will be freed inside this function
*/
static void user_info_free(User_info *self)
{
	user_info_finalize(self);

	free(self);
}

/////////////////////////////

/**
	Constructor for Chat_memory
	
	@param self
		reset or initialize the Chat_memory struct.
	@returns
		initialized Chat_memory struct.
*/
static Chat_memory *chat_memory_init(Chat_memory *self,const char *data,size_t len,uint8_t free_data)
{
	memset(self,'\0',sizeof(Chat_memory));

	self->data=strndup(data,len);
	self->len=len;
	self->time=time(NULL);
	self->free_data=free_data;

	return self;
}

/**
	De-initialize the struct but does not free the input.
	
	@param self
		Struct to handle
*/
static void chat_memory_finalize(Chat_memory *self)
{
	free(self->data);
}

/////////////////////////////

static void memory_holder_finalize(Memory_holder *self);

/**
	Constructor for Memory_holder
	
	@param self
		reset or initialize the Memory_holder struct.
	@returns
		initialized Memory_holder struct.
*/
static Memory_holder *memory_holder_init(Memory_holder *self, const char *name)
{
	memset(self,'\0',sizeof(Memory_holder));

	self->mhistory=g_array_sized_new(FALSE,FALSE,sizeof(Chat_memory),10);
	g_array_set_clear_func(self->mhistory,(GDestroyNotify)chat_memory_finalize);

	self->name=g_strdup(name);

	return self;
}

/**
	De-initialize the struct but does not free the input.
	
	@param self
		Struct to handle
*/
static void memory_holder_finalize(Memory_holder *self)
{
	free(self->name);
	g_array_free(self->mhistory,TRUE);
}

static Memory_holder *memory_holder_array_get_memory_holder(GArray *holder, const char *name)
{
	for(guint i=0;i<holder->len;i++)
	{
		Memory_holder *mholder=&g_array_index(holder,Memory_holder,i);
		
		if(mholder && g_strcmp0(mholder->name,name)==0)
		{
			return mholder;
		}
	}
	
	return NULL;
}

/////////////////////////////

int send_global_data(struct lws_context *context, struct lws_protocols *protocol, const char *memory, Message_type mtype, const char *data, size_t len)
{
	auto_mutex(GLOBAL_SEND_TO_ALL_MEMORY_MUTEX);
	if(GLOBAL_SEND_TO_ALL_MEMORY==NULL)
	{
		GLOBAL_SEND_TO_ALL_MEMORY=g_array_sized_new(FALSE,FALSE,sizeof(Memory_holder),10);
		g_array_set_clear_func(GLOBAL_SEND_TO_ALL_MEMORY,(GDestroyNotify)memory_holder_finalize);
	}
	
	Memory_holder *amholder=memory_holder_array_get_memory_holder(GLOBAL_SEND_TO_ALL_MEMORY,memory);
	
	if(amholder==NULL)
	{
		Memory_holder mholder;
		memory_holder_init(&mholder,memory);
	
		g_array_append_val(GLOBAL_SEND_TO_ALL_MEMORY,mholder);
		
		amholder=&g_array_index(GLOBAL_SEND_TO_ALL_MEMORY,Memory_holder,GLOBAL_SEND_TO_ALL_MEMORY->len-1);
	}
	
	Chat_memory cdata;
	chat_memory_init(&cdata,data,len,mtype==SEND_ERASE?TRUE:FALSE);
	
	if(mtype==SEND_HAVE_ONE)
	{
		g_array_remove_range(amholder->mhistory,0,amholder->mhistory->len);
	}
	
	g_array_append_val(amholder->mhistory,cdata);
	
	lws_callback_on_writable_all_protocol(context, protocol);
	
	return 0;
}

int send_global_message_v(struct lws_context *context, struct lws_protocols *protocol, const char *memory, Message_type mtype, const char *data, va_list args)
{
	char *sdata=NULL;
	size_t len=vasprintf(&sdata,data,args);
	
	int ret=send_global_data(context, protocol, memory, mtype, sdata, len);
	
	return ret;
}

int send_global_message(struct lws_context *context, struct lws_protocols *protocol, const char *memory, Message_type mtype, const char *data, ...)
{
	va_list args;

	va_start(args, data);

	int ret = send_global_message_v(context, protocol, memory, mtype, data, args);

	va_end(args);

	return ret;
}

/// SEND USER DATA

int init_user(struct lws *wsi,void *user_id,void *user)
{
	if(global_user_send_table_get()==NULL)
	{
		global_user_send_table_set(g_hash_table_new_full(g_direct_hash,g_direct_equal,NULL,(GDestroyNotify)user_info_free));
	}
	
	User_info *cdata=user_info_new(user_id,user);
	
	pthread_mutex_lock(&GLOBAL_USER_SEND_TABLE_MUTEX);
	g_hash_table_insert(GLOBAL_USER_SEND_TABLE,wsi,cdata);
	pthread_mutex_unlock(&GLOBAL_USER_SEND_TABLE_MUTEX);
	
	lws_callback_on_writable(wsi);
	
	return 0;
}

int deinit_user(struct lws *wsi)
{
	auto_mutex(GLOBAL_USER_SEND_TABLE_MUTEX);

	if(GLOBAL_USER_SEND_TABLE==NULL)
	{
		return 1;
	}
	
	if(g_hash_table_remove(GLOBAL_USER_SEND_TABLE,wsi)==FALSE)
	{
		return 2;
	}
	
	return 0;
}

ssize_t number_of_connected_users()
{
	pthread_mutex_lock(&GLOBAL_USER_SEND_TABLE_MUTEX);
	ssize_t connected_len=GLOBAL_USER_SEND_TABLE?g_hash_table_size(GLOBAL_USER_SEND_TABLE):0;
	pthread_mutex_unlock(&GLOBAL_USER_SEND_TABLE_MUTEX);
	return connected_len;
}

static int send_user_data_from_struct(User_info *user, struct lws *wsi, const char *memory, Message_type mtype, const char *data, size_t len)
{
	if(global_user_send_table_get()==NULL)
	{
		return 1;
	}
	
	if(user->send_array==NULL)
	{
		user->send_array=g_array_sized_new(FALSE,FALSE,sizeof(Memory_holder),10);
		g_array_set_clear_func(user->send_array,(GDestroyNotify)memory_holder_finalize);
	}
	
	Memory_holder *amholder=memory_holder_array_get_memory_holder(user->send_array,memory);
	
	if(amholder==NULL)
	{
		Memory_holder mholder;
		memory_holder_init(&mholder,memory);
	
		g_array_append_val(user->send_array,mholder);
		
		amholder=&g_array_index(user->send_array,Memory_holder,user->send_array->len-1);
	}
	
	Chat_memory cdata;
	chat_memory_init(&cdata,data,len,mtype==SEND_ERASE?TRUE:FALSE);
	
	if(mtype==SEND_HAVE_ONE)
	{
		g_array_remove_range(amholder->mhistory,0,amholder->mhistory->len);
	}
	
	g_array_append_val(amholder->mhistory,cdata);

	lws_callback_on_writable(wsi);
	
	return 0;
}

int send_user_data(struct lws *wsi, const char *memory, Message_type mtype, const char *data, size_t len)
{
	if(global_user_send_table_get()==NULL)
	{
		return 1;
	}
	
	void *orig_key;
	void *orig_val;
	
	if(g_hash_table_lookup_extended(global_user_send_table_get(),wsi,&orig_key,&orig_val)==FALSE)
	{
		return 2;
	}
	
	User_info *useri=orig_val;
	int ret=send_user_data_from_struct(useri,wsi,memory,mtype,data,len);
	
	return ret;
}

int send_user_message_v(struct lws *wsi, const char *memory, Message_type mtype, const char *data, va_list args)
{
	char *sdata=NULL;
	size_t len=vasprintf(&sdata,data,args);
	
	int ret=send_user_data(wsi,memory,mtype,sdata,len);
	
	return ret;
}

int send_user_message(struct lws *wsi, const char *memory, Message_type mtype, const char *data, ...)
{
	va_list args;

	va_start(args, data);

	int ret = send_user_message_v(wsi, memory, mtype, data, args);

	va_end(args);

	return ret;
}

static void _send_global_data_from_id_itr(gpointer key, gpointer value, gpointer user_data)
{
	void **forward=user_data;
	User_info *user=value;
	struct lws *wsi=key;
	
	void *user_id=forward[0];
	const char *memory=forward[1];
	Message_type mtype=(intptr_t)forward[2];
	const char *data=forward[3];
	size_t len=(intptr_t)forward[4];
	
	if(user && user->user_id==user_id)
	{
		send_user_data_from_struct(user,wsi,memory,mtype,data,len);
	}
}

/**
	Send global data to all users with a user_id
	
	@param user_id
		user it to query
	
*/
int send_global_data_from_id(void *user_id, const char *memory, Message_type mtype, const char *data, size_t len)
{
	void *send_data[]={user_id,(void*)memory,(void*)(intptr_t)mtype,(void*)data,(void*)(intptr_t)len};

	//iterate users and insert to user, if matching id
	g_hash_table_foreach(global_user_send_table_get(),_send_global_data_from_id_itr,send_data);
	
	return 0;
}

int send_global_message_from_id_v(void *user_id, const char *memory, Message_type mtype, const char *data, va_list args)
{
	char *sdata=NULL;
	size_t len=vasprintf(&sdata,data,args);
	
	int ret=send_global_data_from_id(user_id,memory,mtype,sdata,len);
	
	return ret;
}

int send_global_message_from_id(void *user_id, const char *memory, Message_type mtype, const char *data, ...)
{
	va_list args;

	va_start(args, data);

	int ret = send_global_message_from_id_v(user_id,memory,mtype,data, args);

	va_end(args);

	return ret;
}

//////////////////////////////////

static int do_send_message(Chat_memory *memory,struct lws *wsi)
{
	char *real_data=malloc(LWS_SEND_BUFFER_PRE_PADDING+memory->len+LWS_SEND_BUFFER_POST_PADDING+1);
	
	memcpy(real_data+LWS_SEND_BUFFER_PRE_PADDING,memory->data,memory->len);
	
	lws_write( wsi, (unsigned char*)&real_data[LWS_SEND_BUFFER_PRE_PADDING], memory->len, LWS_WRITE_TEXT );
	memory->history=1;
	
	free(real_data);
	
	return 0;
}

void print_mhandler(GArray *mhandler)
{
	printf("********===memory hold-len:: [[%d]]\n",mhandler->len);
	
	//tmp
	for(guint i=0;i<mhandler->len;i++)
	{
		Memory_holder *mholder=&g_array_index(mhandler,Memory_holder,i);
		
		printf("********===memory hold:: [[%s]]{%u}\n",mholder->name,mholder->mhistory->len);
		
		for(guint j=0;j<mholder->mhistory->len;j++)
		{
			Chat_memory *chat_memory=&g_array_index(mholder->mhistory,Chat_memory,j);
			
			printf("***********memory saved:: [[%s]]{%ld,%u,%u}\n",chat_memory->data,chat_memory->time,i,j);
		}
	}
}

static int handle_message_array(GArray *mhandler, time_t not_lower,time_t *current_check_time, Chat_memory **result, guint *curr_index)
{
	int found=0;

	//on two of the same timestamps (if only on the previous)
	for(guint i=0;i<mhandler->len;i++)
	{
		Memory_holder *mholder=&g_array_index(mhandler,Memory_holder,i);
		
		for(guint j=0;j<mholder->mhistory->len;j++)
		{
			Chat_memory *chat_memory=&g_array_index(mholder->mhistory,Chat_memory,j);
			
			if(not_lower==chat_memory->time)
			{
				if(found>*curr_index)
				{
					*result=chat_memory;
					*current_check_time=chat_memory->time;
					*curr_index=found;
					return 0;
				}
				found++;
			}
		}
	}

	//get highest
	for(guint i=0;i<mhandler->len;i++)
	{
		Memory_holder *mholder=&g_array_index(mhandler,Memory_holder,i);
		
		for(guint j=0;j<mholder->mhistory->len;j++)
		{
			Chat_memory *chat_memory=&g_array_index(mholder->mhistory,Chat_memory,j);

			if(chat_memory->time>not_lower && chat_memory->time>*current_check_time)
			{
				*current_check_time=chat_memory->time;
				*result=chat_memory;
				*curr_index=0;
			}
		}
	}

	//get lowest of them
	for(guint i=0;i<mhandler->len;i++)
	{
		Memory_holder *mholder=&g_array_index(mhandler,Memory_holder,i);
		
		for(guint j=0;j<mholder->mhistory->len;j++)
		{
			Chat_memory *chat_memory=&g_array_index(mholder->mhistory,Chat_memory,j);

			if(chat_memory->time>not_lower && chat_memory->time<*current_check_time)
			{
				*current_check_time=chat_memory->time;
				*result=chat_memory;
				*curr_index=0;
			}
		}
	}
	
	return 0;
}

static int handle_two_message_array(GArray *mhandler1,GArray *mhandler2, time_t not_lower,time_t *current_check_time, Chat_memory **result, guint *curr_index, int *picked)
{
	Chat_memory *test1=NULL;
	time_t test_time1=*current_check_time;
	guint conflict_index1=*curr_index;

	if(mhandler1)
	{
		handle_message_array(mhandler1,not_lower,&test_time1,&test1,&conflict_index1);
	}
	
	Chat_memory *test2=NULL;
	time_t test_time2=*current_check_time;
	guint conflict_index2=*curr_index;

	if(mhandler2)
	{
		handle_message_array(mhandler2,not_lower,&test_time2,&test2,&conflict_index2);
	}
	
	if((test1 && test2==NULL) || (test1 && test2 && test_time1<test_time2))
	{
		*result=test1;
		*current_check_time=test_time1;
		*curr_index=conflict_index1;
		*picked=0;
	}
	else if(test1 && test2 && test_time1==test_time2)
	{
		printf("TODO\n");
		*result=test1;
		*current_check_time=test_time1;
		*curr_index=conflict_index1;
		*picked=0;
	}
	else if((test1==NULL && test2) || (test1 && test2 && test_time1>test_time2))
	{
		*result=test2;
		*current_check_time=test_time2;
		*curr_index=conflict_index2;
		*picked=1;
	}
	
	return 0;
}

int send_messages(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	void *orig_key;
	void *orig_val;
	
	if(g_hash_table_lookup_extended(global_user_send_table_get(),wsi,&orig_key,&orig_val)==FALSE)
	{
		return 1;
	}
	
	User_info *useri=orig_val;
	
	Chat_memory *test=NULL;
	time_t test_time=useri->curr_message_time;
	guint conflict_index=useri->curr_message_i;
	int picked=0;
	
	auto_mutex(GLOBAL_SEND_TO_ALL_MEMORY_MUTEX);
	handle_two_message_array(GLOBAL_SEND_TO_ALL_MEMORY,useri->send_array,useri->curr_message_time,&test_time,&test,&conflict_index,&picked);
	
	if(test)
	{
		useri->curr_message_time=test_time;
		useri->curr_message_i=conflict_index;
		
		do_send_message(test,wsi);
		
		Chat_memory *old_test=test;
		//test if not last
		handle_two_message_array(GLOBAL_SEND_TO_ALL_MEMORY,useri->send_array,useri->curr_message_time,&test_time,&test,&conflict_index,&picked);
		
		//if new is found
		if(old_test!=test)
		{
			if(picked==0 && test->history==0)
			{
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else
			{
				lws_callback_on_writable(wsi);
			}
		}
	}
	
	return 0;
}
