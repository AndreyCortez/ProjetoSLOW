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

int main() {
    // --- 1. SETUP DA CONEXÃO (como antes) ---
    SLOW_connection_header connection;
    const char* server_hostname = "slow.gmelodie.com";
    uint16_t server_port = 7033;

    if (SLOW_open_socket(&connection, NULL, 0) == NULL) return 1;
    
    printf("Resolvendo hostname: %s...\n", server_hostname);
    if (SLOW_resolve_server_hostname(&connection, server_hostname, server_port) == NULL) {
        fprintf(stderr, "Falha ao resolver o endereço do servidor.\n");
        return 1;
    }
    printf("Hostname resolvido para o IP: %s\n\n", connection.server_ip);

    // --- 2. MONTAR E ENVIAR O PACOTE DE CONNECT ---
    SLOW_header_t connect_header;
    
    // Zera toda a estrutura. Isso já define sid, sttl, seqnum, acknum, fid, fo para 0.
    memset(&connect_header, 0, sizeof(SLOW_header_t));

    // Define a flag 'Connect' (bit 27) como 1.
    // sttl é 0, então não precisamos somar nada.
    connect_header.sttl_n_flags = (1 << 27);

    // Define nossa janela de recebimento.
    connect_header.window = 1440;

    print_hex("Cabeçalho de Conexão a ser Enviado", (unsigned char*)&connect_header, sizeof(SLOW_header_t));


    printf("Enviando pacote de 3-way connect para o servidor...\n");
    // Envia o pacote. Note que os dados são NULL e o tamanho é 0.
    if (!SLOW_send_packet(&connection, &connect_header, NULL, 0)) {
        fprintf(stderr, "Falha ao enviar o pacote de conexão.\n");
        SLOW_close_socket(&connection);
        return 1;
    }

    // --- 3. AGUARDAR E ANALISAR A RESPOSTA DO SERVIDOR ---
    SLOW_header_t response_header;
    uint8_t response_data_buffer[1440];
    uint16_t response_data_len;

    printf("Pacote enviado. Aguardando resposta do servidor...\n");
    if (SLOW_receive_packet(&connection, &response_header, response_data_buffer, &response_data_len)) {
        printf("\n--- RESPOSTA DO SERVIDOR RECEBIDA ---\n");

        // A flag Accept/Reject (A/R) é o bit 30. Vamos checar se a conexão foi aceita.
        int connection_accepted = (response_header.sttl_n_flags >> 30) & 1;

        if (connection_accepted) {
            printf("Conexão ACEITA pelo servidor!\n");
            // O servidor envia de volta um novo SID para a sessão.
            // (Aqui você precisaria de uma função para imprimir o __uint128_t do SID)
            printf("O ID da Sessão (SID) agora é: [valor do response_header.uuid]\n");
            printf("O primeiro seqnum do servidor é: %u\n", response_header.seqnum);
        } else {
            printf("Conexão REJEITADA pelo servidor.\n");
        }
    } else {
        printf("\n--- FALHA ---\n");
        printf("Nenhuma resposta recebida do servidor.\n");
    }

    // --- 4. FECHAR O SOCKET ---
    SLOW_close_socket(&connection);

    return 0;
}