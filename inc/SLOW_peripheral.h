#pragma once 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <netdb.h>

#include <stdint.h>

#define BUFFER_SIZE 1472

typedef struct 
{
    char server_ip[16];
    struct sockaddr_in serv_addr;
    uint16_t server_port;
    uint64_t sockfd;
} SLOW_connection_header;

typedef struct __attribute__((packed)) {
    __uint128_t uuid;
    uint32_t sttl_n_flags;
    uint32_t seqnum;
    uint32_t acknum;
    uint16_t window;
    uint8_t fid;
    uint8_t fo;
} SLOW_header_t;

#include "SLOW_peripheral.h"

SLOW_connection_header* SLOW_resolve_server_address(
    SLOW_connection_header* connection, 
    char server_ip[16], 
    uint16_t server_port
);

SLOW_connection_header* SLOW_resolve_server_hostname(
    SLOW_connection_header* connection, 
    const char* hostname,        
    uint16_t server_port
);

SLOW_connection_header* SLOW_open_socket(
    SLOW_connection_header* connection, 
    char server_ip[16], 
    uint16_t server_port
);

SLOW_connection_header* SLOW_close_socket(
    SLOW_connection_header* connection
);

bool SLOW_send_packet(
    SLOW_connection_header* connection,
    SLOW_header_t* header, 
    uint8_t data[1440], 
    uint16_t data_len
);

bool SLOW_receive_packet(
    SLOW_connection_header* connection, 
    SLOW_header_t* header_out,         
    uint8_t* data_out,                 
    uint16_t* data_len_out             
);

