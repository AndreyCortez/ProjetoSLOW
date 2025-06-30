#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "SLOW_peripheral.h"

int main()
{
	SLOW_connection_t connection_state = {0}; // Inicializa a struct com zeros
	const char *server_hostname = "slow.gmelodie.com";
	uint16_t server_port = 7033;

	printf("### INICIANDO SESSÃO SLOW ###\n\n");

	// =======================================================================
	// FASE 1: HANDSHAKE
	// =======================================================================
	if (SLOW_3_way_handshake(&connection_state, server_hostname, server_port) != NULL)
	{
		printf("\n[MAIN] Conexão estabelecida com sucesso!\n");

		// Inicializa o estado da janela deslizante
		connection_state.last_ack_from_server = 0;
		connection_state.remote_window_size = 10; // Começamos com uma janela conservadora
	}
	else
	{
		printf("\n[MAIN] Falha ao estabelecer conexão com o servidor.\n");
		if (connection_state.sockfd > 0)
		{
			SLOW_close_socket(&connection_state);
		}
		return 1;
	}

	// =======================================================================
	// FASE 2: ENVIO DE DADOS (COM FRAGMENTAÇÃO)
	// =======================================================================
	printf("\n### FASE DE ENVIO DE DADOS ###\n");
	// Cria um bloco de dados com 3000 bytes para forçar a fragmentação
	uint8_t big_data[3000];
	memset(big_data, 'S', sizeof(big_data)); // Preenche os dados com a letra 'S'

	if (!SLOW_send_data(&connection_state, big_data, sizeof(big_data)))
	{
		fprintf(stderr, "[MAIN] Falha ao iniciar o envio dos dados.\n");
	}
	else
	{
		printf("[MAIN] Todos os fragmentos foram enviados para a rede.\n");

		// =======================================================================
		// FASE 3: RECEBIMENTO DOS ACKs (JANELA DESLIZANTE)
		// =======================================================================
		printf("\n### FASE DE RECEBIMENTO DE ACKs ###\n");
		while (connection_state.last_ack_from_server < connection_state.our_next_seqnum - 1)
		{
			SLOW_header_t ack_response;
			uint16_t data_len;

			if (SLOW_receive_packet(&connection_state, &ack_response, NULL, &data_len))
			{
				uint32_t acked_seq = ntohl(ack_response.acknum);
				uint16_t server_window = ntohs(ack_response.window);

				printf("[ACK Recebido] Servidor confirmou até o seqnum: %u. Janela do servidor: %u\n", acked_seq, server_window);

				// Atualiza o estado da nossa janela deslizante
				if (acked_seq > connection_state.last_ack_from_server)
				{
					connection_state.last_ack_from_server = acked_seq;
				}
				connection_state.remote_window_size = server_window;
				connection_state.last_server_seqnum = ntohl(ack_response.seqnum);
			}
			else
			{
				printf("[MAIN] Timeout esperando por ACK. Encerrando escuta.\n");
				break; // Sai do loop em caso de timeout
			}
		}
		printf("[MAIN] Todos os pacotes enviados foram confirmados pelo servidor!\n");
	}

	// =======================================================================
	// FASE 4: DISCONNECT
	// =======================================================================
	printf("\n### FASE DE DESCONEXÃO ###\n");
	if (SLOW_disconnect(&connection_state))
	{
		printf("[MAIN] Pacote de disconnect enviado. Sessão encerrada.\n");
	}
	else
	{
		printf("[MAIN] Falha ao enviar o pacote de disconnect.\n");
	}

	// Fecha o socket
	SLOW_close_socket(&connection_state);

	return 0;
}