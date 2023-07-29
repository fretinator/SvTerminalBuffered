# SvTerminalBuffered
A newer version of SvTerminal with support for more ANSI escape sequences

Includes 2 shell scripts:

  socket_listener.sh - the shell script to run on your destination server - I use a Raspberry Pi.
  
  senv.sh - once connected, run this to set the terminal properties.

NOTE: Included basic color support and enough ANSI Escape Codes to handle a VI session for editing.

NOTE: You must inlucde a secrets.h with this information:

#ifndef SECRETS_H

#define SECRETS_H

const String host = "Server IP"; // Example 192.168.1.233

const int port = ####; // Example 8080 

const char* my_ssid = "Wifi ESSID"; // Replace with you WiFi network name

const char* my_password = "WIF PASSWORD"; // Replace with your WiFi password

#endif
