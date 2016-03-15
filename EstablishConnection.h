#pragma once
#include <thread>
#include <libwebsockets.h>
#include "ConnectionData.h"

class EstablishConnection
{
  private:
    struct lws_context_creation_info context_creation_info;
    struct lws_client_connect_info client_connect_info;  
    struct lws_protocols protocols[2];                        //The list is ended with an entry that has a NULL callback pointer.
    struct lws_context *context;
    struct lws *wsi;

    int force_exit;

    int set_context_creation_info(struct lws_context_creation_info* context_creation_info, struct lws_protocols* protocols);

    int set_protocol_data(struct lws_protocols* protocols, int prot_num, 
                              const char * name, lws_callback_function * callback,  
                              size_t per_session_data_size, size_t rx_buffer_size,
                              unsigned int id, void * user);
    

  public:

    EstablishConnection(); 

    int close_connection();  

    int connect(int (*ws_service_callback_pointer)(struct lws *, enum lws_callback_reasons, void* , void* , size_t),
                ConnectionData user_data, void* thread_args_struct, void (*thread_routine_function)(void*, struct lws_context*, struct lws*));
};