#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

typedef struct Cliente{
    int id_cliente;                         //ID para identificar o cliente
    int id_cidade;                         //Os ID's estão na Matriz de distâncias
    int tempo_ultima_visita;              //(dias)     //Tempo desde a última visita.
    float insatisfacao_cliente;          //(0-1)      //Nível de insatisfação do cliente.
    int freq_uso_suporte;               //(dias/mês) //Freq. de uso do suporte.
    float PRIORIDADE;                  //Será usado para alocar os Clientes sequencialmente.
    int tipo_software;    //1:Shop 2:Pack 3:Bimer  //Tipo do software que o cliente usa.
}Cliente;

typedef struct Analista{
    int id_cidade_base;                //ID da cidade base do analista.
    int id_cidade_atual;              //ID da cidade onde o analista está.
    float grau_ociosidade;           //(0-1)(%dia)    //Grau de ocupação|ociosidade do Analista.
    int tipo_software;      //1:Shop 2:Pack 3:Bimer  //Tipo do software que o Analista entende.
    int velocidade_media;          //(km/h)
    float custo_por_km;           //(R$)

}Analista;

int total_cidades; // Número total de cidades (incluindo a base).
float** matriz_distancias;
 /*                                //(em km)
float matriz_distancias[NUM_CIDADES][NUM_CIDADES]={
    {0.0, 27.2,  55.7, 58.5},  // 0: Sede(Macaé)
    {26.5, 0.0, 87.9, 83.2},  // 1: Rio das Ostras
    {49, 88.1, 0.0, 47.5},   // 2: Conceição de Macabu
    {55.7, 109, 47.6, 0.0}  // 3: Quissam˜ã
};
*/

void CarregaMatriz(){
    FILE *f = fopen("matriz.txt", "r");
    if(f == NULL){
        printf("ERRO: Nao foi possivel abrir matriz.txt\n");
        exit(-1);
    }

    fscanf(f, "%d", &total_cidades); // Lê o tamanho da matriz (atribui valor à total_cidades)

    // Alocação dinâmica da matriz de distâncias
    matriz_distancias = (float **)malloc(total_cidades * sizeof(float *));
    for(int i=0; i<total_cidades; i++){
        matriz_distancias[i] = (float *)malloc(total_cidades * sizeof(float));
        for(int j=0; j<total_cidades; j++)
            fscanf(f, "%f", &matriz_distancias[i][j]);

    }
    fclose(f);
    printf("Matriz %dx%d carregada com sucesso.\n", total_cidades, total_cidades); //[DEBUG]
}

int** matriz_tempos_atendimento;
void CarregaTemposAtendimento(int num_analistas, int num_clientes){
    // Aloca a memória para os Analistas (linhas)
    matriz_tempos_atendimento = (int **)malloc(num_analistas * sizeof(int *));

    FILE *arquivo = fopen("tempos_atendimento.txt", "r");
    if(arquivo == NULL){
        printf("Erro ao abrir tempos_atendimento.txt\n");
        return;
    }

    for(int i=0; i<num_analistas; i++){
        // Aloca a memória para os Clientes (colunas)
        matriz_tempos_atendimento[i] = (int *)malloc(num_clientes * sizeof(int));

        for(int j=0; j<num_clientes; j++)
            fscanf(arquivo, "%d", &matriz_tempos_atendimento[i][j]);
    }
    fclose(arquivo);
}

