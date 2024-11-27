#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

// User AP data
#define USER_SSID	"YOUR_WIFI_SSID"
#define USER_PASS	"YOUR_PASS"

// Configuration: UDP port and maximum number of LEDs in payload
#define MXP_UDP_PORT 2711
#define MXP_MAXLEN 300

// Uncomment the following line to set a static IP address for user Wi-Fi AP. Useful if you want to send UDP packet to an specific IP address (no broadcast).
//#define SET_STATIC_IP

#ifdef SET_STATIC_IP
#define USER_AP_IP_ADDRESS		"192.168.1.10"
#define USER_AP_GATEWAY 		"192.168.1.1"
#define USER_AP_NETMASK			"255.255.255.0"
#endif

#endif