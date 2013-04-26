// Copyright (c) 2013 John R. Bandela
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "asio_helper.hpp"
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>



int main()
{
    boost::asio::io_service io;
    boost::asio::deadline_timer t(io, boost::posix_time::seconds(1));


    try{


        // This allows us to  do our await magic
        asio_helper::do_async(io,[&](asio_helper::async_helper helper){

            // Notice how we can use a real loop
            for(int i = 0; i < 5; i++){

                // Call the async function with the results of make_callback
                auto ec = helper.await<asio_helper::handlers::wait_handler>(
                    [&](asio_helper::handlers::wait_handler::callback_type cb){
                        t.async_wait(cb);
                });

                // Print a message about the timer
                std::cout << "Timer went off " << (i+1) << " times with ec = " << ec.message() << std::endl;

                // Set up a new expiration for the timer
                t.expires_from_now(boost::posix_time::seconds(1));

            }

        });

     io.run();

    }
    catch(std::exception& e){
        std::cerr << e.what() << "\n";
    }



    return 0;
}
