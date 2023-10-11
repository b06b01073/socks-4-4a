#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>

using namespace std;
using boost::asio::ip::tcp;

// struct envVars{
//     string request_method;
//     string request_uri;
//     string query_string;
//     string server_protocol;
//     string http_host;
//     string server_addr;
//     string server_port;
//     string remote_addr;
//     string remote_port;
// };


// class session
//   : public std::enable_shared_from_this<session>
// {
// public:
//     session(tcp::socket socket)
//         : socket_(std::move(socket)) {
//     }

//     void start() {
//         do_read();
//     }

// private:
//     void do_read() {
//         auto self(shared_from_this());
//         socket_.async_read_some(boost::asio::buffer(data_, max_length),
//             [this, self](boost::system::error_code ec, std::size_t length) {
//             if (!ec) {              
//                 string msg = data_;
//                 // A request example
//                 // GET /panel.cgi?queryString=hello HTTP/1.1   => REQUEST_METHOD and REQUEST_URI(and query string)
//                 // Host: 140.113.235.221:7000   => HTTP_HOST 
//                 // User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:107.0) Gecko/20100101 Firefox/107.0
//                 // Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8
//                 // Accept-Language: zh-TW,zh;q=0.8,en-US;q=0.5,en;q=0.3
//                 // Accept-Encoding: gzip, deflate
//                 // DNT: 1
//                 // Connection: keep-alive
//                 // Upgrade-Insecure-Requests: 1      
//                 vector<string> e;
//                 boost::split(e, msg, boost::is_any_of(" \n"), boost::token_compress_on);

//                 envVars envs = initVars(e);    

//                 processRequest(envs);
//             }
//             });
//     }

//     envVars initVars(vector<string>& e) {

//         envVars envs;
//         envs.request_method = e[0];
//         envs.server_protocol = e[2];
//         envs.http_host = e[4];

//         string uri = e[1];
//         int delimIndex = uri.find_first_of('?');

//         envs.request_uri = uri;
        
//         string requestedPath = uri.substr(0, delimIndex);

//         // no query string or ? is at the end of requestedPath
//         if(delimIndex == string::npos || delimIndex == requestedPath.size() - 1) {
//             envs.query_string = "";
//         } else{
//             // +1 to skip the ?
//             envs.query_string = uri.substr(delimIndex + 1);
//         }


//         // https://stackoverflow.com/questions/601763/how-to-get-ip-address-of-boostasioiptcpsocket
//         // port is uint type
//         envs.server_addr = socket_.local_endpoint().address().to_string();
//         envs.server_port = to_string(socket_.local_endpoint().port());
//         envs.remote_addr = socket_.remote_endpoint().address().to_string();
//         envs.remote_port = to_string(socket_.remote_endpoint().port());
//         return envs;
//     }

//     void processRequest(envVars envs) {
//         auto self(shared_from_this());

//         strcpy(data_, "HTTP/1.0 200 OK\r\n");
//         boost::asio::async_write(socket_, boost::asio::buffer(data_, strlen(data_)),
//             [this, self, envs](boost::system::error_code ec, std::size_t /*length*/) {
//             if (!ec) {
//                 // for a child to process the request
//                 int pid = fork();
//                 if(pid == 0) {
//                     initEnvs(envs);
//                     // This function may be used to obtain the underlying representation of the socket. This is intended to allow access to native socket functionality that is not otherwise provided. 
//                     // redirect the stdio to the socket fd
//                     dup2(socket_.native_handle(), STDIN_FILENO);
//                     dup2(socket_.native_handle(), STDOUT_FILENO);

//                     // no need the use the fd anymore
//                     close(socket_.native_handle());

//                     string cgiPath;
//                     if(envs.request_uri.find('?') == string::npos) {
//                         cgiPath = "." + envs.request_uri;
//                     } else {
//                         cgiPath = "." + envs.request_uri.substr(0, envs.request_uri.find('?'));
//                     }

