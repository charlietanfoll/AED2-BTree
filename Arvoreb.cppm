// Charlie Tanfoll Pereira Lobo - 16827968
// Marina Cintra Queiroz - 17074404

#pragma once

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <span>
#include <memory>
#include <optional>
#include <utility>

using namespace std;

class Node;
class Btree;

struct header {
    int root;           // RRN da raiz (-1 se vazia)
    int ordem;          // Grau M da árvore (M vias, M-1 chaves)
    int pilhaDaLixeira; // Topo da pilha de nós excluídos
};

struct caminhoDaBusca {
    bool encontrado = false;                 // Indica se a chave foi encontrada
    int acessosDisco = 0;                    // Quantidade de acessos (leituras) a disco durante a busca
    vector<int> posicoesInternas;            // Índices internos no nó durante o percurso
    vector<unique_ptr<Node>> nodesAcessados; // Pilha de nós visitados da raiz até a parada
};

class Node {
public:
    // Carrega um nó existente do disco a partir do RRN
    Node(int rrn, int ordem, fstream * file) : rrn(rrn) {
        int tamanho = ordem * 2;
        buffer.resize(tamanho);

        int posicao = sizeof(header) + (rrn * (ordem * 2 * sizeof(int)));
        file->clear();
        file->seekg(posicao);
        file->read(reinterpret_cast<char *>(buffer.data()), buffer.size() * sizeof(int));

        chaves = span<int>(buffer.data() + 1, ordem - 1);
        nos = span<int>(buffer.data() + ordem, ordem);
    }

    // Inicializa um nó novo em memória (evita ler dados inexistentes do disco)
    Node(int rrn, int ordem) : rrn(rrn) {
        int tamanho = ordem * 2;
        buffer.assign(tamanho, -1);
        buffer[0] = 0; // Quantidade de chaves iniciais
        for (int i = 1; i < ordem; ++i) {
            buffer[i] = 0;
        }
        chaves = span<int>(buffer.data() + 1, ordem - 1);
        nos = span<int>(buffer.data() + ordem, ordem);
    }

    // Construtor de Movimento
    Node(Node&& other) noexcept : rrn(other.rrn), buffer(std::move(other.buffer)) {
        int ordem = static_cast<int>(buffer.size() / 2);
        chaves = span<int>(buffer.data() + 1, ordem - 1);
        nos = span<int>(buffer.data() + ordem, ordem);
    }

    // Operador de Atribuição por Movimento
    Node& operator=(Node&& other) noexcept {
        if (this != &other) {
            rrn = other.rrn;
            buffer = std::move(other.buffer);
            int ordem = static_cast<int>(buffer.size() / 2);
            chaves = span<int>(buffer.data() + 1, ordem - 1);
            nos = span<int>(buffer.data() + ordem, ordem);
        }
        return *this;
    }

    // Construtor de Cópia Deletado
    Node(const Node&) = delete;
    // Operador de Atribuição por Cópia Deletado
    Node& operator=(const Node&) = delete;

    int& chavesTotais() { return buffer[0]; }
    const int& chavesTotais() const { return buffer[0]; }

    int rrn;
    vector<int> buffer;
    span<int> chaves;
    span<int> nos;
};

class Btree {
public:
    Btree(const string& alias, int ordem = 10) {
        if (!filesystem::exists(alias) || filesystem::file_size(alias) < sizeof(struct header)) {
            file.open(alias, ios::out | ios::in | ios::binary | ios::trunc);
            header.root = -1;
            header.ordem = ordem;
            header.pilhaDaLixeira = -1;
            file.write(reinterpret_cast<char *>(&header), sizeof(header));
            file.flush();
        } else {
            file.open(alias, ios::out | ios::in | ios::binary);
            file.read(reinterpret_cast<char *>(&header), sizeof(header));
        }
    }

