// This file is adapted almost verbative where possible from
// the boost asio async http example
//
// async_client.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Modified by John R. Bandela

#include "asio_helper.hpp"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <tuple>
#include <vector>
#include <thread>

using boost::asio::ip::tcp;

void get_http(boost::asio::io_service& io,std::string server, std::string path){

    using namespace asio_helper::handlers;
    // This allows us to do await
    asio_helper::do_async(io,[=,&io](asio_helper::async_helper helper){
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

        // Do async resolve
        tcp::resolver::iterator endpoint_iterator;
        boost::system::error_code ec;
        std::tie(ec,endpoint_iterator) =  helper.await<resolve_handler>(
            [&](resolve_handler::callback_type cb){
                resolver_.async_resolve(query,cb);
        });
        if(ec) {throw boost::system::system_error(ec);}

        // Do async connect
        std::tie(ec,std::ignore) = helper.await<composedconnect_handler>(
            [&](composedconnect_handler::callback_type cb){
                boost::asio::async_connect(socket_,endpoint_iterator,cb);    
        });
        if(ec){throw boost::system::system_error(ec);}

        // Connection was successful, send request
        std::tie(ec,std::ignore) = helper.await<write_handler>(
            [&](write_handler::callback_type cb){
                boost::asio::async_write(socket_,request_,cb);
        });
        if(ec){throw boost::system::system_error(ec);}

        // Read the response status line
        std::tie(ec,std::ignore) = helper.await<read_handler>(
            [&](read_handler::callback_type cb){
                boost::asio::async_read_until(socket_,response_,"\r\n",cb);
        });
        if(ec){throw boost::system::system_error(ec);}

        // Check that the response is OK
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
        std::tie(ec,std::ignore) = helper.await<read_handler>(
           [&](read_handler::callback_type cb){
                boost::asio::async_read_until(socket_, response_, "\r\n\r\n",cb);
        });
        if(ec){throw boost::system::system_error(ec);}

        // Process the response headers.
        std::istream response_stream2(&response_);
        std::string header;
        while (std::getline(response_stream2, header) && header != "\r")
            std::cout << header << "\n";
        std::cout << "\n";

        // Write whatever content we already have to output.
        if (response_.size() > 0)
            std::cout << &response_;

        // Continue reading remaining data until EOF.
        bool done = false;
        while(!done){

            std::tie(ec,std::ignore) = helper.await<read_handler>(
                [&](read_handler::callback_type cb){ 
                    boost::asio::async_read(socket_, response_,
                        boost::asio::transfer_at_least(1), cb);         
            });
            if(ec && ec != boost::asio::error::eof){throw boost::system::system_error(ec);}
            done = (ec == boost::asio::error::eof);
            // Write all of the data so far
            std::cout << &response_;
        }
   });
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cout << "Usage: example2 <server> <path>\n";
            std::cout << "Example:\n";
            std::cout << "  Example2 www.boost.org /LICENSE_1_0.txt\n";
            return 1;
        }

        boost::asio::io_service io_service;

        
        get_http(io_service,argv[1],argv[2]);

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}