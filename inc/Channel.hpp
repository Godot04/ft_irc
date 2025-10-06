#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
// #include <atomic>

class Client;

class Channel {
private:
    std::string                 _name;
    std::string                 _topic;
    std::vector<Client*>        _clients;
    std::vector<Client*>        _operators;
    // std::uint64_t id_;
	// static std::atomic<std::uint64_t> next_id_;
public:
    Channel(const std::string& name);
    ~Channel();

    void                addClient(Client* client);
    void                removeClient(Client* client);
    void                addOperator(Client* client);
    void                removeOperator(Client* client);
    void                broadcast(const std::string& message, Client* sender = NULL);
    bool                isClientInChannel(Client* client) const;
    bool                isOperator(Client* client) const;

    // Getters & Setters
    const std::string&  getName() const;
    const std::string&  getTopic() const;
    void                setTopic(const std::string& topic);
    std::vector<Client*>& getClients();
    std::vector<Client*>& getOperators();
};

#endif