    ~Btree() {
        if (file.is_open()) {
            file.seekp(0);
            file.write(reinterpret_cast<char *>(&header), sizeof(header));
            file.flush();
            file.close();
        }
    }

    int getRaiz() const { return header.root; }
    int getOrdem() const { return header.ordem; }
    int getLixeira() const { return header.pilhaDaLixeira; }

    /**
     * @brief Realiza a busca iterativa de uma chave na Árvore B.
     * @param elemento Chave inteira a ser pesquisada.
     * @return caminhoDaBusca Estrutura contendo flag de sucesso, contador de leituras e a pilha de nós acessados.
     * @pre Arquivo da árvore aberto e válido.
     * @post Retorna o caminho da raiz até o nó onde a busca terminou sem alterar o disco.
     */
    caminhoDaBusca mSearch(int elemento) {
        caminhoDaBusca caminho;
        caminho.encontrado = false;
        caminho.acessosDisco = 0;

        if (header.root == -1) {
            return caminho;
        }

        int rrnAtual = header.root;

        while (rrnAtual != -1) {
            auto noAtual = make_unique<Node>(rrnAtual, header.ordem, &file);
            caminho.acessosDisco++; // 1 leitura de nó em disco

            int inicio = 0;
            int fim = noAtual->chavesTotais() - 1;
            int indice = 0;
            bool achouNoNode = false;

            // Busca binária dentro do nó
            while (inicio <= fim) {
                int meio = inicio + (fim - inicio) / 2;

                if (noAtual->chaves[meio] == elemento) {
                    indice = meio;
                    achouNoNode = true;
                    break;
                } else if (elemento < noAtual->chaves[meio]) {
                    fim = meio - 1;
                } else {
                    inicio = meio + 1;
                }
            }

            if (achouNoNode) {
                caminho.encontrado = true;
                caminho.posicoesInternas.push_back(indice);
                caminho.nodesAcessados.push_back(move(noAtual));
                return caminho;
            }

            indice = inicio;
            int proximoRrn = noAtual->nos[indice];

            caminho.posicoesInternas.push_back(indice);
            caminho.nodesAcessados.push_back(move(noAtual));

            rrnAtual = proximoRrn;
        }

        return caminho;
    }

