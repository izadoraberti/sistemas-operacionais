/******************************************************************************
* Universidade Federal do Rio Grande do Sul
* Instituto de Informática
* Cursos de Ciência da Computação e Engenharia da Computação
* Disciplina de Sistemas Operacionais I N - 2018/1
* Trabalho 2 - Biblioteca Sistemas de Arquivos T2FS
* Adriano Gebert Gomes - 209865
* Izadora Dourado Berti - 275606
* Letícia dos Santos - 275604
******************************************************************************/
#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/t2fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*** VARIAVEIS GLOBAIS ***/
typedef struct t2fs_superbloco T_SUPERBLOCK; //SB_t
typedef struct t2fs_inode T_INODE; //INO_t
typedef struct t2fs_record T_RECORD; //T_RECORD
typedef struct arquivos_abertos T_ARQ_ABERTO;
struct arquivos_abertos{
	int arqaberto;
	int inode;
	int record_block;
	char nome[58];
	int tamanho;
	int offset;
	BYTE tipo;
	T_ARQ_ABERTO* prox;
};
typedef struct diretorio_struct T_DIR;
struct diretorio_struct{
	char nome[58];
	int record_block;
	int dir_block;
	T_DIR* pai;
	T_DIR* filho;
	T_DIR* irmao;
	T_RECORD* record;
};


T_ARQ_ABERTO arq_abertos;
T_DIR *dir_atual;
T_DIR raiz;
T_RECORD *record_atual;



int debug = 1;
static int disco_inicializado=0;
static int opendir_da_create = 0;
unsigned char buffer[SECTOR_SIZE];
unsigned char blocobuffer[16*SECTOR_SIZE];
T_SUPERBLOCK *superbloco;
T_INODE global_inode;

int read_block(int setor);
int write_block(int setor);
void info_disco();
int inicializa_disco();

/** Funções auxiliares **/

/*Retorna o primeiro setor da área de dados no disco */
int obtem_primeiro_setor_dados(){
	return superbloco->blockSize * (superbloco->freeBlocksBitmapSize + superbloco->freeInodeBitmapSize + superbloco->inodeAreaSize);
}



void info_disco(){
  printf("Informações disco\n");
	printf("%d\n%d\n%d\n%d\n%d\n%d\n", superbloco->superblockSize, superbloco->freeBlocksBitmapSize, superbloco->freeInodeBitmapSize, superbloco->inodeAreaSize, superbloco->blockSize, superbloco->diskSize);
}





int read_block(int sector)
{
  int i;
  int x = 0;
  for (i = 0; i < 16; ++i)
  {
    if(read_sector(sector + i, buffer) != 0)
        return -1;
    memcpy((blocobuffer + x), buffer, sizeof(buffer));
    x = x + 256;
  }
  return 0;
}


int write_block(int sector)
{
  int i;
  int x = 0;
  for (i = 0; i < 32; ++i)
  {
    memcpy(buffer, (blocobuffer + x), sizeof(buffer));
    if(write_sector(sector + i, buffer) != 0)
        return -1;
    x = x + 256;
  }

  return 0;
}

#define inode_area 3  //deixando esse valor pq seila ne
int get_block_from_inode(T_INODE* novoinode, int inode)
{
  int offset = inode*32;
  int bloco_para_ler = 0;
  while (offset >= 8192) //256*32if(debug)
  {
    offset = offset - 8192;
    ++bloco_para_ler;
  }
  read_block(inode_area + bloco_para_ler*32);

  novoinode->blocksFileSize = *((int *)(blocobuffer + offset));
  novoinode->bytesFileSize = *((int *)(blocobuffer + offset+4));
  novoinode->dataPtr[0] = *((int *)(blocobuffer + offset+8));
  novoinode->dataPtr[1] = *((int *)(blocobuffer + offset + 12));
  novoinode->singleIndPtr = *((int *)(blocobuffer + offset + 16));
  novoinode->doubleIndPtr = *((int *)(blocobuffer + offset + 20));
  //novoinode->(????RESERVADO) = *((int *)(blocobuffer + offset+24));

  return novoinode->blocksFileSize;
}

