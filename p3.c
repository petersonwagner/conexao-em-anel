#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
 
#define SERVER_prev "10.254.223.62"
#define SERVER_me   "10.254.223.64"
#define SERVER_next "10.254.223.61"

//PORTAS
#define PLAYER1 8880
#define PLAYER2 8880
#define PLAYER3 8880
#define PLAYER4 8880

#define MSG_SIZE sizeof(message_t)  //Max length of buffer
#define DATA_SIZE 18200

#define ATTACK 0
#define ALERT 1
#define SHIP_SINKED 1
#define POSITIONING_PHASE 2
#define HIT 1
#define MISS 0

typedef struct data_t
{
    uint8_t msg_type; //0-ATTACK, 1-ALERT, 2-POSITIONING-PHASE
    uint8_t x;
    uint8_t y;
    uint8_t alert_player;
    uint8_t player_died;
    uint8_t unused_data[18192];
} data_t;

typedef struct status_t
{
	uint8_t msg_arrived  : 1;
	uint8_t ack 		 : 1;
	uint8_t result		 : 1;
	uint8_t ship_sinked  : 1;
	uint8_t player_sank  : 3;
	uint8_t player_died  : 1;
} status_t;

typedef struct message_t
{
    uint8_t start_delim;
    uint8_t access_ctrl;
    uint8_t msg_ctrl;
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    data_t data;
    uint32_t crc;
    uint8_t end_delim;
    status_t status;
} message_t;

uint32_t crc (unsigned char *message, int size)
{
   int i, j;
   uint32_t byte, crc, mask;

   crc = 0xFFFFFFFF;
   for (i = 0; i < size; i++)
   {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--)     // Do eight times.
      {
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
   }
   return ~crc;
}


void assign_adress (struct sockaddr_in *s_addr, char* server, int port)
{
    memset((char *) s_addr, 0, sizeof(*s_addr));
    s_addr->sin_family = AF_INET;
    s_addr->sin_port = htons(port);
    inet_aton(server, &s_addr->sin_addr);  //converte string "num.num.num.num" para 4 bytes e joga em sin_addr
}

int get_port (int argc, char **argv)
{
    if (argc != 2)
    {
        perror ("Parametro invalido.\nExecução: ./player player_num");
        abort();
    }

    return (atoi(argv[1]) - 1);
}

void pack_msg (message_t *message, data_t *data, uint16_t dest, uint16_t src, char access_ctrl)
{
    memset (message, 0, sizeof(message_t));

    message->start_delim = 0x7E;                    //delimitador de inicio
    message->access_ctrl = access_ctrl;             //controle de acesso
    message->msg_ctrl    = 0x00;                    //controle de mensagem

    memcpy (message->dst_addr, &dest, 2);            //endereco de destino
    memcpy (message->src_addr, &src,  2);            //endereco de origem
    
    memcpy (&message->data, data, DATA_SIZE);       //dados
    
    message->crc = crc((char*)message, 15 + sizeof(data_t));
    /*printf("message[18213]: %u\n", (char*) message + 18213);
    printf("message[18214]: %u\n", (char*) message + 18214);
    printf("message[18215]: %u\n", (char*) message + 18215);
    printf("crc_calc: %d\n", message->crc);*/

    message->end_delim = 0x81;                      //delimitador de fim
    memset (&message->status, 0, 1);
}

int check_crc (message_t *message)
{
    uint32_t crc_recalc;

    crc_recalc = crc ((char*) message, 15 + sizeof(data_t));

    if (message->crc == crc_recalc)
        return 1;
    else
    {
        printf("crc errado, msg->crc: %u | crc-calc: %u\n", message->crc, crc_recalc);
        return 0;
    }
}

void ack_msg (message_t *message)
{
    message->status.msg_arrived = 1;
    message->status.ack = 1;
}

void nack_msg (message_t *message)
{
    message->status.msg_arrived = 1;
    message->status.ack = 0;
}

int check_ack (message_t *message)
{
    if(message->status.msg_arrived && message->status.ack)
        return 1;
    else
        return 0;
}


int check_src (message_t *message, uint16_t player_addr)
{
    if (memcmp (message->src_addr, &player_addr, 2) == 0)
        return 1;
    else
        return 0;
}

int check_dst (message_t *message, uint16_t player_addr)
{
    if (memcmp (message->dst_addr, &player_addr, 2) == 0)
        return 1;
    else
        return 0;
}

