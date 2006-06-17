#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <asio.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>

using asio::ip::tcp;

class client
{
public:
  client(asio::io_service& io_service,
      const std::string& server, const std::string& path)
    : resolver_(io_service),
      socket_(io_service)
  {
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
    resolver_.async_resolve(query,
        boost::bind(&client::handle_resolve, this,
          asio::placeholders::error,
          asio::placeholders::iterator));
  }

private:
  void handle_resolve(const asio::error& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      // Attempt a connection to the first endpoint in the list. Each endpoint
      // will be tried until we successfully establish a connection.
      tcp::endpoint endpoint = *endpoint_iterator;
      socket_.async_connect(endpoint,
          boost::bind(&client::handle_connect, this,
            asio::placeholders::error, ++endpoint_iterator));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_connect(const asio::error& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      // The connection was successful. Send the request.
      asio::async_write(socket_, request_,
          boost::bind(&client::handle_write_request, this,
            asio::placeholders::error));
    }
    else if (endpoint_iterator != tcp::resolver::iterator())
    {
      // The connection failed. Try the next endpoint in the list.
      socket_.close();
      tcp::endpoint endpoint = *endpoint_iterator;
      socket_.async_connect(endpoint,
          boost::bind(&client::handle_connect, this,
            asio::placeholders::error, ++endpoint_iterator));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_write_request(const asio::error& err)
  {
    if (!err)
    {
      // Read the response status line.
      asio::async_read_until(socket_, response_, boost::regex("\r\n"),
          boost::bind(&client::handle_read_status_line, this,
            asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_status_line(const asio::error& err)
  {
    if (!err)
    {
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
      asio::async_read_until(socket_, response_, boost::regex("\r\n\r\n"),
          boost::bind(&client::handle_read_headers, this,
            asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_headers(const asio::error& err)
  {
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        std::cout << header << "\n";
      std::cout << "\n";

      // Write whatever content we already have to output.
      if (response_.size() > 0)
        std::cout << &response_;

      // Start reading remaining data until EOF.
      asio::async_read(socket_, response_,
          asio::transfer_at_least(1),
          boost::bind(&client::handle_read_content, this,
            asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_content(const asio::error& err)
  {
    if (!err)
    {
      // Write all of the data that has been read so far.
      std::cout << &response_;

      // Continue reading remaining data until EOF.
      asio::async_read(socket_, response_,
          asio::transfer_at_least(1),
          boost::bind(&client::handle_read_content, this,
            asio::placeholders::error));
    }
    else if (err != asio::error::eof)
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  tcp::resolver resolver_;
  tcp::socket socket_;
  asio::streambuf request_;
  asio::streambuf response_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cout << "Usage: async_client <server> <path>\n";
      std::cout << "Example:\n";
      std::cout << "  async_client www.boost.org /LICENSE_1_0.txt\n";
      return 1;
    }

    asio::io_service io_service;
    client c(io_service, argv[1], argv[2]);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }

  return 0;
}