int populate_dir_struct_from_block(int bloco)
{
  int auxbloco = bloco;
  if (bloco == 0)
    dir_atual = &raiz;

  if (debug) printf("[populate_dir_struct_from_block] populando: %s\n", dir_atual->nome);
  int iterador = 1;
  while (iterador < 64)
  {
    read_block(obtem_primeiro_setor_dados() + bloco*16);
    T_DIR* auxdir = malloc(sizeof(T_DIR));
    T_RECORD* esserecord = malloc(16*sizeof(int));
		T_RECORD* auxrecord = malloc(16*sizeof(int));

    auxrecord->TypeVal = *((BYTE *)(blocobuffer + iterador*64));
    strncpy(auxrecord->name, blocobuffer + iterador*64 + 1, 58);
    auxrecord->inodeNumber = *((int *)(blocobuffer + iterador*64 + 61));

    if (esserecord->TypeVal == 0x02)
    {
      if (auxbloco == 0) //populando raiz
      {
        strncpy(auxdir->nome, esserecord->name, 58);
        auxdir->pai = dir_atual;
        auxdir->record_block = dir_atual->dir_block;
        auxdir->filho = NULL;
        auxdir->irmao = NULL;
        auxdir->record = esserecord;
        dir_atual->filho = auxdir;
        auxdir->dir_block = get_block_from_inode(&global_inode, auxdir->record->inodeNumber);

        auxbloco = -1;
      } else if (auxbloco != 0)
        {
          if (dir_atual->filho != NULL)
          {
            dir_atual = dir_atual->filho;
            while (dir_atual->irmao != NULL)
            {
              dir_atual = dir_atual->irmao;
            }

          strncpy(auxdir->nome, esserecord->name, 58);
          auxdir->pai = dir_atual->pai;
          auxdir->record_block = dir_atual->pai->dir_block;
          auxdir->filho = NULL;
          auxdir->irmao = NULL;
          auxdir->record = esserecord;
          dir_atual->irmao = auxdir;

          auxdir->dir_block = get_block_from_inode(&global_inode, auxdir->record->inodeNumber);

          populate_dir_struct_from_block(auxdir->dir_block);
          dir_atual = auxdir->pai;
          } else if (dir_atual->filho == NULL)
            {
              strncpy(auxdir->nome, esserecord->name, 58);
              auxdir->pai = dir_atual;
              auxdir->record_block = dir_atual->dir_block;
              auxdir->filho = NULL;
              auxdir->irmao = NULL;
              auxdir->record = esserecord;
              dir_atual->filho = auxdir;
              auxdir->dir_block = get_block_from_inode(&global_inode, auxdir->record->inodeNumber);
              populate_dir_struct_from_block(auxdir->dir_block);

              dir_atual = auxdir->pai;
            }
        } else
          {
          free(auxdir);
          free(esserecord);
          }
    }
    ++iterador;
  }
}


int path_parser(char* path, char* pathfound)
{
  char* token;
  char aux[2048];
  strcpy(aux, path);

  if(debug) printf ("[path_parser] caminho: \"%s\"\n", path);

  int char_size = 1;
  token = strtok (aux, "/");
  while (token != NULL)
  {
    token = strtok (NULL, "/");
    ++char_size;
  }

  const char *paths[char_size];
  strcpy(aux, path);

  char_size = 0;
  token = strtok (aux, "/");
  while (token != NULL)
  {
    if(debug) printf("token: %s\n", token);
    paths[char_size] = token;
    ++char_size;
    token = strtok (NULL, "/");
  }

  strcpy(pathfound, paths);
  return char_size;
}


int find_record_in_blocobuffer(T_RECORD* auxrecord, BYTE type, char* filename)
{
    T_INODE newinode;
   // int block_to_read = get_block_from_inode(&newinode, current_dir->record->inodeNumber);
    // printf("[find_record_in_blocobuffer] lendo bloco: %d\n", block_to_read);
   // read_block(data_area + block_to_read*16);

  int iterador = 0;
  while (iterador < 64)
  {
    auxrecord->TypeVal = *((BYTE *)(blocobuffer + iterador*64));
    strncpy(auxrecord->name, blocobuffer + iterador*64 + 1, 58);
    auxrecord->inodeNumber = *((int *)(blocobuffer + iterador*64 + 61));


    if (auxrecord->TypeVal == type && (strcmp(filename, auxrecord->name) == 0))
      return iterador-100;

    ++iterador;
  }
  return -1;
}


