// -*- mode: c++; -*-

#ifndef POF_LOG_HH
#define POF_LOG_HH

#define BOOST_LOG_DYN_LINK 1

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <string>

using pof_logger_type = boost::log::sources::severity_channel_logger_mt<
    boost::log::trivial::severity_level, std::string >;

BOOST_LOG_GLOBAL_LOGGER(pof_logger, pof_logger_type)

#define FS2_LOG(channel, severity) BOOST_LOG_CHANNEL_SEV( \
        pof_logger::get (), channel, boost::log::trivial::severity)

#define FS2_EE(channel) FS2_LOG (channel, error)
#define FS2_WW(channel) FS2_LOG (channel, warning)
#define FS2_II(channel) FS2_LOG (channel, info)

#define EE  FS2_EE("general")
#define WW  FS2_WW("general")
#define II  FS2_II("general")

#endif // POF_LOG_LOG_HH