    /**
     * @brief Insere uma chave iterativamente na Árvore B com split e overflow.
     * @param chave Inteiro a ser inserido.
     * @return optional<int> Quantidade total de acessos ao disco (leituras + escritas) se inserido com sucesso; nullopt se chave duplicada.
     * @pre Arquivo da árvore aberto e válido.
     * @post Chave persistida mantendo balanceamento e propriedades da Árvore B.
     */
    optional<int> insertB(int chave) {
        int acessosDisco = 0;

        // Caso 1: Árvore vazia
        if (header.root == -1) {
            int novoId = obterNovoId(&acessosDisco);
            Node raiz(novoId, header.ordem);

            raiz.chavesTotais() = 1;
            raiz.chaves[0] = chave;

            salvarNoNoDisco(novoId, &raiz, &acessosDisco);

            header.root = novoId;
            file.seekp(0);
            file.write(reinterpret_cast<char*>(&header), sizeof(header));
            file.flush();
            acessosDisco++; // Escrita do cabeçalho
            return acessosDisco;
        }

        // Caso 2: Chamada obrigatória ao mSearch
        caminhoDaBusca busca = mSearch(chave);
        acessosDisco += busca.acessosDisco;

        if (busca.encontrado) {
            return nullopt; // Chave já existe
        }

        int chavePromovida = chave;
        int filhoDireito = -1;
        bool propagar = true;

        // Caso 3: Desempilha nós da folha até a raiz
        while (propagar && !busca.nodesAcessados.empty()) {
            auto noAtual = move(busca.nodesAcessados.back());
            busca.nodesAcessados.pop_back();
            int rrnAtual = noAtual->rrn;

            // Sem overflow: cabe no nó atual
            if (noAtual->chavesTotais() < header.ordem - 1) {
                int i = noAtual->chavesTotais() - 1;
                while (i >= 0 && noAtual->chaves[i] > chavePromovida) {
                    noAtual->chaves[i + 1] = noAtual->chaves[i];
                    noAtual->nos[i + 2] = noAtual->nos[i + 1];
                    i--;
                }
                noAtual->chaves[i + 1] = chavePromovida;
                noAtual->nos[i + 2] = filhoDireito;
                noAtual->chavesTotais()++;

                salvarNoNoDisco(rrnAtual, noAtual.get(), &acessosDisco);
                propagar = false;
                break;
            }

            // COM OVERFLOW: Nó cheio -> SPLIT (Cisão)
            vector<int> tempChaves(header.ordem);
            vector<int> tempNos(header.ordem + 1);

            tempNos[0] = noAtual->nos[0];
            int pos = 0;
            while (pos < noAtual->chavesTotais() && noAtual->chaves[pos] < chavePromovida) {
                tempChaves[pos] = noAtual->chaves[pos];
                tempNos[pos + 1] = noAtual->nos[pos + 1];
                pos++;
            }
            tempChaves[pos] = chavePromovida;
            tempNos[pos + 1] = filhoDireito;
            for (int k = pos; k < noAtual->chavesTotais(); ++k) {
                tempChaves[k + 1] = noAtual->chaves[k];
                tempNos[k + 2] = noAtual->nos[k + 1];
            }

            // Divisão exata conforme slide da aula (teto(ordem / 2))
            int meio = (header.ordem + 1) / 2 - 1;
            chavePromovida = tempChaves[meio];

            // Atualiza nó esquerdo (reaproveita noAtual)
            noAtual->chavesTotais() = meio;
            noAtual->nos[0] = tempNos[0];
            for (int k = 0; k < meio; ++k) {
                noAtual->chaves[k] = tempChaves[k];
                noAtual->nos[k + 1] = tempNos[k + 1];
            }
            for (int k = meio; k < header.ordem - 1; ++k) noAtual->chaves[k] = 0;
            for (int k = meio + 1; k < header.ordem; ++k) noAtual->nos[k] = -1;
            salvarNoNoDisco(rrnAtual, noAtual.get(), &acessosDisco);

            // Cria nó direito
            int novoRrn = obterNovoId(&acessosDisco);
            Node novoNo(novoRrn, header.ordem);

            int qtdDireita = (header.ordem - 1) - meio;
            novoNo.chavesTotais() = qtdDireita;
            novoNo.nos[0] = tempNos[meio + 1];
            for (int k = 0; k < qtdDireita; ++k) {
                novoNo.chaves[k] = tempChaves[meio + 1 + k];
                novoNo.nos[k + 1] = tempNos[meio + 1 + k + 1];
            }
            salvarNoNoDisco(novoRrn, &novoNo, &acessosDisco);

            filhoDireito = novoRrn;
        }

        // Caso 4: Split atingiu a raiz -> Nova raiz
        if (propagar) {
            int novaRaizRrn = obterNovoId(&acessosDisco);
            Node novaRaiz(novaRaizRrn, header.ordem);

            novaRaiz.chavesTotais() = 1;
            novaRaiz.chaves[0] = chavePromovida;
            novaRaiz.nos[0] = header.root;
            novaRaiz.nos[1] = filhoDireito;

            salvarNoNoDisco(novaRaizRrn, &novaRaiz, &acessosDisco);

            header.root = novaRaizRrn;
            file.seekp(0);
            file.write(reinterpret_cast<char*>(&header), sizeof(header));
            file.flush();
            acessosDisco++;
        }

        return acessosDisco;
    }

