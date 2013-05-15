#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
#include "asio_helper.hpp"

int main()
{
  namespace asio = boost::asio;
  asio::io_service io;

  // Turn off newline buffering
  ::termios term;
  ::tcgetattr( STDIN_FILENO, &term );
  term.c_lflag &= ~(ICANON);
  ::tcsetattr( STDIN_FILENO, TCSANOW, &term );

  asio::posix::stream_descriptor in( io, ::dup( STDIN_FILENO ) );
  asio::posix::stream_descriptor out( io, ::dup( STDOUT_FILENO ) );
  using namespace asio_helper::handlers;
  
  asio_helper::do_async(io,[&io,&in,&out](asio_helper::async_helper helper){
  boost::system::error_code ec;
  std::tie(ec,std::ignore) = helper.await<write_handler>(
     [&](write_handler::callback_type cb){
  asio::async_write(
    out,
    asio::buffer( "Please type in your name: " ),cb);
    });

      const std::size_t bufferSize = 10;
      boost::asio::streambuf response( bufferSize );
      response.prepare( bufferSize );
     
    std::tie(ec,std::ignore) = helper.await<read_handler>(
      [&](read_handler::callback_type cb){
      asio::async_read_until(
        in,
        response,
        '\n',
        cb);});
          if( ec )
          {
            if( ec == asio::error::not_found ) {
     std::tie(ec,std::ignore) = helper.await<write_handler>(
         [&](write_handler::callback_type cb){
              asio::async_write(
                out,
                asio::buffer( "\nYour name is too long, try a nickname\n" ),cb);
             });
            } else {
              auto message = std::string("Error: ") + ec.message() + "\n" ;
      std::tie(ec,std::ignore) = helper.await<write_handler>(
          [&](write_handler::callback_type cb){
              asio::async_write(
                out,
                asio::buffer( message ),cb);});
            }
          }
          else
          {
            std::istream is( &response);
            std::string name;
            std::getline( is, name, '\n' );
            auto message = 
              "Hello " + name + "!\n" ;
           helper.await<write_handler>(
          [&](write_handler::callback_type cb){
            asio::async_write(
              out,
              asio::buffer( message ),cb);});
          }
});
  io.run();
}
