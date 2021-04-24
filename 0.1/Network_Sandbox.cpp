#include <iostream>
#include <cstdlib>
#include <string>
#include <SFML/Network.hpp>
#include <iostream>
#include <algorithm>
#include <set>
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

class Timer
{
private:
    // Псевдонимы типов используются для удобного доступа к вложенным типам
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double, std::ratio<1> >;

    mutable std::chrono::time_point<clock_t> m_beg;

public:
    Timer() : m_beg(clock_t::now())
    {
    }

    void reset() const
    {
        m_beg = clock_t::now();
    }

    double elapsed() const
    {
        return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};

class Client {
public:
    sf::IpAddress Adress;
    unsigned short Port;
    string nickname;
    Timer timer;
    friend bool operator<(const Client& lhs, const Client& rhs) {
        return lhs.Adress < rhs.Adress or lhs.Port < rhs.Port;
    }
    friend bool operator==(const Client& lhs, const Client& rhs) {
        return lhs.Adress == rhs.Adress and lhs.Port == rhs.Port;
    }
};

int main()
{
    // Choose an arbitrary port for opening sockets
    const unsigned short port = 35296;
    const sf::IpAddress serverIP("89.179.127.216");

    if (sf::IpAddress::getPublicAddress() == serverIP
        and sf::IpAddress::getLocalAddress().toString() == "192.168.1.70") {
        cout << "YOU ARE SERVER!!!" << endl;
        sf::UdpSocket socket;
        set<Client> clients;
        thread pinging([&]() {
            while (true) {
                this_thread::sleep_for(10s);
                cout << "-----Clients: " << clients.size() << endl;
            }
            });
        thread pinging_port([&]() {
            while (true) {
                this_thread::sleep_for(25s);
                for (auto client = clients.begin(); client != clients.end(); client++) {
                    auto [adress, port, nick, timer] = *client;
                    if (timer.elapsed() > 50.0) {
                        cout << nick << " was disconected!" << endl;
                        clients.erase(client);
                    }
                    else if (adress.getPublicAddress() == serverIP.getPublicAddress()
                        and adress.getLocalAddress() == serverIP.getLocalAddress()) {
                        sf::Packet emptyString;
                        emptyString << string();
                        socket.send(emptyString, adress, port);
                    }
                }
            }
            }); pinging_port.detach();
            if (socket.bind(port) != sf::Socket::Done)
            {
                for (int i = 0; i < 7; i++) cerr << "I can't binding with port " << port << '\n';
                return 1;
            }
            string mess;
            sf::Packet pack;
            sf::IpAddress senderAdress;
            unsigned short senderPort;
            bool first_detach = false;
            while (true) {
                socket.receive(pack, senderAdress, senderPort);
                pack >> mess;
                if (mess[0] == '/') {
                    mess.erase(mess.begin());
                    if (clients.find({ senderAdress, senderPort, mess }) == clients.end())
                        first_detach = true;
                    clients.insert({ senderAdress, senderPort, mess });
                }
                else if (mess.empty()) {
                    cout << (*find_if(clients.begin(), clients.end(), [&](const Client& clnt) {
                        return clnt == Client{ senderAdress, senderPort };
                        })).nickname << " is alive!" << endl;
                    (*find_if(clients.begin(), clients.end(), [&](const Client& clnt) {
                        return clnt == Client{ senderAdress, senderPort };
                        })).timer.reset();
                        continue;
                }
                pack.clear();
                if (first_detach) {
                    mess = mess + " was join to the chat! OwO";
                    pack << mess;
                    first_detach = false;
                }
                else {
                    pack << (*find_if(clients.begin(), clients.end(),
                        [&](Client client) {return client.Adress == senderAdress and client.Port == senderPort; })).nickname
                        + ": " + mess;
                }
                cout << "From " << senderAdress.toString() << ':' << senderPort << endl
                    << "From " << (*find_if(clients.begin(), clients.end(),
                        [&](Client client) {return client.Adress == senderAdress and client.Port == senderPort; })).nickname
                    << " was received message: " << mess << endl;
                for (auto [adress, port, nick, time_gone] : clients) {
                    if ((adress.getPublicAddress() == serverIP.getPublicAddress()
                        and adress.getLocalAddress() == serverIP.getLocalAddress())
                        and (adress != senderAdress or port != senderPort)
                        ) {
                        cout << "To " << adress << ':' << port
                            << " was sending message: " << mess << endl;
                        socket.send(pack, adress, port);
                    }
                }
            }
    }
    else {
        sf::Packet pack;
        string mess;
        sf::UdpSocket socket;
        socket.bind(sf::UdpSocket::AnyPort);
        cout << "Write your name to join the chat: ";
        getline(cin, mess);
        pack << ("/" + mess);
        socket.send(pack, serverIP, port);
        cout << "Welcome to the chat!" << endl;
        thread thr([&]() {
            sf::Packet pack;
            string mess;
            sf::IpAddress senderAdress;
            unsigned short senderPort;
            while (true) {
                if (socket.receive(pack, senderAdress, senderPort) == sf::Socket::Status::Done) {
                    pack >> mess;
                    if (mess.empty()) {
                        pack.clear();
                        pack << mess;
                        socket.send(pack, serverIP, port);
                    }
                    else {
                        cout << mess << endl;
                    }
                }
            }
            for (;;) cout << "ERR!\t";
            });
        thr.detach();
        while (true) {
            getline(cin, mess);
            pack.clear();
            pack << mess;
            socket.send(pack, serverIP, port);
        }
    }

    return 0;
}
