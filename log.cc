// -*- mode: c++; -*-

#define BOOST_LOG_DYN_LINK 1

#include <cstdarg>
#include <cstddef>
#include <thread>
#include <mutex>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::log;

#include "log.hh"

BOOST_LOG_ATTRIBUTE_KEYWORD (severity, "Severity", trivial::severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD (channel, "Channel", std::string)

BOOST_LOG_GLOBAL_LOGGER_INIT(pof_logger, pof_logger_type) {
    pof_logger_type logger;

    using min_severity_filter = expressions::channel_severity_filter_actor<
        std::string, trivial::severity_level >;

    min_severity_filter filter = expressions::channel_severity_filter (
        channel, severity);

    filter ["general"] = trivial::warning;

    add_console_log (
        std::clog,
        keywords::filter = filter || severity >= trivial::error,
        keywords::format = expressions::stream
        << expressions::format_date_time< boost::posix_time::ptime > (
            "TimeStamp", "%Y%m%dT%H%M%S.%f") << ": ["
        << channel << "] "
        << expressions::attr< trivial::severity_level >("Severity") << ": "
        << expressions::smessage );

    return add_common_attributes (), logger;
}
