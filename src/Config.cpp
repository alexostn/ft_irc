// Config implementation
// Manages server configuration settings

#include "irc/Config.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <cctype>

// Initialize static server name
const std::string Config::serverName_ = "ft_irc";

Config::Config(int port, const std::string& password)
	: port_(port), password_(password) {
}

int Config::getPort() const {
	return port_;
}

const std::string& Config::getPassword() const {
	return password_;
}

const std::string& Config::getServerName() {
	return serverName_;
}

void Config::setPort(int port) {
	port_ = port;
}

void Config::setPassword(const std::string& password) {
	password_ = password;
}

Config Config::parseArgs(int argc, char** argv) {
	if (argc != 3) {
		throw std::runtime_error("Usage: ./ircserv <port> <password>");
	}

	std::string portStr = argv[1];
	if (portStr.length() > 5)
		throw std::runtime_error("Invalid port: value too large");
	for (size_t i = 0; i < portStr.length(); ++i) {
		if (!std::isdigit(static_cast<unsigned char>(portStr[i])))
			throw std::runtime_error("Invalid port: must be a number");
	}

	int port = std::atoi(portStr.c_str());
	if (port < 1 || port > 65535)
		throw std::runtime_error("Invalid port: expected range: 1-65535");

	return Config(port, argv[2]);
}
