#include "asio_helper.hpp"
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>



int main()
{
    boost::asio::io_service io;
    boost::asio::deadline_timer t(io, boost::posix_time::seconds(1));


    try{

        asio_helper::do_async(io,[&](asio_helper::async_helper helper){

            for(int i = 0; i < 5; i++){
                auto cb = helper.make_callback([&](const boost::system::error_code& ec)->boost::system::error_code{return ec;});

                t.async_wait(cb);
                auto ec = helper.await<boost::system::error_code>();
                std::cout << "Timer went off " << (i+1) << " times with ec = " << ec.message() << std::endl;
                //if(i==1) throw std::exception("Exception");
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
