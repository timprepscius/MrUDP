
#define CATCH_CONFIG_NO_POSIX_SIGNALS
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "../mrudp/base/Core.h"

using namespace timprepscius;
using namespace core;

int main( int argc, char* argv[] )
{
	xLogInitialize("mrudp-test.log");
	xLogActivateStory("testing");
//	xLogActivateStory("com::closes");
//	xLogActivateStory("mrudp::opens");
//	xLogActivateStory("mrudp::receive");
//	xLogActivateStory("mrudp::send");
//	xLogActivateStory("mrudp::probe");
//	xLogActivateStory("mrudp::proxy::compress");
//	xLogActivateStory("mrudp::proxy");
	
	using namespace Catch::clara;

	Catch::Session session;
	auto cli = session.cli();
	session.cli(cli);
	
	session.applyCommandLine(argc, argv);

	int result = 0;
//	while (true)
	{
		result = session.run( argc, argv );
	}
	
	return result;
}
