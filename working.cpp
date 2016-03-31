/*
 * libwebsockets-client
 *
 * Copyright (C) 2016 Alex Serbin
 *
 * g++ test_it_here.c EstablishConnection.cpp ConnectionData.cpp StatisticsFileSystem.cpp ParseCmdArgs.cpp -L/usr/local/lib -lwebsockets -pthread -g -std=c++11
 */

#include <thread>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include <libwebsockets.h>
#include "ParseCmdArgs.h"
#include "ConnectionData.h"
#include "EstablishConnection.h"
#include "StatisticsFileSystem.h"

#include "json11/json11.hpp"

#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

const int MAX_SERVER_REQUEST_LEN = 500;        //define -> const int 
const int MAX_PROGRAMM_PATH_LEN = 500;
const int AUTHORIZED_CONTEXT_CLOSE = 1;
const int ASSETS_ANMOUNT = 20;       //txlib -> examples ldview -- graph algorithms doxygen help
const int MAX_SERVER_RESPOND_LENGTH = 200;                                                          //---use namespase
const int MAX_RECONNECT_ATTEMPTS_NUM = 5;

struct pthread_routine_tool 
{
  char first_server_request[MAX_SERVER_REQUEST_LEN];
  int is_there_first_request;
};

                                                                                      //data to stuct -> dynamic array for using threads;
static const std::string records_file_name = "/Creation_data";
static const std::string servertime = "\"servertime\":";
static const std::string price_time = "\"time\":";
static const std::string close_pattern = "\"close\":";
static wsclient::StatisticsFileSystem my_stat;

static std::string RECORDS_FILENAME;
int connection_flag = 0;           //global variables starts with capital      doxygen.org -> manual   txlib -> help in every function + tx/doc files
int authorized_context_close_flag = 0;
int reconnection_attempt_num = 0;
int first_price_appeard = 0;
int current_process_num = 0;
std::string current_statistics_file_pathname;
std::string current_servertime;
std::string prev_servertime;
std::string close_price; 
int const_price_last_second; 
FILE* stat_file = NULL;

int delete_bracket(const char* str_to_clean);
int load_all_names(std::string names[], FILE* file_from);
int run_process(wsclient::ParseCmdArgs* parsed_args, std::string* record_name);
static void pthread_routine(void *tool_in, struct lws *wsi);
static int websocket_write_back(struct lws *wsi_in, char *str_data_to_send);
static int ws_service_callback(struct lws *wsi,
                               enum lws_callback_reasons reason, 
                               void *user, void *in, size_t len);
static void record_current_second_data(void* in);
static void set_sigint_handler();
static void sighandler(int sig);

//need initialization (must be called first time at the beginning)
//first time parameter -  EstablishConnection*, next time - null
static void close_connection(void* parameter);        
static void reconnect(void* parameter);



int main(int argc, char **argv)
{
  pid_t pid = 1;

  int records_amount = 0;

  char* getcwd_result_ptr = getcwd(NULL, MAX_PROGRAMM_PATH_LEN);

  RECORDS_FILENAME = getcwd_result_ptr + records_file_name;

  free(getcwd_result_ptr);

  wsclient::ParseCmdArgs parsed_args(argc, argv);

  std::string asset_names_array[ASSETS_ANMOUNT];

  if(parsed_args.load_all_data())
  {
    FILE* my_file = fopen(RECORDS_FILENAME.c_str(), "r");
    
    if(!my_file)
    {
      printf("Error opening file with records\n");
      return -1;
    }
    
    records_amount = load_all_names(asset_names_array, my_file);
    
    fclose(my_file);

    for(int i = 0; i < records_amount; i++)
    {
      if(pid > 0)
      {
        current_process_num = i;
        if ((pid = fork()) < 0)
        {
          printf("Bad fork!\n");
          exit(1);
        }
      }
      else
        break;
    }

    if (pid > 0)   /***** Parent *****/
    {
      sigset(SIGINT, SIG_IGN);

      for(int j = 0; j < records_amount; j++);
        wait(NULL);
    }
    else
    {
      my_stat.construct_statistics(current_process_num);
      current_statistics_file_pathname = my_stat.get_current_filepath_to_use();
      stat_file = fopen(current_statistics_file_pathname.c_str(), "a+");
      
      if(!stat_file)
      {
        printf("Error opening statistics file\n");
        return -1;
      }

      run_process(NULL, &asset_names_array[current_process_num]);
      fclose(stat_file);
    }
  }
  else
    run_process(&parsed_args, NULL);

  return 0; 
}

int delete_bracket(const char* str_to_clean)
{
  if(!str_to_clean)
  {
    printf("[Delete bracket]:Invalid string to clean pointer\n");
    return -1;
  }
  int str_length = strlen(str_to_clean);

  if(((char*)str_to_clean)[str_length - 2] != ']')
    ((char*)str_to_clean)[str_length - 1] = '\0';
  else
    ((char*)str_to_clean)[str_length - 2] = '\0';
}

