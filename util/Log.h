#ifndef LOG_H__
#define LOG_H__

#ifndef I2PD_NO_LOGGING

#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>

enum LogLevel
{
    eLogFatal = 0,
    eLogCritical,
    eLogError,
    eLogWarning,
    eLogMissing, //< use in stubs for non-implemented specs
	eLogInfo,
    eLogDebug,
    /// Use for internal debugging, don't commit code with these
    eLogDebug1,
    eLogDebug2,
    eLogTrace,
    eLogTrace1,
    eLogTrace2,
    eNumLogLevels
};

namespace i2p
{
    namespace log
    {

        /** \brief Initialize Boost.Log v2 with setting from configuration file.
          *
          * Register log level formatter, add Scope, Sent, Received,
          * and [standard attributes](http://www.boost.org/doc/libs/develop/libs/log/doc/html/log/tutorial/attributes.html)
          * @param[in] config_file configuration file name
          */
        void log_setup(const std::string& config_file);

        void UpdateSent(uint64_t x); //< \internal Update log attribute for transmitted data counter
        void UpdateReceived(uint64_t x); //< \internal Update log attribute for received data counter
#define I2PD_LOG_UPDATE_SENT(x) i2p::log::UpdateSent(x)
#define I2PD_LOG_UPDATE_RECEIVED(x) i2p::log::UpdateReceived(x)

        /**
          * \brief Severity & channel aware logger class
          *
          * \note Base classes will use descendant's channel.
          */
        class logger : public boost::log::sources::severity_channel_logger < LogLevel > {
        public:
            logger(const std::string &name, const void* inst=NULL);
            void SetChannel(const std::string &name); //< Update channel name
            void SetInstance(const std::string &name); //< Update instance name
        private:
            logger(); //< Disable unnamed instantiation
            boost::log::attributes::mutable_constant< std::string > instance;
        };
        extern logger g_logger; //< FIXME: global catch all for all other LogPrint statements for now
#define I2PD_SEV(s) BOOST_LOG_SEV(m_logger, s)
#define I2PD_LOG BOOST_LOG(m_logger)
#define I2PD_ERR BOOST_LOG_SEV(m_logger, eLogError)
#define I2PD_WARN BOOST_LOG_SEV(m_logger, eLogWarning)
#define I2PD_DBG BOOST_LOG_SEV(m_logger, eLogDebug)
#define I2PD_DECLARE_STATIC_LOGGER static i2p::log::logger m_logger;
#define I2PD_DEFINE_STATIC_LOGGER(name) i2p::log::logger name::m_logger (#name)
#define I2PD_DECLARE_LOGGER i2p::log::logger m_logger; i2p::log::logger& getLogger() { return m_logger; }
        /**
          * \internal \brief Extract name part from function name obtained from __FUNCTION__ , __func__ and alike
          */
        inline const char* nameOnly(const char* name)
        {
            const char *out;
            if (NULL != (out = strrchr(name, ':'))) return out + 1;
            if (NULL != (out = strrchr(name, ' '))) return out + 1;
            return name;
        }

        template<typename TValue>
        void LogPrint(boost::log::record_ostream& s, TValue arg)
        {
            s << arg;
        }

        template<typename TValue, typename... TArgs>
        void LogPrint(boost::log::record_ostream& s, TValue arg, TArgs... args)
        {
            LogPrint(s, arg);
            LogPrint(s, args...);
        }
        template<typename... TArgs>
        void LogPrintImpl(logger& lg, LogLevel level, TArgs... args)
        {
            boost::log::record rec = lg.open_record(boost::log::keywords::severity = level);
            if(rec)
            {
                boost::log::record_ostream strm(rec);
                LogPrint(strm, args...);
                strm.flush();
                lg.push_record(boost::move(rec));
            }
        }

// #define I2PD_DEFINE_LOGGER m_logger( nameOnly(typeid(*this).name()) )
#define I2PD_DEFINE_LOGGER i2p::log::log_enabled( i2p::log::nameOnly(__FUNCTION__) )
#define I2PD_LOG_ENABLED public virtual i2p::log::log_enabled