    /**
     * @brief Remove uma chave da Árvore B de forma iterativa com redistribuição e fusão.
     * @param chave Inteiro a ser removido.
     * @return optional<int> Quantidade total de acessos ao disco (leituras + escritas) se removido com sucesso; nullopt se não encontrado.
     * @pre Arquivo da árvore aberto e válido.
     * @post Chave removida mantendo as propriedades da Árvore B (mínimo de chaves por nó),
     *       e nós descartados são movidos para a pilha da lixeira em disco.
     */
    optional<int> deleteB(int chave) {
        if (header.root == -1) {
            return nullopt;
        }

        int acessosDisco = 0;

        // 1. Busca obrigatória via mSearch
        caminhoDaBusca busca = mSearch(chave);
        acessosDisco += busca.acessosDisco;

        if (!busca.encontrado) {
            return nullopt; // Chave inexistente
        }

        int idxChave = busca.posicoesInternas.back();
        auto noAlvo = busca.nodesAcessados.back().get();

        // 2. Se a chave está em NÓ INTERNO (tem filhos): troca pelo sucessor imediato
        if (noAlvo->nos[0] != -1) {
            // Desce para a subárvore direita (via nos[idxChave + 1])
            int rrnDescida = noAlvo->nos[idxChave + 1];
            busca.posicoesInternas.push_back(idxChave + 1);

            // Caminha sempre pelo filho mais à esquerda até atingir uma folha
            while (rrnDescida != -1) {
                auto noDescida = make_unique<Node>(rrnDescida, header.ordem, &file);
                acessosDisco++;
                int proximo = noDescida->nos[0];
                busca.posicoesInternas.push_back(0);
                busca.nodesAcessados.push_back(move(noDescida));
                rrnDescida = proximo;
            }

            // O topo da pilha agora é a folha que contém o sucessor
            auto noFolha = busca.nodesAcessados.back().get();
            int chaveSucessora = noFolha->chaves[0];

            // Substitui a chave no nó interno pelo sucessor e grava no disco
            noAlvo->chaves[idxChave] = chaveSucessora;
            salvarNoNoDisco(noAlvo->rrn, noAlvo, &acessosDisco);

            // A remoção física agora ocorrerá na primeira chave da folha
            idxChave = 0;
        }

        // 3. Remoção física da chave na folha (topo da pilha)
        auto noFolha = busca.nodesAcessados.back().get();
        for (int i = idxChave; i < noFolha->chavesTotais() - 1; ++i) {
            noFolha->chaves[i] = noFolha->chaves[i + 1];
        }
        noFolha->chavesTotais()--;
        salvarNoNoDisco(noFolha->rrn, noFolha, &acessosDisco);

        // 4. Rebalanceamento de Underflow (subindo iterativamente pela pilha)
        int minChaves = (header.ordem - 1) / 2;

        while (!busca.nodesAcessados.empty()) {
            auto noAtual = move(busca.nodesAcessados.back());
            busca.nodesAcessados.pop_back();
            busca.posicoesInternas.pop_back();

            // CASO RAIZ
            if (noAtual->rrn == header.root) {
                if (noAtual->chavesTotais() == 0) {
                    if (noAtual->nos[0] != -1) {
                        // A raiz esvaziou mas tem filho: o filho se torna a nova raiz
                        header.root = noAtual->nos[0];
                    } else {
                        // Árvore esvaziou completamente
                        header.root = -1;
                    }
                    moverParaLixeira(noAtual->rrn, &acessosDisco);
                    file.seekp(0);
                    file.write(reinterpret_cast<char*>(&header), sizeof(header));
                    file.flush();
                    acessosDisco++;
                }
                break; // Raiz tratada, fim do rebalanceamento
            }

            // Se o nó possui o mínimo de chaves, não há underflow
            if (noAtual->chavesTotais() >= minChaves) {
                break;
            }

            // UNDERFLOW: Necessita rebalanceamento com o pai
            if (busca.nodesAcessados.empty()) {
                break;
            }

            auto noPai = busca.nodesAcessados.back().get();

            // Localiza a posição de noAtual dentro do pai
            int posNoPai = -1;
            for (int i = 0; i <= noPai->chavesTotais(); ++i) {
                if (noPai->nos[i] == noAtual->rrn) {
                    posNoPai = i;
                    break;
                }
            }
            if (posNoPai == -1) break;

            // TENTATIVA 1: Empréstimo do Irmão Esquerdo
            if (posNoPai > 0) {
                int rrnEsq = noPai->nos[posNoPai - 1];
                Node irmaoEsq(rrnEsq, header.ordem, &file);
                acessosDisco++;

                if (irmaoEsq.chavesTotais() > minChaves) {
                    // Abre espaço na primeira posição do nó atual
                    for (int i = noAtual->chavesTotais(); i > 0; --i) {
                        noAtual->chaves[i] = noAtual->chaves[i - 1];
                    }
                    for (int i = noAtual->chavesTotais() + 1; i > 0; --i) {
                        noAtual->nos[i] = noAtual->nos[i - 1];
                    }

                    // Chave separadora do pai desce para a primeira posição
                    noAtual->chaves[0] = noPai->chaves[posNoPai - 1];
                    // Último filho do irmão esquerdo passa para o nó atual
                    noAtual->nos[0] = irmaoEsq.nos[irmaoEsq.chavesTotais()];

                    // Maior chave do irmão esquerdo sobe para o pai
                    noPai->chaves[posNoPai - 1] = irmaoEsq.chaves[irmaoEsq.chavesTotais() - 1];

                    noAtual->chavesTotais()++;
                    irmaoEsq.chavesTotais()--;

                    salvarNoNoDisco(noAtual->rrn, noAtual.get(), &acessosDisco);
                    salvarNoNoDisco(irmaoEsq.rrn, &irmaoEsq, &acessosDisco);
                    salvarNoNoDisco(noPai->rrn, noPai, &acessosDisco);
                    break; // Rebalanceamento concluído
                }
            }

            // TENTATIVA 2: Empréstimo do Irmão Direito
            if (posNoPai < noPai->chavesTotais()) {
                int rrnDir = noPai->nos[posNoPai + 1];
                Node irmaoDir(rrnDir, header.ordem, &file);
                acessosDisco++;

                if (irmaoDir.chavesTotais() > minChaves) {
                    // Chave separadora do pai desce para a última posição
                    noAtual->chaves[noAtual->chavesTotais()] = noPai->chaves[posNoPai];
                    // Primeiro filho do irmão direito passa para o nó atual
                    noAtual->nos[noAtual->chavesTotais() + 1] = irmaoDir.nos[0];

                    // Menor chave do irmão direito sobe para o pai
                    noPai->chaves[posNoPai] = irmaoDir.chaves[0];

                    // Desloca elementos do irmão direito para a esquerda
                    for (int i = 0; i < irmaoDir.chavesTotais() - 1; ++i) {
                        irmaoDir.chaves[i] = irmaoDir.chaves[i + 1];
                    }
                    for (int i = 0; i < irmaoDir.chavesTotais(); ++i) {
                        irmaoDir.nos[i] = irmaoDir.nos[i + 1];
                    }

                    noAtual->chavesTotais()++;
                    irmaoDir.chavesTotais()--;

                    salvarNoNoDisco(noAtual->rrn, noAtual.get(), &acessosDisco);
                    salvarNoNoDisco(irmaoDir.rrn, &irmaoDir, &acessosDisco);
                    salvarNoNoDisco(noPai->rrn, noPai, &acessosDisco);
                    break; // Rebalanceamento concluído
                }
            }

            // TENTATIVA 3: Fusão (Concatenação / Merge)
            // Se nenhum irmão pode emprestar, junta dois nós com minChaves + chave do pai
            if (posNoPai > 0) {
                // Fusão de noAtual dentro do irmão esquerdo
                int rrnEsq = noPai->nos[posNoPai - 1];
                Node irmaoEsq(rrnEsq, header.ordem, &file);
                acessosDisco++;

                // Chave separadora do pai desce
                irmaoEsq.chaves[irmaoEsq.chavesTotais()] = noPai->chaves[posNoPai - 1];
                irmaoEsq.chavesTotais()++;

                // Copia chaves e ponteiros de noAtual para irmaoEsq
                for (int i = 0; i < noAtual->chavesTotais(); ++i) {
                    irmaoEsq.chaves[irmaoEsq.chavesTotais()] = noAtual->chaves[i];
                    irmaoEsq.nos[irmaoEsq.chavesTotais()] = noAtual->nos[i];
                    irmaoEsq.chavesTotais()++;
                }
                irmaoEsq.nos[irmaoEsq.chavesTotais()] = noAtual->nos[noAtual->chavesTotais()];

                // Remove a chave e o ponteiro separador no pai
                for (int i = posNoPai - 1; i < noPai->chavesTotais() - 1; ++i) {
                    noPai->chaves[i] = noPai->chaves[i + 1];
                }
                for (int i = posNoPai; i < noPai->chavesTotais(); ++i) {
                    noPai->nos[i] = noPai->nos[i + 1];
                }
                noPai->chavesTotais()--;

                salvarNoNoDisco(irmaoEsq.rrn, &irmaoEsq, &acessosDisco);
                salvarNoNoDisco(noPai->rrn, noPai, &acessosDisco);

                // noAtual foi esvaziado: seu RRN é reaproveitado na lixeira
                moverParaLixeira(noAtual->rrn, &acessosDisco);
            } else {
                // Fusão do irmão direito dentro de noAtual
                int rrnDir = noPai->nos[posNoPai + 1];
                Node irmaoDir(rrnDir, header.ordem, &file);
                acessosDisco++;

                // Chave separadora do pai desce
                noAtual->chaves[noAtual->chavesTotais()] = noPai->chaves[posNoPai];
                noAtual->chavesTotais()++;

                // Copia chaves e ponteiros do irmão direito
                for (int i = 0; i < irmaoDir.chavesTotais(); ++i) {
                    noAtual->chaves[noAtual->chavesTotais()] = irmaoDir.chaves[i];
                    noAtual->nos[noAtual->chavesTotais()] = irmaoDir.nos[i];
                    noAtual->chavesTotais()++;
                }
                noAtual->nos[noAtual->chavesTotais()] = irmaoDir.nos[irmaoDir.chavesTotais()];

                // Remove a chave e o ponteiro separador no pai
                for (int i = posNoPai; i < noPai->chavesTotais() - 1; ++i) {
                    noPai->chaves[i] = noPai->chaves[i + 1];
                }
                for (int i = posNoPai + 1; i < noPai->chavesTotais(); ++i) {
                    noPai->nos[i] = noPai->nos[i + 1];
                }
                noPai->chavesTotais()--;

                salvarNoNoDisco(noAtual->rrn, noAtual.get(), &acessosDisco);
                salvarNoNoDisco(noPai->rrn, noPai, &acessosDisco);

                // irmaoDir foi esvaziado: seu RRN é reaproveitado na lixeira
                moverParaLixeira(irmaoDir.rrn, &acessosDisco);
            }
        }

        return acessosDisco;
    }