int load_all_names(std::string names[], FILE* file_from)
{
  if(!file_from)
  {
    printf("[Load records names]: Invalid file pointer to read from\n");
  }

  int counter = 0;

  char current_name[20] = {};

  while(!feof(file_from))
  {
    fscanf(file_from, "[[Record_name:%s]", current_name);
    delete_bracket(current_name);
   
    names[counter++].assign(current_name, strlen(current_name));
   
    fscanf(file_from, "%*[^\n]\n", NULL);
  }

  return counter;
}

int run_process(wsclient::ParseCmdArgs* parsed_args, std::string* record_name)
{
  wsclient::ConnectionData con_data;
  wsclient::EstablishConnection connection;

  close_connection((void*)&connection);
  reconnect((void*)&connection);

  set_sigint_handler();

  if(parsed_args)
  {
    if(parsed_args -> get_load_session())
    {
      con_data.LoadSession(parsed_args -> get_char_record_name(), (char*)RECORDS_FILENAME.c_str());
    }
    else
    {
      if(parsed_args -> get_session_record())
        con_data.CreateSession(*parsed_args, (char*)RECORDS_FILENAME.c_str());
      else
        con_data.CreateSession(*parsed_args, NULL);
    }
  }
  else
  {
    if(record_name)
    {
      con_data.LoadSession(record_name -> c_str(), (char*)RECORDS_FILENAME.c_str());
    }
    else
      return -1;
  }

  struct pthread_routine_tool tool;
    tool.is_there_first_request = con_data.is_there_query();

  if(tool.is_there_first_request);
    con_data.get_first_query(tool.first_server_request);

  connection.connect(ws_service_callback, con_data, (void*)&tool, pthread_routine);
}

static int websocket_write_back(struct lws *wsi_in, char *str_data_to_send) //static functions are functions that are only visible to other functions in the same file
{

  if(!wsi_in || !str_data_to_send)
  {
    printf("Websocket_write_back: Invalid wsi_in or str_data_to_send pointer.\n");
    return -1;
  }

  int bytes_amount_written = 0;
  int string_length = strlen(str_data_to_send);
  char *str_to_send_out = NULL;

  str_to_send_out = (char*)malloc((LWS_SEND_BUFFER_PRE_PADDING + string_length + LWS_SEND_BUFFER_POST_PADDING) * sizeof(char));
  //* setup the buffer*/
  memcpy (str_to_send_out + LWS_SEND_BUFFER_PRE_PADDING, str_data_to_send, string_length);
  //* write out*/
  bytes_amount_written = lws_write(wsi_in, (unsigned char*)str_to_send_out + LWS_SEND_BUFFER_PRE_PADDING, string_length, LWS_WRITE_TEXT);

  printf("[websocket_write_back] %s\n", str_data_to_send);
  //* free the buffer*/
  free(str_to_send_out);

  return bytes_amount_written;
}

static int ws_service_callback(struct lws *wsi,
                               enum lws_callback_reasons reason, 
                               void *user, void *in, size_t len)
{
  switch (reason) 
  {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        
      if(reconnection_attempt_num)
      {
        printf("[Main Service] Reconnection success.\n");
        reconnection_attempt_num = 0;
      }
      else
        printf("[Main Service] Connection with server established.\n");
      
      connection_flag = 1;
      break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        
      printf("[Main Service] Connect with server error.\n");

      connection_flag = 0;
      
      if(reconnection_attempt_num < MAX_RECONNECT_ATTEMPTS_NUM)
      {
      	printf("[Main Service] Attempt to reconnect %d...\n", reconnection_attempt_num++ + 1);
      	reconnect(NULL);
        close_connection(NULL);
      }
      else
      {
      	close_connection((void*)AUTHORIZED_CONTEXT_CLOSE);
      }

      break;

    case LWS_CALLBACK_CLOSED:                                               //end of websocket session
      
      printf("[Main Service] LWS_CALLBACK_CLOSED\n");

      connection_flag = 0;
      
      if(reconnection_attempt_num < MAX_RECONNECT_ATTEMPTS_NUM && !authorized_context_close_flag) 
      {
		    printf("[Main Service] Attempt to reconnect %d...\n", reconnection_attempt_num++ + 1);
      	reconnect(NULL);
        close_connection(NULL);
      }
      else
      {
      	close_connection((void*)AUTHORIZED_CONTEXT_CLOSE);
      }
      break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
      if(my_stat.update_time())
      {
        fclose(stat_file);
        current_statistics_file_pathname = my_stat.get_current_filepath_to_use();
        stat_file = fopen(current_statistics_file_pathname.c_str(), "a+");
      }

      if(strlen((char*)in) < MAX_SERVER_RESPOND_LENGTH)
      {
        record_current_second_data(in);
      } 
      
      break;

    default:
      
      break;
  }

  return 0;
}

