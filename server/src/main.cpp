#include <unistd.h>
#include <signal.h>

#include <boost/program_options.hpp>

#include <fcgiapp.h>
#include <fcgio.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>

#include <roller/core/types.h>
#include <roller/core/log.h>
#include <roller/core/util.h>

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
}

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

	// initialize cgicc
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

		// cgicc::CgiInput input;
		// cgicc::Cgicc parser( &input );

		// TODO: handle request
		std::string response = "{ response: \"hello\" }";

		FCGX_FPrintF( request.out, "Status: %s\r\n", "200 OK" );
		FCGX_FPrintF( request.out, "Content-Type: application/json; charset=utf-8\r\n" );
		FCGX_FPrintF( request.out, "Content-Length: %d\r\n", response.size() );
		FCGX_FPrintF( request.out, "\r\n" );
		FCGX_PutStr( response.c_str(), response.size(), request.out );

		FCGX_Finish_r( &request );

		usleep( 3 * 1000 );
	}

	return 0;
}