/*
int find_free_record_in_blockbuffer(REC_t* auxrecord)
{
  INO_t *newinode;
  int block_to_read = get_block_from_inode(&newinode, current_dir->record->inodeNumber);
  read_block(data_area + block_to_read*16);

  int iterator = 0;
  while (iterator < 64)
  {

    auxrecord->TypeVal = *((BYTE *)(blockbuffer + iterator*64));
    strncpy(auxrecord->name, blockbuffer + iterator*64 + 1, 32);
    auxrecord->blocksFileSize = *((DWORD *)(blockbuffer + iterator*64 + 33));
    auxrecord->bytesFileSize = *((DWORD *)(blockbuffer + iterator*64 + 37));
    auxrecord->inodeNumber = *((int *)(blockbuffer + iterator*64 + 41));
    if (auxrecord->TypeVal == 0x00)
    {
      // printf("[find_free_record_in_blockbuffer] record livre encontrado!\n");
      return iterator*64;
    }
    ++iterator;
  }
  return ERROR;
}
*/
int create_record_write_to_disk(int freeblock, int freeinode, char* filename, BYTE type)
{
  int bloco_atual = dir_atual->dir_block;


  if (read_block(inode_area + bloco_atual * 32) != 0)
  {
    if(debug) printf("[create_record_write_to_disk] erro ao ler bloco em create_record_write_to_disk\n");
    return -1;
  }

  T_RECORD* auxrecord = malloc(32*sizeof(int));

  if (find_record_in_blocobuffer(auxrecord, type, filename) < -1)
  {
    free(auxrecord);
    return -1;
  }
/*
  int offset = find_free_record_in_blocobuffer(auxrecord);
  if (offset == -1)
  {
    if(debug) printf("[create_record_write_to_disk] nenhum record livre disponivel no bloco\n");
    free(auxrecord);
    return -1;
  } else if (offset < -1)
    {
      free(auxrecord);
      return -1;
    }

  auxrecord->TypeVal = type;
  strcpy(auxrecord->name, filename);
  auxrecord->blocksFileSize = 1;
  if (type == 0x02)
    auxrecord->bytesFileSize = 4096;
  else
    auxrecord->bytesFileSize = 0;

  auxrecord->inodeNumber = freeinode;

  memcpy((blocobuffer + offset), &auxrecord->TypeVal, sizeof(BYTE));
  memcpy((blocobuffer + offset + 1), &auxrecord->name, 32*sizeof(char));
  memcpy((blocobuffer + offset + 33), &auxrecord->blocksFileSize, sizeof(DWORD));
  memcpy((blocobuffer + offset + 37), &auxrecord->bytesFileSize, sizeof(DWORD));
  memcpy((blocobuffer + offset + 41), &auxrecord->inodeNumber, sizeof(int));

  if(write_block(data_area + bloco_atual*16) != 0)
  {
    free(auxrecord);
    return -1;
  }

	if (type == 0x02) //é dir
  {
    DIR_t* newdir = malloc(sizeof(DIR_t));

    if (dir_atual->filho != NULL)
    {
      dir_atual = dir_atual->filho;
      while (dir_atual->irmao != NULL)
      {
        dir_atual = dir_atual->irmao;
      }

      newdir->pai = dir_atual->pai;
      newdir->record_block = dir_atual->pai->dir_block;
      newdir->dir_block = bloco_atual;
      newdir->filho = NULL;
      newdir->irmao = NULL;
      newdir->record = auxrecord;
      strncpy(newdir->name, newdir->record->name, 58);

      dir_atual->irmao = newdir;
//
      return 0;
    } else if (dir_atual->filho == NULL)
      {
        newdir->pai = dir_atual;
        newdir->record_block = dir_atual->dir_block;
        newdir->dir_block = bloco_atual;
        newdir->filho = NULL;
        newdir->irmao = NULL;
        newdir->record = auxrecord;
        dir_atual->filho = newdir;
        return 0;
      }

    free(newdir);
    printf("[create_record_write_to_disk] falha ao atualizar arvore de diretorios\n");
    return -1;
  }
  free(auxrecord);
  return 0;*/
}


int create_inode_write_to_disk(int freeblock, int freeinode)
{
  int offset = freeinode*32;
  read_block(inode_area);

  T_INODE newinode;

  newinode.dataPtr[0] = freeblock;
  newinode.dataPtr[1] = INVALID_PTR;
  newinode.singleIndPtr = INVALID_PTR;
  newinode.doubleIndPtr = INVALID_PTR;
  memcpy((blocobuffer + offset), &newinode.dataPtr[0], sizeof(int));
  memcpy((blocobuffer + offset + 4), &newinode.dataPtr[1], sizeof(int));
  memcpy((blocobuffer + offset + 8), &newinode.singleIndPtr, sizeof(int));
  memcpy((blocobuffer + offset + 12), &newinode.doubleIndPtr, sizeof(int));

  newinode.dataPtr[0] = *((int *)(blocobuffer + offset));
  newinode.dataPtr[1] = *((int *)(blocobuffer + offset + 4));
  newinode.singleIndPtr = *((int *)(blocobuffer + offset + 8));
  newinode.doubleIndPtr = *((int *)(blocobuffer + offset + 12));

  if (newinode.dataPtr[0] != freeblock)
  {
    if (debug) printf("erro ao escrever inode no buffer\n");
  }

  if (write_block(inode_area) != 0)
      return -1;

  return 0;
}