struct Record {
  uint64_t timestamp;
  double close;
};

Record last;

static void record_current_second_data(void* in_str)
{
  /*
  {
    static struct timespec last_ts = {0};
    struct timespec cur_ts;
    clock_gettime(CLOCK_REALTIME, &cur_ts);

    double delta = cur_ts.tv_sec + cur_ts.tv_nsec/1e9 - (last_ts.tv_sec + last_ts.tv_nsec/1e9);
    last_ts = cur_ts;

    if (delta < 2)
      printf("record_current_second_data: pid %d delta %f data '%s'\n", getpid(), delta, (char*)in);
    else
      printf("record_current_second_data: pid %d WARNING delta %f data '%s'\n", getpid(), delta, (char*)in);
  }
  */
  
  std::string in_error;
  json11::Json in_json = json11::Json::parse((const char *)in_str, in_error);
  if (!in_error.empty()) {
    printf("json '%s' parse error '%s', skipping\n", in_str, in_error.c_str());
    return;
  }

  assert(in_json.is_object());
  auto in = in_json.object_items();

  if(in.count("time")) {
    printf("[pid %d] received data: '%s'\n", getpid(), in_str);

    assert(in["time"].is_number());
    assert(in.count("close") && in["close"].is_number());

    Record r;
    r.timestamp = in["time"].number_value();
    r.close = in["close"].number_value();

    assert(last.timestamp < r.timestamp);
    if (last.timestamp != 0) {
      for (uint64_t t = last.timestamp + 1; t <= r.timestamp; ++t) {
        fprintf(stat_file, "%llu %f\n", t, last.close);
      }
      fflush(stat_file);
    }

    last = r;
  } else {
    printf("[pid %d] received non-data: '%s'\n", getpid(), in_str);

    assert(in.count("servertime") && in["servertime"].is_number());
    uint64_t timestamp = in["servertime"].number_value();

    /* never print _current_ timestamp -- it may get superseded by next "full" data
     * if the server goes mad */
    if (last.timestamp != 0) {
      for (uint64_t t = last.timestamp + 1; t < timestamp; ++t) {
        fprintf(stat_file, "%llu %f\n", t, last.close);
        last.timestamp = t;
      }
      fflush(stat_file);
    }
  }
}

static void pthread_routine(void *tool_in, struct lws *wsi_pointer)
{
  if(!tool_in || !wsi_pointer)
  {
    printf("Pthread_routine: Invalid tool_in or wsi pinter");
    kill(0, SIGINT);
  }
 
  char server_request[MAX_SERVER_REQUEST_LEN] = {};

  struct pthread_routine_tool *tool = (struct pthread_routine_tool *)tool_in;

  printf("[pthread_routine] Greetings. This is pthread_routine.\n");

  //* waiting for connection with server done.*/
  while(!connection_flag)
      usleep(1000*20);

  printf("[pthread_routine] Server is ready to recieve messages send.\n");

  if(tool -> is_there_first_request)
  {
    websocket_write_back(wsi_pointer, tool -> first_server_request);
  
    lws_callback_on_writable(wsi_pointer);
  }

  /*
  while(1)
  {    
    fgets(server_request, MAX_SERVER_REQUEST_LEN, stdin);

    websocket_write_back(wsi, server_request);
  
    lws_callback_on_writable(wsi);
  }
  */
}

static void close_connection(void* parameter)
{
  static int call_counter = 0;
  static wsclient::EstablishConnection *current_connection = NULL;

  call_counter++;

  if(call_counter == 1 && parameter != NULL)
    current_connection = (wsclient::EstablishConnection*)parameter;
  else
  {
    if(current_connection)
    {
      if(parameter == (void*)AUTHORIZED_CONTEXT_CLOSE)
        authorized_context_close_flag = 1;

      current_connection -> close_connection();
    }
  }
}

static void reconnect(void* parameter)
{
  static int call_counter = 0;
  static wsclient::EstablishConnection *current_connection = NULL;

  call_counter++;

  if(call_counter == 1 && parameter != NULL)
    current_connection = (wsclient::EstablishConnection*)parameter;
  else
  {
    if(current_connection)
      current_connection -> try_to_reconnect(); 
  }
}

static void set_sigint_handler()                            // register the signal SIGINT handler 
{
  struct sigaction act;
  act.sa_handler = sighandler;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction(SIGINT, &act, 0);
}

static void sighandler(int sig)     
{
  close_connection((void*)AUTHORIZED_CONTEXT_CLOSE);
}