#include "ConnectionData.h"

void ConnectionData::data_dump(FILE* dump, const ConnectionData* current_connection)
    {
      fprintf(dump, "===================================================================\n\n");
      fprintf(dump, "current connection class adress: %p\n", current_connection);
      if(connection_protocol)
        fprintf(dump, "connection_protocol: %s\n", connection_protocol);
      if(server_address)
        fprintf(dump, "server_address: %s\n", server_address);
      fprintf(dump, "server_path: %s\n", server_path);
      fprintf(dump, "connection_origin: %s\n", connection_origin);
      fprintf(dump, "ietf_version: %d\n", ietf_version);
      fprintf(dump, "port_used: %d\n", port_used);
      fprintf(dump, "ssl_used: %d\n", ssl_used);
      fprintf(dump, "log_level: %d\n", log_level);
      fprintf(dump, "is_there_first_query: %d\n", is_there_first_query);
      fprintf(dump, "first_query: %s\n\n", first_query);
      fprintf(dump, "===================================================================\n\n");
    }

int ConnectionData::record_iput_data(char* record_name, char* filename_to_record_in)
{
  FILE* file_to_record_in = NULL;
  int is_the_name_used = 0;

  if( access( filename_to_record_in, F_OK ) != -1 ) 
  {
    file_to_record_in = fopen(filename_to_record_in, "r+");
  
    if(NULL == file_to_record_in)
      return -1;

    int is_the_name_used = check_name_existance(record_name, file_to_record_in);
  }
  else
  {
    file_to_record_in = fopen(filename_to_record_in, "w");
  
    if(NULL == file_to_record_in)
      return -1;
  }

  if(!is_the_name_used)
  {
    fprintf(file_to_record_in, "[[Record_name:%s] [connection_protocol:%s] [server_address:%s] [first_query:%s] [connection_origin:%s] [server_path:%s] [port_used:%d] [ietf_version:%d] [ssl_used:%d] [log_level:%d]]\n",record_name, connection_protocol, server_address, first_query, connection_origin, server_path, port_used, ietf_version, ssl_used, log_level);
    fclose(file_to_record_in);
    return 0;
  }
  else
  {
    printf("Current name has already been used! Cannot make a record.\n");
    fclose(file_to_record_in);
    return -1;
  }
}  

int ConnectionData::check_name_existance (char* record_name, FILE* file_check_in)
{
  char current_record_name[MAX_RECORD_LEN] = {};

  while(!feof(file_check_in))
  {
    fscanf(file_check_in, "[[Record_name:%s]", current_record_name);
    delete_bracket(current_record_name);
   
    if(!strcmp(record_name, current_record_name))
    {
      return 1;
    }  

    fscanf(file_check_in, "%*[^\n]\n", NULL);
  }

  return 0;
}

int ConnectionData::load_record(FILE* file_get_from, char* record_name)
{
  if(!file_get_from || ! record_name)
    return -1;

  int name_exists = check_name_existance(record_name, file_get_from);

  if(name_exists) 
  {

    int opened_brakets = 1;
    int start_parsing = 0;
    int my_counter = 0;
    char temp[10] = {};
    char ch;
    connection_protocol = (char*)calloc(MAX_PROTOCOL_LEN, sizeof(char));
    server_address = (char*)calloc(MAX_SERVER_ADRESS_LEN, sizeof(char));

    fscanf(file_get_from, " [connection_protocol:%s]", connection_protocol);
    delete_bracket(connection_protocol);

    fscanf(file_get_from, " [server_address:%s]", server_address);
    delete_bracket(server_address);

    ch = fgetc(file_get_from);
    ch = fgetc(file_get_from);

    while(opened_brakets)
    {
      ch = fgetc(file_get_from);
      
      if(ch == '[')
        opened_brakets++;

      if(ch == ']')
        opened_brakets--;

      if(start_parsing)
      {
        first_query[my_counter++] = ch;
      }

      if(ch == ':')
        start_parsing = 1;
    }

    my_counter--;

    first_query[my_counter] = '\0';

    if(strlen(first_query))
    {
      is_there_first_query = 1;      
    }

    fscanf(file_get_from, " [connection_origin:%s]", connection_origin);
    delete_bracket(connection_origin);

    fscanf(file_get_from, " [server_path:%s]", server_path);
    delete_bracket(server_path);

    fscanf(file_get_from, " [port_used:%s]", temp);
    delete_bracket(temp);
    port_used = atoi(temp);

    fscanf(file_get_from, " [ietf_version:%s]", temp);
    delete_bracket(temp);
    ietf_version = atoi(temp);

    fscanf(file_get_from, " [ssl_used:%s]", temp);
    delete_bracket(temp);
    ssl_used= atoi(temp);

    fscanf(file_get_from, " [log_level:%s]]", temp);
    delete_bracket(temp);
    log_level= atoi(temp);
  }
  else 
  {
    printf("No such name\n");
    return -1;
  }
}

