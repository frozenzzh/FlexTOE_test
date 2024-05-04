#include <cstddef>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <chrono>
#include <fcntl.h>


const int header_size = 66;
const int print_timer_period = 1;
struct stats{
    double start_tns;
    double start_cap_tns;
    unsigned int tx_pkts;
    unsigned int tx_bytes;
};
const int capture_wait_time = 0;


void tcp_client(const char* ip, int port, int packet_size, unsigned send_rate) {
    int  payload_size = packet_size - header_size;
    double timer_tns ,prev_tns;
    struct stats stats ={
        .start_tns = 0.0,
        .start_cap_tns = 0.0,
        .tx_pkts = 0,
        .tx_bytes = 0
        
    };
    double interval_ns = 1.0 * 1e9 / send_rate;

    char data_packet[payload_size];
    memset(data_packet, 'a', payload_size);  

    printf("interval_ns: %f ns\n",interval_ns);
    // create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Error creating socket\n";
        return;
    }

    // create a sockaddr_in struct
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    // connect to the server
    if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        std::cerr << "Error connecting to the server\n";
        close(client_socket);
        return;
    }

    // calculate the interval between two packets
    
    timer_tns = 0.0;
    prev_tns = 0.0;
    
    unsigned int call_time =0;
    stats.start_tns = static_cast<double>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    
    try {
        while (true) {
            double curr_tns = static_cast<double>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) ;
            double diff_tns = (curr_tns - prev_tns);

            
            if (diff_tns >= interval_ns)
            {
                call_time += 1;   
                size_t ret = send(client_socket, data_packet, payload_size, 0);
                if (ret == -1) {
                    perror("Error sending data");
                    close(client_socket);
                    return;
                } else if (ret == 0) {
                    std::cerr << "Connection closed by peer\n";
                    close(client_socket);
                    return;
                }
                // update prev_tns
                prev_tns = curr_tns;
                
                if (curr_tns - stats.start_tns < capture_wait_time * 1e9)
                    continue;
                else if(stats.start_cap_tns == 0.0)
                    stats.start_cap_tns = curr_tns;
                
                //start print stats
                if(ret > 0)
                {
                    stats.tx_pkts += 1;
                }
                
                timer_tns += diff_tns;
                if (timer_tns >= print_timer_period * 1e9)
                {   
                    
                    double spend_time = curr_tns - stats.start_cap_tns; 
                    double send_ppns = stats.tx_pkts / spend_time;
                    double send_bpns = send_ppns * packet_size * 8.0;  
                    printf("send %.4f Gbps, %.2f Mpps\n",send_bpns * 1e9 /1024/1024/1024,send_ppns * 1e9 /1000/1000);
                    printf("call_time: %u, spend_time: %f\n",call_time,(curr_tns - stats.start_tns )/ 1e9);
                    timer_tns = 0.0;
                }
            }

            
        }
    } catch (...) {
        std::cerr << "Error during data transmission\n";
    } 

    // close the socket
    close(client_socket);
}

int main(int argc, char *argv[]) {
    // check for the right number of command line arguments
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <ip_address> <port> <packet_size> <send_pps>\n";
        return 1;
    }

    // get the server IP and port
    const char* server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    int packet_size = std::stoi(argv[3]);

    // send rate
    unsigned send_pps = std::strtoul(argv[4],NULL,10); 

    // start the client
    tcp_client(server_ip, server_port, packet_size, send_pps);

    return 0;
}