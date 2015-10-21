#include <unistd.h>
#include <signal.h>

#include <boost/program_options.hpp>

#include <fcgiapp.h>
#include <fcgio.h>

#include <roller/core/types.h>
#include <roller/core/log.h>
#include <roller/core/util.h>

#include "pid.h"
#include "server_controller.h"
#include "dummy_controller.h"

#define AB_SERVER_FASTCGI_SOCKET "/var/run/ab.socket"
#define AB_SERVER_FASTCGI_BACKLOG 8

using namespace roller;
namespace po = boost::program_options;

// globals
bool g_appRunning = false;

// handleSignal
void handleSignal( i32 sig ) {
	Log::i( "sig %d caught, flagging app to stop running", sig );
	g_appRunning = false;
	FCGX_ShutdownPending();
}

// handleRequest
void handleRequest( FCGX_Request& request );

// test PID controller
void handleStartPID();
void handleStopPID();
std::shared_ptr<DummyController> s_controller;

// mainNULL
i32 main( i32 argc, char** argv ) {

	printf( "ab server starting\n" );

	g_appRunning = true;

	setbuf( stdout, nullptr );

	signal( SIGINT, handleSignal );
	signal( SIGHUP, handleSignal );
	signal( SIGKILL, handleSignal );
	signal( SIGTERM, handleSignal );

	Log::setLogLevelMode( LOG_LEVEL_MODE_UNIX_TERMINAL );

	// initialize fast cgi
	printf( "initializing fcgx\n" );
	int result = FCGX_Init();
	if ( result ) {
		fprintf( stderr, "FCGX_Init() failed: %d\n", result );
		return 1;
	}

	// TODO: make socket file configurable
	int listenSocket = FCGX_OpenSocket( AB_SERVER_FASTCGI_SOCKET, AB_SERVER_FASTCGI_BACKLOG );
	if ( listenSocket < 0 ) {
		fprintf( stderr, "FCGX_OpenSocket(%s) failed: %d\n", AB_SERVER_FASTCGI_SOCKET, listenSocket);
		return 1;
	}

	FCGX_Request request;
	result = FCGX_InitRequest( &request, listenSocket, FCGI_FAIL_ACCEPT_ON_INTR );
	if ( result ) {
		fprintf( stderr, "FCGX_InitRequest() failed: %d\n", result );
	}

	while ( g_appRunning ) {

		result = FCGX_Accept_r( &request );
		if ( result ) {
			fprintf( stderr, "FCGX_Request_r failed: %d\n", result );
			break;
		}

		try {

			handleRequest( request);

		} catch( exception& e ) {
			Log::w( "Caught exception while trying to handle request (ignoring): %s", e.what());
			continue;
		}

	}

	return 0;
}

// handleRequest
void handleRequest( FCGX_Request& request ) {

	std::string scriptName = FCGX_GetParam("SCRIPT_NAME", request.envp);
	std::string pathInfo = FCGX_GetParam("PATH_INFO", request.envp);
	std::string requestMethod = FCGX_GetParam("REQUEST_METHOD", request.envp);
	std::string requestUri = FCGX_GetParam("REQUEST_URI", request.envp);

	Log::i( "handling request uri:    %s", requestUri.c_str());
	Log::i( "         script name:    %s", scriptName.c_str());
	Log::i( "         path info:      %s", pathInfo.c_str());
	Log::i( "         request method: %s", requestMethod.c_str());

	
	// get last part of URI
	// TODO: clean this up
	std::string handlerName = requestUri.substr(4, (requestUri.size() - 4));
	Log::i( "         request handler name: %s", handlerName.c_str());

	std::string jsonResponse = "{}";
	i32 responseCode = 200;
	if (handlerName == "start_pid") {

		try {
			handleStartPID();

			jsonResponse = "{ \"response\": \"OK\" }";
			responseCode = 200;

		} catch ( exception& e ) {
			Log::w( "caught exception in handleStartPID: %s", e.what() );

			jsonResponse = roller::makeString( "{ \"response\": \"Failed to start PID controller\", \"reason\": \"%s\"}",
					e.what());
			responseCode = 500;
		}

	} else if (handlerName == "stop_pid") {

		try {
			handleStopPID();

			jsonResponse = "{ \"response\": \"OK\" }";
			responseCode = 200;

		} catch ( exception& e ) {
			Log::w( "caught exception in handleStopPID: %s", e.what() );

			jsonResponse = roller::makeString( "{ \"response\": \"Failed to stop PID controller\", \"reason\": \"%s\"}",
					e.what());
			responseCode = 500;
		}

	} else if (handlerName == "status") {

		jsonResponse = "{ \"response\": \"All systems go\" }";
		responseCode = 200;

	} else {

		jsonResponse = "{ \"response\": \"Unrecognized Handler\" }";
		responseCode = 400;
	}

	// assemble HTTP response
	std::string statusResponse;
	switch (responseCode)
	{
		case 200:
			statusResponse = "200 OK";
			break;

		case 400:
			statusResponse = "400 Bad Request";
			break;

		default:
			Log::w( "HTTP response code not set, assuming 500" );
			// intentionally fall through

		case 500:
			statusResponse = "500 Internal Server Error";
			break;
	}

	Log::i( "Sending response.." );
	FCGX_FPrintF( request.out, "Status: %s\r\n", statusResponse.c_str() );
	FCGX_FPrintF( request.out, "Content-Type: application/json; charset=utf-8\r\n" );
	FCGX_FPrintF( request.out, "Content-Length: %d\r\n", jsonResponse.size() );
	FCGX_FPrintF( request.out, "\r\n" );
	FCGX_PutStr( jsonResponse.c_str(), jsonResponse.size(), request.out );

	FCGX_Finish_r( &request );
}

void handleStartPID() {

	if ( s_controller ) {
		throw RollerException( "Can't start dummy controller; already running" );
	}

	s_controller.reset( new DummyController());
	s_controller->start();
}

void handleStopPID() {

	if ( ! s_controller ) {
		throw RollerException( "Can't stop dummy controller; none running" );
	}

	s_controller->stop();
	s_controller->join();
	s_controller.reset();
}
