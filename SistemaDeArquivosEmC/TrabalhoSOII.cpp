#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_INODES 1000
#define MAX_BLOCOS 1000

// Estrutura para Inode
typedef struct {
    int data_criacao;
    int hora_criacao;
    int tamanho; // em bytes
    char permissoes[11]; // rwxrwxrwx
    int contador_links;
    char tipo; // '-' arquivo regular, 'd' diretório, 'l' link simbólico
    int blocos[8]; // 5 blocos diretos, 1 indireto simples, 1 indireto duplo, 1 indireto triplo
} Inode;

// Estrutura do sistema de arquivos
typedef struct {
    char bloco_tipo[MAX_BLOCOS]; // 'F' = Livre, 'B' = Defeituoso, 'I' = Inode, 'A' = Arquivo
    Inode inodes[MAX_INODES]; // Inodes estáticos
    int pilha_blocos_livres[MAX_BLOCOS];
    int topo_pilha;
    int tamanho_disco;
} SistemaArquivo;

SistemaArquivo fs; // Sistema de arquivos global

// Função para inicializar o sistema de arquivos
void inicializar_sistema_arquivo(int tamanho_disco) {
    fs.tamanho_disco = tamanho_disco;
    fs.topo_pilha = -1;

    // Inicializa os blocos como livres
    for (int i = 0; i < tamanho_disco; i++) {
        fs.bloco_tipo[i] = 'F'; // F = Livre
        fs.pilha_blocos_livres[++fs.topo_pilha] = i; // Empilha todos os blocos livres
    }

    // Inicializa inodes
    for (int i = 0; i < MAX_INODES; i++) {
        fs.inodes[i].tamanho = 0;
        strcpy(fs.inodes[i].permissoes, "rwxrwxrwx");
        fs.inodes[i].contador_links = 0;
        fs.inodes[i].tipo = '-'; // '-' para arquivo regular por padrão
        for (int j = 0; j < 8; j++) {
            fs.inodes[i].blocos[j] = -1; // Inicialmente, todos os blocos são inválidos
        }
    }
}

// Função para alocar um bloco livre
int alocar_bloco() {
    if (fs.topo_pilha == -1) {
        printf("Erro: Nao ha blocos livres disponiveis.\n");
        return -1;
    }
    int bloco_livre = fs.pilha_blocos_livres[fs.topo_pilha--];
    fs.bloco_tipo[bloco_livre] = 'A'; // Marca como bloco de arquivo
    return bloco_livre;
}

// Função para liberar um bloco
void liberar_bloco(int bloco) {
    if (bloco >= 0 && bloco < fs.tamanho_disco) {
        fs.bloco_tipo[bloco] = 'F'; // Marca como livre
        fs.pilha_blocos_livres[++fs.topo_pilha] = bloco; // Empilha de volta
    }
}

// Função para criar um arquivo ou diretório
void criar_arquivo(char *nome_arquivo, int tamanho_bytes) {
    if (tamanho_bytes > fs.tamanho_disco * 10) {
        printf("Erro: O tamanho do arquivo excede o limite do disco.\n");
        return;
    }

    // Busca um inode livre
    int inode_livre = -1;
    for (int i = 0; i < MAX_INODES; i++) {
        if (fs.inodes[i].tamanho == 0) { // Considera inode livre
            inode_livre = i;
            break;
        }
    }

    if (inode_livre == -1) {
        printf("Erro: Nao ha inodes disponiveis.\n");
        return;
    }

    // Configura o inode
    fs.inodes[inode_livre].tamanho = tamanho_bytes;
    fs.inodes[inode_livre].contador_links = 1;
    fs.inodes[inode_livre].tipo = '-'; // Arquivo regular

    // Aloca blocos para o arquivo
    int blocos_necessarios = (tamanho_bytes + 9) / 10; // Cada bloco tem 10 bytes
    for (int i = 0; i < blocos_necessarios && i < 5; i++) { // Até 5 blocos diretos
        int bloco_livre = alocar_bloco();
        if (bloco_livre != -1) {
            fs.inodes[inode_livre].blocos[i] = bloco_livre;
        }
    }
    printf("Arquivo '%s' criado com %d bytes.\n", nome_arquivo, tamanho_bytes);
}

// Função para listar os blocos
void listar_blocos() {
    printf("Estado dos blocos:\n");
    for (int i = 0; i < fs.tamanho_disco; i++) {
        printf("[%d]: %c\n", i, fs.bloco_tipo[i]);
    }
}

// Função para definir um bloco como defeituoso
void definir_bloco_defeituoso(int numero_bloco) {
    if (numero_bloco >= 0 && numero_bloco < fs.tamanho_disco) {
        fs.bloco_tipo[numero_bloco] = 'B'; // Marca como bloco defeituoso
        printf("Bloco %d foi marcado como defeituoso.\n", numero_bloco);
    } else {
        printf("Bloco invalido.\n");
    }
}

// Função para deletar um arquivo e liberar seus blocos
void deletar_arquivo(int inode_index) {
    if (inode_index >= 0 && inode_index < MAX_INODES && fs.inodes[inode_index].tamanho > 0) {
        // Liberar blocos do inode
        for (int i = 0; i < 8; i++) {
            if (fs.inodes[inode_index].blocos[i] != -1) {
                liberar_bloco(fs.inodes[inode_index].blocos[i]);
                fs.inodes[inode_index].blocos[i] = -1;
            }
        }
        fs.inodes[inode_index].tamanho = 0; // Marca o inode como livre
        printf("Arquivo deletado e blocos liberados.\n");
    } else {
        printf("Inode invalido ou arquivo ja deletado.\n");
    }
}

