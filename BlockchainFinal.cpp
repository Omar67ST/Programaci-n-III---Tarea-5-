#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>

enum class Candidate {
    KEIKO_FUJIMORI,
    RAFAEL_LOPEZ,
    CESAR_ACUNA,
    VLADIMIR_CERRON,
    ROBERTO_PALOMINO
};

struct Vote {
    std::string hash_votante;
    Candidate candidate;
};

class Block {
    friend class BlockChain;
private:
    int id;
    std::string previous_hash;
    std::vector<Vote> list_votes;
    int nonce;
    std::string current_hash;

public:
    Block(int _id, std::string _prevHash, const std::vector<Vote>& _votes)
        : id(_id), previous_hash(std::move(_prevHash)), list_votes(_votes), nonce(0) {}

    [[nodiscard]] std::string calcularHash() const {
        std::stringstream ss;
        ss << id << previous_hash << nonce;
        for (const auto& vote : list_votes) {
            ss << vote.hash_votante << static_cast<int>(vote.candidate);
        }

        std::hash<std::string> hasher;
        size_t hash_number = hasher(ss.str());

        std::stringstream hex_ss;
        hex_ss << std::hex << std::setw(16) << std::setfill('0') << hash_number;
        return hex_ss.str();
    }

    void mineBlock(int difficulty) {
        const std::string target(difficulty, '0');
        current_hash = calcularHash(); 
        while (current_hash.substr(0, difficulty) != target) {
            nonce++;
            current_hash = calcularHash();
        }
    }

    int getNonce() const { return nonce; }
    std::string getCurrentHash() const { return current_hash; }
};

class BlockChain {
private:
    static BlockChain* instance;
    std::vector<Block> blocks;   
    int difficulty = 3;          

    explicit BlockChain() = default;

public:
    static BlockChain* getInstance() {
        if (instance == nullptr) {
            instance = new BlockChain();
        }
        return instance;
    }

    BlockChain(const BlockChain&) = delete;
    BlockChain& operator=(const BlockChain&) = delete;

    [[nodiscard]] bool isChainValid() const {
        if (blocks.empty()) return true;

        for (size_t i = 1; i < blocks.size(); ++i) {
            const Block& current = blocks[i];
            const Block& prev = blocks[i - 1];

            if (current.current_hash != current.calcularHash()) {
                return false;
            }
            if (current.previous_hash != prev.current_hash) {
                return false;
            }
        }
        return true;
    }

    void addBlock(Block block) {
        for (const auto& b : blocks) {
            if (b.id == block.id) {
                return;
            }
        }
        if (block.current_hash.empty()) {
            const std::string prevHash = blocks.empty() ? "0" : blocks.back().current_hash; 
            block.previous_hash = prevHash;  
            block.mineBlock(difficulty);     
        }
        blocks.push_back(std::move(block)); 
        
        std::cout << "[BlockChain] Bloque #" << blocks.back().id
                  << " en la cadena global. Hash: " << blocks.back().current_hash
                  << " | Nonce: " << blocks.back().nonce << "\n";
    }

    [[nodiscard]] const std::vector<Block>& getBlocks() const { return blocks; }
};

BlockChain* BlockChain::instance = nullptr;

class MesaElectoralObserver {
public:
    virtual ~MesaElectoralObserver() = default;
    virtual void update(const Block& nuevoBloque) = 0;
};

class MesaElectoral : public MesaElectoralObserver {
    std::string name;        
    BlockChain* local_chain; 

public:
    explicit MesaElectoral(const std::string& _name)
        : name(_name), local_chain(BlockChain::getInstance()) {}

    void update(const Block& nuevoBloque) override {
        if (nuevoBloque.getCurrentHash().empty()) {
            std::cout << "[" << name << "] Bloque recibido invalido, descartando.\n";
            return;
        }
        
        local_chain->addBlock(nuevoBloque); 
        
        std::cout << "[" << name << "] Bloque procesado. Cadena valida: "
                  << (local_chain->isChainValid() ? "SI" : "NO") << "\n";
    }

    const std::string& getName() const { return name; }
};

class CentroElectoralSubject {
    std::vector<MesaElectoralObserver*> observers;
public:
    void attach(MesaElectoralObserver* observer) {
        observers.push_back(observer);
    }
    
    void notificarNuevoBloque(const Block& b) {
        std::cout << "[Red Electoral] Notificando bloque a "
                  << observers.size() << " mesa(s)..\n";
                  
        for (auto* obs : observers) {
            obs->update(b); 
        }
    }
};

int main() {
    std::cout << "=== SIMULACION SISTEMA DE VOTACION BLOCKCHAIN ===\n\n";

    MesaElectoral mesa1("Mesa 1 - Lima");
    MesaElectoral mesa2("Mesa 2 - Arequipa");
    MesaElectoral mesa3("Mesa 3 - Cusco");

    CentroElectoralSubject red_electoral;
    red_electoral.attach(&mesa1); 
    red_electoral.attach(&mesa2); 
    red_electoral.attach(&mesa3); 

    std::cout << "3 mesas registradas en la red electoral.\n\n";

    std::vector<Vote> lote1 = {
        {"hash_votante_001", Candidate::KEIKO_FUJIMORI},
        {"hash_votante_002", Candidate::ROBERTO_PALOMINO},
        {"hash_votante_003", Candidate::RAFAEL_LOPEZ},
        {"hash_votante_004", Candidate::CESAR_ACUNA},
        {"hash_votante_005", Candidate::KEIKO_FUJIMORI}
    };

    std::cout << "--- Mesa 1 mina el Bloque 1 ---\n";

    Block bloque1(1, "0", lote1); 
    bloque1.mineBlock(3);         

    std::cout << "Bloque 1 minado con nonce=" << bloque1.getNonce()
              << " | Hash: " << bloque1.getCurrentHash() << "\n\n";

    std::cout << "--- Propagando bloque a la red ---\n";
    red_electoral.notificarNuevoBloque(bloque1); 

    std::cout << "\n--- Segundo lote de votos ---\n";

    std::vector<Vote> lote2 = {
        {"hash_votante_006", Candidate::KEIKO_FUJIMORI},
        {"hash_votante_007", Candidate::ROBERTO_PALOMINO}
    };

    Block bloque2(2, bloque1.getCurrentHash(), lote2); 
    bloque2.mineBlock(3);

    std::cout << "Bloque 2 minado con nonce=" << bloque2.getNonce()
              << " | Hash: " << bloque2.getCurrentHash() << "\n\n";

    std::cout << "--- Propagando bloque 2 a la red ---\n";
    red_electoral.notificarNuevoBloque(bloque2);

    std::cout << "\n=== FIN DE SIMULACION ===\n";

    /* Trampa
    std::cout << "\n--- Intentando introducir un bloque adulterado (Fraude) ---\n";

    std::vector<Vote> lote_trampa = {
        {"hash_votante_008", Candidate::VLADIMIR_CERRON}
    };

    Block bloque_trampa(3, "HASH_FALSO_123X", lote_trampa); 
    bloque_trampa.mineBlock(3);

    std::cout << "Bloque trampa minado con Hash: " << bloque_trampa.getCurrentHash() << "\n\n";

    std::cout << "--- Propagando bloque trampa a la red ---\n";
    red_electoral.notificarNuevoBloque(bloque_trampa);
*/
    std::cout << "\n";
    return 0;
}