#include <stdio.h>
#include <string.h> // Para memset
#include "SLOW_peripheral.h"

void print_hex(const char* label, const unsigned char* buffer, int size) {
    printf("%s (%d bytes):\n", label, size);
    for (int i = 0; i < size; i++) {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n\n");
}


// Função para decodificar e imprimir a mensagem de dados
void print_data_as_string(const uint8_t* data, uint16_t len) {
    printf("Dados como String: \"");
    for(int i = 0; i < len; i++) {
        // Imprime apenas caracteres imprimíveis
        if (data[i] >= 32 && data[i] <= 126) {
            putchar(data[i]);
        } else {
            putchar('.'); // Usa um ponto para caracteres não imprimíveis
        }
    }
    printf("\"\n\n");
}

int main() {
    SLOW_connection_header connection;
    const char* server_hostname = "slow.gmelodie.com";
    uint16_t server_port = 7033;

    if (SLOW_open_socket(&connection, NULL, 0) == NULL) return 1;
    if (SLOW_resolve_server_hostname(&connection, server_hostname, server_port) == NULL) return 1;
    printf("Hostname resolvido para o IP: %s\n\n", connection.server_ip);

    // --- Montando o pacote de Connect mais simples possível ---
    SLOW_header_t connect_header;
    memset(&connect_header, 0, sizeof(SLOW_header_t));

    // Tentativa 1: Big-Endian (Padrão de Rede)
    connect_header.sttl_n_flags = htonl(1 << 27); // Apenas a flag Connect (bit 27)

    // Deixamos window e todos os outros campos como 0.

    printf("--- ENVIANDO PACOTE DE TESTE ---\n");
    print_hex("Cabeçalho de Conexão a ser Enviado", (unsigned char*)&connect_header, sizeof(SLOW_header_t));
    SLOW_send_packet(&connection, &connect_header, NULL, 0);

    // --- AGUARDANDO E ANALISANDO A RESPOSTA EM DETALHES ---
    SLOW_header_t response_header;
    uint8_t response_data_buffer[1440];
    uint16_t response_data_len;

    printf("\n--- AGUARDANDO RESPOSTA ---\n");
    if (SLOW_receive_packet(&connection, &response_header, response_data_buffer, &response_data_len)) {
        printf("\n--- RESPOSTA DO SERVIDOR RECEBIDA ---\n");
        
        print_hex("Cabeçalho da Resposta Recebida", (unsigned char*)&response_header, sizeof(SLOW_header_t));

        if (response_data_len > 0) {
            print_hex("Dados da Resposta Recebidos", response_data_buffer, response_data_len);
            print_data_as_string(response_data_buffer, response_data_len);
        }

        // Analisa as flags recebidas
        uint32_t received_flags = ntohl(response_header.sttl_n_flags);
        int accepted = (received_flags >> 30) & 1;

        if (accepted) {
            printf("RESULTADO: Conexão ACEITA.\n");
        } else {
            printf("RESULTADO: Conexão REJEITADA.\n");
        }
    } else {
        printf("\n--- FALHA NA REDE ---\n");
        printf("Nenhuma resposta recebida do servidor.\n");
    }

    SLOW_close_socket(&connection);
    return 0;
}