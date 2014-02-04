//
// stream_msg_client.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STREAM_MSG_CLIENT_HPP
#define STREAM_MSG_CLIENT_HPP

#include <functional>

#include <cstdlib>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

template<typename Message, typename StreamProtocol = boost::asio::ip::tcp>
class stream_msg_client
{
private:
  typedef Message message;
  typedef StreamProtocol stream_protocol;
  typedef typename StreamProtocol::socket socket;

protected:
  boost::asio::io_service& io_service_;
  socket socket_;

public:

  stream_msg_client(boost::asio::io_service& io_service)
    : io_service_(io_service),
      socket_(io_service)
  {
  }

  template<typename ReadHandler>
  void async_receive(Message * msg, ReadHandler handler)
  {
      boost::asio::async_read(socket_,
          boost::asio::buffer(msg->data(), message::header_length),
          std::bind(
            &stream_msg_client::handle_read_header<ReadHandler>,
            this, msg, handler, std::placeholders::_1, std::placeholders::_2));
  }

  template<typename ReadHandler>
  void async_receive(Message * msg, boost::asio::socket_base::message_flags flags, ReadHandler handler)
  {
      boost::asio::async_read(socket_,
          boost::asio::buffer(msg->data(), message::header_length),
          flags,
          std::bind(
            &stream_msg_client::handle_read_header<ReadHandler>,
            this, msg, handler, std::placeholders::_1, std::placeholders::_2));
  }

protected:

  template<typename ReadHandler>
  void handle_read_header(Message * msg, ReadHandler handler, const boost::system::error_code& error, std::size_t bytes_transferred)
  {
    if (!error && msg->decode_header())
    {
      boost::asio::async_read(socket_,
          boost::asio::buffer(msg->body(), msg->body_length()),
          handler);
    }
    else
    {
      handler(error);
    }
  }

};

#endif // STREAM_MSG_CLIENT_HPP
