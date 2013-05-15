#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <boost/make_shared.hpp>

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
  asio::async_write(
    out,
    asio::buffer( "Please type in your name: " ),
    [&]( const boost::system::error_code & ec, std::size_t transferred )
    {
      const std::size_t bufferSize = 10;
      auto response = boost::make_shared< boost::asio::streambuf >( bufferSize );
      response->prepare( bufferSize );
      asio::async_read_until(
        in,
        *response,
        '\n',
        [&io,&in,&out,response](
          const boost::system::error_code & ec,
          std::size_t transferred )
        {
          if( ec )
          {
            if( ec == asio::error::not_found ) {
              asio::async_write(
                out,
                asio::buffer( "\nYour name is too long, try a nickname\n" ),
                []( const boost::system::error_code & ec, std::size_t transferred ) {} );
            } else {
              auto message = boost::make_shared<std::string>( "Error: " + ec.message() + "\n" );
              asio::async_write(
                out,
                asio::buffer( *message ),
                [&message]( const boost::system::error_code & ec, std::size_t transferred ) {} );
            }
          }
          else
          {
            std::istream is( response.get() );
            std::string name;
            std::getline( is, name, '\n' );
            auto message = boost::make_shared<std::string>(
              "Hello " + name + "!\n" );
            asio::async_write(
              out,
              asio::buffer( *message ),
              [&message]( const boost::system::error_code & ec, std::size_t transferred ) {} );
          }
        } );
    } );
  io.run();
}