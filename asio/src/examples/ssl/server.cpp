#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include "asio.hpp"
#include "asio/ssl.hpp"

class session
{
public:
  session(asio::io_service& io_service, asio::ssl::context& context)
    : socket_(io_service, context)
  {
  }

  asio::stream_socket& socket()
  {
    return socket_.lowest_layer();
  }

  void start()
  {
    socket_.async_handshake(asio::ssl::stream_base::server,
        boost::bind(&session::handle_handshake, this,
          asio::placeholders::error));
  }

  void handle_handshake(const asio::error& error)
  {
    if (!error)
    {
      socket_.async_read_some(asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
    }
    else
    {
      delete this;
    }
  }

  void handle_read(const asio::error& error, size_t bytes_transferred)
  {
    if (!error)
    {
      asio::async_write(socket_,
          asio::buffer(data_, bytes_transferred),
          boost::bind(&session::handle_write, this,
            asio::placeholders::error));
    }
    else
    {
      delete this;
    }
  }

  void handle_write(const asio::error& error)
  {
    if (!error)
    {
      socket_.async_read_some(asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
    }
    else
    {
      delete this;
    }
  }

private:
  asio::ssl::stream<asio::stream_socket> socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(asio::io_service& io_service, unsigned short port)
    : io_service_(io_service),
      acceptor_(io_service, asio::ipv4::tcp::endpoint(port)),
      context_(io_service, asio::ssl::context::sslv23)
  {
    context_.set_options(
        asio::ssl::context::default_workarounds
        | asio::ssl::context::no_sslv2
        | asio::ssl::context::single_dh_use);
    context_.use_certificate_chain_file("server.pem");
    context_.use_private_key_file("server.pem", asio::ssl::context::pem);
    context_.use_tmp_dh_file("dh512.pem");

    session* new_session = new session(io_service_, context_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          asio::placeholders::error));
  }

  void handle_accept(session* new_session, const asio::error& error)
  {
    if (!error)
    {
      new_session->start();
      new_session = new session(io_service_, context_);
      acceptor_.async_accept(new_session->socket(),
          boost::bind(&server::handle_accept, this, new_session,
            asio::placeholders::error));
    }
    else if (error == asio::error::connection_aborted)
    {
      acceptor_.async_accept(new_session->socket(),
          boost::bind(&server::handle_accept, this, new_session,
            asio::placeholders::error));
    }
    else
    {
      delete new_session;
    }
  }

private:
  asio::io_service& io_service_;
  asio::socket_acceptor acceptor_;
  asio::ssl::context context_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: server <port>\n";
      return 1;
    }

    asio::io_service io_service;

    using namespace std; // For atoi.
    server s(io_service, atoi(argv[1]));

    io_service.run();
  }
  catch (asio::error& e)
  {
    std::cerr << e << "\n";
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
