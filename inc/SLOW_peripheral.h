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
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include "helper_functions.h"

#define BUFFER_SIZE 1472

typedef struct
{
    // Informações de rede
    int sockfd; // int é o tipo padrão para socket file descriptors
    struct sockaddr_in serv_addr;
    char server_ip[16];
    uint16_t server_port;

    // Estado do protocolo SLOW
    __uint128_t sid;
    uint32_t our_next_seqnum;
    uint32_t last_server_seqnum;

    uint32_t last_ack_from_server;
    uint16_t remote_window_size;
    uint32_t sttl;
} SLOW_connection_t; // Renomeado para indicar que é a conexão toda

typedef struct __attribute__((packed))
{
    __uint128_t uuid;
    uint32_t sttl_n_flags;
    uint32_t seqnum;
    uint32_t acknum;
    uint16_t window;
    uint8_t fid;
    uint8_t fo;
} SLOW_header_t;

void SLOW_print_header(
    const SLOW_header_t *header, 
    const char *label);

SLOW_connection_t *SLOW_resolve_server_address(
    SLOW_connection_t *connection,
    char server_ip[16],
    uint16_t server_port);

SLOW_connection_t *SLOW_resolve_server_hostname(
    SLOW_connection_t *connection,
    const char *hostname,
    uint16_t server_port);

SLOW_connection_t *SLOW_open_socket(
    SLOW_connection_t *connection,
    char server_ip[16],
    uint16_t server_port);

SLOW_connection_t *SLOW_close_socket(
    SLOW_connection_t *connection);

bool SLOW_send_packet(
    SLOW_connection_t *connection,
    const SLOW_header_t *header,
    const uint8_t data[],
    uint16_t data_len);

bool SLOW_receive_packet(
    SLOW_connection_t *connection,
    SLOW_header_t *header_out,
    uint8_t *data_out,
    uint16_t *data_len_out);

SLOW_connection_t *SLOW_3_way_handshake(
    SLOW_connection_t *connection,
    const char *hostname,
    uint16_t server_port);

bool SLOW_disconnect(SLOW_connection_t *connection);

bool SLOW_send_data(
    SLOW_connection_t *connection,
    const uint8_t *data,
    uint16_t data_len);

bool SLOW_revive_and_send_data(
    SLOW_connection_t *connection,
    const uint8_t *data,
    uint16_t data_len);
