/**
#if WINDOWS
#define popen _popen
#endif
*/

#include <stdio.h>
#include <unistd.h>
#include "forecast.c"
#include "brain.h"

/* *******************************
 *      CHAMADA PARA O SWMM      *
 *********************************/
int SwmmCall()
{
    // Nomes dos arquivos setados "manualmente", assim como na previsao.c em SalvaResultados.
    // Isso deve ser acertado durante a fase de sincronizacao.
    char swmmPath[100] = "\"..\\..\\EPA_SWMM_5.0\\swmm5.exe "; // caminho para o executavel do swmm
    char inpFName[50] = "FrutalTeste.inp "; //nome do arquivo de entrada
    char rptFName[50] = "FrutalTesteOut1.rpt "; // nome do arquivo report saida
    char outFName[50] = "FrutalTesteOut2.out\""; // nome do arquivo binario saida
    char swmmCallCat[300] = {0};
    int res;

    strcat(swmmCallCat, swmmPath);
    strcat(swmmCallCat, inpFName);
    strcat(swmmCallCat, rptFName);
    strcat(swmmCallCat, outFName);

    if( ( system(swmmCallCat) ) != 0)
        return 1; // erro

    return 0;
}

/* *******************************
 * CHECK/UTILIZACAO DE DADOS VGI *
 *********************************/
int VGICheck()
{
    // se ha dados vgi, modifica .inp com novos niveis
    // senao, prossegue
    return 0;
}

/* *******************************
 *    COLETA DE DADOS DO BANCO   *
 *********************************/
int LoadDBData(FILE *inFile, stationData *stat_F, double *pobs_F)
{
    /* Por enquanto, na verdade, carregando os dados do arquivo.
     * Quando (se) um dia os dados chegarem pelo banco, refazer a funcao. */

    int dadoSendoLido = 1; //identifica o dado a ser lido do arquivo. 1 = no. da estacao, 2 = year, ...
    char c = NULL; //caractere sendo lido
    int linhaInvalida = 0;
    char year[10] = ""; //variaveis locais, receberao os dados do arquivo
    char date[10] = "";
    char time[10] = "";
    char temperatura[10] = "";
    char umidade[10] = "";
    char pressao[10] = "";
    char pobs[10] = "";
    int horas, minutos, resto;

    if ( feof(inFile) ) // Se a leitura anterior foi feita sobre a ultima linha, entao chegou ao fim do arquivo
        return 1;      // E a main deve ser avisada com o 1 para parar a leitura/calculos.

    while( c != '\n' && c != EOF ){ //le caractere por caractere ate o final da linha, armazenando os valores nas devidas variaveis
        c = fgetc(inFile);
        if(c != ','){
            switch(dadoSendoLido){
                case 1: //primeiro dado a ser lido: numero da estacao
                    if(c == '2') //se for a estacao 2, a flag linhaInvalida eh acionada porque a linha nao contem os dados completos
                        linhaInvalida = 1; //depois do switch case ela retornara 0 para "avisar" a main
                    break;
                case 2:
                    year[strlen(year)] = c;
                    break;
                case 3:
                    date[strlen(date)] = c;
                    break;
                case 4:
                    time[strlen(time)] = c;
                    break;
                case 5:
                    temperatura[strlen(temperatura)] = c;
                    break;
                case 6:
                    umidade[strlen(umidade)] = c;
                    break;
                case 7:
                    //radioacao
                    break;
                case 8:
                    //radioacaoref
                    break;
                case 9:
                    //radioacaoliq
                    break;
                case 10:
                    pressao[strlen(pressao)] = c;
                    break;
                case 11:
                    pobs[strlen(pobs)] = c;
                    break;
                default:
                    break;
            }
        }
        else{
            dadoSendoLido++;
        }
    }
    if(linhaInvalida == 1){
        LoadDBData(inFile, stat_F, pobs_F);
        return 0; //sucesso
    }
    else
    {
    /* Cada variavel local (ex.: "year") recebeu os dados direto do arquivo (em formato de string).
     * Agora elas retornam os valores para a main atraves das variaveis passadas por referencia
     * (ex.: "*year_I").*/
    stat_F->year = atoi(year);
    stat_F->date = atoi(date);
    stat_F->time = atoi(time);
    stat_F->temperatura = atof(temperatura); //retornado como float
    stat_F->umidade = atof(umidade);
    stat_F->pressao = atof(pressao);
    *pobs_F = atof(pobs);

    //radar, ifs..
    stat_F->topoDosEcos = -1;
    stat_F->VIL = -1;
    }
    return 0;          // Se chegou aqui, tudo esta correto e a main pode chamar os calculos pra linha lida.
}


/* *******************************
 * CONEXAO COM O DB (ou arquivo) *
 *********************************/
FILE *DBConnect()
{
    /* Codigo para o caso dos dados virem em arquivos. *
     * Quando (se) for por DB, colocar a conexao aqui. */

    FILE *estFile = NULL; // ponteiro para arquivo da estacao
    char estFname[100] = "DATA085_275a277.dat"; // nome do arquivo da estacao

    if( access(estFname, F_OK ) != -1 ) // Checa existencia do arquivo da estacao
    {
        if( (estFile = fopen(estFname, "r")) == NULL)
        {
            printf("Nao foi possivel abrir o arquivo de entrada da estacao.\n");
            return estFile; // Erro repassado a main
        }
        else{ // Se for possivel abrir o arqivo, retorna ponteiro a main
                printf("%d", estFile);
            printf("\n%p", estFile);
            return estFile;
        }
    }
    else
    {
        printf("Arquivo de entrada nao encontrado.\n");
        return estFile; // Erro repassado a main
    }
}

/* *******************************
 *             MAIN              *
 *********************************/
int main()
{
    FILE *DBConn; // Arquivo .dat de entrada com dados da estacao
    int iteracao = 1; // = "linha" do original. Iteracao = 1 -> realiza os calculos em cima de uma linha de dados. Iteracao = 2 -> realiza calculos 2a vez...
    double pobs; // Precipitacao observada
    stationData stat; // Struct stationData com os dados da estacao

    // --- ESTABELECE CONEXAO COM O "DB" (arquivo)
    DBConn = DBConnect();
    if (DBConn == NULL) // Erro
    {
        printf("\nErro no retorno da conexao com 'banco'.");
        return 1;
    }

    // --- CARREGA DADOS DO BANCO (arquivo) NAS VARIAVEIS E CHAMA PREVISAO DE PRECIPITACAO
    while ( ( LoadDBData(DBConn, &stat, &pobs) ) == 0 )
    {
        PrecForecast(&stat, &pobs, &iteracao);
    }

    // --- CHECAGEM E UTILIZACAO DE DADOS VGI
    VGICheck();

    // --- SWMM
    if ( (SwmmCall() ) != 0)
        return 1; // Erro

    return 0;
}
