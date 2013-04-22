#include "simple_async_function.hpp"
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>



int main()
{
  boost::asio::io_service io;
  boost::asio::deadline_timer t(io, boost::posix_time::seconds(1));


  try{

do_async(io,[&](simple_async_function_helper helper){
      
      for(int i = 0; i < 5; i++){
          auto cb = helper.make_callback([&](const boost::system::error_code& ec)->boost::system::error_code{return ec;});

        t.async_wait(cb);
        auto ec = helper.await<boost::system::error_code>();
        std::cout << "Timer went off " << (i+1) << " times with ec = " << ec.message() << std::endl;
        t.expires_from_now(boost::posix_time::seconds(1));

      }

  });

  }
  catch(std::exception& e){
      std::cerr << e.what() << "\n";
  }



  return 0;
}
