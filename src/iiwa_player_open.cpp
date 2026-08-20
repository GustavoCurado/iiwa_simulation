//Esse código aqui foi Gemini que fez total
//Revisa isso daqui depois pra extirpar da Terra todas as heresias codificadas nesse código
//Em nome de Linus Torvalds, a main()

#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

using namespace std::chrono_literals;

class IiwaPlayer : public rclcpp::Node {
public:
    IiwaPlayer() : Node("iiwa_player"), step_(0), gazebo_ready_(false) {
        // Cria os 7 publishers mapeados para os tópicos do Gazebo
        for (int i = 1; i <= 7; ++i) {
            std::string topic = "/model/iiwa14/joint/iiwa_joint_" + std::to_string(i) + "/cmd_force";
            pubs_.push_back(this->create_publisher<std_msgs::msg::Float64>(topic, 10));
        }

        // Lê o CSV
        std::ifstream arquivo("trajetoria_iiwa.csv");
        std::string linha, valor;
        std::getline(arquivo, linha); // Pula o cabeçalho

        while (std::getline(arquivo, linha)) {
            std::vector<double> linha_dados;
            std::stringstream ss(linha);
            while (std::getline(ss, valor, ',')) {
                linha_dados.push_back(std::stod(valor));
            }
            // Os 7 torques são as últimas 7 colunas (índices 14 a 20)
            std::vector<double> torques(linha_dados.begin() + 14, linha_dados.begin() + 21);
            torques_.push_back(torques);
        }

        RCLCPP_INFO(this->get_logger(), "Trajetoria carregada: %zu passos.", torques_.size());

        // Inicializa a memória do sensor e o arquivo de log
        x_atual_.assign(14, 0.0);
        log_real_.open("trajetoria_real_iiwa.csv");
        if (log_real_.is_open()) {
            log_real_ << "x1,x2,x3,x4,x5,x6,x7\n";
        }

        // Subscriber APENAS para monitorar e gravar a realidade (não afeta os torques)
        sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "/world/zero_gravity_world/model/iiwa14/joint_state", 10,
            std::bind(&IiwaPlayer::joint_state_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "Aguardando Gazebo para iniciar playback de Malha Aberta...");

        // Timer disparando a cada 0.01s (100 Hz) para coincidir com o seu h = 0.01
        timer_ = this->create_wall_timer(10ms, std::bind(&IiwaPlayer::timer_callback, this));
    }

private:
    // Ouve a ponte do Gazebo e salva a posição atual na memória
    void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg) {
        gazebo_ready_ = true; // Libera o timer para começar
        
        for (size_t i = 0; i < msg->name.size(); ++i) {
            std::string nome = msg->name[i];
            int indice_junta = std::stoi(nome.substr(nome.find_last_of('_') + 1)) - 1;
            
            if (indice_junta >= 0 && indice_junta < 7) {
                x_atual_[indice_junta] = msg->position[i];
            }
        }
    }

    void timer_callback() {
        // Trava o relógio do WallTimer até a física do Gazebo começar
        if (!gazebo_ready_) return; 

        if (step_ < torques_.size()) {
            
            // 1. Grava a posição real atual no log
            if (log_real_.is_open()) {
                log_real_ << x_atual_[0] << "," << x_atual_[1] << "," << x_atual_[2] << ","
                          << x_atual_[3] << "," << x_atual_[4] << "," << x_atual_[5] << "," << x_atual_[6] << "\n";
            }

            // 2. Dispara os torques cegos (Malha Aberta)
            for (int i = 0; i < 7; ++i) {
                std_msgs::msg::Float64 msg;
                msg.data = torques_[step_][i];
                pubs_[i]->publish(msg);
            }
            step_++;
            
        } else {
            RCLCPP_INFO(this->get_logger(), "Trajetoria finalizada! Cortando forcas...");
            
            // Desliga os motores (0.0 Nm) para anular o Latch do Gazebo
            for (int i = 0; i < 7; ++i) {
                std_msgs::msg::Float64 msg;
                msg.data = 0.0;
                pubs_[i]->publish(msg);
            }

            // Fecha o arquivo e desliga o nó
            if (log_real_.is_open()) log_real_.close();
            rclcpp::shutdown();
        }
    }

    std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> pubs_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr sub_;
    std::vector<std::vector<double>> torques_;
    std::vector<double> x_atual_;
    std::ofstream log_real_;
    rclcpp::TimerBase::SharedPtr timer_;
    
    size_t step_;
    bool gazebo_ready_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<IiwaPlayer>());
    return 0;
}