        /**
          * \brief LogPrint compatibility layer
          *
          * Allows to add some extra information (class, instance) into log with existing LogPrint statements.
          */
        class log_enabled {
        public:
            log_enabled(const std::string& name) : m_logger(name, this) {}
            logger m_logger;
            template<typename... TArgs>
            void LogPrint(LogLevel level, TArgs... args)
            {
                LogPrintImpl(m_logger, level, args...);
            }
            template<typename... TArgs>
            void LogPrint(TArgs... args) { LogPrint(eLogInfo, args...); }
        }; // class log_enabled
        /** \brief Name containing instance for loggin purposes, e.g. with an abbreviated hash */
#define I2PD_LOG_INSTANCE(name) m_logger.SetInstance(name)
#define I2PD_LOG_TAG_SCOPE(tg) BOOST_LOG_SCOPED_LOGGER_TAG(m_logger, "Tag", tg)
        // the following can't be filtered http://boost-log.sourceforge.net/libs/log/doc/html/log/rationale/why_attribute_manips_dont_affect_filters.html
        // #define I2PD_WARN_TAG(tg) I2PD_WARN << boost::log::add_value("Tag", tg)

        //BOOST_LOG_ATTRIBUTE_KEYWORD(instance_attr, "Instance", void*)
        //BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr, "Tag", std::string)
        //#define I2PD_TAG(tg) BOOST_LOG_WITH_PARAMS(m_logger, (boost::log::keywords::severity = eLogDebug)(tag::tag_attr = (tg)))
        // this requires feature definition http://boost-log.sourceforge.net/libs/log/doc/html/log/extension/sources.html
        // BOOST_PARAMETER_KEYWORD(tag_ns, tag)
        //#define I2PD_WARN_TAG(tg) BOOST_LOG_WITH_PARAMS(m_logger, (boost::log::keywords::severity = eLogWarning)(tag = (tg)))
    } // namespace log
} // namespace i2p

template<typename... TArgs>
void LogPrint(LogLevel level, TArgs... args)
{
    using namespace i2p::log;
    LogPrintImpl(g_logger, level, args...);
}

template<typename... TArgs>
void LogPrint (TArgs... args)
{
	LogPrint (eLogInfo, args...);
}

#else // I2PD_NO_LOGGING
// squeeze out stuff for embedded devices
namespace i2p
{
    namespace log
    {
        // optimize out http://stackoverflow.com/a/16028265/673826
        static class logger {
        public:
            logger () {};
            logger (const char* name) {};
        } dev_null;
        template <typename T>
        inline const logger & operator<<(const logger & dest, T) { return dest; }
        struct log_enabled { log_enabled() {} };
#define I2PD_SEV(s) i2p::log::dev_null
#define I2PD_LOG i2p::log::dev_null
#define I2PD_ERR i2p::log::dev_null
#define I2PD_WARN i2p::log::dev_null
#define I2PD_DBG i2p::log::dev_null
#define I2PD_WARN_TAG(tg) i2p::log::dev_null
#define I2PD_DECLARE_STATIC_LOGGER
#define I2PD_DEFINE_STATIC_LOGGER(name)
#define I2PD_DECLARE_LOGGER
#define I2PD_LOG_INSTANCE(name)
#define I2PD_LOG_TAG_SCOPE(name)
#define BOOST_LOG_FUNCTION()
#define LogPrint(...)
#define I2PD_DEFINE_LOGGER i2p::log::log_enabled()
#define I2PD_LOG_ENABLED public virtual i2p::log::log_enabled
#define I2PD_LOG_UPDATE_SENT(x)
#define I2PD_LOG_UPDATE_RECEIVED(x)
    } // namespace log
} // namespace i2p
#endif // I2PD_NO_LOGGING

#endif
