#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/asio/ip/address.hpp>
#include <fstream> 
#include <boost/algorithm/string.hpp>

#define MAX_SESSION 5

using namespace std;
using boost::asio::ip::tcp;
boost::asio::io_context io_context;
string proxyHost;
string proxyPort;
tcp::endpoint proxyEndPoint;

int reconnectAttemp = 5;

struct queryFields {
    string host;
    string port;
    string file;
};

class session
    : public std::enable_shared_from_this<session>
{
    public:
    session(tcp::socket socket, tcp::endpoint endpoint, tcp::endpoint proxyEndPoint, queryFields qf, int id)
        : socket_(std::move(socket)), endpoint{endpoint}, proxyEndPoint{proxyEndPoint}, qf{qf}, id{id}
    {
        string filePath = "./test_case/" + qf.file;
        file.open(filePath);
        
        string command;
        while(getline(file, command)) {
            commands.push_back(command);
        }

        // the endpoint of the application server
        DSTIP = endpoint.address().to_string();
        DSTPORT = endpoint.port();

    }

    void start()
    {
        buildRequest();
        connect();
    }

    private:

    void buildRequest() {
        clearBuffer(outBuffer);
        //                 +----+----+----+----+----+----+----+----+----+----+....+----+
        //                 | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
        //                 +----+----+----+----+----+----+----+----+----+----+....+----+
        // # of bytes:	     1    1      2              4           variable       1

        // fill the request in the outBuffer and write it to the socket

        vector<string> ipString;
        vector<int> ips;
        boost::split(ipString, DSTIP, boost::is_any_of("."), boost::token_compress_on);
        for(auto& field: ipString) {
            ips.push_back(stoi(field));
        }



        request[0] = 4;
        request[1] = 1; // connect
        request[2] = DSTPORT / 256;
        request[3] = DSTPORT % 256;
        request[4] = (unsigned char)ips[0];
        request[5] = (unsigned char)ips[1];
        request[6] = (unsigned char)ips[2];
        request[7] = (unsigned char)ips[3];
        request[8] = 0;   // Null

        memcpy(outBuffer, request, requestSize);
    }

    // connect first, then read write loop
    void connect() {
        auto self(shared_from_this());

        // it connect to the proxy now
        socket_.async_connect(proxyEndPoint, [this, self](const boost::system::error_code ec)
        {
                if(!ec) {
                    // client needs to follow socks4 protocol now
                    
                    initProxyConnection();
                } else {
                    cout << "connection error: Reconnect..." << endl;
                    connect();
                    reconnectAttemp--;
                    if(reconnectAttemp == 0)
                        exit(1);
                }
            }
        );
    }

    void getServerReply() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(inBuffer, max_length), [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    clearBuffer(inBuffer);
                    clearBuffer(outBuffer);
                    do_read();
                }
            }
        );
    }

    void initProxyConnection() {
        auto self(shared_from_this());
        // write the request to the socket

        socket_.async_write_some(boost::asio::buffer(outBuffer, max_length), [this, self](boost::system::error_code ec, size_t length) {
                if(!ec) {

                    getServerReply();
                } else {
                    cout << "initProxyConnection error" << endl;
                }
            }
        );
    }



    // the data here is read from the np_single_golden
    void do_read()
    {
        // cout << "do_reading..." << endl;
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(inBuffer, max_length),
            [this, self](boost::system::error_code ec, std::size_t length)
            {
                // cout << "Data read: " << inBuffer << endl;
                if (!ec)
                {
                    string msg = string(inBuffer);
                    clearBuffer(inBuffer);

                    outputShell(msg, false);
                    // check this condition, otherwise it will get caught in an infinite loop, note that the welcome messge contains %
                    if(msg.find('%') == string::npos) {
                        // the np_single_golden is still outputting messages
                        // remember to do_read again, otherwise the pipe will be broken
                        do_read();
                    } else {
                        // the np_single_golde is wating for input(and the input is from the file)
                        do_write();
                    }
                } else {
                    cerr << "error_code: " << ec << endl;
                }
            });
    }

    void clearBuffer(char buffer[]) {
        memset(buffer, '\0', max_length);
    }

    void do_write()
    {
        
        auto self(shared_from_this());

        commands[commandsIdx].push_back('\n');
        strcpy(outBuffer, commands[commandsIdx].c_str());
        // cerr << "do_writing..." << endl;
        // cerr << "The msg in the outBuffer is: " << outBuffer << endl;
        string msg = string(outBuffer);
        outputShell(msg, true);

        commandsIdx++;
        boost::asio::async_write(socket_, boost::asio::buffer(outBuffer, strlen(outBuffer)),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    clearBuffer(outBuffer);

                    // cerr << "back to do_read" << endl;
                    do_read();
                }
                else
                    cerr << "error_code: " << ec << endl;
            });
    }

    void outputShell(string msg, bool isCommand){
        // insert js in html
        string processedMsg = preprocess(msg);
        if(isCommand) 
            processedMsg = "<span>" + processedMsg + "</span>";
        cout << "<script>document.getElementById(\'s" + to_string(id) + "\').innerHTML += \'" + processedMsg + "\';</script>" << endl;
    }

    string preprocess(string msg) {
        // string literal contains an unescaped line break, t1 use LF t2~t5 use CRLF
        boost::algorithm::replace_all(msg, "\n", "<br>");
        boost::algorithm::replace_all(msg, "\r", "");

        // escape single quote in msg string
        boost::algorithm::replace_all(msg, "'", "&apos;");

        return msg;
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    queryFields qf;
    int id;
    tcp::endpoint endpoint;
    tcp::endpoint proxyEndPoint;
    string DSTIP;
    unsigned short DSTPORT;
    ifstream file;
    vector<string> commands;
    int commandsIdx = 0;
    char inBuffer[max_length];
    char outBuffer[max_length];
    int requestSize = 9;
    unsigned char request[9];
};

        
vector<queryFields> parseQueryStrings();
void printHtml(string);
string getQueryValue(string);
// void modifyTableHeader(int);

