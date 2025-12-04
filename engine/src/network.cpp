#include "../include/network.hpp"
#include <sstream>
#include <iomanip>
#include <regex>
#include <iostream>

std::string NetworkBase::getLocalIP() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    
    if (getifaddrs(&ifaddr) == -1) {
        return "127.0.0.1";
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (std::string(ifa->ifa_name) == "lo") {
                continue;
            }
            
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                               host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            
            if (s == 0) {
                std::string ip = host;
                if (ip.find("127.") != 0 && ip.find("169.254.") != 0) {
                    freeifaddrs(ifaddr);
                    return ip;
                }
            }
        }
    }
    
    freeifaddrs(ifaddr);
    return "127.0.0.1";
}

bool NetworkBase::isValidIP(const std::string& ip) {
    std::regex ip_regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
                       "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return std::regex_match(ip, ip_regex);
}

bool NetworkBase::isValidPort(int port) {
    return port > 0 && port <= 65535;
}

bool NetworkBase::setSocketNonBlocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) != -1;
}

bool NetworkBase::setSocketTimeout(int socket, int seconds) {
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    
    return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0 &&
           setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
}

int NetworkBase::getSendFlags() {
    #ifdef __linux__
        return MSG_CONFIRM;
    #else
        return 0; // macOS doesn't support MSG_CONFIRM
    #endif
}