int CarregaClientes(Cliente **clientes){
    FILE *f = fopen("clientes.txt", "r");
    if(f == NULL){
        printf("ERRO: Nao foi possivel abrir clientes.txt\n");
        exit(-1);
    }

    int qtd;
    fscanf(f, "%d", &qtd); // Lê a quantidade de clientes

    // Aloca a memória exata para a quantidade lida
    *clientes = (Cliente *)malloc(qtd * sizeof(Cliente));

    for(int i=0; i<qtd; i++){
        fscanf(f, "%d %d %f %d %d",
               &(*clientes)[i].id_cidade,
               &(*clientes)[i].tempo_ultima_visita,
               &(*clientes)[i].insatisfacao_cliente,
               &(*clientes)[i].freq_uso_suporte,
               &(*clientes)[i].tipo_software);

               (*clientes)[i].id_cliente = i;
               (*clientes)[i].PRIORIDADE = 0; // PRIORIDADE será calculado depois
    }
    fclose(f);
    printf("%d Clientes carregados com sucesso.\n", qtd); //[DEBUG]
    return qtd; // Retorna quantos clientes foram lidos
}
int CarregaAnalistas(Analista **analistas){
    FILE *f = fopen("analistas.txt", "r");
    if(f == NULL){
        printf("ERRO: Nao foi possivel abrir analistas.txt\n");
        exit(-1);
    }

    int qtd;
    fscanf(f, "%d", &qtd);

    *analistas = (Analista *)malloc(qtd * sizeof(Analista));

    for(int i=0; i<qtd; i++){
        fscanf(f, "%d %f %d %d %f",
               &(*analistas)[i].id_cidade_base,
               &(*analistas)[i].grau_ociosidade,
               &(*analistas)[i].tipo_software,
               &(*analistas)[i].velocidade_media,
               &(*analistas)[i].custo_por_km);

        // A cidade atual começa igual à cidade base
        (*analistas)[i].id_cidade_atual = (*analistas)[i].id_cidade_base;
    }
    fclose(f);
    printf("%d Analistas carregados com sucesso.\n\n", qtd); //[DEBUG]
    return qtd; //Retorna a quantos analistas foram lidos
}

float CalculaDistancia(int id_origem, int id_destino){
    return matriz_distancias[id_origem][id_destino];
}
float CalculaTempoDeslocamento(Cliente C, Analista A){
    float distancia = (CalculaDistancia(A.id_cidade_atual, C.id_cidade));
    return (distancia / A.velocidade_media) * 60; //*60 para a unidade ser em (minutos)
}
float CalculaCustoDeslocamento(Cliente C, Analista A){
    float distancia = CalculaDistancia(A.id_cidade_atual, C.id_cidade);
    return distancia * A.custo_por_km; //(R$)
}

float CalculaPrioridade(Cliente C){
 if(C.tempo_ultima_visita < 90)
    return -1; //Ignorado, regra de negócio

    float PRIORIDADE = C.tempo_ultima_visita*0.5 + C.insatisfacao_cliente*0.3 + C.freq_uso_suporte*0.2;
    if(C.tempo_ultima_visita > 150)
        PRIORIDADE += 1000; //Bônus para priorizar o atendimento

    return PRIORIDADE;
}
float CalculaScore(Cliente C, Analista A){
    if(C.tipo_software != A.tipo_software)
        return 1000000.0; //valor muito alto qualquer
    else{
        float SCORE = CalculaTempoDeslocamento(C,A)*0.4 + CalculaCustoDeslocamento(C,A)*0.4 - A.grau_ociosidade*0.2;
        return SCORE;
    }
}

//Soma o custo(R$) e aplica punição(multa) caso algum cliente que deveria, mas não fora atendido.
float CalculaCustoTotalSolucao(int* alocacao, Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas){
    float custo_total = 0.0;
    int cidades_atuais[num_analistas];

    // Todos começam na base
    for(int j=0; j<num_analistas; j++)
        cidades_atuais[j] = analistas[j].id_cidade_base;


    // Calcula o custo da rota de atendimento
    for(int i=0; i<num_clientes; i++){
        int a = alocacao[i]; // i:ID do analista que vai atender

        if(a >= 0){ // Se foi atendido
            float dist = CalculaDistancia(cidades_atuais[a], clientes[i].id_cidade);
            custo_total += dist * analistas[a].custo_por_km;
            cidades_atuais[a] = clientes[i].id_cidade; // Atualiza a localização do Analista
        }
        else if(a == -1) // Se ficou sem atendimento
            custo_total += 100000.0; // Valor muito alto para forçar a busca de alocações
    }

    // Custo de voltar para a base no fim do dia
    for(int j=0; j<num_analistas; j++){
        float dist = CalculaDistancia(cidades_atuais[j], analistas[j].id_cidade_base);
        custo_total += dist * analistas[j].custo_por_km;
    }

    return custo_total;
}