int main(int argc, char* argv[])
{
    try
    {
        string html = R"(
            <!DOCTYPE html>
                <html lang="en">
                <head>
                    <meta charset="UTF-8" />
                    <title>NP Project 3 Console</title>
                    <link
                    rel="stylesheet"
                    href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
                    integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
                    crossorigin="anonymous"
                    />
                    <link
                    href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
                    rel="stylesheet"
                    />
                    <link
                    rel="icon"
                    type="image/png"
                    href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
                    />
                    <style>
                    * {
                        font-family: 'Source Code Pro', monospace;
                        font-size: 1rem !important;
                    }
                    body {
                        background-color: #212529;
                    }
                    pre {
                        color: #cccccc;
                    }
                    b, span {
                        color: #01b468;
                    }
                    span {
                        font-weight: bold;
                    }
                    </style>
                </head>
                <body>
                    <table class="table table-dark table-bordered">
                    <thead>
                        <tr>
                        <th scope="col">ip 0</th>
                        <th scope="col">ip 1</th>
                        <th scope="col">ip 2</th>
                        <th scope="col">ip 3</th>
                        <th scope="col">ip 4</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr>
                        <td><pre id="s0" class="mb-0"></pre></td>
                        <td><pre id="s1" class="mb-0"></pre></td>
                        <td><pre id="s2" class="mb-0"></pre></td>
                        <td><pre id="s3" class="mb-0"></pre></td>
                        <td><pre id="s4" class="mb-0"></pre></td>
                        </tr>
                    </tbody>
                    </table>
                </body>
            </html>)";



        // parse the query string, discard the keys
        vector<queryFields> q = parseQueryStrings();

        for(int i = 0; i < MAX_SESSION; i++) {
            if(q[i].host != "") {
                string host = q[i].host;
                string port = q[i].port;
                string ip = host + ":" + port;
                string replacedSubstr = "ip " + to_string(i);
                boost::algorithm::replace_all(html, replacedSubstr, ip);
            }
        }

        cout << html << endl;

        tcp::resolver resolver(io_context);
        tcp::resolver::query query(proxyHost, proxyPort);
        tcp::resolver::iterator iter = resolver.resolve(query);
        proxyEndPoint = iter->endpoint();

        for(int i = 0; i < MAX_SESSION; i++) {
            // no session
            if(q[i].host == "" || q[i].port == "" || q[i].file == "") 
                break;


            tcp::resolver resolver(io_context);
            tcp::resolver::query query(q[i].host, q[i].port);
            tcp::resolver::iterator iter = resolver.resolve(query);
            tcp::endpoint endpoint = iter -> endpoint();


            tcp::socket socket(io_context);

            // connect to the np_single_golden here, note that the i need to be passed so that the getElementById can work
            make_shared<session>(move(socket), endpoint, proxyEndPoint, q[i], i)->start();
        }


        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

string getQueryValue(string field) {
    return field.substr(field.find('=') + 1);
}



vector<queryFields> parseQueryStrings() {
    string query = "QUERY_STRING";
    string queryString = getenv(query.c_str());


    vector<string> queries;
    boost::split(queries, queryString, boost::is_any_of("&"), boost::token_compress_on);

    vector<queryFields> q(MAX_SESSION);

    // assume that the order is always host, port, file
    int nonProxyCount = 0;
    for(int i = 0; i < queries.size(); i++) {
        string value = getQueryValue(queries[i]);
        if(queries[i].find("sh") != string::npos) {
            proxyHost = value;
        } else if(queries[i].find("sp") != string::npos) {
            proxyPort = value;
        } else {
            int index = nonProxyCount / 3;
            if(nonProxyCount % 3 == 0) {
                q[index].host = value;
            } else if(i % 3 == 1) {
                q[index].port = value;
            } else {
                q[index].file = value;
            }
            nonProxyCount++;
        }
    }

    return q;
}

