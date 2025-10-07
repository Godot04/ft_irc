#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

class Client {
private:
    int                 _fd;
    std::string         _nickname;
    std::string         _username;
    std::string         _realname;
    std::string         _hostname;
    bool                _authenticated;
    bool                _registered;
    bool                _isCAPNegotiation;
    std::vector<std::string> _channels;
    std::string         _buffer;

public:
    Client(int fd);
    ~Client();

    void                addToBuffer(const std::string& msg);
    const std::string&  getBuffer() const;
    bool                hasCompleteMessage() const;
    std::string         getNextMessage();
    void                clearBuffer();
	void				printBuffer() const;
    void                sendMessage(const std::string& msg) const;
    void				addChannel(const std::string& channel);
    // Getters & Setters
    int                 getFd() const;
    const std::string&  getNickname() const;
    void                setNickname(const std::string& nickname);
    const std::string&  getUsername() const;
    void                setUsername(const std::string& username);
    const std::string&  getRealname() const;
    void                setRealname(const std::string& realname);
    const std::string&  getHostname() const;
    void                setHostname(const std::string& hostname);
    bool                isAuthenticated() const;
	bool				isNicknameSet() const { return !_nickname.empty(); }
    bool                isUsernameSet() const { return !_username.empty(); }
    void                setAuthenticated(bool authenticated);
    bool                isRegistered() const;
    void                setRegistered(bool registered);
    std::vector<std::string>& getChannels();
	void				printClientInfo() const;
    bool				isCAPNegotiation() const;
    void                setCAPNegotiation(bool status);

};

#endif
