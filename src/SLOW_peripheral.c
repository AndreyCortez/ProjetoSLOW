#include "SLOW_peripheral.h"

void SLOW_print_header(const SLOW_header_t *header, const char *label)
{
    printf("--- Detalhes do Cabeçalho: %s ---\n", label);

    printf("  SID               : ");
    print_uuid(header->uuid);
    printf("\n");

    uint32_t sttl_n_flags_host = ntohl(header->sttl_n_flags);
    uint32_t sttl = sttl_n_flags_host & 0x07FFFFFF;

    printf("  STTL              : %u\n", sttl);
    printf("  Flags (Hex)       : 0x%08X\n", sttl_n_flags_host);
    printf("    > Connect (28)  : %d\n", (sttl_n_flags_host >> 28) & 1);
    printf("    > Revive (27)   : %d\n", (sttl_n_flags_host >> 27) & 1);
    printf("    > ACK (29)      : %d\n", (sttl_n_flags_host >> 29) & 1);
    printf("    > Accept/Rej(30): %d\n", (sttl_n_flags_host >> 30) & 1);
    printf("    > More Bits (31): %d\n", (sttl_n_flags_host >> 31) & 1);

    printf("  Sequence Number   : %u\n", ntohl(header->seqnum));
    printf("  Acknowledge Number: %u\n", ntohl(header->acknum));
    printf("  Window Size       : %u\n", ntohs(header->window));
    printf("  Fragment ID       : %u\n", header->fid);
    printf("  Fragment Offset   : %u\n", header->fo);
    printf("---------------------------------------------------\n");
}

SLOW_connection_t *SLOW_resolve_server_hostname(
    SLOW_connection_t *connection,
    const char *hostname,
    uint16_t server_port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int status;
    char port_str[6];

    snprintf(port_str, sizeof(port_str), "%d", server_port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    status = getaddrinfo(hostname, port_str, &hints, &result);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return NULL;
    }

    rp = result;
    if (rp != NULL)
    {
        memcpy(&(connection->serv_addr), rp->ai_addr, rp->ai_addrlen);

        inet_ntop(rp->ai_family, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr),
                  connection->server_ip, sizeof(connection->server_ip));

        connection->server_port = server_port;
    }
    else
    {
        fprintf(stderr, "Não foi possível resolver o hostname: %s\n", hostname);
        freeaddrinfo(result);
        return NULL;
    }

    freeaddrinfo(result);

    return connection;
}

SLOW_connection_t *SLOW_resolve_server_address(
    SLOW_connection_t *connection,
    char server_ip[16],
    uint16_t server_port)
{

    struct sockaddr_in serv_addr;

    // Zera a estrutura de endereço e a preenche
    memset(&serv_addr, 0, sizeof(serv_addr));

    // 2. Configurar o endereço do servidor (Central)
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    // Converte o endereço IP de string para formato de rede
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        perror("Endereço inválido");
        return NULL;
    }

    connection->serv_addr = serv_addr;
    strcpy(connection->server_ip, server_ip);
    connection->server_port = server_port;

    return connection;
}

SLOW_connection_t *SLOW_open_socket(
    SLOW_connection_t *connection,
    char server_ip[16],
    uint16_t server_port)
{
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Erro ao criar socket");
        return NULL;
    }

    connection->sockfd = sockfd;

    return connection;
}

SLOW_connection_t *SLOW_close_socket(
    SLOW_connection_t *connection)
{
    printf("Fechando o socket.\n");
    close(connection->sockfd);
    connection->sockfd = -1;
    return connection;
}

bool SLOW_send_packet(
    SLOW_connection_t *connection,
    const SLOW_header_t *header,
    const uint8_t data[],
    uint16_t data_len)
{

    char buffer[BUFFER_SIZE];

    // --- Início da Lógica do Protocolo SLOW ---

    memcpy(buffer, header, sizeof(SLOW_header_t));
    memcpy(buffer + sizeof(SLOW_header_t), data, data_len);

    printf("Enviando pacote de conexão para %s:%d\n",
           connection->server_ip,
           connection->server_port);

    // 4. Enviar o pacote para o servidor
    uint16_t total_len = sizeof(SLOW_header_t) + data_len;
    if (sendto(connection->sockfd, buffer, total_len, 0, (const struct sockaddr *)&(connection->serv_addr), sizeof(connection->serv_addr)) < 0)
    {
        perror("Erro no sendto");
        close(connection->sockfd);
        return false;
    }

    return true;
}