void get_ships (char board[5][5])
{
    int x, y;
    
    do
    {
        printf("Digite a primeira coordenada do navio horizontal:\n");
        printf("x1: ");
        scanf("%d", &x);
        printf("y1: ");
        scanf("%d", &y);

        if (x > 2 || x < 0 || y < 0 || y > 4)
            printf("coordenadas invalidas\n");
    } while (x > 2 || x < 0 || y < 0 || y > 4);

    board[y][x]   = 'H';
    board[y][x+1] = 'H';
    board[y][x+2] = 'H';
    
    do
    {
        printf("Digite a primeira coordenada do navio vertical:\n");
        printf("x1: ");
        scanf("%d", &x);
        printf("y1: ");
        scanf("%d", &y);

        if (x > 4 || x < 0 || y < 0 || y > 2)
            printf("coordenadas invalidas\n");
    } while (x > 4 || x < 0 || y < 0 || y > 2);

    board[y][x]   = 'V';
    board[y+1][x] = 'V';
    board[y+2][x] = 'V';

    /*board[0][0] = 'H';
    board[0][1] = 'H';
    board[0][2] = 'H';

    board[0][4] = 'V';
    board[1][4] = 'V';
    board[2][4] = 'V';*/
}

void make_attack (message_t *message, int player_num, uint16_t *players_ports, int players_alive[4])
{
    int player_attacked;
    data_t *data = (data_t*) malloc (1 * sizeof(data_t));
    memset (data, 0, sizeof(data_t));

    data->msg_type = ATTACK;

    do
    {
        printf("Qual jogador deseja atacar? ");
        scanf("%d", &player_attacked);
        player_attacked--;

        if (player_attacked < 0 || player_attacked > 3 || player_attacked == player_num || !players_alive[player_attacked])
            printf("Player invalido\n");
    } while (player_attacked < 0 || player_attacked > 3 || player_attacked == player_num || !players_alive[player_attacked]);

    do
    {
        printf("Digite as coordenadas do ataque\n");
        printf("x: ");
        scanf("%d", (int*) &data->x);
        printf("y: ");
        scanf("%d", (int*) &data->y);

        if (data->x < 0 || data-> y < 0 || data->x > 4 || data->y > 4)
            printf("Coordenadas invalidas\n");
    } while (data->x < 0 || data-> y < 0 || data->x > 4 || data->y > 4);

    pack_msg (message, data, players_ports[player_attacked], players_ports[player_num], 0x00);
    free (data);
}

void create_token (uint8_t *message, int new_token)
{
    message[0] = 0x7E;

    if (new_token)
        message[1] = 0x18;
    else
        message[1] = 0x10;

    message[2] = 0x81;
}

int check_token (message_t *message)
{
    if (message->access_ctrl == 0x18 || message->access_ctrl == 0x10)
        return 1;
    else
        return 0;
}

void print_board(char board[5][5])
{
    printf("\n");
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            if (board[i][j] == 0)
                printf("| ~ ");
            else
                printf("| %c ", board[i][j]);
        }
        printf("|\n");
    }
}

int score_attack (char board[5][5], int y, int x)
{
    int count = 0;
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j)
            if (board[i][j] == board[y][x])
                count++;

    board[y][x] = 0;

    if (count == 1)
        return 1;

    return 0;
}

int check_gameover(char board[5][5])
{
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j)
            if (board[i][j] != 0)
                return 0;

    return 1;
}

