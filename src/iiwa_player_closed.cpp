//Esse código aqui foi Gemini que fez total
//Revisa isso daqui depois pra extirpar da Terra todas as heresias codificadas nesse código
//Em nome de Linus Torvalds, a main()

#include <fstream> //Malha fechada com 2 threads que gera um csv
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

class IiwaPlayer : public rclcpp::Node {
public:
    IiwaPlayer() : Node("iiwa_player"), h_(0.01), step_(0), started_(false) {
        for (int i = 1; i <= 7; ++i) {
            std::string topic = "/model/iiwa14/joint/iiwa_joint_" + std::to_string(i) + "/cmd_force";
            pubs_.push_back(this->create_publisher<std_msgs::msg::Float64>(topic, 10));
        }

        x_atual_.assign(14, 0.0);
        carregar_trajetoria();

        // Inicializa o arquivo de gravação do Gazebo
        log_real_.open("trajetoria_real_iiwa.csv");
        if (log_real_.is_open()) {
            log_real_ << "x1,x2,x3,x4,x5,x6,x7\n";
        }

        sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "/world/zero_gravity_world/model/iiwa14/joint_state", 10,
            std::bind(&IiwaPlayer::joint_state_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "Aguardando Gazebo (Pronto para gravar e controlar)...");
    }

private:
    void carregar_trajetoria() {
        std::ifstream arquivo("trajetoria_iiwa.csv");
        if (!arquivo.is_open()) return;

        std::string linha, valor;
        std::getline(arquivo, linha); 

        while (std::getline(arquivo, linha)) {
            std::vector<double> dados;
            std::stringstream ss(linha);
            while (std::getline(ss, valor, ',')) dados.push_back(std::stod(valor));

            if (dados.size() >= 119) {
                x_star_.push_back(std::vector<double>(dados.begin(), dados.begin() + 14));
                u_star_.push_back(std::vector<double>(dados.begin() + 14, dados.begin() + 21));
                K_.push_back(std::vector<double>(dados.begin() + 21, dados.begin() + 119));
            }
        }
    }

    void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg) {
        rclcpp::Time current_time = msg->header.stamp;

        if (!started_) {
            start_time_ = current_time;
            started_ = true;
            RCLCPP_INFO(this->get_logger(), "Fisica detectada! Gravando e controlando...");
        }

        for (size_t i = 0; i < msg->name.size(); ++i) {
            std::string nome = msg->name[i];
            int indice_junta = std::stoi(nome.substr(nome.find_last_of('_') + 1)) - 1;
            
            if (indice_junta >= 0 && indice_junta < 7) {
                x_atual_[indice_junta] = msg->position[i];
                if (!msg->velocity.empty() && msg->velocity.size() > i) {
                    x_atual_[indice_junta + 7] = msg->velocity[i];
                }
            }
        }

        double elapsed_time = (current_time - start_time_).seconds();
        size_t expected_step = static_cast<size_t>(elapsed_time / h_);

        if (expected_step > step_) {
            step_ = expected_step;

            // Grava a posição atual no CSV a cada passo do controle
            if (log_real_.is_open()) {
                log_real_ << x_atual_[0] << "," << x_atual_[1] << "," << x_atual_[2] << ","
                          << x_atual_[3] << "," << x_atual_[4] << "," << x_atual_[5] << "," << x_atual_[6] << "\n";
            }

            if (step_ >= x_star_.size() - 1) {
                RCLCPP_INFO(this->get_logger(), "Destino alcancado! Salvando log e encerrando...");
                
                for (int i = 0; i < 7; ++i) {
                    std_msgs::msg::Float64 torque_msg;
                    torque_msg.data = 0.0;
                    pubs_[i]->publish(torque_msg);
                }

                if (log_real_.is_open()) log_real_.close();
                rclcpp::shutdown();
                return;
            }

            for (int i = 0; i < 7; ++i) {
                double correcao = 0.0;
                for (int j = 0; j < 14; ++j) {
                    double delta_x = x_atual_[j] - x_star_[step_][j];
                    correcao += K_[step_][i * 14 + j] * delta_x;
                }
                
                double u_cmd = u_star_[step_][i] - correcao;
                u_cmd = std::clamp(u_cmd, -60.0, 60.0);

                std_msgs::msg::Float64 torque_msg;
                torque_msg.data = u_cmd;
                pubs_[i]->publish(torque_msg);
            }
        }
    }

    std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> pubs_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr sub_;
    
    std::vector<std::vector<double>> x_star_, u_star_, K_;
    std::vector<double> x_atual_;
    std::ofstream log_real_;
    
    double h_;
    size_t step_;
    bool started_;
    rclcpp::Time start_time_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<IiwaPlayer>());
    rclcpp::shutdown();
    return 0;
}

