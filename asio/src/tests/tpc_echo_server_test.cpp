#include "asio.hpp"
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace asio;

typedef boost::shared_ptr<stream_socket> stream_socket_ptr;

void tpc_echo_session(stream_socket_ptr sock)
{
  try
  {
    enum { max_length = 512 };
    char data[max_length];

    int length;
    while ((length = sock->recv(data, max_length)) > 0)
      if (sock->send_n(data, length) <= 0)
        break;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

void tpc_echo_server(demuxer& d)
{
  socket_acceptor a(d, inet_address_v4(12345));
  for (;;)
  {
    stream_socket_ptr sock(new stream_socket(d));
    a.accept(*sock);
    boost::thread t(boost::bind(tpc_echo_session, sock));
  }
}

int main()
{
  try
  {
    demuxer d;
    tpc_echo_server(d);
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