void ConnectionData::delete_bracket(const char* str_to_clean)
{
  int str_length = strlen(str_to_clean);

  if(((char*)str_to_clean)[str_length - 2] != ']')
    ((char*)str_to_clean)[str_length - 1] = '\0';
  else
    ((char*)str_to_clean)[str_length - 2] = '\0';
}

void ConnectionData::print_usage()
{
  fprintf(stderr, "Usage: libwebsockets-client(programm name)\n"
      "<server address>(ws://echo.websocket.org)\n" 
      "[--port=<p>] [--origin=<o>](localhost)\n"
      "[--record=<r>(name)] [--query=<q>](\"my_query\")\n"
      "[-v <ver> (ietf_version)] [-d <log bitfield>]\n");
}

ConnectionData::ConnectionData()
{
  memset(first_query, 0, sizeof first_query);
  strncpy(connection_origin, "localhost", sizeof(connection_origin));
  server_path[0] = '/';
  is_there_first_query = 0;
  ietf_version = -1;
  port_used = 0;
  log_level = 0;
  ssl_used = 0;
}

void ConnectionData::LoadSession (std::string record_to_load, char* filename_to_open)
{
  if(!filename_to_open)
  {
    printf("Invalid filename to open!\n");
    exit(0);
  }

  FILE* record_filename = fopen(filename_to_open, "r");
  
  if(!record_filename)
  {
    printf("Cannot open file with records.\n");
    exit(0);
  }
  
  if(load_record(record_filename, (char*)record_to_load.c_str()))
  {
    fclose(record_filename);
    exit(0);
  }

  fclose(record_filename);
}

void ConnectionData::CreateSession(ParseCmdArgs args_parsed, char* file_to_record_in)
{
  const char *parsed_path_ptr = NULL;

  port_used = args_parsed.get_port_used();
  
  is_there_first_query = args_parsed.get_query_existance();

  if(is_there_first_query)
  {
    strncpy(first_query, args_parsed.get_char_first_query(), MAX_SERVER_REQUEST_LEN);
  }

  if (lws_parse_uri(args_parsed.get_char_server_info(), &connection_protocol, &server_address, &port_used, &parsed_path_ptr))
  {
    print_usage() ;
    exit(0);
  }

  strncpy(server_path + 1, parsed_path_ptr, sizeof(server_path) - 2);
  server_path[sizeof(server_path) - 1] = '\0';

  if (!strcmp(connection_protocol, "ws"))
  {
    ssl_used = 0;

    if(!port_used)
      port_used = 80;
  }

  if (!strcmp(connection_protocol, "wss"))
  {
    ssl_used = 1;

    if(!port_used)
      port_used = 443;
  }

  if(args_parsed.get_session_record())
     record_iput_data(args_parsed.get_char_record_name(), file_to_record_in);
}

int ConnectionData::is_there_query()
{
  return is_there_first_query;
}

int ConnectionData::get_first_query(std::string* str_to_write)
{
  if(!is_there_first_query || NULL == str_to_write)
    return -1;
  else
    *str_to_write = first_query;
}

int ConnectionData::get_first_query(char* str_to_write)
{
  if(!is_there_first_query || NULL == str_to_write)
    return -1;
  else
    strncpy(str_to_write, first_query, MAX_SERVER_REQUEST_LEN);
}

int ConnectionData::construct_creation_info(struct lws_client_connect_info* client_connect_info, struct lws_context * context)
{
  client_connect_info -> address = server_address;
  client_connect_info -> path = server_path;
  client_connect_info -> port = port_used;
  client_connect_info -> ietf_version_or_minus_one = ietf_version;
  client_connect_info -> host = client_connect_info -> address;
  client_connect_info -> ssl_connection = ssl_used;
  client_connect_info -> origin = connection_origin;
  client_connect_info -> path = server_path;
  client_connect_info -> context = context;
  client_connect_info -> protocol = "";                             //name of the protocol which will be used for the connection       
}