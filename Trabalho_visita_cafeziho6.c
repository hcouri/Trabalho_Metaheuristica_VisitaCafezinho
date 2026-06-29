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
    int tipo_software;      //1:Shop 2:Pack 3:Bimer  //Tipo do software que o Analist0a entende.
    int velocidade_media;          //(km/h)
    float custo_por_km;           //(R$)

}Analista;
#define TAM_POPULACAO 1000
#define NUM_GERACOES 500000
#define TAXA_CRUZAMENTO 0.75
#define TAXA_MUTACAO 0.25
#define QNT_VISITAS 5
#define MULTA_SOFTWARE_INCOMPATIVEL 5000000.0

typedef struct{
    int* alocacao;  // Vetor de ID's Analista-Cliente (Cromossomo)
    float custo;   // Fitness
}Individuo;

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

//Soma o custo(R$) e aplica punição(multa) caso algum cliente que deveria, mas não fora atendido, ou analista ocioso.
float CalculaCustoTotalSolucao(int* alocacao, Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas){
    float custo_total = 0.0;
    int cidades_atuais[num_analistas];
    int visitas_realizadas[num_analistas];
    float tempo_trabalhado[num_analistas];

    for(int j=0; j<num_analistas; j++){
        cidades_atuais[j] = analistas[j].id_cidade_base; //"Reset" na localização
        visitas_realizadas[j] = 0; //"Reset" nas visitas
        tempo_trabalhado[j] = 0.0;
    }

    //Calcula o custo da rota de atendimento
    for(int i=0; i<num_clientes; i++){
        int id = alocacao[i]; //"id" do analista que vai atender

        if(id >= 0){ //Se foi atendido
            if(clientes[i].tipo_software != analistas[id].tipo_software)
                custo_total += MULTA_SOFTWARE_INCOMPATIVEL;

            float dist = CalculaDistancia(cidades_atuais[id], clientes[i].id_cidade);
            custo_total += dist * analistas[id].custo_por_km;
            float tempo_viagem = (dist/analistas[id].velocidade_media) * 60.0;
            tempo_trabalhado[id] += tempo_viagem + matriz_tempos_atendimento[id][i];
            cidades_atuais[id] = clientes[i].id_cidade;
            visitas_realizadas[id]++;
        }
        else if(id == -1) //Se não foi atendido
            custo_total += 500000.0; //Multa muito alta pelo cliente ignorado
    }

    //Custo do Analista voltar para sua cidade base no fim do dia
    for(int j=0; j<num_analistas; j++){
        float dist = CalculaDistancia(cidades_atuais[j], analistas[j].id_cidade_base);
        custo_total += dist * analistas[j].custo_por_km;

        if(visitas_realizadas[j] > 0){
            float tempo_volta = (dist/analistas[j].velocidade_media) * 60.0;
            tempo_trabalhado[j] += tempo_volta;
        }

        //Validando as regras de negócio
        int meta = 0;
        if(analistas[j].tipo_software == 1)
            meta = 4;
        else if(analistas[j].tipo_software == 2)
            meta = 3;
        else if(analistas[j].tipo_software == 3)
            meta = 2;

        if(visitas_realizadas[j] > 0){
            if(visitas_realizadas[j] < meta){ //Multa por não bater a meta de visitas
                int faltou = meta - visitas_realizadas[j];
                custo_total += (faltou * 50000.0);
            }
            if(tempo_trabalhado[j] < 300.0){ //Multa por trabalhar menos de 5 horas(300 minutos)
                float tempo_ocioso = 300.0 - tempo_trabalhado[j];
                custo_total += (tempo_ocioso * 1000.0);
            }
            if(tempo_trabalhado[j] > 480.0){ //Multa por trabalhar mais de 8 horas(480 minutos)
                float hora_extra = tempo_trabalhado[j] - 480.0;
                custo_total += 2000000.0;
            }
            if(visitas_realizadas[j] > QNT_VISITAS)
                custo_total += 2000000.0;
        }
    }

    return custo_total;
}