bool SLOW_receive_packet(
    SLOW_connection_t *connection,
    SLOW_header_t *header_out, // Ponteiro para preencher o cabeçalho recebido
    uint8_t *data_out,         // Buffer para preencher com os dados recebidos
    uint16_t *data_len_out     // Ponteiro para retornar o tamanho dos dados recebidos
)
{
    unsigned char buffer[BUFFER_SIZE]; // Buffer local para receber o pacote completo
    socklen_t addr_len = sizeof(connection->serv_addr);

    int n_bytes = recvfrom(
        connection->sockfd,
        buffer,
        BUFFER_SIZE,
        0,
        (struct sockaddr *)&(connection->serv_addr),
        &addr_len);

    if (n_bytes < 0)
    {
        // Isso pode acontecer por timeout, o que é normal, então não imprimimos erro.
        return false;
    }

    if (n_bytes < sizeof(SLOW_header_t))
    {
        fprintf(stderr, "Erro: Pacote recebido é menor que o cabeçalho SLOW.\n");
        return false;
    }

    if (header_out != NULL)
    {
        memcpy(header_out, buffer, sizeof(SLOW_header_t));
    }

    uint16_t received_data_len = n_bytes - sizeof(SLOW_header_t);

    // Retorna o tamanho dos dados se o chamador forneceu um ponteiro para ele.
    if (data_len_out != NULL)
    {
        *data_len_out = received_data_len;
    }

    // Copia os dados se o chamador forneceu um buffer e se de fato há dados.
    if (data_out != NULL && received_data_len > 0)
    {
        memcpy(data_out, buffer + sizeof(SLOW_header_t), received_data_len);
    }
    // --- FIM DA CORREÇÃO ---

    return true;
}