void BuscaLocalRealocacao(int* alocacao, int* tempo_analistas, Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas){
    bool melhorou = true;
    float custo_atual = CalculaCustoTotalSolucao(alocacao, clientes, analistas, num_clientes, num_analistas);

    printf("\n[BUSCA LOCAL:Realocacao] Custo inicial anteriormente: R$ %.2f\n", custo_atual);

    while(melhorou){
        melhorou = false;

        for(int i=0; i<num_clientes; i++){
            if(alocacao[i] == -2) //Ignora visitas desnecessárias(regra de negócio)
                continue;

            int analista_antigo = alocacao[i];

            //Tentativa de colocar o Cliente para outro Analista
            for(int j=0; j<num_analistas; j++){
                if(j == analista_antigo) //Evitando o caso de trocar ele com ele mesmo
                    continue;
                if(clientes[i].tipo_software != analistas[j].tipo_software) //Incompatibilidade do tipo de software
                    continue;
                int tempo_gasto = matriz_tempos_atendimento[j][i];
                if(tempo_analistas[j] + tempo_gasto > 480) //Tempo máximo de 8h atingido (carga de trabalho/dia)
                    continue;

                //Testa a troca
                alocacao[i] = j;
                float novo_custo = CalculaCustoTotalSolucao(alocacao, clientes, analistas, num_clientes, num_analistas);

                if(novo_custo < custo_atual){ //Melhor, confirma-se a troca
                    custo_atual = novo_custo;
                    melhorou = true;

                    //Atualiza a tabela de tempos de atendimento
                    tempo_analistas[j] += tempo_gasto;
                    if(analista_antigo >= 0)
                        tempo_analistas[analista_antigo] -= matriz_tempos_atendimento[analista_antigo][i]; //Devolve tempo do antigo
                    analista_antigo = j;
                }

                else //Piorou, desfaz a troca
                    alocacao[i] = analista_antigo;
            }
        }
    }

    printf("[BUSCA LOCAL:Realocacao] Custo inicial apos a Realocacao: R$ %.2f\n", custo_atual);
}
void BuscaLocalTroca(int* alocacao, int* tempo_analistas, Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas){ //Ideal para os casos onde a Realocação excede o tempo máximo de 8 horas(regra de negócio)
    bool melhorou = true;
    float custo_atual = CalculaCustoTotalSolucao(alocacao, clientes, analistas, num_clientes, num_analistas);
    printf("\n[BUSCA LOCAL:Troca] Custo inicial anteriormente: R$ %.2f\n", custo_atual);

    while(melhorou){
        melhorou = false;

        //Comparando os pares de clientes (i e k)
        for(int i=0; i<num_clientes-1; i++){
            for(int k=i+1; k<num_clientes; k++){

                int analista_i = alocacao[i];
                int analista_k = alocacao[k];

                if(analista_i<0 || analista_k<0) //Visitas canceladas ou não alocadas
                    continue;
                if(analista_i == analista_k) //Mesmo Analista
                    continue;

                //Incompatibilidade do tipo de software (regra de negócio)
                if(clientes[k].tipo_software != analistas[analista_i].tipo_software)
                    continue;
                if(clientes[i].tipo_software != analistas[analista_k].tipo_software)
                    continue;

                //Verificando o limite de 8 horas (regra de negócio)
                int tempo_i_no_a = matriz_tempos_atendimento[analista_i][i];
                int tempo_k_no_a = matriz_tempos_atendimento[analista_i][k];

                int tempo_k_no_b = matriz_tempos_atendimento[analista_k][k];
                int tempo_i_no_b = matriz_tempos_atendimento[analista_k][i];


                int novo_tempo_analista_i = tempo_analistas[analista_i] - tempo_i_no_a + tempo_k_no_a;
                int novo_tempo_analista_k = tempo_analistas[analista_k] - tempo_k_no_b + tempo_i_no_b;


                if(novo_tempo_analista_i > 480 || novo_tempo_analista_k > 480)
                    continue;

                //Testa a troca
                alocacao[i] = analista_k;
                alocacao[k] = analista_i;

                float novo_custo = CalculaCustoTotalSolucao(alocacao, clientes, analistas, num_clientes, num_analistas);

                if(novo_custo < custo_atual){ //Melhorou
                    custo_atual = novo_custo;
                    melhorou = true;

                    tempo_analistas[analista_i] = novo_tempo_analista_i;
                    tempo_analistas[analista_k] = novo_tempo_analista_k;
                    break;
                }
                else{ //Piorou
                    alocacao[i] = analista_i;
                    alocacao[k] = analista_k;
                }
            }
            if(melhorou)
                break;
        }
    }
    printf("[BUSCA LOCAL:Troca] Custo inicial apos a Troca: R$ %.2f\n", custo_atual);
}


