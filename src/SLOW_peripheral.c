#include "SLOW_peripheral.h"


SLOW_connection_header* SLOW_resolve_server_hostname(
    SLOW_connection_header* connection, 
    const char* hostname,        
    uint16_t server_port
) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int status;
    char port_str[6]; 

    snprintf(port_str, sizeof(port_str), "%d", server_port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_DGRAM;  


    status = getaddrinfo(hostname, port_str, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return NULL;
    }

    rp = result;
    if (rp != NULL) {
        memcpy(&(connection->serv_addr), rp->ai_addr, rp->ai_addrlen);
        
        inet_ntop(rp->ai_family, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr), 
                  connection->server_ip, sizeof(connection->server_ip));
        
        connection->server_port = server_port;
    } else {
        fprintf(stderr, "Não foi possível resolver o hostname: %s\n", hostname);
        freeaddrinfo(result); 
        return NULL;
    }

    freeaddrinfo(result);

    return connection;
}

SLOW_connection_header* SLOW_resolve_server_address(
    SLOW_connection_header* connection, 
    char server_ip[16], 
    uint16_t server_port
) {

    struct sockaddr_in serv_addr;

    // Zera a estrutura de endereço e a preenche
    memset(&serv_addr, 0, sizeof(serv_addr));

    // 2. Configurar o endereço do servidor (Central)
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(server_port); 
    
    // Converte o endereço IP de string para formato de rede
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Endereço inválido");
        return NULL; 
    }

    connection->serv_addr = serv_addr;
    strcpy(connection->server_ip, server_ip);
    connection->server_port = server_port;

    return connection;
}

SLOW_connection_header* SLOW_open_socket(
    SLOW_connection_header* connection, 
    char server_ip[16], 
    uint16_t server_port
) {
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        return NULL;
    }

    connection ->sockfd = sockfd;

    return connection;
}

SLOW_connection_header* SLOW_close_socket(
    SLOW_connection_header* connection
) {
    printf("Fechando o socket.\n");
    close(connection->sockfd); 
    connection->sockfd = -1;
    return connection;
}


bool SLOW_send_packet(
    SLOW_connection_header* connection,
    SLOW_header_t* header, 
    uint8_t data[1440], 
    uint16_t data_len
) {
    
    char buffer[BUFFER_SIZE];

    // --- Início da Lógica do Protocolo SLOW ---

    memcpy(buffer, header, sizeof(SLOW_header_t));
    memcpy(buffer + sizeof(SLOW_header_t), data, data_len);    

    printf("Enviando pacote de conexão para %s:%d\n", 
            connection->server_ip, 
            connection->server_port);

    // 4. Enviar o pacote para o servidor
    uint16_t total_len = sizeof(SLOW_header_t) + data_len;
    if (sendto(connection->sockfd, buffer, total_len, 0, (const struct sockaddr *)&(connection->serv_addr), sizeof(connection->serv_addr)) < 0) {
        perror("Erro no sendto");
        close(connection->sockfd);
        return false;
    }

    return true;
}


bool SLOW_receive_packet(
    SLOW_connection_header* connection, 
    SLOW_header_t* header_out,         // Ponteiro para preencher o cabeçalho recebido
    uint8_t* data_out,                 // Buffer para preencher com os dados recebidos
    uint16_t* data_len_out             // Ponteiro para retornar o tamanho dos dados recebidos
) {
    unsigned char buffer[BUFFER_SIZE]; // Buffer local para receber o pacote completo
    socklen_t addr_len = sizeof(connection->serv_addr);

    // 1. Receber o pacote do socket
    int n_bytes = recvfrom(
        connection->sockfd, 
        buffer, 
        BUFFER_SIZE, 
        0, // Flags
        (struct sockaddr *)&(connection->serv_addr), 
        &addr_len
    );
    
    if (n_bytes < 0) {
        perror("Erro no recvfrom");
        return false;
    }

    // 2. Validar se o pacote recebido tem pelo menos o tamanho do cabeçalho
    if (n_bytes < sizeof(SLOW_header_t)) {
        fprintf(stderr, "Erro: Pacote recebido é menor que o cabeçalho SLOW.\n");
        return false;
    }

    // 3. Desmontar o pacote: copiar as partes do buffer para as variáveis de saída
    
    memcpy(header_out, buffer, sizeof(SLOW_header_t));

    *data_len_out = n_bytes - sizeof(SLOW_header_t);

    if (*data_len_out > 0) {
        memcpy(data_out, buffer + sizeof(SLOW_header_t), *data_len_out);
    }

    return true;
}