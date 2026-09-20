// Charlie Tanfoll Pereira Lobo - 16827968
// Marina Cintra Queiroz - 17074404

#include "Arvoreb.cppm"
#include <iostream>
#include <limits>
#include <string>
#include <filesystem>

using namespace std;

/**
 * @brief Exibe o menu principal de opções para o gerenciamento da Árvore B.
 */
void exibirMenu() {
    cout << "\n========================================\n";
    cout << "       GERENCIADOR DE ARVORE B          \n";
    cout << "========================================\n";
    cout << " 1. Inserir chave (insertB)\n";
    cout << " 2. Remover chave (deleteB)\n";
    cout << " 3. Buscar chave (mSearch)\n";
    cout << " 4. Imprimir Arvore B (imprimirArvore)\n";
    cout << " 5. Exibir cabecalho e metadados\n";
    cout << " 0. Sair\n";
    cout << "========================================\n";
    cout << " Escolha uma opcao: ";
}

/**
 * @brief Realiza a leitura robusta de um numero inteiro a partir da entrada padrao.
 * @param prompt Mensagem exibida para orientar o usuario.
 * @return int Valor inteiro valido digitado pelo usuario.
 */
int lerInteiro(const string& prompt) {
    int valor;
    while (true) {
        cout << prompt;
        if (cin >> valor) {
            return valor;
        }
        cout << "[Erro] Entrada invalida! Digite apenas numeros inteiros.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int main() {
    const string arquivoIndice = "arvore.idx";
    int ordem = 4;

    cout << "=== Inicializacao da Arvore B em Disco ===\n";

    // Parametrizacao dinamica caso o arquivo ainda nao exista
    if (!filesystem::exists(arquivoIndice) || filesystem::file_size(arquivoIndice) < sizeof(struct header)) {
        cout << "Arquivo de indice nao encontrado. Criando nova Arvore B.\n";
        cout << "Defina a ordem M da arvore (pressione Enter para usar o padrao M=4): ";
        string entrada;
        getline(cin, entrada);
        if (!entrada.empty()) {
            try {
                int m = stoi(entrada);
                if (m >= 3) {
                    ordem = m;
                } else {
                    cout << "[Aviso] Ordem minima permitida e 3. Utilizando ordem 4.\n";
                }
            } catch (...) {
                cout << "[Aviso] Entrada invalida. Utilizando ordem padrao 4.\n";
            }
        }
    }

    Btree arvore(arquivoIndice, ordem);
    cout << "Arvore B carregada com sucesso! (Arquivo: '" << arquivoIndice << "', Ordem M = " << arvore.getOrdem() << ")\n";

    bool rodando = true;
    while (rodando) {
        exibirMenu();

        int opcao;
        if (!(cin >> opcao)) {
            cout << "[Erro] Opcao invalida! Digite um numero de 0 a 5.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (opcao) {
            case 1: {
                int chave = lerInteiro("\nDigite a chave inteira para insercao: ");
                auto acessos = arvore.insertB(chave);
                if (acessos.has_value()) {
                    cout << "[Sucesso] Chave " << chave << " inserida com sucesso!\n";
                    cout << "  -> Total de acessos a disco (leituras/escritas): " << *acessos << "\n";
                } else {
                    cout << "[Falha] Nao foi possivel inserir: chave " << chave << " ja existe na Arvore B.\n";
                }
                break;
            }

            case 2: {
                int chave = lerInteiro("\nDigite a chave inteira para remocao: ");
                auto acessos = arvore.deleteB(chave);
                if (acessos.has_value()) {
                    cout << "[Sucesso] Chave " << chave << " removida com sucesso!\n";
                    cout << "  -> Total de acessos a disco (leituras/escritas): " << *acessos << "\n";
                } else {
                    cout << "[Falha] Nao foi possivel remover: chave " << chave << " nao encontrada na Arvore B.\n";
                }
                break;
            }

            case 3: {
                int chave = lerInteiro("\nDigite a chave inteira para buscar: ");
                caminhoDaBusca busca = arvore.mSearch(chave);

                cout << "\n------------------ RESULTADO DA BUSCA (mSearch) ------------------\n";
                if (busca.encontrado) {
                    int rrnNo = busca.nodesAcessados.back()->rrn;
                    int posInterna = busca.posicoesInternas.back();
                    cout << " Status: ENCONTRADO\n";
                    cout << " Tupla: (No RRN: " << rrnNo 
                         << ", Posicao Interna: " << posInterna 
                         << ", Encontrado: true)\n";
                } else {
                    int rrnUltimo = busca.nodesAcessados.empty() ? -1 : busca.nodesAcessados.back()->rrn;
                    int posDescida = busca.posicoesInternas.empty() ? -1 : busca.posicoesInternas.back();
                    cout << " Status: NAO ENCONTRADO\n";
                    cout << " Tupla: (Ultimo No RRN: " << rrnUltimo 
                         << ", Posicao de Insercao: " << posDescida 
                         << ", Encontrado: false)\n";
                }
                cout << " Profundidade percorrida (nos visitados): " << busca.nodesAcessados.size() << "\n";
                cout << " Total de acessos (leituras) a disco: " << busca.acessosDisco << "\n";
                cout << "------------------------------------------------------------------\n";
                break;
            }

            case 4: {
                arvore.imprimirArvore();
                break;
            }

            case 5: {
                cout << "\n------------------ METADADOS DO CABECALHO ------------------\n";
                cout << "  Arquivo em disco: " << arquivoIndice << "\n";
                cout << "  Raiz atual (RRN): " << arvore.getRaiz() << (arvore.getRaiz() == -1 ? " (Arvore vazia)" : "") << "\n";
                cout << "  Ordem da arvore (M): " << arvore.getOrdem() << " vias\n";
                cout << "  Capacidade maxima por no: " << arvore.getOrdem() - 1 << " chaves\n";
                cout << "  Capacidade minima por no: " << (arvore.getOrdem() - 1) / 2 << " chaves\n";
                cout << "  Topo da pilha da lixeira (RRN): " << arvore.getLixeira() 
                     << (arvore.getLixeira() == -1 ? " (Lixeira vazia)" : " (Espaco disponivel para reuso)") << "\n";
                cout << "------------------------------------------------------------\n";
                break;
            }

            case 0: {
                cout << "\nFinalizando o programa... Todos os dados estao persistidos em disco.\n";
                rodando = false;
                break;
            }

            default: {
                cout << "[Aviso] Opcao desconhecida. Por favor, selecione uma opcao valida do menu.\n";
                break;
            }
        }
    }

    return 0;
}
