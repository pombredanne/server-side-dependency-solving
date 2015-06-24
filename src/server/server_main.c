/* Server side dependency solving - transfer of dependency solving from local machine to server when installing new packages
 * Copyright (C) 2015  Michal Ruprich, Josef Řídký
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "server.h"


#if 0
void session(ip::tcp::socket sock,boost::system::error_code ec){
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
#endif

int main(int argc, char* argv[]) {
  parse_params_srv(argc, argv);
    
  
  /*************************************************************************
  * 
  * 	Establishing port, socket etc for the communication
  * 
  *************************************************************************/
  int socket_desc, new_sock;
  char* client_ip;

  char client_msg[1000];
  socket_desc=socket(AF_INET, SOCK_STREAM, 0);//AF_INET = IPv4, SOCK_STREAM = TCP, 0 = IP
  
  if(socket_desc==-1)
  {
    ssds_log(logERROR, "Server encountered an error when creating socket for communication");
    return 1;
  }
  
  struct sockaddr_in server, client;
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=INADDR_ANY;
  server.sin_port=htons(2345);
  
  if(bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) <0)
  {
    ssds_log(logERROR, "Server wasn't able to bind with socket\n");
    return 1;
  }
  
  ssds_log(logINFO, "Server started. Waiting for incoming connections\n");
  
  if(listen(socket_desc, 5)!=0)
  {
    ssds_log(logERROR, "Listen failed on server\n");
    return 1;
  }
  
  int addr_len = sizeof(server);
  char* buf;
  
  while(1)
  {
    if((new_sock=accept(socket_desc, (struct sockaddr *) &client, (socklen_t*)&addr_len))<0)
    {
      ssds_log(logERROR, "Accept connection has failed");
      return 1;
    }
    
    buf=sock_recv(new_sock);
    
    if(buf == NULL)
    {
      ssds_log(logERROR, "Recieving of data has failed\n");
      return 1;
    }
    
    client_ip=inet_ntoa(client.sin_addr);
    ssds_log(logMESSAGE, "Connection accepted from ip address %s\n", client_ip);
    
    SsdsJsonRead* json = ssds_json_read_init();
    if(!ssds_read_parse(buf, json))//parse incoming message
    {
      ssds_log(logERROR, "False data recieved from %s. Client rejected\n", client_ip);
      continue;
    }
    
    /* Dependency solving part */
    
    SsdsPkgInfo* pkgs = ssds_read_pkginfo_init();
    ssds_read_get_packages(pkgs, json);
  
    SsdsRepoInfoList* list = ssds_read_list_init();
    ssds_read_repo_info(json, list);
    
    guint len=g_slist_length(list->repoInfoList);
    guint i;
    
    SsdsRepoMetadataList* meta_list = ssds_repo_metadata_init();
    ssds_locate_repo_metadata(json, list, meta_list);
    
    
    //TODO - change this so that it doesn't need to be created manually
    HySack sack = hy_sack_create(NULL, NULL, NULL,HY_MAKE_CACHE_DIR);
    hy_sack_load_system_repo(sack, NULL, HY_BUILD_CACHE);
    HySack* sack_p = &sack;
    ssds_fill_sack(sack_p, meta_list);

    SsdsJsonCreate* answer = ssds_js_cr_init();
    ssds_dep_answer(json, answer, sack_p);
    
    char* message = ssds_js_to_string(answer);
    write(new_sock, message, strlen(message));
  }
  //ssds_solving::solve solveHandler;

  
  
//   HySack* sack = ssds_solve_init();
//   ssds_fill_sack(sack, list);

#if 0
  try {
    ip::tcp::iostream streams("");
    while(42) {
      ip::tcp::socket sock(ios);
      acceptor_.accept(sock); //second argument can be error handler

      std::cout<<"Some connection was accepted" << std::endl;

      mainserver.process_connection(sock);
#if 0
      int64_t size;
      boost::asio::read(sock,boost::asio::buffer(&size,sizeof(size)));

      std::vector<char> buf(size);
      size_t len = sock.read_some(buffer(buf), ec);
      std::string input_message = std::string(buf.begin(), buf.end());

      std::cout<< "Message has " << len << " characters." << std::endl;

      solving
      std::string message = solvePoint.answer(input_message);
#endif	

      sock.close();

      //far future		    
      //std::thread(session, std::move(sock).std::move(ec)).detach();

    }
  } /*handling exceptions*/
  catch (std::exception& e){
    //std::ostringstream os;
    //os << "Server: "<< e.what();
    /*std::cerr*///my_log.add_log(logERROR,e.what());
    ssds_log("pokus", logERROR);
    //ssds_log(e.what(), logERROR); //might not work since e.what returns virtual const char*
  }

#endif
  return 0;
}