//                     char **args = new char*[2];
//                     args[0] = new char[cgiPath.size() + 1];
//                     strcpy(args[0], cgiPath.c_str());

//                     args[1] = nullptr;

//                     cout << cgiPath << endl;
//                     if(execv(cgiPath.c_str(), args) == -1) {
//                         cout << strerror(errno) << endl;
//                     }

//                     exit(0);
//                 } else {
//                     waitpid(pid, nullptr, 0);
//                     socket_.close();
//                 }
//             }
//             });
//     }

//     void initEnvs(envVars envs) {
//         // The following environment variables are required to set:
//         // (a) REQUEST METHOD
//         // (b) REQUEST URI
//         // (c) QUERY STRING
//         // (d) SERVER PROTOCOL
//         // (e) HTTP HOST
//         // (f) SERVER ADDR
//         // (g) SERVER PORT
//         // (h) REMOTE ADDR
//         // (i) REMOTE PORT

//         setenv("REQUEST_METHOD", envs.request_method.c_str(), 1);
//         setenv("REQUEST_URI", envs.request_uri.c_str(), 1);
//         setenv("QUERY_STRING", envs.query_string.c_str(), 1);
//         setenv("SERVER_PROTOCOL", envs.server_protocol.c_str(), 1);
//         setenv("HTTP_HOST", envs.http_host.c_str(), 1);
//         setenv("SERVER_ADDR", envs.server_addr.c_str(), 1);
//         setenv("SERVER_PORT", envs.server_port.c_str(), 1);
//         setenv("REMOTE_ADDR", envs.remote_addr.c_str(), 1);
//         setenv("REMOTE_PORT", envs.remote_port.c_str(), 1);
//     }

//     enum { max_length = 1024 };
//     char data_[max_length];
//     tcp::socket socket_;
// };

const int replyLen = 8;
unsigned char okReply[replyLen] = {0, 90, 0, 0, 0, 0, 0, 0};
unsigned char badReply[replyLen] = {0, 91, 0, 0, 0, 0, 0, 0};

struct Request {
    int VN;
    int CD;
    string DSTPORT;
    string DSTIP;
    string domain;
};

class ConnectRequest: public enable_shared_from_this<ConnectRequest> {
public:

    /*
        the traffic of data goes like this

                readFromServer                          writeToClient
        server       ->           bufferForClient             ->      client

                readFromClient                          writeToServer
        client       ->           bufferForServer             ->      server

    */

    ConnectRequest(tcp::socket client, tcp::socket server, tcp::endpoint endpoint)
        : client(std::move(client)), server(std::move(server)), endpoint{endpoint} {
    }

    void start() {
        buildConnection();
    }

    void buildConnection() {
        auto self(shared_from_this());
        server.async_connect(endpoint, [this, self](const boost::system::error_code& ec){
                if(!ec) {
                    sendSocksReply();
                }
                else{
                    cout << "buildConnection error" << endl;
                }
            }
        );
    }

    void clearClientBuffer() {
        memset(bufferForClient, 0, max_length);
    }


    void sendSocksReply() {
        // cout << "sending reply..." << endl;
        auto self(shared_from_this());
        clearClientBuffer();
        memcpy(bufferForClient, okReply, replyLen + 1);

        // this part act as server
        boost::asio::async_write(client, boost::asio::buffer(bufferForClient, replyLen), [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                // cout << "reply sent!" << endl;
                clearClientBuffer();
                readFromClient();
                readFromServer();
            }
        }
        );

    }

    // proxy read from client and write to server
    void readFromClient() {
        auto self(shared_from_this());
        client.async_read_some(boost::asio::buffer(bufferForServer, max_length), [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                writeToServer(length);
            }
        }
    );
    }

    // proxy read from server and write to client
    void readFromServer() {
        // cout << "reading from server..." << endl;
        auto self(shared_from_this());
        server.async_read_some(boost::asio::buffer(bufferForClient, max_length), [this, self](boost::system::error_code ec, size_t length) {
            // cout << "readFromServer: " << bufferForClient << endl; 
            if (!ec) {
                writeToClient(length);
            }
        }
    );
    }


    // proxy write to client and start the next read from server
    void writeToClient(int length) {
        auto self(shared_from_this());
        boost::asio::async_write(client, boost::asio::buffer(bufferForClient, length), [this, self](boost::system::error_code ec, size_t length) {
            // cout << "writeToClient: " << bufferForClient << endl; 
            if (!ec) {
                readFromServer();
            }
        }
    );
    }

    // proxy write to server and start the next read from server
    void writeToServer(int length) {
        auto self(shared_from_this());
        boost::asio::async_write(server, boost::asio::buffer(bufferForServer, length), [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                // cout << "writeToServer: " << bufferForClient << endl; 
                readFromClient();
            }
        }
    );
    }