void BuscaLocalRealocacao(int* alocacao, int* tempo_analistas, Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas){
    bool melhorou = true;
    float custo_atual = CalculaCustoTotalSolucao(alocacao, clientes, analistas, num_clientes, num_analistas);

    //printf("\n[BUSCA LOCAL:Realocacao] Custo inicial anteriormente: R$ %.2f\n", custo_atual);

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

    //printf("[BUSCA LOCAL:Realocacao] Custo inicial apos a Realocacao: R$ %.2f\n", custo_atual);
}
void BuscaLocalTroca(int* alocacao, int* tempo_analistas, Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas){ //Ideal para os casos onde a Realocação excede o tempo máximo de 8 horas(regra de negócio)
    bool melhorou = true;
    float custo_atual = CalculaCustoTotalSolucao(alocacao, clientes, analistas, num_clientes, num_analistas);
    //printf("\n[BUSCA LOCAL:Troca] Custo inicial anteriormente: R$ %.2f\n", custo_atual);

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
    //printf("[BUSCA LOCAL:Troca] Custo inicial apos a Troca: R$ %.2f\n", custo_atual);
}

void trocar(Cliente* a, Cliente* b){ //Função auxiliar para o quicksort
    Cliente temp = *a;
    *a = *b;
    *b = temp;
}
int particionar(Cliente array[], int baixo, int alto){ //Função auxiliar para o quicksort
    int pivo = array[alto].PRIORIDADE;

    //Índice dos elementos maiores que pivô
    int i = (baixo-1);

    for(int j=baixo; j<alto; j++){
        if(array[j].PRIORIDADE > pivo){
            i++; // Incrementa o índice do maior elemento se for maior que pivô
            trocar(&array[i], &array[j]);
        }
    }
    trocar(&array[i+1], &array[alto]);

    return i+1;
}
void quickSort(Cliente array[], int baixo, int alto){
    if(baixo < alto){
        int pi = particionar(array, baixo, alto);
        quickSort(array, baixo, pi-1);
        quickSort(array, pi+1, alto);
    }
}

int* GeraSolucaoConstrutiva(Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas, float alpha){
    int* alocacao = (int*)malloc(num_clientes * sizeof(int));
    int* tempo_total_analista = (int*)calloc(num_analistas, sizeof(int)); //Inicializa com '0's
    int* total_visitas_analista = (int*)calloc(num_analistas, sizeof(int));

    //Reseta as cidades atuais dos analistas para a base antes de começar
    for(int j=0; j<num_analistas; j++)
        analistas[j].id_cidade_atual = analistas[j].id_cidade_base;


    for(int i=0; i<num_clientes; i++){
        if(clientes[i].PRIORIDADE < 0){
            alocacao[i] = -2; // Visita desnecessária
            continue;
        }

        float min_score = 1000000.0;
        float max_score = -1000000.0;
        float scores[num_analistas];

        for(int j=0; j<num_analistas; j++){
            float dist = CalculaDistancia(analistas[j].id_cidade_atual, clientes[i].id_cidade);
            float tempo_viagem = (dist / analistas[j].velocidade_media) * 60.0;
            int tempo_gasto = matriz_tempos_atendimento[j][i] + tempo_viagem;

            if(tempo_total_analista[j] + tempo_gasto > 480 || total_visitas_analista[j] >= QNT_VISITAS){
                scores[j] = 1000000.0;//Ultrapassou 8 horas
                continue;
            }

            scores[j] = CalculaScore(clientes[i], analistas[j]);

            if(scores[j] < 1000000.0){
                if(scores[j] < min_score)
                    min_score = scores[j];
                if(scores[j] > max_score)
                    max_score = scores[j];
            }
        }

        if(min_score == 1000000.0){
            alocacao[i] = -1; // Sem analista compatível ou sem tempo
            continue;
        }

        float limite_l = min_score + alpha * (max_score - min_score);
        int LRC[num_analistas];
        int tamanho_LRC = 0;

        for(int j=0; j<num_analistas; j++){
            if(scores[j] <= limite_l){
                LRC[tamanho_LRC] = j;
                tamanho_LRC++;
            }
        }

        int indice_escolhido = rand() % tamanho_LRC;
        int melhor_analista = LRC[indice_escolhido];

        alocacao[i] = melhor_analista;
        tempo_total_analista[melhor_analista] += matriz_tempos_atendimento[melhor_analista][i];
        total_visitas_analista[melhor_analista]++;
        analistas[melhor_analista].id_cidade_atual = clientes[i].id_cidade;
    }
    free(tempo_total_analista);
    free(total_visitas_analista);

    return alocacao;
}

