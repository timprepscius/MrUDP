#pragma once

#if defined __GNUC__
#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wcomma"
//#pragma GCC diagnostic ignored "-Wdocumentation"
//#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//#pragma GCC diagnostic ignored "-W#pragma-messages"

#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-W#pragma-messages"
#pragma clang diagnostic ignored "-Wdocumentation"

#endif

#include <boost/asio.hpp>

#if defined __GNUC__
#pragma GCC diagnostic pop
#endif
