#include <iostream>
#include <fstream>
#include <vector>

/**
 * @brief function for checking if transmission is complete
 */
bool is_transmission_complete(std::vector<bool>* vector)
{
    std::vector<bool>::iterator it;
    for (it = vector->begin(); it != vector->end(); it++)
    {
        if (*it == false)
        {
            std::cout << "error!" << std::endl;
            return false;
        }
    }
    return true;
}

/**
 * @brief function for opening a file
 */
int create_file(const std::string &filename, std::fstream* data_stream)
{
    //open file
    data_stream->open(filename, std::ios::out | std::ios::trunc);
    if (data_stream->is_open())
    {
        std::cout << "file is open" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "error: file not open" << std::endl;
        return -1;
    }
}

/**
 * @brief main function
 */
int main(int argc, char *argv[])
{
    // create a file
    std::string filename = "/home/xiafuning/diplom-arbeit/src/nrf52840-dongle-applications/network_coding/simple_forwarding_simulation/log.dump";
    std::fstream log;
    int ret = create_file(filename, &log);
    if (ret != 0)
        return -1;

    // Seed random number generator to produce different results every time
    srand (static_cast<uint32_t> (time (0)));

    // parameter configuration
    uint32_t generation_size = 10;
    uint32_t symbol_size = 70;
    float packet_loss_rate = 0.3;
    uint32_t tx_num = 0;

    uint32_t rounds = 100;
    uint32_t average_tx_num = 0;

    std::vector<bool> rx_indicator (generation_size);
    rx_indicator.assign (generation_size, false);

    log << "[" << std::endl;

    for (float loss_rate = 0.1; loss_rate < 0.8; loss_rate+=0.2)
    {
        for (uint32_t j = 0; j < rounds; j++)
        {
            for (uint32_t i = 0; i < rx_indicator.size(); i++)
            {
                while (rx_indicator[i] == false)
                {
                    // source -> intermediate
                    tx_num++;
                    if ((rand() % 100) < (int)(loss_rate * 100))
                        continue;
                    // intermediate -> destination
                    tx_num++;
                    if ((rand() % 100) < (int)(loss_rate * 100))
                        continue;
                    rx_indicator[i] = true;
                    break;
                }
            }
            if (is_transmission_complete (&rx_indicator) == true)
            {
                //std::cout << "round " << j << " complete!" << std::endl;
                average_tx_num += tx_num;
                tx_num = 0;
                rx_indicator.assign (generation_size, false);
            }
            else
            {
                std::cout << "error in round " << j << std::endl;
                return -1;
            }
        }
        std::cout << "packet loss rate: " << loss_rate
                  << ", average number of transmission: " << (float)average_tx_num / rounds
                  << std::endl;
        log << "{\"type\": \"no_coding\", \"loss_rate\": " << loss_rate << ", \"average_tx_num\": " << (float)average_tx_num / rounds << " }," << std::endl;
    average_tx_num = 0;
    }
    log << "]" << std::endl;
    return 0;
}