/*#include <fstream> //Malha fechada com 2 threads
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

class IiwaPlayer : public rclcpp::Node {              //Malha fechada com 2 threads
public:
    IiwaPlayer() : Node("iiwa_player"), h_(0.01), step_(0), started_(false) {
        for (int i = 1; i <= 7; ++i) {
            std::string topic = "/model/iiwa14/joint/iiwa_joint_" + std::to_string(i) + "/cmd_force";
            pubs_.push_back(this->create_publisher<std_msgs::msg::Float64>(topic, 10));
        }

        // Inicializa a memória do sensor com zeros
        x_atual_.assign(14, 0.0);

        carregar_trajetoria();

        sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "/world/zero_gravity_world/model/iiwa14/joint_state", 10,
            std::bind(&IiwaPlayer::joint_state_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "Aguardando Gazebo (ZOH de 100Hz Desacoplado)...");
    }

private:
    void carregar_trajetoria() {
        std::ifstream arquivo("trajetoria_iiwa.csv");
        if (!arquivo.is_open()) return;

        std::string linha, valor;
        std::getline(arquivo, linha); 

        while (std::getline(arquivo, linha)) {
            std::vector<double> dados;
            std::stringstream ss(linha);
            while (std::getline(ss, valor, ',')) dados.push_back(std::stod(valor));

            if (dados.size() >= 119) {
                x_star_.push_back(std::vector<double>(dados.begin(), dados.begin() + 14));
                u_star_.push_back(std::vector<double>(dados.begin() + 14, dados.begin() + 21));
                K_.push_back(std::vector<double>(dados.begin() + 21, dados.begin() + 119));
            }
        }
        RCLCPP_INFO(this->get_logger(), "Trajetoria carregada. Passos: %zu", x_star_.size());
    }

    void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg) {
        rclcpp::Time current_time = msg->header.stamp;

        if (!started_) {
            start_time_ = current_time;
            started_ = true;
            RCLCPP_INFO(this->get_logger(), "Fisica detectada! Iniciando laco de controle.");
        }

        // ====================================================================
        // "THREAD" 1: LEITURA DO SENSOR (Assíncrona e rápida)
        // Atualiza a memória x_atual_ independente da frequência do Gazebo
        // ====================================================================
        for (size_t i = 0; i < msg->name.size(); ++i) {
            std::string nome = msg->name[i];
            int indice_junta = std::stoi(nome.substr(nome.find_last_of('_') + 1)) - 1;
            
            if (indice_junta >= 0 && indice_junta < 7) {
                x_atual_[indice_junta] = msg->position[i];
                if (!msg->velocity.empty() && msg->velocity.size() > i) {
                    x_atual_[indice_junta + 7] = msg->velocity[i];
                }
            }
        }

        // ====================================================================
        // "THREAD" 2: GATILHO DE CONTROLE (Zero-Order Hold cravado em 100Hz)
        // ====================================================================
        double elapsed_time = (current_time - start_time_).seconds();
        
        // Calcula matematicamente em qual passo de 0.01s o robô DEVERIA estar agora.
        size_t expected_step = static_cast<size_t>(elapsed_time / h_);

        // Se o expected_step for maior que o step_ atual, significa que a barreira
        // de 0.01s foi cruzada, ou que pacotes foram perdidos e precisamos pular 
        // direto para a linha correta do CSV.
        if (expected_step > step_) {
            
            step_ = expected_step; // Sincroniza o índice da matriz

            // Verifica se a trajetória acabou. Se sim, desliga o nó automaticamente.
            if (step_ >= x_star_.size() - 1) {
                RCLCPP_INFO(this->get_logger(), "Destino alcancado com sucesso! Encerrando...");

		// Desliga os motores: envia 0.0 Nm para evitar que o Gazebo trave a última força
                for (int i = 0; i < 7; ++i) {
                    std_msgs::msg::Float64 torque_msg;
                    torque_msg.data = 0.0;
                    pubs_[i]->publish(torque_msg);
                }

                rclcpp::shutdown();
                return;
            }

            // Lei de Controle TVLQR baseada apenas no estado mais fresco (ZOH)
            for (int i = 0; i < 7; ++i) {
                double correcao = 0.0;
                for (int j = 0; j < 14; ++j) {
                    double delta_x = x_atual_[j] - x_star_[step_][j];
                    correcao += K_[step_][i * 14 + j] * delta_x;
                }
                
                double u_cmd = u_star_[step_][i] - correcao;

                // Clipe de segurança física (Evita que erros matemáticos quebrem o robô)
                u_cmd = std::clamp(u_cmd, -60.0, 60.0);

                std_msgs::msg::Float64 torque_msg;
                torque_msg.data = u_cmd;
                pubs_[i]->publish(torque_msg);
            }
        }
    }

    std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> pubs_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr sub_;
    
    std::vector<std::vector<double>> x_star_, u_star_, K_;
    std::vector<double> x_atual_;
    
    double h_;
    size_t step_;
    bool started_;
    rclcpp::Time start_time_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<IiwaPlayer>());
    rclcpp::shutdown();
    return 0;
}*/