int main(){
    srand(time(NULL));

    Cliente* clientes = NULL;
    Analista* analistas = NULL;

    //Lendo os dados dos arquivos
    CarregaMatriz();
    int numero_clientes = CarregaClientes(&clientes);
    int numero_analistas = CarregaAnalistas(&analistas);
    CarregaTemposAtendimento(numero_analistas, numero_clientes);
    int tempo_total_analista[7] = {0};

    int alocacao_cliente_analista[numero_clientes];

    //Calcula-se PRIORIDADE
    for(int i=0; i<numero_clientes; i++)
        clientes[i].PRIORIDADE = CalculaPrioridade(clientes[i]);

    //Ordenar em ordem decrescente os clientes em relação a PRIORIDADE
    for(int i=0; i<numero_clientes; i++){
        for(int j=0; j<numero_clientes; j++){
            if(clientes[j].PRIORIDADE < clientes[i].PRIORIDADE){
                Cliente temp = clientes[i];
                clientes[i] = clientes[j];
                clientes[j] = temp;
            }
        }
    }

    float alpha = 0.3;

    // Alocação e atualização de rotas
    for(int i=0; i<numero_clientes; i++){
        if(clientes[i].PRIORIDADE < 0){
            alocacao_cliente_analista[i] = -2; //-2: visita desnecessária
            continue;
        }



        float min_score = 1000000.0;   // C(e1)
        float max_score = -1000000.0; // C(e2)
        float scores[numero_analistas];

        //Descobrindo o melhor(e1) e o pior (e2) candidato
        for(int j=0; j<numero_analistas; j++){
            int tempo_gasto = matriz_tempos_atendimento[j][i];
            if(tempo_total_analista[j] + tempo_gasto > 480){  //estourou as 8 horas diárias
                scores[j] = 1000000.0; //Valor muito alto para não haver problemas com lixo de memória
                continue;
            }


            scores[j] = CalculaScore(clientes[i], analistas[j]);

            // Ignoramos os analistas incompatíveis (score 1000000) para não distorcer o max_score
            if(scores[j] < 1000000.0){
                if(scores[j] < min_score)
                    min_score = scores[j];
                if(scores[j] > max_score)
                    max_score = scores[j];
            }
        }


        // Se nenhum analista atende (todos incompatíveis), pula o cliente
        if(min_score == 1000000.0){
            alocacao_cliente_analista[i] = -1; //-1: não há analistas para o cliente
            continue; // Vai para o próximo cliente
        }

        //Calcula o limite 'l'
        float limite_l = min_score + alpha * (max_score - min_score);

        // Constrói a LRC
        int LRC[numero_analistas];
        int tamanho_LRC = 0;

        for(int j=0; j<numero_analistas; j++){
            // Se o custo for menor ou igual a 'l'
            if(scores[j] <= limite_l){
                LRC[tamanho_LRC] = j; // Guarda o ID do analista na lista
                tamanho_LRC++;
            }
        }

        // Escolha aleatória de um candidato da LRC
        int indice_escolhido = rand() % tamanho_LRC;
        int melhor_analista = LRC[indice_escolhido];

        // Aloca
        alocacao_cliente_analista[i] = melhor_analista;
        tempo_total_analista[melhor_analista] += matriz_tempos_atendimento[melhor_analista][i];

        // Atualiza a cidade atual do analista
        analistas[melhor_analista].id_cidade_atual = clientes[i].id_cidade;
    }

    bool otimizando = true;
    printf("\n-------------------------------------------------\n");
    while(otimizando){
        float custo_antes = CalculaCustoTotalSolucao(alocacao_cliente_analista, clientes, analistas, numero_clientes, numero_analistas);
        BuscaLocalRealocacao(alocacao_cliente_analista, tempo_total_analista, clientes, analistas, numero_clientes, numero_analistas);
        BuscaLocalTroca(alocacao_cliente_analista, tempo_total_analista, clientes, analistas, numero_clientes, numero_analistas);
        float custo_depois = CalculaCustoTotalSolucao(alocacao_cliente_analista, clientes, analistas, numero_clientes, numero_analistas);

        if(custo_depois >= custo_antes)
            otimizando = false;
    }


    puts("-----------------------------------RESULTADO DA ALOCACAO----------------------------------");
    for(int i=0; i<numero_clientes; i++){
         //Caso Normal
         if(alocacao_cliente_analista[i] >= 0)
            printf("[Atendido] Cliente %02d (Prioridade: %7.2f | Cidade: %02d) -> Atendido por Analista %02d\n", clientes[i].id_cliente, clientes[i].PRIORIDADE, clientes[i].id_cidade, alocacao_cliente_analista[i]);

         //Faltou analistas compatíveis
         else if(alocacao_cliente_analista[i] == -1)
            printf("\n[Sem atendimento] Cliente %02d (Prioridade: %7.2f | Cidade: %02d) -> ALERTA: SEM ANALISTA COMPATIVEL\n\n", clientes[i].id_cliente, clientes[i].PRIORIDADE, clientes[i].id_cidade);

         // Visita não é necessária(regra de negócio)
         else if (alocacao_cliente_analista[i] == -2)
            printf("[Visita cancelada] Cliente %02d (Dias desde a ultima visita: %02d) -> VISITA DESNECESSARIA (Menos de 90 dias)\n", clientes[i].id_cliente, clientes[i].tempo_ultima_visita);
    }

    puts("\n---RETORNO PARA A BASE NO FIM DO DIA---");
    for(int j=0; j<numero_analistas; j++){
        float dist_retorno = CalculaDistancia(analistas[j].id_cidade_atual, analistas[j].id_cidade_base);
        if(dist_retorno > 0.0)
            printf("Analista %d retorna da Cidade %d para a Base (Cidade %d) - Distancia: %.2f km\n", j, analistas[j].id_cidade_atual, analistas[j].id_cidade_base, dist_retorno);
        else
            printf("Analista %d esta na base (Cidade %d)\n", j,  analistas[j].id_cidade_base);
    }
    puts("------------------------------------------------------------------------------------------\n");

    // Liberação de Memória (da matriz_distancias)
    for(int i=0; i< total_cidades; i++)
        free(matriz_distancias[i]);

    free(matriz_distancias);

    // Liberação de Memória (da matriz_tempos_atendimento)
    for(int i=0; i<numero_analistas; i++)
        free(matriz_tempos_atendimento[i]);

    free(matriz_tempos_atendimento);

    free(clientes);
    free(analistas);

    //Mudar a ordenação para quicksort(por exemplo)
    //Utilizar as buscas locais(Realocacao e Troca) na mutação(AG)
    //Ainda está sendo considerado que os Analistas podem fazer apenas visitas do tipo Cafezinho (desacordo com a regra de négocio)
 return 0;
}