private:
    enum { max_length = 1000000 };
    unsigned char bufferForClient[max_length];
    unsigned char bufferForServer[max_length];
    tcp::socket client;
    tcp::socket server;
    tcp::endpoint endpoint;
};

class SocksProxy: public enable_shared_from_this<SocksProxy>{
public: 
    SocksProxy(tcp::socket socket, boost::asio::io_context& io_context)
        : socket_(std::move(socket)), io_context(io_context) {
            // cout << "Initializing Proxy..." << endl;
    }
    void start() {
        // cout << "Starting..." << endl; 
        parseRequestPacket();
    } 
private:

    // DENY THE CONNECTION HERE
    void rejectReply() {
        auto self(shared_from_this());
        // for(int i = 0; i < replyLen; i++)
        //     cout << to_string(badReply[i]) << ' ';
        // cout << endl;

        memcpy(data_, badReply, replyLen + 1);
        boost::asio::async_write(socket_, boost::asio::buffer(data_, replyLen), [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                // cout << "reply sent!" << endl;
            }
        }
        );
    }

    void parseRequestPacket() {
        auto self(shared_from_this());
        // cout << "Parsing request..." << endl;

        //                 +----+----+----+----+----+----+----+----+----+----+....+----+
        //                 | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
        //                 +----+----+----+----+----+----+----+----+----+----+....+----+
        // # of bytes:	     1    1      2              4           variable       1

        // e.g.
        // DSTIP=140.113.1.2
        // DSPPORT=1234 (hint: 1234 = 4*256 + 210 = DSTPORT[0]*256 + DSTPORT[1])
        // USERID=MOZ
        //          +----+----+----+----+----+----+----+----+----+----+----+----+
        //          | 4 | 1 | 4 |210 |140 |113 | 1 | 2 | M | O | Z | |
        //          +----+----+----+----+----+----+----+----+----+----+----+----+
        // bytes:     1 |   1 |  2 | 4 | variable | 1
        


        socket_.async_read_some(boost::asio::buffer(data_, max_length), [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {  
                Request request;   
                request.VN = data_[0];
                request.CD = data_[1];
                request.DSTPORT = to_string(int(data_[2]) * 256 + int(data_[3])); // the query requires it to be a string
                request.DSTIP = parseDSTIP();
                // string domain = get_domain(length);
                request.domain = parseDomain(length);

                // bind case
                bool useDomain = (request.domain != "");

                


                if(request.CD == 1) {
                    string destination;
                    bool shouldAccept = acceptConnection(request.CD);

                    // choose to connect by domain name or dstip
                    if(useDomain)
                        destination = request.domain;
                    else
                        destination = request.DSTIP;


                    tcp::resolver resolver(io_context);
                    tcp::resolver::query query(destination, request.DSTPORT);
                    tcp::resolver::iterator iter = resolver.resolve(query);
                    tcp::endpoint endpoint = iter -> endpoint();
                    // cout << request.domain << ' ' << request.DSTPORT << endl;
                    if(!shouldAccept) {
                        rejectReply();
                        displayMessage(request, endpoint, shouldAccept);
                        return;
                    }

                    tcp::socket socket(io_context);
                    displayMessage(request, endpoint, shouldAccept);
                    make_shared<ConnectRequest>(move(socket_), move(socket), endpoint) -> start();
                } else if(request.CD == 2) {
                    cout << "bind" << endl;
                }
            } else {
                cerr << "parse request error!" << endl;
            }
        });  

    }

    vector<string> split(string s, char delim) {
        vector<string> res;
        s.push_back(delim);
        string cur;
        for(auto& c: s) {
            if(c == delim) {
                res.push_back(cur);
                cur = "";
            } else {
                cur.push_back(c);
            }
        }

        return res;
    }

    bool acceptConnection(int CD) {
        string clientEndPoint = socket_.remote_endpoint().address().to_string();
        char delim = '.';
        vector<string> clientEndPointFields = split(clientEndPoint, delim);

        ifstream whiteList("socks.conf");
        string permit, connectType, acceptedClient;
        
        while(whiteList >> permit >> connectType >> acceptedClient) {
            vector<string> acceptedClientFields = split(acceptedClient, delim);
            if(connectType != "c")
                continue;

            bool ok = true;
            for(int i = 0; i < 4; i++) {
                if(acceptedClientFields[i] == clientEndPointFields[i] || acceptedClientFields[i] == "*")
                    continue;
                else
                    ok = false;
            }   

            if(ok) {
                whiteList.close();
                return true;
            }
        }   

        whiteList.close();
        return false;
    }

    string parseDSTIP() {
        string ip;
        for(int i = 4; i <= 7; i++) {
            ip += to_string(data_[i]);
            ip.push_back('.');
        }
        ip.pop_back();
        return ip;
    }

    void displayMessage(Request request, tcp::endpoint endpoint, bool shoudlAccept) {
        string CD;
        
        if(request.CD == 1) 
            CD = "CONNECT";
        else if(request.CD == 2)
            CD ="BIND";


        cout << "<S_IP>: " << socket_.remote_endpoint().address().to_string() << '\n';
        cout << "<S_PORT>: " << socket_.remote_endpoint().port() << '\n';
        cout << "<D_IP>: " << endpoint.address().to_string() << '\n';
        cout << "<D_PORT>: " << request.DSTPORT << '\n';
        cout << "<Command>: " << CD << '\n';

        
        cout << "<Reply>: " << (shoudlAccept ? "Accept" : "Reject");
        cout << endl;
    }

    string parseDomain(int length) {
        // domain is between the userId and the domain terminated null

        string domain;
        int domainStart;

        // skip the vn cd and dstport
        for(int i = 8; i < length; i++) {
            if(data_[i] == 0) {
                i++;
                domainStart = i;
                break;
            }
        }

        for(int i = domainStart; i < length; i++) {
            if(data_[i] == 0) {
                break;
            }
            domain.push_back(data_[i]);
        }

        return domain;
    }
    

tcp::socket socket_;
enum { max_length = 1024 };
unsigned char data_[max_length];
boost::asio::io_context& io_context;
int requestSize = 9;
};

class server {
public:
    server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), io_context(io_context) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec)
                {
                    // This function is used to inform the io_service that the process is about to fork, or has just forked. This allows the io_service, and the services it contains, to perform any necessary housekeeping to ensure correct operation following a fork.
                    io_context.notify_fork(boost::asio::io_context::fork_prepare);
                    int pid = fork();
                    // cout << "forking child..." << endl;;
                    if(pid == 0) {
                        io_context.notify_fork(boost::asio::io_context::fork_child);
                        acceptor_.close();
                        std::make_shared<SocksProxy>(std::move(socket), io_context)->start();
                    } else {
                        io_context.notify_fork(boost::asio::io_context::fork_parent);

                        // let the forked child do the task, the parent no longer need to use the socket anymore
                        socket.close();

                        // listen the next connection
                        do_accept();
                        // cout << "child forked" << endl;
                    }

                }
            });
    }

  boost::asio::io_context& io_context;
  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: soc_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        server s(io_context, std::atoi(argv[1]));

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

  return 0;
}