int SelecaoTorneio(Individuo* populacao, int k){
    int melhor = rand() % TAM_POPULACAO; //Melhor é inicializado aleatoriamente
    for(int i=1; i<k; i++){
        int candidato = rand() % TAM_POPULACAO;
        if(populacao[candidato].custo < populacao[melhor].custo)
            melhor = candidato;
    }
    return melhor;
}
void Crossover(Individuo pai1, Individuo pai2, Individuo filho1, Individuo filho2, int num_clientes){
    int ponto_corte = rand() % num_clientes;

    for(int i=0; i<num_clientes; i++){
        if(i<ponto_corte){
            filho1.alocacao[i] = pai1.alocacao[i];
            filho2.alocacao[i] = pai2.alocacao[i];
        }
        else{
            filho1.alocacao[i] = pai2.alocacao[i];
            filho2.alocacao[i] = pai1.alocacao[i];
        }
    }
}
void Mutacao(Individuo ind, Cliente* clientes, Analista* analistas, int num_clientes, int num_analistas){
    for(int i=0; i<num_clientes; i++){
        if(ind.alocacao[i] >= -1/*precisa de visita*/ && ((float)rand() / RAND_MAX) < TAXA_MUTACAO)/*taxa de mutacao atingida*/{

            int compativeis[num_analistas];
            int qnt_compativeis = 0;
            for(int j=0; j<num_analistas; j++){
                if(analistas[j].tipo_software == clientes[i].tipo_software)
                    compativeis[qnt_compativeis++] = j;
            }

            if(qnt_compativeis > 0){ //Havendo Analistas compativeis
                if(rand() % 5 == 0) //20% de chance
                    ind.alocacao[i] = -1; //não haverá alocação(para testar outras rotas)
                else
                    ind.alocacao[i] = compativeis[rand() % qnt_compativeis];
            }
        }
    }
}