/*#include <fstream>      //Esse é um malha fechada que assume que o update rate do simulador é 1000Hz
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

class IiwaPlayer : public rclcpp::Node {
public:
    IiwaPlayer() : Node("iiwa_player"), h_(0.01), started_(false) {
        for (int i = 1; i <= 7; ++i) {
            std::string topic = "/model/iiwa14/joint/iiwa_joint_" + std::to_string(i) + "/cmd_force";
            pubs_.push_back(this->create_publisher<std_msgs::msg::Float64>(topic, 10));
        }

        carregar_trajetoria();

        sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "/world/zero_gravity_world/model/iiwa14/joint_state", 10,
            std::bind(&IiwaPlayer::joint_state_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "Aguardando Gazebo (ZOH ativado)...");
    }

private:
    void carregar_trajetoria() {
        std::ifstream arquivo("trajetoria_iiwa.csv");
        if (!arquivo.is_open()) return;

        std::string linha, valor;
        std::getline(arquivo, linha); 

        while (std::getline(arquivo, linha)) {
            std::vector<double> dados;
            std::stringstream ss(linha);
            while (std::getline(ss, valor, ',')) dados.push_back(std::stod(valor));

            if (dados.size() >= 119) {
                x_star_.push_back(std::vector<double>(dados.begin(), dados.begin() + 14));
                u_star_.push_back(std::vector<double>(dados.begin() + 14, dados.begin() + 21));
                K_.push_back(std::vector<double>(dados.begin() + 21, dados.begin() + 119));
            }
        }
        RCLCPP_INFO(this->get_logger(), "Trajetoria TVLQR carregada. Passos: %zu", x_star_.size());
    }

    void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg) {
        rclcpp::Time current_time = msg->header.stamp;

        // Inicia o cronômetro na primeira mensagem válida recebida
        if (!started_) {
            start_time_ = current_time;
            started_ = true;
            RCLCPP_INFO(this->get_logger(), "Fisica detectada! Aplicando tracking TVLQR.");
        }

        // Calcula o tempo decorrido na simulação
        double elapsed_time = (current_time - start_time_).seconds();
        
        // ZOH: Descobre o índice exato travando a fração do tempo no passo h
        size_t step = static_cast<size_t>(elapsed_time / h_);

        if (step >= x_star_.size() - 1) { // -1 pois a última linha não tem K
            RCLCPP_INFO_ONCE(this->get_logger(), "Destino alcancado!");
	    rclcpp::shutdown();
            return;
        }

        std::vector<double> x_atual(14, 0.0);
        
        // Mapeia o estado real recebido
        for (size_t i = 0; i < msg->name.size(); ++i) {
            std::string nome = msg->name[i];
            int indice_junta = std::stoi(nome.substr(nome.find_last_of('_') + 1)) - 1;
            
            if (indice_junta >= 0 && indice_junta < 7) {
                x_atual[indice_junta] = msg->position[i];
                if (!msg->velocity.empty() && msg->velocity.size() > i) {
                    x_atual[indice_junta + 7] = msg->velocity[i];
                }
            }
        }

        // Lei de Controle TVLQR
        for (int i = 0; i < 7; ++i) {
            double correcao = 0.0;
            for (int j = 0; j < 14; ++j) {
                double delta_x = x_atual[j] - x_star_[step][j];
                correcao += K_[step][i * 14 + j] * delta_x;
            }
            
            double u_cmd = u_star_[step][i] - correcao;

            // Trava física contra torques perigosos
            u_cmd = std::clamp(u_cmd, -60.0, 60.0);

            std_msgs::msg::Float64 torque_msg;
            torque_msg.data = u_cmd;
            pubs_[i]->publish(torque_msg);
        }
    }

    std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> pubs_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr sub_;
    
    std::vector<std::vector<double>> x_star_, u_star_, K_;
    double h_;
    bool started_;
    rclcpp::Time start_time_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<IiwaPlayer>());
    rclcpp::shutdown();
    return 0;
}*/

