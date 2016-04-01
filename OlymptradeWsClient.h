#pragma once
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <libwebsockets.h>
#include "json11/json11.hpp"
#include "StatisticsFileSystem.h"
#include "EstablishConnection.h"

static const int MAX_SERVER_REQUEST_LEN = 500;
static const int MAX_SERVER_RESPOND_LENGTH = 200;
static const int MAX_RECONNECT_ATTEMPTS_NUM = 5;
static const int AUTHORIZED_CONTEXT_CLOSE = 1;

struct pthread_routine_tool 
{
  char first_server_request[MAX_SERVER_REQUEST_LEN];
  int is_there_first_request;
  void* olymptrade_pointer;
};

struct Record 
{
  uint64_t timestamp;
  double close;
};	

class OlymptradeWsClient
{
	private:

		int authorized_context_close_flag;
		int reconnection_attempt_num;
		int connection_flag;
		std::string current_statistics_file_pathname;
		std::string records_filename;
		FILE* stat_file;
		Record last;

		wsclient::StatisticsFileSystem current_asset_statistics;
		wsclient::EstablishConnection current_ws_connection;
		wsclient::ConnectionData con_data;

		int websocket_write_back(struct lws *wsi_in, char *str_data_to_send); 
		void record_current_second_data(void* in_str);
		void reconnect();

		friend int ws_service_callback(struct lws* wsi,
                           enum lws_callback_reasons reason, 
                           void *user, void *in, size_t len);

		friend void pthread_routine(void *tool_in, struct lws* wsi_pointer);

	public:

		OlymptradeWsClient();
		int run_client(std::string current_records_filename, int current_asset_num, std::string asset_names_array[]);
		void close_connection(int parameter);
 };