    /**
     * @brief Imprime a estrutura da Árvore B no terminal de forma hierárquica e organizada.
     * @pre Arquivo da árvore aberto e válido.
     * @post Exibe os metadados do cabeçalho e os nós organizados por níveis.
     */
    void imprimirArvore() {
        if (header.root == -1) {
            cout << "\n------------------------------------------------------------\n";
            cout << " [Árvore B] Árvore vazia (nenhum nó cadastrado).\n";
            cout << "------------------------------------------------------------\n";
            return;
        }

        cout << "\n================================ ÍNDICE (ÁRVORE B) ================================\n";
        cout << " Raiz (RRN): " << header.root 
             << " | Ordem (M): " << header.ordem 
             << " | Topo Lixeira (RRN): " << header.pilhaDaLixeira << "\n";
        cout << "----------------------------------------------------------------------------------\n";

        // Fila para percurso em largura (BFS): pares (RRN, nível)
        vector<pair<int, int>> fila;
        fila.push_back({header.root, 0});
        size_t idxFila = 0;
        int nivelAtual = -1;

        while (idxFila < fila.size()) {
            auto [rrn, nivel] = fila[idxFila++];

            if (nivel != nivelAtual) {
                nivelAtual = nivel;
                cout << "\n>>> NÍVEL " << nivelAtual << " <<<\n";
            }

            Node no(rrn, header.ordem, &file);

            cout << "  [Nó RRN " << rrn << "] "
                 << "Chaves (" << no.chavesTotais() << "/" << (header.ordem - 1) << "): [ ";
            for (int i = 0; i < no.chavesTotais(); ++i) {
                cout << no.chaves[i] << (i + 1 < no.chavesTotais() ? ", " : " ");
            }
            cout << "] | ";

            bool ehFolha = (no.nos[0] == -1);
            if (ehFolha) {
                cout << "Tipo: Folha\n";
            } else {
                cout << "Filhos (RRN): [ ";
                for (int i = 0; i <= no.chavesTotais(); ++i) {
                    cout << no.nos[i] << (i < no.chavesTotais() ? ", " : " ");
                    if (no.nos[i] != -1) {
                        fila.push_back({no.nos[i], nivel + 1});
                    }
                }
                cout << "]\n";
            }
        }
        cout << "==================================================================================\n\n";
    }

private:
    header header;
    fstream file;

