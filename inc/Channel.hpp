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
        bool                        _isInviteOnly;
        bool                        _topicProtected;
        std::string                 _key; // password
        size_t                      _userLimit;

    // Unique identifier for the channel (if needed)
    // std::uint64_t id_;
	// static std::atomic<std::uint64_t> next_id_;
public:
    Channel(const std::string& name);
    ~Channel();

    void                addClient(Client* client);
    void                removeClient(Client* client);
    void                removeClient(const std::string& client);
    void                addOperator(Client* client);
    void                removeOperator(Client* client);
    void                broadcast(const std::string& message, Client* sender = NULL);
    bool                isClientInChannel(Client* client) const;
    bool                isClientInChannel(const std::string& client) const;
    bool                isOperator(Client* client) const;
    void                setInviteOnly(bool status);
    bool                isInviteOnly() const;
    void                setTopicProtected(bool status) { _topicProtected = status; }
    bool                isTopicProtected() const { return _topicProtected; }
    void                setKey(const std::string& key) { _key = key; }
    void                setUserLimit(size_t limit) { _userLimit = limit; }
    size_t              getUserLimit() const { return _userLimit; }
    const std::string&  getKey() const { return _key; }
    // Getters & Setters
    const std::string&  getName() const;
    const std::string&  getTopic() const;
    void                setTopic(const std::string& topic);
    std::vector<Client*>& getClients();
    std::vector<Client*>& getOperators();
};

#endif