int main(int argc, char **argv)
{
    struct sockaddr_in ring_previous, ring_next, ring_me;
    uint16_t players_ports[4] = {PLAYER1, PLAYER2, PLAYER3, PLAYER4};
    uint16_t players_num[4]   = {1,2,3,4};
    int player_num, ownToken, gameOver = 0, players_alive[4] = {1,1,1,1};
    int sock;
    int prev_len = sizeof(ring_previous), next_len = sizeof(ring_next);
    data_t *data;
    message_t *message, *message_copy;
    char board[5][5];
    struct timeval timeout={2,0};

    memset (board, 0, 25);
    message      = (message_t*) malloc (1 * sizeof(message_t));
    message_copy = (message_t*) malloc (1 * sizeof(message_t));
    data = (data_t *) malloc (sizeof(data_t));

    player_num = 2;

    //create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    assign_adress (&ring_me,       SERVER_me,   players_ports[player_num]);
    assign_adress (&ring_previous, SERVER_prev, players_ports[(player_num - 1) % 4]);
    assign_adress (&ring_next,     SERVER_next, players_ports[(player_num + 1) % 4]);
     
    //bind socket to port
    bind(sock, (struct sockaddr*) &ring_me, sizeof(ring_me));

    ownToken = 0;

 
    while(1)
    {
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(message, 0, MSG_SIZE);

        

        if(ownToken)
        {
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));

            //try to receive some data, this is a blocking call
            while (recvfrom(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_previous, &prev_len) < 0)
            {
                printf("TIMEOUT, RESENDING\n");
                sendto(sock, message_copy, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);
            }
        }
        else
        {
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, 0, 0); //sem timeout

            //try to receive some data, this is a blocking call
            recvfrom(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_previous, &prev_len);
        }

        if (gameOver)
        {
        	//apenas manda a mensagem adiante
        	sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);
        }

        else if (check_token(message))
        {
            ownToken = 1;
            make_attack(message, player_num, players_num, players_alive);
            memcpy (message_copy, message, sizeof(message_t));
            sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);	        
        }

        else if ((!ownToken && !check_dst(message, players_num[player_num])) || gameOver)
        {
            //apenas manda a mensagem adiante
            sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);
        }

        else if (!ownToken && check_dst(message, players_num[player_num]))
        {            
            if(check_crc(message))
                ack_msg(message);
            else
            {
                nack_msg(message);
                sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);
                continue;
            }

            if (message->data.msg_type == ATTACK)
            {
                printf("Voce foi atacado na posicao (%d,%d) ", message->data.y, message->data.x);
                if(board[message->data.y][message->data.x])
                {
                    printf("e foi acertado.\n");
                    message->status.result = HIT;

                    if (score_attack(board, message->data.y, message->data.x) == SHIP_SINKED)
                    {
                        message->status.ship_sinked = 1;
                        message->status.player_sank = player_num;
                    }

                    if (check_gameover(board))
                    {
                        gameOver = 1;
                        message->status.player_died = 1;
                        printf("\nVoce perdeu!!!\n");
                    }
                    
                    print_board(board);
                }
                else
                    printf("e nao foi acertado.\n");
            }

            else if (message->data.msg_type == POSITIONING_PHASE)
            {
                get_ships(board);
                print_board(board);
            }

            else if (message->data.msg_type == ALERT)
            {
                printf("Um navio do Player %d foi afundado\n", message->data.alert_player+1);

                if (message->data.player_died)
                {
                	printf("Player %d morreu!\n", message->data.alert_player+1);
                	players_alive[message->data.alert_player] = 0;
                }
            }

            sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);
        }

        else if (ownToken && check_src(message, players_num[player_num]))
        {
            if(!check_ack(message))
            {
                sendto(sock, message_copy, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);
                continue;
            }

            if(message->data.msg_type == ATTACK)
            {
                if (message->status.result == HIT)
                    printf("Voce acertou.\n");
                else
                    printf("Voce errou.\n");


                if(message->status.ship_sinked)
                {
                    memset (data, 0, sizeof(data_t));

                    printf("Voce afundou um navio do Player %d\n", message->status.player_sank+1);

                    if (message->status.player_died)
                    {
                    	printf("Voce matou o player %d\n", message->status.player_sank+1);
                    	data->player_died = 1;
                    }

                    //AVISAR TODOS OS PLAYERS
                    data->msg_type = ALERT;
                    data->alert_player = message->status.player_sank;

                    pack_msg (message, data, players_num[(player_num+1)%4], players_num[player_num], 0x00);
                    sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);


                    recvfrom(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_previous, &prev_len);
                    pack_msg (message, data, players_num[(player_num+2)%4], players_num[player_num], 0x00);
                    sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);


                    recvfrom(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_previous, &prev_len);
                    pack_msg (message, data, players_num[(player_num+3)%4], players_num[player_num], 0x00);
                    sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);

                    recvfrom(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_previous, &prev_len);
                }
            }


            create_token ((uint8_t*) message, 0);
            ownToken = 0;
            sendto(sock, message, MSG_SIZE, 0, (struct sockaddr *) &ring_next, next_len);
        }

    }
 
    close(sock);
    return 0;
}
