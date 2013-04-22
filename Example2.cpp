#include "simple_async_function.hpp"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

int main(){

    std::string server = "www.boost.org";
    std::string path = "/LICENSE_1_0.txt";

    boost::asio::io_service io;


    tcp::resolver resolver_(io);
    tcp::socket socket_(io);
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request_);
    request_stream << "GET " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    tcp::resolver::query query(server, "http");

    // This allows us to do wait_async
    do_async(io,[&](simple_async_function_helper helper){

        // Do async resolve
        auto resolve_cb = helper.make_callback([&](
            const boost::system::error_code& err,
            tcp::resolver::iterator iter)->tcp::resolver::iterator{
                if(err){
                    std::cout << "Error: " << err.message() << "\n";
                    throw std::runtime_error(err.message());
                }
                return iter;
        });
        resolver_.async_resolve(query,resolve_cb);
        auto endpoint_iterator = helper.await<tcp::resolver::iterator>();

        // Do async connect
        boost::asio::async_connect(socket_,
            endpoint_iterator,resolve_cb);
        helper.await<tcp::resolver::iterator>();

        auto readwrite_callback = helper.make_callback(
            [&](const boost::system::error_code& err,std::size_t)->int{
                if(err){
                    std::cout << "Error: " << err.message() << "\n";
                    throw std::runtime_error(err.message());
                }
                return 0;
        });
        // Connection was successful, send request
        boost::asio::async_write(socket_,request_,readwrite_callback);
        helper.await<int>();

        // Read the response status line
        boost::asio::async_read_until(socket_,response_,"\r\n",
            readwrite_callback);
        helper.await<int>();

        // Check that the response is OK
        // Check that response is OK.
        std::istream response_stream(&response_);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            std::cout << "Invalid response\n";
            return;
        }
        if (status_code != 200)
        {
            std::cout << "Response returned with status code ";
            std::cout << status_code << "\n";
            return;
        }
        // Read the response headers, which are terminated by a blank line.
        boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
            readwrite_callback);
        helper.await<int>();

        // Process the response headers

        // Process the response headers.
        std::istream response_stream2(&response_);
        std::string header;
        while (std::getline(response_stream2, header) && header != "\r")
            std::cout << header << "\n";
        std::cout << "\n";

        // Write whatever content we already have to output.
        if (response_.size() > 0)
            std::cout << &response_;

        auto content_callback = helper.make_callback(
            [&](const boost::system::error_code& err,std::size_t)->bool{
                if(!err){
                    return false;
                }else if(err == boost::asio::error::eof){
                    return true;
                }
                else{
                    std::cout << "Error: " << err.message() << "\n";
                    throw std::runtime_error(err.message());
                }
                return 0;
        });


        bool done = false;
        while(!done){
            // Continue reading remaining data until EOF.

            boost::asio::async_read(socket_, response_,
                boost::asio::transfer_at_least(1),
                content_callback);
            done = helper.await<bool>();
            // Write all of the data so far
            std::cout <<&response_<<std::flush;
        }



    });




}