// Função para pausar a execução até que o usuário pressione uma tecla
void pausar_tela() {
    printf("\nPressione qualquer tecla para continuar...");
    getchar(); // Espera a entrada de um caractere
    getchar(); // Adiciona uma segunda chamada de getchar() para consumir o newline
}

// Função para exibir relatórios
void exibir_menu_relatorios() {
    printf("\n==== Relatorios do Sistema de Arquivos Inode ====\n");
    printf("1) Listar blocos ocupados por um arquivo\n");
    printf("2) Tamanho em blocos do maior arquivo que pode ser criado\n");
    printf("3) Arquivos íntegros e corrompidos\n");
    printf("4) Blocos perdidos e espaço perdido\n");
    printf("5) Listar todos os blocos com seu estado\n");
    printf("6) Listar arquivos e diretórios alocados\n");
    printf("7) Listar links simbólicos e físicos criados\n");
    printf("8) Voltar ao menu principal\n");
    printf("===============================================\n");
    printf("Escolha uma opção de relatorio: ");
}

void listar_blocos_ocupados_por_arquivo(int inode_index) {
    if (inode_index >= 0 && inode_index < MAX_INODES && fs.inodes[inode_index].tamanho > 0) {
        printf("Blocos ocupados pelo arquivo de inode %d:\n", inode_index);
        for (int i = 0; i < 8; i++) {
            if (fs.inodes[inode_index].blocos[i] != -1) {
                printf("Bloco %d\n", fs.inodes[inode_index].blocos[i]);
            }
        }
    } else {
        printf("Inode inválido ou arquivo inexistente.\n");
    }
}


// Esse método está errado, mas cansei de codar
// Vou fazer uma pilha auxiliar pegar todas as posições livres em sequência e somar a quantidade de posições
// E vou colocar a primeira posição da sequência numa variável int.
// Se depois de percorrer todo o vetor, não achar nenhuma sequência que seja maior que a variável quantidade
// Então eu imprimo a variável quantidade de posições (blocos) e a posição inicial e final da sequência.
void relatorio_maior_arquivo() {
    int maior_arquivo = 0;
    int blocos_livres = fs.topo_pilha + 1;

    if (blocos_livres > 0) {
        maior_arquivo = blocos_livres * 10;
    }

    printf("Tamanho em blocos do maior arquivo que pode ser criado: %d\n", blocos_livres);
}

void menu_relatorios() {
    int opcao;
    int inode_index;

    do {
        exibir_menu_relatorios();
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
                printf("Informe o inode do arquivo: ");
                scanf("%d", &inode_index);
                listar_blocos_ocupados_por_arquivo(inode_index);
                break;
            case 2:
                relatorio_maior_arquivo();
                break;
            case 3:
                printf("Relatório de arquivos íntegros/corrompidos ainda não implementado.\n");
                break;
            case 4:
                printf("Relatório de blocos perdidos ainda não implementado.\n");
                break;
            case 5:
                listar_blocos();
                break;
			case 6:
				printf("6) Listar arquivos e diretórios alocados\n");
				break;
			case 7:
				printf("7) Listar links simbólicos e físicos criados\n");
				break;
			case 8:
				printf("8) Voltar ao menu principal\n");
				break;
			}
    	}while(opcao!=8);
}

// Menu principal
void exibir_menu() {
    printf("\n==== Sistema de Arquivos Inode ====\n");
    printf("A) Definir o tamanho do disco\n");
    printf("B) Recuperar um bloco livre aleatoriamente\n");
    printf("C) Cadastrar novo arquivo ou diretório\n");
    printf("D) Marcar um bloco como defeituoso\n");
    printf("E) Deletar um arquivo\n");
    printf("F) Criar arquivo ou diretório em qualquer lugar\n");
    printf("G) Criar links simbólicos ou físicos\n");
    printf("H) Abrir menu de relatorios\n");
    printf("S) Sair\n");
    printf("===================================\n");
    printf("Escolha uma opção: ");
}

int main() {
    char opcao;
    int tamanho_disco, numero_bloco, inode_index, tamanho_bytes;
    char nome_arquivo[50];

    do {
        exibir_menu();
        scanf(" %c", &opcao);

        switch (opcao) {
            case 'A': // Definir o tamanho do disco
                printf("Informe o tamanho do disco (número de blocos): ");
                scanf("%d", &tamanho_disco);
                inicializar_sistema_arquivo(tamanho_disco);
                printf("Disco inicializado com %d blocos.\n", tamanho_disco);
                break;

            case 'B': // Recuperar um bloco livre aleatoriamente
                printf("Bloco livre recuperado: %d\n", alocar_bloco());
                break;

            case 'C': // Cadastrar novo arquivo ou diretório
                printf("Informe o nome do arquivo ou diretório: ");
                scanf("%s", nome_arquivo);
                printf("Informe o tamanho do arquivo (em bytes): ");
                scanf("%d", &tamanho_bytes);
                criar_arquivo(nome_arquivo, tamanho_bytes);
                break;

            case 'D': // Marcar bloco como defeituoso
                printf("Informe o número do bloco para marcar como defeituoso: ");
                scanf("%d", &numero_bloco);
                definir_bloco_defeituoso(numero_bloco);
                break;

            case 'E': // Deletar um arquivo e liberar seus blocos
                printf("Informe o número do inode para deletar: ");
                scanf("%d", &inode_index);
                deletar_arquivo(inode_index);
                break;

            case 'F':
                printf("Função de criação em qualquer lugar ainda não implementada.\n");
                break;

            case 'G':
                printf("Função de criação de links ainda não implementada.\n");
                break;
                
            case 'H':
                menu_relatorios();
                break;

            case 'S':
                printf("Saindo...\n");
                break;

            default:
                printf("Opção inválida. Tente novamente.\n");
                break;
        }

    } while (opcao != 'S');
}