int inicializa_disco(){
  /* Alocando espaço em memória para guardar o superbloco */
  superbloco = malloc(sizeof(T_SUPERBLOCK));

  /* Lendo as informações do superbloco do disco*/
  if (read_sector(0, buffer)){
    if (debug) printf ("Erro ao ler as informações do superbloco no disco \n");
    return -1;
  }

  /* Recuperando as informações do superbloco do disco */
  //strncpy(superbloco->id, buffer, 4);
  //superbloco->id = ;
  superbloco->superblockSize =  *((WORD *) (buffer + 6)) ;
  superbloco->freeBlocksBitmapSize = *((WORD *) (buffer + 8)) ;
  superbloco->freeInodeBitmapSize = *((WORD *) (buffer + 10)) ;
  superbloco->inodeAreaSize = *((WORD *) (buffer + 12)) ;
  superbloco->blockSize = *((WORD *) (buffer + 14)) ;
  superbloco->diskSize = *((DWORD *) (buffer + 16)) ;

  /* Validações do superbloco do disco */
  /*if (superbloco->id != "T2FS"){
    if (debug) printf ("Sistema de arquivo inválido \n");
    return -1;
  }*/

  /*if (superbloco->version != "0x7E21"){
    if (debug) printf ("Versão do sistema de arquivo inválida \n");
    return -1;
  }*/

	arq_abertos.arqaberto = 0;
	arq_abertos.inode = NULL;
	arq_abertos.prox = NULL;

	dir_atual = malloc(sizeof(T_DIR));
	
	char aux_nome[]="raiz";
	strncpy(raiz.nome,aux_nome,4);
	raiz.pai=NULL;
	raiz.record_block=0;
	raiz.dir_block=0;
	raiz.filho=NULL;
	raiz.irmao=NULL;

	//T_RECORD *raiz_record = malloc(16*sizeof(int));
	T_RECORD *raiz_record = malloc(sizeof(T_RECORD));
	raiz_record->TypeVal=0x02;
	strncpy(raiz_record->name,aux_nome,4);
	raiz_record->inodeNumber=0;
	raiz.record=raiz_record;

/*raiz_record->blocksFileSize=1;  //pq n tem nessa versao do trabalho
	raiz_record->bytesFileSize=-1;
	*/

	if (read_sector(obtem_primeiro_setor_dados(), buffer) != 0){
		if (debug)printf ("Erro na leitura do setor de dados \n");
	}
	
	record_atual=malloc(sizeof(T_RECORD));
	record_atual=NULL;


  populate_dir_struct_from_block(0);


  dir_atual = &raiz;
	if (debug) printf("[disk_info] diretorio %s: '/'\n", dir_atual->nome);

  if(raiz.filho)
  {
		dir_atual = raiz.filho;
		if (debug) printf("[disk_info] subdiretorios na raiz:\n");
    do
    {
		if (debug) printf("\"/%s\"\n", dir_atual->nome);
      dir_atual = dir_atual->irmao;
    } while(dir_atual != NULL);
  }

	if (debug) printf("\n\n[disk_info] >> T2FS inicializado!\n\n");
	disco_inicializado=1;
  dir_atual = &raiz;

  return 0;
}


////////////////////////////////////////////////////TEM QUE BOTAR AQUELA DO ADRIANO PQ ELE DIZ QUE TA MAIS CERTA, MUDEI, E DEU PREGUIÇA DE MUDAR DE VOLTA
int identify2 (char *name, int size)
{
  if(disco_inicializado == 0)
    inicializa_disco();
  char* grupo = "Adriano Gebert Gomes \t 209865 \n Izadora Dourado Berti \t 275606 \n Letícia dos Santos \t 275604\n";
  if(size < sizeof(grupo))
    return -1;
  strncpy(name, grupo, sizeof(grupo));
  return 0;
}


