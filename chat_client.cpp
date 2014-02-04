//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include "chat_message.hpp"
#include "stream_msg_client.hpp"

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

template<typename StreamProtocol = tcp>
class chat_client
  : private stream_msg_client<chat_message, StreamProtocol>
{
public:
  chat_client(boost::asio::io_service& io_service,
      typename StreamProtocol::resolver::iterator endpoint_iterator)
    : stream_msg_client<chat_message, StreamProtocol>(io_service)
  {
    boost::asio::async_connect(this->socket_, endpoint_iterator,
        boost::bind(&chat_client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  void write(const chat_message& msg)
  {
    this->io_service_.post(boost::bind(&chat_client::do_write, this, msg));
  }

  void close()
  {
    this->io_service_.post(boost::bind(&chat_client::do_close, this));
  }

private:

  void handle_connect(const boost::system::error_code& error)
  {
    if (!error)
    {
      this->async_receive(&read_msg_,
          boost::bind(&chat_client::handle_read,
              this, boost::asio::placeholders::error));
    }
  }

  void handle_read(const boost::system::error_code& error)
  {
    if (!error)
    {
      std::cout << ">> ";
      std::cout.write(read_msg_.body(), read_msg_.body_length());
      std::cout << "\n";
      this->async_receive(&read_msg_,
          boost::bind(&chat_client::handle_read,
              this, boost::asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void do_write(chat_message msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      boost::asio::async_write(this->socket_,
          boost::asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_client::handle_write, this,
            boost::asio::placeholders::error));
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      write_msgs_.pop_front();
      if (!write_msgs_.empty())
      {
        boost::asio::async_write(this->socket_,
            boost::asio::buffer(write_msgs_.front().data(),
              write_msgs_.front().length()),
            boost::bind(&chat_client::handle_write, this,
              boost::asio::placeholders::error));
      }
    }
    else
    {
      do_close();
    }
  }

  void do_close()
  {
    this->socket_.close();
  }

private:
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], argv[2]);
    tcp::resolver::iterator iterator = resolver.resolve(query);

    chat_client<> c(io_service, iterator);

    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      using namespace std; // For strlen and memcpy.
      chat_message msg;
      msg.body_length(strlen(line));
      memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      c.write(msg);
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
