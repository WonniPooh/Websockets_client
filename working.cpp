/*
 * libwebsockets-client
 *
 * Copyright (C) 2016 Alex Serbin
 *
 * g++ working.cpp OlymptradeWsClient.cpp EstablishConnection.cpp ConnectionData.cpp StatisticsFileSystem.cpp ParseCmdArgs.cpp -L/usr/local/lib -lwebsockets -pthread -g -std=c++11
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "ParseCmdArgs.h"
#include "OlymptradeWsClient.h"
     
static const std::string RECORDS_FILENAME = "/Creation_data";
const int MAX_PROGRAMM_PATH_LEN = 500;
const int ASSETS_ANMOUNT = 20;                                   //txlib -> examples ldview -- graph algorithms doxygen help + dynamic array for using threads;
                                        
static std::string RECORDS_FILEPATH;                            //global variables starts with capital      doxygen.org -> manual   txlib -> help in every function + tx/doc files

int load_all_names(std::string names[], FILE* file_from);
int delete_bracket(const char* str_to_clean);
static void set_sigint_handler();
//needs initialization (first time call with olymp pointer as data)
static void sighandler(int sig, siginfo_t* siginfo, void* data);

int main(int argc, char **argv)
{
  pid_t pid = 1;
  int records_amount = 0;
  int current_process_num = 0;

  char* getcwd_result_ptr = getcwd(NULL, MAX_PROGRAMM_PATH_LEN);

  RECORDS_FILEPATH = getcwd_result_ptr + RECORDS_FILENAME;

  free(getcwd_result_ptr);

  wsclient::ParseCmdArgs parsed_args(argc, argv);

  std::string asset_names_array[ASSETS_ANMOUNT];

  if(parsed_args.load_all_data())
  {
    FILE* my_file = fopen(RECORDS_FILEPATH.c_str(), "r");
    
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
      OlymptradeWsClient olymp_client;
      
      sighandler(0, NULL, (void*)&olymp_client);
      set_sigint_handler();

      olymp_client.run_client(RECORDS_FILEPATH, current_process_num, asset_names_array);
    }
  }
  else
    printf("Current version works only with Olymptrade\n");

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

static void set_sigint_handler()                            // register the signal SIGINT handler 
{
  struct sigaction act;
  act.sa_sigaction = sighandler;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  sigaction(SIGINT, &act, 0);
}

static void sighandler(int sig, siginfo_t* siginfo, void* data)   
{
  static int call_counter = 0;

  static OlymptradeWsClient* current_client = NULL;

  if(!call_counter++)
    current_client = (OlymptradeWsClient*)data;
  else
    current_client -> close_connection(AUTHORIZED_CONTEXT_CLOSE);
}