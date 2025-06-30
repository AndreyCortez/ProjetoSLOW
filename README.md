# Protocolo SLOW

Essa implementação do protocolo SLOW foi feita em C, com a seguintr organização:

```bash
ProjetoSLOW/
├── inc	#Pasta com os headers
│   ├── helper_functions.h
│   └── SLOW_peripheral.h
├── makefile
├── README.md
└── src #Pasta com os códigos
    ├── helper_functions.c
    ├── main.c
    └── SLOW_peripheral.c
```

## Funcionamento

O projeto consiste na implementação do protocolo SLOW em C, utilizando conexão
UDP. Para isso, o código é dividido em 4 fases. A primeira fase consiste em
enviar uma 3-way-connect para estabelecer uma conexão com o servidor. 

A função `SLOW_3_way_handshake` tem objetivo de realizar essa conexão.
Primeiro, ela prepara a rede, depois tentar enviar um pacote de connect, sendo
a primeira fase do 3-way-handshake. Se for possível enviar o pacote de connect,
o programa espera o servidor responder ao connect, ou seja, receber o pacote de
setup. Se o programa receber o pacote do setup, a função envia um pacote de
acknowledgment para indicar que a conexão foi feita.

A segunda fase consiste em enviar dados para o servidor. A função responsável
por essa fase é a `SLOW_send_data`. Os dados serão fragmentados e enviados para
o servidor. A terceira fase consiste em receber os pacotes de acknowledgment
para indicar que todos os dados foram recebidos com sucesso. A função
responsável por essa fase é a `SLOW_receive_packet`.

Depois disso, o programa entra na 4° fase, que consiste em desconectar do
servidor. Para isso, chama a função `SLOW_disconnect` que envia um pacote de
disconnect para o servidor para encerrar a conexão.

O arquivo *main.c* já fornece um exemplo de utilização do protocolo SLOW.
Nesse caso, os dados podem ser definidos na fase dois, como está na linha 41 da *main.c*:
```C
/* preencher os dados com qualquer dado que o usuário desejar. */
memset(big_data, 'S', sizeof(big_data)); // Preenche os dados com a letra 'S'

if (!SLOW_send_data(&connection_state, big_data, sizeof(big_data)))
{
	fprintf(stderr, "[MAIN] Falha ao iniciar o envio dos dados.\n");
}
```

## Build e execução
Primeiro temos que copiar o repositório:

```bash
git clone https://github.com/AndreyCortez/ProjetoSLOW.git
```

Em seguida, para compilar o projeto basta fazer:

```bash
cd ProjetoSLOW/
make all
```

Por fim, para executar, faça:

```bash
./peripheral
```

## Alunos

| NOME | NUSP |
| :---: | :---: |
| Hélio Márcio Cabral Santos | 14577862 |
| Andrey Cortez Rufino | 11819487 |
| Gabriel Martins Monteiro | 14572099 |