SLOW_connection_t *SLOW_3_way_handshake(SLOW_connection_t *connection, const char *hostname, uint16_t server_port)
{
    // --- FASE 1: PREPARAÇÃO DA REDE ---
    if (SLOW_open_socket(connection, NULL, 0) == NULL)
        return NULL;
    if (SLOW_resolve_server_hostname(connection, hostname, server_port) == NULL)
    {
        SLOW_close_socket(connection);
        return NULL;
    }

    // Configura um timeout de 2 segundos para a resposta
    struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
    if (setsockopt(connection->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Erro ao configurar o timeout do socket");
        SLOW_close_socket(connection);
        return NULL;
    }

    // --- FASE 2: ENVIAR PACOTE CONNECT (Passo 1 do handshake) ---
    SLOW_header_t connect_header;
    memset(&connect_header, 0, sizeof(connect_header));
    connect_header.sttl_n_flags = htonl(1 << 28); // Flag Connect no bit 28
    connect_header.window = htons(1440);          // Window não pode ser zero

    printf("Handshake: Enviando Connect...\n");
    if (!SLOW_send_packet(connection, &connect_header, NULL, 0))
    {
        fprintf(stderr, "Handshake falhou: não foi possível enviar o pacote Connect.\n");
        SLOW_close_socket(connection);
        return NULL;
    }

    // --- FASE 3: RECEBER PACOTE SETUP (Passo 2 do handshake) ---
    SLOW_header_t setup_header;
    uint8_t temp_buffer[1440];
    uint16_t temp_len;

    printf("Handshake: Aguardando Setup...\n");
    if (!SLOW_receive_packet(connection, &setup_header, temp_buffer, &temp_len))
    {
        fprintf(stderr, "Handshake falhou: servidor não respondeu ao Connect.\n");
        SLOW_close_socket(connection);
        return NULL;
    }

    uint32_t received_sttl_n_flags = ntohl(setup_header.sttl_n_flags);
    int accepted = (received_sttl_n_flags >> 30) & 1;

    if (!accepted)
    {
        fprintf(stderr, "Handshake falhou: servidor rejeitou a conexão.\n");
        SLOW_close_socket(connection);
        return NULL;
    }

    printf("Handshake: Conexão aceita pelo servidor.\n");

    // Extrai e guarda o sttl (os 27 bits inferiores)
    connection->sttl = received_sttl_n_flags & 0x07FFFFFF;

    // Guarda o estado da conexão
    connection->sid = setup_header.uuid;
    connection->last_server_seqnum = ntohl(setup_header.seqnum);
    connection->our_next_seqnum = 1; // Nosso primeiro seqnum será 1 (0 foi o connect)

    // --- FASE 4: ENVIAR ACK FINAL (Passo 3 do handshake) ---
    SLOW_header_t final_ack_header;
    memset(&final_ack_header, 0, sizeof(final_ack_header));

    final_ack_header.uuid = connection->sid; // Usa o SID da sessão
    // Regra do ACK puro: seqnum == acknum
    final_ack_header.seqnum = htonl(connection->last_server_seqnum);
    final_ack_header.acknum = htonl(connection->last_server_seqnum);
    final_ack_header.sttl_n_flags = htonl(1 << 29); // Flag ACK no bit 29
    final_ack_header.window = htons(1440);

    printf("Handshake: Enviando ACK final...\n");
    if (!SLOW_send_packet(connection, &final_ack_header, NULL, 0))
    {
        fprintf(stderr, "Handshake falhou: não foi possível enviar o ACK final.\n");
        SLOW_close_socket(connection);
        return NULL;
    }

    printf("Handshake de 3 vias concluído com sucesso!\n");
    return connection;
}

bool SLOW_disconnect(SLOW_connection_t *connection)
{
    SLOW_header_t disconnect_header;
    memset(&disconnect_header, 0, sizeof(disconnect_header));

    disconnect_header.uuid = connection->sid;

    uint32_t disconnect_flags = (1 << 28) | (1 << 27) | (1 << 29);
    disconnect_header.sttl_n_flags = htonl(disconnect_flags);

    disconnect_header.seqnum = htonl(connection->our_next_seqnum);
    disconnect_header.acknum = htonl(connection->last_server_seqnum);

    disconnect_header.window = htons(0);

    // printf("Enviando pacote de Disconnect...\n");
    // print_hex("Cabeçalho de Disconnect", (unsigned char*)&disconnect_header, sizeof(disconnect_header));

    return SLOW_send_packet(connection, &disconnect_header, NULL, 0);
}

bool SLOW_send_data(SLOW_connection_t *connection, const uint8_t *data, uint16_t data_len)
{
    if (data == NULL || data_len == 0)
        return false;

    // --- LÓGICA DE FRAGMENTAÇÃO ---
    int num_fragments = (data_len + MAX_DATA_PER_PACKET - 1) / MAX_DATA_PER_PACKET;
    // Gera um Fragment ID aleatório para este conjunto de fragmentos
    srand(time(NULL));
    uint8_t fragment_id = rand() % 256;

    for (int i = 0; i < num_fragments; i++)
    {
        // --- LÓGICA DA JANELA DESLIZANTE ---
        uint32_t packets_in_flight = connection->our_next_seqnum - connection->last_ack_from_server;
        if (connection->remote_window_size <= packets_in_flight)
        {
            fprintf(stderr, "Janela deslizante fechada! Esperando por ACKs do servidor...\n");
            // Em uma implementação real, você poderia esperar aqui ou ter um mecanismo de reenvio.
            // Por enquanto, vamos apenas falhar.
            return false;
        }

        SLOW_header_t data_header;
        memset(&data_header, 0, sizeof(data_header));

        // Preenche o cabeçalho
        data_header.uuid = connection->sid;
        data_header.seqnum = htonl(connection->our_next_seqnum);
        data_header.acknum = htonl(connection->last_server_seqnum);
        data_header.window = htons(1440); // Nossa própria janela de recebimento

        // Configura a fragmentação
        data_header.fid = fragment_id;
        data_header.fo = i;

        // Configura as flags
        uint32_t flags = (1 << 29); // Flag ACK
        if (i < num_fragments - 1)
        {
            flags |= (1 << 31); // Flag More Bits (MB)
        }
        data_header.sttl_n_flags = htonl(flags);

        // Calcula o ponteiro e o tamanho dos dados para este fragmento
        const uint8_t *data_ptr = data + (i * MAX_DATA_PER_PACKET);
        uint16_t chunk_len = (i == num_fragments - 1) ? (data_len % MAX_DATA_PER_PACKET) : MAX_DATA_PER_PACKET;
        if (chunk_len == 0 && data_len > 0)
            chunk_len = MAX_DATA_PER_PACKET; // caso especial de data_len ser múltiplo de 1440

        printf("Enviando fragmento %d de %d (seq=%u)...\n", i + 1, num_fragments, connection->our_next_seqnum);
        if (!SLOW_send_packet(connection, &data_header, data_ptr, chunk_len))
        {
            fprintf(stderr, "Falha ao enviar o fragmento %d.\n", i + 1);
            return false;
        }

        // Atualiza o estado para o próximo pacote
        connection->our_next_seqnum++;
    }

    return true;
}

bool SLOW_revive_and_send_data(SLOW_connection_t *connection, const uint8_t *data, uint16_t data_len)
{

    if (data == NULL || data_len == 0)
        return false;

    int num_fragments = (data_len + MAX_DATA_PER_PACKET - 1) / MAX_DATA_PER_PACKET;
    srand(time(NULL)); // Idealmente, chame srand() apenas uma vez no início do main()
    uint8_t fragment_id = rand() % 256;

    for (int i = 0; i < num_fragments; i++)
    {
        // --- INÍCIO DA LÓGICA CORRIGIDA ---

        // 1. Declara e zera o cabeçalho no início de cada iteração
        SLOW_header_t data_header;
        memset(&data_header, 0, sizeof(data_header));

        // 2. Preenche os campos básicos do cabeçalho
        data_header.uuid = connection->sid;
        data_header.seqnum = htonl(connection->our_next_seqnum);
        data_header.acknum = htonl(connection->last_server_seqnum);
        data_header.window = htons(1440);
        data_header.fid = fragment_id;
        data_header.fo = i;

        // 3. Constrói o campo sttl_n_flags com a lógica correta e unificada
        uint32_t flags = 0;
        // O primeiro pacote (i=0) deve ter a flag Revive
        if (i == 0)
        {
            flags |= (1 << 28); // Revive flag (que descobrimos ser o mesmo bit do Connect)
        }
        // Todos os pacotes devem ter a flag ACK
        flags |= (1 << 29);
        // Pacotes intermediários devem ter a flag More Bits
        if (i < num_fragments - 1)
        {
            flags |= (1 << 31);
        }

        // Combina o STTL salvo da sessão com as flags calculadas
        uint32_t combined_field = connection->sttl | flags;
        data_header.sttl_n_flags = htonl(combined_field);

        // --- FIM DA LÓGICA CORRIGIDA ---

        // 4. Prepara e envia o fragmento de dados
        const uint8_t *data_ptr = data + (i * MAX_DATA_PER_PACKET);
        uint16_t chunk_len = (i == num_fragments - 1) ? (data_len % MAX_DATA_PER_PACKET) : MAX_DATA_PER_PACKET;
        if (chunk_len == 0 && data_len > 0)
            chunk_len = MAX_DATA_PER_PACKET;

        printf("Enviando fragmento %d de %d (seq=%u) com Revive...\n", i + 1, num_fragments, connection->our_next_seqnum);
        if (!SLOW_send_packet(connection, &data_header, data_ptr, chunk_len))
        {
            fprintf(stderr, "Falha ao enviar o fragmento %d.\n", i + 1);
            return false;
        }

        connection->our_next_seqnum++;
    }

    return true;
}