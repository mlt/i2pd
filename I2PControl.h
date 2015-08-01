#ifndef I2PCONTROL_H__
#define I2PCONTROL_H__

#include <boost/property_tree/ptree.hpp>
#include <string>
#include <map>
#include <functional>
#include <boost/asio.hpp>

namespace i2p
{
namespace client
{

	const char I2P_CONTROL_DEFAULT_PASSWORD[] = "itoopie";

	const char I2P_CONTROL_PROPERTY_ID[] = "id";
	const char I2P_CONTROL_PROPERTY_METHOD[] = "method";
	const char I2P_CONTROL_PROPERTY_PARAMS[] = "params";
	const char I2P_CONTROL_PROPERTY_RESULT[] = "result";

// methods
	const char I2P_CONTROL_METHOD_AUTHENTICATE[] = "Authenticate";
	const char I2P_CONTROL_METHOD_ECHO[] = "Echo";
	const char I2P_CONTROL_METHOD_I2PCONTROL[] = "I2PControl";
	const char I2P_CONTROL_METHOD_ROUTER_INFO[] = "RouterInfo";
	const char I2P_CONTROL_METHOD_ROUTER_MANAGER[] = "RouterManager";
	const char I2P_CONTROL_METHOD_NETWORK_SETTING[] = "NetworkSetting";

// params
	const char I2P_CONTROL_PARAM_API[] = "API";
	const char I2P_CONTROL_PARAM_PASSWORD[] = "Password";
	const char I2P_CONTROL_PARAM_TOKEN[] = "Token";
	const char I2P_CONTROL_PARAM_ECHO[] = "Echo";
	const char I2P_CONTROL_PARAM_RESULT[] = "Result";

// I2PControl
	const char I2P_CONTROL_I2PCONTROL_ADDRESS[] = "i2pcontrol.address";
	const char I2P_CONTROL_I2PCONTROL_PASSWORD[] = "i2pcontrol.password";
	const char I2P_CONTROL_I2PCONTROL_PORT[] = "i2pcontrol.port";

// RouterInfo requests
	const char I2P_CONTROL_ROUTER_INFO_UPTIME[] = "i2p.router.uptime";
	const char I2P_CONTROL_ROUTER_INFO_VERSION[] = "i2p.router.version";
	const char I2P_CONTROL_ROUTER_INFO_STATUS[] = "i2p.router.status";
	const char I2P_CONTROL_ROUTER_INFO_NETDB_KNOWNPEERS[] = "i2p.router.netdb.knownpeers";
	const char I2P_CONTROL_ROUTER_INFO_NETDB_ACTIVEPEERS[] = "i2p.router.netdb.activepeers";
	const char I2P_CONTROL_ROUTER_INFO_NET_STATUS[] = "i2p.router.net.status";
	const char I2P_CONTROL_ROUTER_INFO_TUNNELS_PARTICIPATING[] = "i2p.router.net.tunnels.participating";
	const char I2P_CONTROL_ROUTER_INFO_BW_IB_1S[] = "i2p.router.net.bw.inbound.1s";
	const char I2P_CONTROL_ROUTER_INFO_BW_OB_1S[] = "i2p.router.net.bw.outbound.1s";

// RouterManager requests
	const char I2P_CONTROL_ROUTER_MANAGER_SHUTDOWN[] = "Shutdown";
	const char I2P_CONTROL_ROUTER_MANAGER_SHUTDOWN_GRACEFUL[] = "ShutdownGraceful";
	const char I2P_CONTROL_ROUTER_MANAGER_RESEED[] = "Reseed";

	/**
	 * "Null" I2P control implementation, does not do actual networking.
	 */
	class I2PControlSession
	{

		public:
			class Response
			{
					std::string id;
					std::string version;
					std::map<std::string, std::string> parameters;
				public:
					Response(const std::string& id, const std::string& version = "2.0");
					std::string toJsonString() const;

					/**
					 * Set an ouptut parameter to a specified string.
					 * @todo escape quotes
					 */
					void setParam(const std::string& param, const std::string& value);
					void setParam(const std::string& param, int value);
					void setParam(const std::string& param, double value);
			};

			/**
			 * Sets up the appropriate handlers.
			 * @param ios the parent io_service object
			 */
			I2PControlSession(boost::asio::io_service& ios);

			/**
			 * Handle a json string with I2PControl instructions.
			 */
			Response handleRequest(std::stringstream& request);
		private:
			// For convenience
			typedef boost::property_tree::ptree PropertyTree;
			// Handler types
			typedef void (I2PControlSession::*MethodHandler)(
			    const PropertyTree& pt, Response& results
			);
			typedef void (I2PControlSession::*RequestHandler)(Response& results);

			// Method handlers
			void handleAuthenticate(const PropertyTree& pt, Response& response);
			void handleEcho(const PropertyTree& pt, Response& response);
			void handleI2PControl(const PropertyTree& pt, Response& response);
			void handleRouterInfo(const PropertyTree& pt, Response& response);
			void handleRouterManager(const PropertyTree& pt, Response& response);
			void handleNetworkSetting(const PropertyTree& pt, Response& response);

			// RouterInfo handlers
			void handleUptime(Response& response);
			void handleVersion(Response& response);
			void handleStatus(Response& response);
			void handleNetDbKnownPeers(Response& response);
			void handleNetDbActivePeers(Response& response);
			void handleNetStatus(Response& response);
			void handleTunnelsParticipating(Response& response);
			void handleInBandwidth1S(Response& response);
			void handleOutBandwidth1S(Response& response);

			// RouterManager handlers
			void handleShutdown(Response& response);
			void handleShutdownGraceful(Response& response);
			void handleReseed(Response& response);

			std::string password;

			std::map<std::string, MethodHandler> methodHandlers;
			std::map<std::string, RequestHandler> routerInfoHandlers;
			std::map<std::string, RequestHandler> routerManagerHandlers;
			std::map<std::string, RequestHandler> networkSettingHandlers;

			boost::asio::io_service& service;
			boost::asio::deadline_timer shutdownTimer;
	};

}
}

#endif // I2PCONTROL_H__
