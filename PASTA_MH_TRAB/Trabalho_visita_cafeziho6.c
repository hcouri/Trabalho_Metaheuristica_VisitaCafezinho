#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct Cliente{
    int id_cidade;                         //Os ID's estão na Matriz de distâncias
    int tempo_ultima_visita;              //(dias)     //Tempo desde a última visita.
    float insatisfacao_cliente;          // (0-1)     //Nível de insatisfação do cliente.
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
float **matriz_distancias;
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
int CarregaClientes(Cliente **clientes) {
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


/*
Cliente CriaCliente(int a , int b, float c, int d, int e){
    Cliente C;
    C.id_cidade = a;
    C.tempo_ultima_visita = b;
    C.insatisfacao_cliente = c;
    C.freq_uso_suporte = d;
    C.tipo_software = e;
    return C;
}
Analista CriaAnalista(int a, float b, int c, int d, float e){
    Analista A;
    A.id_cidade_base = a;
    A.id_cidade_atual = a;
    A.grau_ociosidade = b;
    A.tipo_software = c;
    A.velocidade_media = d;
    A.custo_por_km = e;
    return A;
}

Cliente AtualizaCliente(Cliente C, int a, int b, float c, int d, int e, float f){
    C.id_cidade = a;
    C.tempo_ultima_visita = b;
    C.insatisfacao_cliente = c;
    C.freq_uso_suporte = d;
    C.tipo_software = e;
    C.PRIORIDADE = f;
    return C;
}
Analista AtualizaAnalista(Analista A, int a, int b, float c, int d, int e, float f){
    A.id_cidade_base = a;
    A.id_cidade_atual = b;
    A.grau_ociosidade = c;
    A.tipo_software = d;
    A.velocidade_media = e;
    A.custo_por_km = f;
    return A;
}
*/


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

int main(){
    srand(time(NULL));

    Cliente* clientes = NULL;
    Analista* analistas = NULL;

    //Lendo os dados dos arquivos
    CarregaMatriz();
    int numero_clientes = CarregaClientes(&clientes);
    int numero_analistas = CarregaAnalistas(&analistas);

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



        float min_score = 1000000.0; // C(e1)
        float max_score = -1000000.0; // C(e2)
        float scores[numero_analistas];

        //Descobrindo o melhor(e1) e o pior (e2) candidato
        for(int j=0; j<numero_analistas; j++){
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

        // Atualiza a cidade atual do analista
        analistas[melhor_analista].id_cidade_atual = clientes[i].id_cidade;
    }


    puts("---RESULTADO DA ALOCACAO---");
    for(int i=0; i<numero_clientes; i++){
         //Caso Normal
         if(alocacao_cliente_analista[i] >= 0)
            printf("Cliente %d (Prioridade: %.2f | Cidade: %d) -> Atendido por Analista %d\n", i, clientes[i].PRIORIDADE, clientes[i].id_cidade, alocacao_cliente_analista[i]);

         //Faltou analistas compatíveis
         else if(alocacao_cliente_analista[i] == -1)
            printf("Cliente %d (Prioridade: %.2f | Cidade: %d) -> ALERTA: SEM ANALISTA COMPATIVEL\n", i, clientes[i].PRIORIDADE, clientes[i].id_cidade);

         // Visita não é necessária(regra de negócio)
         else if (alocacao_cliente_analista[i] == -2)
            printf("Cliente %d (Dias desde a ultima visita: %d) -> VISITA DESNECESSARIA (Menos de 90 dias)\n", i, clientes[i].tempo_ultima_visita);
    }

    puts("\n---RETORNO PARA A BASE NO FIM DO DIA---");
    for(int j=0; j<numero_analistas; j++){
        float dist_retorno = CalculaDistancia(analistas[j].id_cidade_atual, analistas[j].id_cidade_base);
        if(dist_retorno > 0.0)
            printf("Analista %d retorna da Cidade %d para a Base (Cidade %d) - Distancia: %.2f km\n", j, analistas[j].id_cidade_atual, analistas[j].id_cidade_base, dist_retorno);
        else
            printf("Analista %d esta na base (Cidade %d)\n", j,  analistas[j].id_cidade_base);
    }

    // Liberação de Memória
    for(int i=0; i< total_cidades; i++)
        free(matriz_distancias[i]);

    free(matriz_distancias);
    free(clientes);
    free(analistas);

    //Mudar a ordenação para quicksort(por exemplo)
    //Implementar a carga horária|jornada de trabalho e suas regras de negócio
 return 0;
}
