//============================================================================
// Name		: server.cpp
// Author	: brumlablo
// Editor	: Jozkar, mruprich	
// Version	: 0.2
// Copyright	: GNU GPL
// Description	: Server side of SSDS
//============================================================================

/*BUILDED WITH THESE FLAGS: -O2 -g -Wall -Wextra -pedantic  -std=c++11 -lboost_thread -lboost_system -fmessage-length=0*/
/*synchronous start of server based on boost.asio library*/

#include "server.hpp"

namespace ssds_server {
  using namespace boost::asio;
  
  server::server(){
  }

  server::~server() {
    
  }
  
  int server::newPort(int newport){
	  port = newport;
	  return port;
  }
  
  boost::asio::io_service& server::getIo(){
	  return io_service_;
  }

  /*
   * This function processes incoming data - it will probably be forked later
   */
  void server::process_connection(boost::asio::ip::tcp::socket& sock)
  {
    ssds_solving::solve solvePoint;
    ssds_xml::read_xml xml;
    ssds_repo::repo_metadata metadata;
    ssds_xml::xml_debug debug;
    
    boost::system::error_code ec;
    int64_t size;

    //recieving data
    boost::asio::read(sock,boost::asio::buffer(&size,sizeof(size)));
    std::vector<char> buf(size);
    size_t len = sock.read_some(buffer(buf), ec);
    std::string input_message = std::string(buf.begin(), buf.end());
    
    xml.parse_xml_string(input_message);
    /*
     * Here I would put something that will decide what to do according to the code from the client
     */
    
    
    
    xml.get_node_by_path((xmlChar* )"//data/repolist/repo", metadata.urls);
    
    metadata.locate_repo_metadata_by_url();
    
    //debug.flush_url_vector(metadata.urls);
    int count = 0;
    
    std::cout<< "Message has " << len << " characters." << std::endl;
    solvePoint.fill_sack(metadata);
    
    std::string message = "this is some random message from server\n";
    write(sock, buffer(message), ec);
  }//process_connection


  void server::session(ip::tcp::socket sock,boost::system::error_code ec){
    try {
      while(42)
      {
        char data[1024];
        size_t lengthsome = sock.read_some(boost::asio::buffer(data), ec);
        if (ec == boost::asio::error::eof) {
          std::cout << "Connection closed." << "\n";
          break; // Connection closed cleanly by peer.
        }
        else if (ec)
          throw boost::system::system_error(ec);
        
        boost::asio::write(sock, boost::asio::buffer(data, lengthsome));
      }//while
    }//try
    catch (std::exception& e) {
                  std::cerr << "Exception in thread: " << e.what() << "\n";
    }
  }
}