/** OBJETIVO: Função usada para criar um novo arquivo no disco **/
FILE2 create2 (char *filename){
  if(disco_inicializado == 0)
    inicializa_disco();

  if (strlen(filename) > 58)
  {
    if (debug) printf("[create2] limite de 58 caracteres para nome de arquivo, tente novamente\n\n\n");
    return -1;
  }

  int freeinode = searchBitmap2(BITMAP_INODE, 0);
  if (freeinode < 0)
  {
    if(debug) printf("[create2]nao existe inode disponivel, apague algum arquivo antes de criar outro\n\n\n");
    return -1;
  }

  int freeblock = searchBitmap2(BITMAP_DADOS, 0);
  if(freeblock < 0)
  {
    if(debug) printf("[create2] disco cheio, apague algum arquivo e tenta novamente\n\n\n");
    return -1;
  }

  if(debug) printf("[create2] criando arquivo \"%s\"\n", filename);

  char *path[25];
  int dirs = path_parser(filename, &path);
  char *nome = path[dirs-1];

  if (dirs > 1){
    dir_atual = raiz.filho;
    opendir_da_create = 1;
    opendir2(filename);
    opendir_da_create = 0;
  } else dir_atual = &raiz;

	if (debug) printf("[create2] inode livre encontrado: %d\n", freeinode);
	if (debug)printf("[create2] bloco livre encontrado: %d\n", freeblock);
	if (debug)printf("[create2] diretorio atual: %s\n", dir_atual->nome);
/*
  if (create_record_write_to_disk(freeblock, freeinode, name, 0x01) != 0)
  {
		if (debug) printf("[create2] falha ao criar record, diretorio cheio ou arquivo ja existe\n\n\n");
    return -1;
  }

  if (create_inode_write_to_disk(freeblock, freeinode) != 0)
  {
		if (debug) printf("[create2] falha ao escrever inode no disco\n\n\n");
    return -1;
  }
*/
  if (setBitmap2 (BITMAP_INODE, freeinode, 1) < 0)
  {
		if (debug)printf("[create2] falaha o setar bitmap inode\n\n\n");
    return -1;
  }

  if (setBitmap2 (BITMAP_DADOS, freeblock, 1) < 0)
  {
		if (debug)printf("[create2] falha o setar bitmap bloco\n\n\n");
    return -1;
  }

	if (debug)printf("[create2] \"%s\" criado com sucesso\n", filename);
	if (debug)printf("\n\n");
  return freeinode;
}

/** OBJETIVO: Função usada para remover (apagar) um arquivo no disco **/
int delete2(char *filename){
  return -1;
}

/** OBJETIVO: Função que abre um arquivo existente no disco **/
FILE2 open2(char *filename){
  return -1;
}

/** OBJETIVO: Função usada para fechar um arquivo **/
int close2 (FILE2 handle){
  return -1;
}

/** OBJETIVO: Função usada para realizar a leitura de uma certa quantidade de bytes(size) em um arquivo **/
int read2(FILE2 handle, char* buffer, int size){
  return -1;
}

/** OBJETIVO: Função usada para realizar a escrita de uma certa quantidade de bytes(size) em um arquivo **/
int write2(FILE2 handle, char* buffer, int size){
  return -1;
}

/** OBJETIVO: Função usada para truncar um arquivo. Remove do arquivo todos os bytes a partir da posição atual do contatdor de posição(current pointer), inclusive, até o seu final.**/
int truncate2(FILE2 handle){
  return -1;
}

/** OBJETIVO: Altera o contador de posição (current pointer) do arquivo para offset **/
int seek2 (FILE2 handle, unsigned int offset){
  return -1;
}

/** OBJETIVO: Função usada para criar um novo diretório **/
int mkdir2 (char *pathname){
  return -1;
}

/** OBJETIVO: Função usada para remover (apagar) um diretório do disco **/
int rmdir2 (char *pathname){
  return -1;
}

/** OBJETIVO: Função usada para alterar o CP (current path) **/
int chdir2 (char*pathname){
  return -1;
}

/** OBJETIVO: Função usada para obter o caminho do diretório existente **/
int getcwd2 (char *pathname, int size){
  return -1;
}

/** OBJETIVO: Função que abre um diretório existente no disco **/
DIR2 opendir2(char*pathname){
  return -1;
}

/** OBJETIVO: Função usada para ler as entradas de um diretório **/
int readdir2(DIR2 handle, DIRENT2 *dentry){
  return -1;
}

/** OBJETIVO: Função usada para fechar um diretório **/
int closedir2 (DIR2 handle){
  return -1;
}


/*************** MAIN DE TESTE *********************/
/*************** MAIN DE TESTE *********************/
/*************** MAIN DE TESTE *********************/

int main(){

	inicializa_disco();
  info_disco();
	
  return 0;
}

/*************** FIM DA MAIN DE TESTE *********************/
/*************** FIM DA MAIN DE TESTE *********************/
/*************** FIM DA MAIN DE TESTE *********************/
