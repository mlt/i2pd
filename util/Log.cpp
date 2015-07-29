#include "Log.h"
//#include <boost/log/core/core.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/keywords/channel.hpp>
#include <boost/log/keywords/severity.hpp>

#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <fstream>
//#include <string>
#include <boost/format.hpp>

namespace i2p
{
    namespace log
    {

        namespace keywords = boost::log::keywords;

        logger g_logger("global"); // FIXME: global catch all for now

        static const char * g_LogLevelStr[eNumLogLevels] =
        {
            "fatal",    // eLogFatal
            "critical", // eLogCritical
            "error",    // eLogError
            "warn",     // eLogWarning
            "missing",  // eLogMissing
            "info",     // eLogInfo
            "debug",    // eLogDebug
            "debug1",   // eLogDebug1
            "debug2",   // eLogDebug2
            "trace",    // eLogTrace
            "trace1",   // eLogTrace1
            "trace2"    // eLogTrace2
        };

        boost::log::attributes::mutable_constant< uint64_t > sent_attr(0);
        boost::log::attributes::mutable_constant< uint64_t > received_attr(0);

        void UpdateSent(uint64_t x) { sent_attr.set(x); }
        void UpdateReceived(uint64_t x) { received_attr.set(x); }

        void log_setup(const std::string& config_file) {
            namespace logging = boost::log;
            logging::register_simple_formatter_factory< LogLevel, char >("Severity");
            std::ifstream file(config_file);
            if (file.is_open())
                logging::init_from_stream(file);
            logging::add_common_attributes(); // registers "LineID", "TimeStamp", "ProcessID" and "ThreadID"
            boost::shared_ptr<logging::core> pCore = logging::core::get();
            pCore->add_global_attribute("Scope", logging::attributes::named_scope());
            pCore->add_global_attribute("Sent", sent_attr);
            pCore->add_global_attribute("Received", received_attr);
        }

        logger::logger(const std::string &name, const void *inst)
            : severity_channel_logger(keywords::severity = eLogInfo, keywords::channel = name)
            , instance((boost::format("0x%1$p") % inst).str())

        {
            add_attribute("Instance", instance);
        }

        logger::logger()
        : severity_channel_logger(keywords::severity = eLogInfo)//, keywords::channel = "")
        , instance("")
        {
        }

        void logger::SetChannel(const std::string &name)
        {
            channel(name);
        }

        void logger::SetInstance(const std::string &name)
        {
            instance.set(name);
        }

    } // namespace log
} // namespace i2p

/**
  * This is used by boost log formatting via [register_simple_formatter_factory](http://www.boost.org/doc/libs/1_58_0/libs/log/doc/html/boost/log/register_simpl_idp52561968.html)
  */
template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, LogLevel lvl)
{
    if (0 <= lvl && lvl < eNumLogLevels)
        strm << i2p::log::g_LogLevelStr[lvl];
    else
        strm << static_cast<int>(lvl);
    return strm;
}
