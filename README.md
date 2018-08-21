# Conexao Rawsockets - Trabalho de Redes de Computadores I (Prof. Albini)

- Montar um jogo de batalha naval em 4 jogadores.
- Montar uma rede em anel com 4 máquinas usando socket Datagram.
- Implementar o controle de acesso de passagem de bastão.
- Tabuleiro 5x5, 2 navios de 3x1
- Inicio:
    - Jogadores distribuem os navios pelo seu tabuleiro
    - Jogador com bastão escolhe um oponente e uma área para atacar, cria a mensagem e envia
    - Jogadores sem bastão repassam a mensagem se não forem atacados. Se forem, adiciona o resultado do ataque na mensagem e manda pra frente.
- Só tira a mensagem do anel quem a enviou.
- Quando o navio é afundado, todas as máquinas devem ser avisadas que o navio do jogador foi afundado nas posições Z1, Z2, Z3.
- O bastão não é temporizado.
- Deve ter timeout na mensagem.