    // Grava o buffer contínuo do nó no disco em chamada única
    void salvarNoNoDisco(int indice, Node* no, int* acessosDisco = nullptr) {
        int posicao = sizeof(header) + (indice * header.ordem * 2 * sizeof(int));
        file.seekp(posicao);
        file.write(reinterpret_cast<char*>(no->buffer.data()), no->buffer.size() * sizeof(int));
        file.flush();
        if (acessosDisco) (*acessosDisco)++;
    }

    // Obtém RRN da lixeira ou expande o arquivo
    int obterNovoId(int* acessosDisco = nullptr) {
        if (header.pilhaDaLixeira != -1) {
            int rrnReutilizado = header.pilhaDaLixeira;
            int posicao = sizeof(header) + (rrnReutilizado * header.ordem * 2 * sizeof(int));
            file.seekg(posicao);

            int proximoLixo;
            file.read(reinterpret_cast<char*>(&proximoLixo), sizeof(int));
            if (acessosDisco) (*acessosDisco)++;
            header.pilhaDaLixeira = proximoLixo;

            file.seekp(0);
            file.write(reinterpret_cast<char*>(&header), sizeof(header));
            file.flush();
            if (acessosDisco) (*acessosDisco)++;
            return rrnReutilizado;
        }

        file.seekg(0, ios::end);
        int tamanhoArquivo = file.tellg();
        if (tamanhoArquivo == sizeof(header)) {
            return 0;
        }
        return (tamanhoArquivo - sizeof(header)) / (header.ordem * 2 * sizeof(int));
    }

    // Encadeia nó excluído no topo da pilha da lixeira
    void moverParaLixeira(int indiceDescartado, int* acessosDisco = nullptr) {
        int posicao = sizeof(header) + (indiceDescartado * header.ordem * 2 * sizeof(int));
        file.seekp(posicao);
        file.write(reinterpret_cast<char*>(&header.pilhaDaLixeira), sizeof(int));
        if (acessosDisco) (*acessosDisco)++;

        header.pilhaDaLixeira = indiceDescartado;
        file.seekp(0);
        file.write(reinterpret_cast<char*>(&header), sizeof(header));
        file.flush();
        if (acessosDisco) (*acessosDisco)++;
    }
};