/* #include <fstream>  //Esse é o original de malha fechada, que estoura
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

class IiwaPlayer : public rclcpp::Node {
public:
    IiwaPlayer() : Node("iiwa_player"), step_(0) {
        // Inicializa publishers para cada junta
        for (int i = 1; i <= 7; ++i) {
            std::string topic = "/model/iiwa14/joint/iiwa_joint_" + std::to_string(i) + "/cmd_force";
            pubs_.push_back(this->create_publisher<std_msgs::msg::Float64>(topic, 10));
        }

        carregar_trajetoria();

        // Inscreve-se no tópico de estados do Gazebo
        sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "/world/zero_gravity_world/model/iiwa14/joint_state", 10,
            std::bind(&IiwaPlayer::joint_state_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "Aguardando Gazebo publicar joint_states...");
    }

private:
    void carregar_trajetoria() {
        std::ifstream arquivo("trajetoria_iiwa.csv");
        if (!arquivo.is_open()) {
            RCLCPP_ERROR(this->get_logger(), "Falha ao abrir o CSV!");
            return;
        }

        std::string linha, valor;
        std::getline(arquivo, linha); // Pula cabeçalho

        while (std::getline(arquivo, linha)) {
            std::vector<double> dados;
            std::stringstream ss(linha);
            while (std::getline(ss, valor, ',')) {
                dados.push_back(std::stod(valor));
            }

            // O CSV possui: 14 valores de x* | 7 valores de u* | 98 valores de K
            if (dados.size() >= 119) {
                x_star_.push_back(std::vector<double>(dados.begin(), dados.begin() + 14));
                u_star_.push_back(std::vector<double>(dados.begin() + 14, dados.begin() + 21));
                K_.push_back(std::vector<double>(dados.begin() + 21, dados.begin() + 119));
            }
        }
        RCLCPP_INFO(this->get_logger(), "Trajetoria TVLQR carregada: %zu passos.", x_star_.size());
    }

    void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg) {
        if (step_ >= x_star_.size() - 1) { // -1 porque a última linha não tem K e u
            RCLCPP_INFO_ONCE(this->get_logger(), "Destino alcancado!");
            return;
        }

        std::vector<double> x_atual(14, 0.0);
        
        // Garante que pegamos as juntas na ordem correta (1 a 7)
        for (size_t i = 0; i < msg->name.size(); ++i) {
            std::string nome = msg->name[i];
            int indice_junta = std::stoi(nome.substr(nome.find_last_of('_') + 1)) - 1;
            
            if (indice_junta >= 0 && indice_junta < 7) {
                x_atual[indice_junta] = msg->position[i];
                if (!msg->velocity.empty()) {
                    x_atual[indice_junta + 7] = msg->velocity[i];
                }
            }
        }

        std::vector<double> u_cmd(7, 0.0);
        
        // Aplica u_cmd = u* - K(x_atual - x*)
        for (int i = 0; i < 7; ++i) {
            double correcao = 0.0;
            for (int j = 0; j < 14; ++j) {
                double delta_x = x_atual[j] - x_star_[step_][j];
                correcao += K_[step_][i * 14 + j] * delta_x;
            }
            u_cmd[i] = u_star_[step_][i] + correcao; //Não sabemos se é + ou -

            // Publica o torque
            std_msgs::msg::Float64 torque_msg;
            torque_msg.data = u_cmd[i];
            pubs_[i]->publish(torque_msg);
        }

        step_++;
    }

    std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> pubs_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr sub_;
    
    std::vector<std::vector<double>> x_star_;
    std::vector<std::vector<double>> u_star_;
    std::vector<std::vector<double>> K_;
    size_t step_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<IiwaPlayer>());
    rclcpp::shutdown();
    return 0;
}*/