void LiberaMemoria(Individuo* populacao, Individuo* nova_populacao, int* alocacao_melhor_global, Cliente* clientes, Analista* analistas, int numero_analistas){
    // Liberação de Memória (AG)
    for(int i=0; i<TAM_POPULACAO; i++){
        free(populacao[i].alocacao);
        free(nova_populacao[i].alocacao);
    }
    free(alocacao_melhor_global);

    //Liberação de Memória (Matrizes)
    for(int i=0; i< total_cidades; i++)
        free(matriz_distancias[i]);
    free(matriz_distancias);

    for(int i=0; i<numero_analistas; i++)
        free(matriz_tempos_atendimento[i]);
    free(matriz_tempos_atendimento);

    free(clientes);
    free(analistas);
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

    //Calcula-se PRIORIDADE
    for(int i=0; i<numero_clientes; i++)
        clientes[i].PRIORIDADE = CalculaPrioridade(clientes[i]);

    //Ordena em ordem decrescente os clientes em relação a PRIORIDADE
    quickSort(clientes, 0, numero_clientes-1);


    Individuo populacao[TAM_POPULACAO];
    Individuo nova_populacao[TAM_POPULACAO];

    for(int i=0; i<TAM_POPULACAO; i++){
        populacao[i].alocacao = GeraSolucaoConstrutiva(clientes, analistas, numero_clientes, numero_analistas, 0.3);
        populacao[i].custo = CalculaCustoTotalSolucao(populacao[i].alocacao, clientes, analistas, numero_clientes, numero_analistas);

        nova_populacao[i].alocacao =(int*)malloc(numero_clientes*sizeof(int));
        nova_populacao[i].custo = 0;
    }

    Individuo melhor_global;
    melhor_global.alocacao = (int*)malloc(numero_clientes*sizeof(int));
    melhor_global.custo = 100000000; //Inicializando com valor muito alto

    printf("\nAG: Iniciando evolucao com %d geracoes\n", NUM_GERACOES);
    //Loop das gerações(g) [AG]
    for(int g=0; g<NUM_GERACOES; g++){

        //Crossover e Mutação
        int filhos_gerados = 0;
        while(filhos_gerados < TAM_POPULACAO){
            int pai1 = SelecaoTorneio(populacao, 3);
            int pai2 = SelecaoTorneio(populacao, 3);

            if(((float)rand()/RAND_MAX) < TAXA_CRUZAMENTO)
                Crossover(populacao[pai1], populacao[pai2], nova_populacao[filhos_gerados],nova_populacao[filhos_gerados+1], numero_clientes);
            else{
                //Só copia dos pais
                for(int k=0; k<numero_clientes; k++){
                    nova_populacao[filhos_gerados].alocacao[k] = populacao[pai1].alocacao[k];
                    nova_populacao[filhos_gerados+1].alocacao[k] = populacao[pai2].alocacao[k];
                }
            }

            Mutacao(nova_populacao[filhos_gerados], clientes, analistas, numero_clientes, numero_analistas);
            Mutacao(nova_populacao[filhos_gerados+1], clientes, analistas, numero_clientes, numero_analistas);

            filhos_gerados += 2;
        }

        //Avaliando a nova geração
        int melhor_da_geracao = 0;
        for(int i=0; i<TAM_POPULACAO; i++){
            nova_populacao[i].custo = CalculaCustoTotalSolucao(nova_populacao[i].alocacao, clientes, analistas, numero_clientes, numero_analistas);

            for(int j=0; j<numero_clientes; j++){
                populacao[i].alocacao[j] = nova_populacao[i].alocacao[j];
                populacao[i].custo = nova_populacao[i].custo;
            }
            if(populacao[i].custo < populacao[melhor_da_geracao].custo)
                melhor_da_geracao = i;
        }

        //Salvando o custo antes das buscas locais(printf)
        float custo_antes_busca = populacao[melhor_da_geracao].custo;

         //Verificar se é válido com o professor
        //Lapidando o melhor indivíduo da geração com as buscas locais
        int* tempo_fake_analistas = (int*)calloc(numero_analistas , sizeof(int)); //Inicializa com '0's um vetor de tempos fake (necessário para as buscas locais)
        for(int i=0; i<numero_clientes; i++){
            int analista = populacao[melhor_da_geracao].alocacao[i];
            if(analista >= 0)
                tempo_fake_analistas[analista] += matriz_tempos_atendimento[analista][i];
        }

        BuscaLocalRealocacao(populacao[melhor_da_geracao].alocacao, tempo_fake_analistas, clientes, analistas, numero_clientes, numero_analistas);
        BuscaLocalTroca(populacao[melhor_da_geracao].alocacao, tempo_fake_analistas, clientes, analistas, numero_clientes, numero_analistas);

        //Recalculando o custo
        populacao[melhor_da_geracao].custo = CalculaCustoTotalSolucao(populacao[melhor_da_geracao].alocacao, clientes, analistas, numero_clientes, numero_analistas);
        free(tempo_fake_analistas);

        printf("[Geracao %02d] AG: R$ %10.2f  --> Pos-Busca Local: R$ %10.2f", g + 1, custo_antes_busca, populacao[melhor_da_geracao].custo);

        //Salvando melhor solucao das geracoes(Elitismo)
        if(populacao[melhor_da_geracao].custo < melhor_global.custo){
            melhor_global.custo = populacao[melhor_da_geracao].custo;
            for(int i=0; i<numero_clientes; i++)
                melhor_global.alocacao[i] = populacao[melhor_da_geracao].alocacao[i];
            printf(" -> NOVO MELHOR GLOBAL");
        }
        printf("\n");
    }


    puts("\n-----------------------------------RESULTADO FINAL DA ALOCACAO (AG)----------------------------------");
    for(int i=0; i<numero_clientes; i++){
         if(melhor_global.alocacao[i] >= 0)
            printf("[Atendido] Cliente %02d (Prioridade: %7.2f) -> Atendido por Analista %02d\n", clientes[i].id_cliente, clientes[i].PRIORIDADE, melhor_global.alocacao[i]);
         else if(melhor_global.alocacao[i] == -1)
            printf("[Sem atendimento] Cliente %02d (Prioridade: %7.2f) -> ALERTA: MULTA APLICADA\n", clientes[i].id_cliente, clientes[i].PRIORIDADE);
         else if (melhor_global.alocacao[i] == -2)
            printf("[Visita cancelada] Cliente %02d -> Menos de 90 dias\n", clientes[i].id_cliente);
    }
    printf("\n>>> CUSTO DA MELHOR SOLUCAO EXECUTADA: R$ %.2f <<<\n", melhor_global.custo);
    puts("------------------------------------------------------------------------------------------------------\n");

    //Detalhando as visitas de cada Analista
    for(int i=0; i<numero_analistas; i++){
        int visitas = 0;

        for(int j=0; j<numero_clientes; j++)
            if(melhor_global.alocacao[j] == i)
                visitas++;

        printf("Analista %02d | Total de visitas: %02d | Clientes: ", i, visitas);
        if(visitas == 0)
            printf("Nenhum (Ocioso)");
        else{
            for(int j=0; j<numero_clientes; j++)
                if(melhor_global.alocacao[j] == i)
                    printf("[%02d] ", clientes[j].id_cliente);

        }
        puts("");
    }

    puts("\n------------------------------------- MAPA DE ROTAS E DISTANCIAS ------------------------------------\n");
    const char* nomes_cidades[] = {"Macae", "Rio das Ostras", "Conceicao de Macabu", "Quissama"}; //Traduz os ID's dos arquivos de entrada
    float custo_financeiro_total_empresa = 0.0;

    for(int i=0; i<numero_analistas; i++){
        int visitas = 0;
        float dist_total = 0.0;
        int tempo_atendimento_total = 0;
        int cidade_atual = analistas[i].id_cidade_base;

        for(int j=0; j<numero_clientes; j++){
            if(melhor_global.alocacao[j] == i){
                visitas++;
                dist_total += CalculaDistancia(cidade_atual, clientes[j].id_cidade);
                tempo_atendimento_total += matriz_tempos_atendimento[i][j];
                cidade_atual = clientes[j].id_cidade;
            }
        }

        //Se ele saiu da base, ele precisa voltar no fim do dia
        if(visitas > 0)
            dist_total += CalculaDistancia(cidade_atual, analistas[i].id_cidade_base);

float tempo_viagem_minutos = (dist_total / analistas[i].velocidade_media) * 60.0;
        float jornada_total_minutos = tempo_viagem_minutos + tempo_atendimento_total;

        int horas = (int)(jornada_total_minutos / 60);
        int minutos = (int)jornada_total_minutos % 60;

        //Custo Financeiro (Dinheiro Real - Gasolina)
        float custo_financeiro = dist_total * analistas[i].custo_por_km;
        custo_financeiro_total_empresa += custo_financeiro;

        //Custo Fitness (Financeiro + Multas daquele analista)
        float custo_fitness = custo_financeiro;

        for(int j=0; j<numero_clientes; j++){
            if(melhor_global.alocacao[j] == i){
                if(clientes[j].tipo_software != analistas[i].tipo_software)
                    custo_fitness += MULTA_SOFTWARE_INCOMPATIVEL;
            }
        }

        if(visitas > 0){
            // Verifica a meta
            int meta = 0;
            if(analistas[i].tipo_software == 1)
                meta = 4;
            else if(analistas[i].tipo_software == 2)
                meta = 3;
            else if(analistas[i].tipo_software == 3)
                meta = 2;

            if(visitas < meta)
                custo_fitness += (meta - visitas) * 50000.0; // Multa de meta

            if(jornada_total_minutos < 300.0)
                custo_fitness += (300.0 - jornada_total_minutos) * 1000.0; // Multa de ociosidade

            if(jornada_total_minutos > 480.0)
                custo_fitness += 2000000.0; // Multa de hora extra

            if(visitas > QNT_VISITAS)
                custo_fitness += 2000000.0;
        }

        //Imprime o cabeçalho do Analista com a separação dos Custos
        printf("\n[Analista %02d] Visitas: %02d | Distancia: %5.2f km | Jornada: %02dh %02dm\n", i, visitas, dist_total, horas, minutos);
        printf(" -> Custo Financeiro Real: R$ %7.2f | Custo Fitness (com multas): R$ %7.2f\n", custo_financeiro, custo_fitness);


        if(visitas == 0)
            printf(" -> ROTA: Ocioso na Base (%s)\n", nomes_cidades[analistas[i].id_cidade_base]);
        else{
            printf(" -> ROTA: Base(%s) ", nomes_cidades[analistas[i].id_cidade_base]);
            for(int j=0; j<numero_clientes; j++)
                if(melhor_global.alocacao[j] == i)
                    printf("-> Cliente %02d(%s) ", clientes[j].id_cliente, nomes_cidades[clientes[j].id_cidade]);
            printf("-> Retorno Base(%s)\n", nomes_cidades[analistas[i].id_cidade_base]);
        }
    }

    printf("\n--------------------------------------------------------------------------------------------------\n");
    printf("--> RESUMO FINANCEIRO GLOBAL <--\n");
    printf("CUSTO REAL TOTAL DA FROTA NO DIA: R$ %.2f\n", custo_financeiro_total_empresa);
    printf("CUSTO FITNESS DO AG (Global com clientes ignorados): R$ %.2f\n", melhor_global.custo);
    printf("----------------------------------------------------------------------------------------------------\n");

    int clientes_precisavam = 0;
    int clientes_atendidos = 0;
    int clientes_nao_atendidos = 0;

    for(int i=0; i<numero_clientes; i++){
        if(melhor_global.alocacao[i] != -2){
            clientes_precisavam++;

            if(melhor_global.alocacao[i] >= 0)
                clientes_atendidos++;

            else if(melhor_global.alocacao[i] == -1)
                clientes_nao_atendidos++;
        }
    }

    printf("\n--------------------------------------------------------------------------------------------------\n");
    printf("\n--> RESUMO DE ATENDIMENTOS <--\n");
    printf("Total de clientes que precisavam de visita: %02d\n", clientes_precisavam);
    printf("Quantidade de clientes ATENDIDOS:           %02d\n", clientes_atendidos);
    printf("Quantidade de clientes IGNORADOS:           %02d\n", clientes_nao_atendidos);
    printf("----------------------------------------------------------------------------------------------------\n");


    int qtd_incompativel = 0;
    int visitas_faltantes = 0;
    float minutos_ociosos = 0.0;
    int qtd_hora_extra = 0;
    int qtd_excesso_visitas = 0;

    for(int i=0; i<numero_analistas; i++){
        int visitas = 0;
        float dist_total = 0.0;
        int tempo_atendimento_total = 0;
        int cidade_atual = analistas[i].id_cidade_base;

        for(int j=0; j<numero_clientes; j++){
            if(melhor_global.alocacao[j] == i){
                visitas++;
                dist_total += CalculaDistancia(cidade_atual, clientes[j].id_cidade);
                tempo_atendimento_total += matriz_tempos_atendimento[i][j];
                cidade_atual = clientes[j].id_cidade;

                if(clientes[j].tipo_software != analistas[i].tipo_software)
                    qtd_incompativel++;
            }
        }

        if(visitas > 0){
            dist_total += CalculaDistancia(cidade_atual, analistas[i].id_cidade_base);
            float tempo_viagem = (dist_total / analistas[i].velocidade_media) * 60.0;
            float jornada = tempo_viagem + tempo_atendimento_total;

            int meta = 0;
            if(analistas[i].tipo_software == 1)
                meta = 4;
            else if(analistas[i].tipo_software == 2)
                meta = 3;
            else
                meta = 2;

            if(visitas < meta)
                visitas_faltantes += (meta - visitas);

            if(jornada < 300.0)
                minutos_ociosos += (300.0 - jornada);

            if(jornada > 480.0)
                qtd_hora_extra++;

            if(visitas > QNT_VISITAS)
                qtd_excesso_visitas++;
        }
    }

    printf("\n--------------------------------------------------------------------------------------------------\n");
    printf("\n--- RESUMO DE MULTAS APLICADAS ---\n");
    if(clientes_nao_atendidos > 0)
        printf("-> %d clientes ignorados. Multa: R$ %.2f\n", clientes_nao_atendidos, clientes_nao_atendidos * 500000.0);

    if(qtd_incompativel > 0)
        printf("-> %d analistas com software errado. Multa: R$ %.2f\n", qtd_incompativel, qtd_incompativel * 5000000.0);

    if(visitas_faltantes > 0)
        printf("-> Faltaram %d visitas na meta. Multa: R$ %.2f\n", visitas_faltantes, visitas_faltantes * 50000.0);

    if(minutos_ociosos > 0)
        printf("-> %.0f minutos de ociosidade. Multa: R$ %.2f\n", minutos_ociosos, minutos_ociosos * 1000.0);

    if(qtd_hora_extra > 0)
        printf("-> %d analistas fizeram hora extra. Multa: R$ %.2f\n", qtd_hora_extra, qtd_hora_extra * 2000000.0);

    if(qtd_excesso_visitas > 0)
        printf("-> %d analistas passaram do limite de visitas. Multa: R$ %.2f\n", qtd_excesso_visitas, qtd_excesso_visitas * 2000000.0);

    if(clientes_nao_atendidos == 0 && qtd_incompativel == 0 && visitas_faltantes == 0 && minutos_ociosos == 0 && qtd_hora_extra == 0 && qtd_excesso_visitas == 0)
        printf("-> Nenhuma regra de negocio foi violada.\n");
    printf("\n--------------------------------------------------------------------------------------------------\n");

    LiberaMemoria(populacao, nova_populacao, melhor_global.alocacao, clientes, analistas, numero_analistas);

 return 0;
}
