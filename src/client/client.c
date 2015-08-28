#include "client.h"

int ssds_get_new_id(int socket, char **id, char *arch, char *release)
{
  char *message;
  SsdsJsonCreate* json_gen = ssds_js_cr_init(GENERATE_ID);
        
  ssds_log(logDEBUG, "Inserted code %d into json.\n", GENERATE_ID);

  ssds_js_cr_gen_id(json_gen, arch, release);    // generate message for server
  ssds_log(logDEBUG, "Generated JSON for server with params: arch=%s, release=%s.\n", arch, release);

  ssds_log(logDEBUG, "Generating message string.\n");
  message = ssds_js_cr_to_string(json_gen);
  ssds_log(logDEBUG, "Message string generated: \t%s\n", message);

  ssds_log(logMESSAGE, "Sending message to server.\n");
  write(socket, message, strlen(message));
  ssds_log(logMESSAGE, "Message sent.\n");

  *id = NULL;
  //TODO: read answer from server
  ssds_free(json_gen);

  return OK;
}

int ssds_send_System_solv(int comm_sock, int data_sock, char *path)
{
  SsdsJsonCreate* json_msg = ssds_js_cr_init(SEND_SOLV), *json_solv = ssds_js_cr_init(SOLV_MORE_FRAGMENT);
  SsdsJsonRead* json_read = ssds_js_rd_init();

  char* msg_output;
  ssds_log(logDEBUG, "Generating output message with info about sending @System.solv file to server.\n");
  msg_output = ssds_js_cr_to_string(json_msg);
  ssds_log(logDEBUG, "Message generated.\n\n%s\n\n---- END OF PARSING PART ----\n\n", msg_output);

  /***********************************************************/
  /* Sending @System.solv file                               */
  /***********************************************************/
  ssds_log(logMESSAGE, "Sending info about sending @System.solv file to server.\n");
  write(comm_sock, msg_output, strlen(msg_output));
  ssds_log(logMESSAGE, "Message sent.\n");

  ssds_log(logDEBUG, "Path to sys.solv file : %s\n",path);
  FILE * f;
  f = fopen(path,"rb");

  ssds_log(logDEBUG, "Opening @System.solv file.\n");
  if(f == NULL)
  {
    ssds_log(logERROR,"Error while opening @System.solv file.\n");
    return FILE_ERROR;
  }

  ssds_log(logDEBUG, "Preparing .solv variables.\n");

  char buffer[131072];
  char* msg_length;
  char* server_response;
  size_t bytes_read = 0;

  ssds_log(logDEBUG, "Sending @System.solv file.\n");
  while((bytes_read = fread(buffer, 1, 131072, f)) != 0)
  {
      ssds_js_cr_set_read_bytes(json_solv, (int) bytes_read);
      msg_length = ssds_js_cr_to_string(json_solv);  

      write(comm_sock, msg_length, strlen(msg_length));
      ssds_log(logDEBUG, "Command sent.\n");
      write(data_sock, buffer, bytes_read);
      ssds_log(logDEBUG, "Data sent.\n");

      ssds_free(json_solv);
      json_solv = ssds_js_cr_init(SOLV_MORE_FRAGMENT);
      
      server_response = sock_recv(comm_sock);
      ssds_js_rd_parse(server_response, json_read);
  
      if(ssds_js_rd_get_code(json_read) != ANSWER_OK)
      {
         ssds_log(logERROR, "%s\n", ssds_js_rd_get_message(json_read));
         return NETWORKING_ERROR;
      }
     
      ssds_free(json_read);
      json_read = ssds_js_rd_init();
  }

  ssds_js_cr_insert_code(json_msg, SOLV_NO_MORE_FRAGMENT);
  msg_output = ssds_js_cr_to_string(json_msg);
  write(comm_sock, msg_output, strlen(msg_output));
  ssds_log(logDEBUG, "Message sent.\n");

  return OK; 
}

int ssds_send_repo(ParamOptsCl* params, char *arch, char *release, int comm_sock, int action)
{
  
  ssds_log(logDEBUG, "Client repo info JSON creating.\n");

  SsdsJsonCreate *json_gen = ssds_js_cr_init(action);
  SsdsLocalRepoInfo* local_repo = ssds_repo_parse_init();

  ssds_log(logDEBUG, "Local repo info initialized.\n");

  // parsing local repo
  if(!ssds_parse_default_repo(local_repo))
  {
     return REPO_ERROR;
  }
  ssds_log(logDEBUG, "Local repo is parsed.\n");
  
  ssds_get_repo_urls(local_repo, json_gen, arch, release);
  ssds_log(logDEBUG, "Getting repo urls.\n");
  
  ssds_log(logDEBUG, "Loop thrue required packages.\n");
  for(int i = 0; i < params->pkg_count; i++)
  {
     char* pkg = (char*)g_slist_nth_data(params->pkgs, i);
     ssds_js_cr_add_package(json_gen, pkg);
     ssds_log(logDEBUG, "Added %s package as %d in order.\n", pkg, i+1);
  }
  ssds_log(logDEBUG, "Loop is done.\n");
  
  char* repo_output;
  ssds_log(logDEBUG, "Generating output message with repo info to server.\n");
  repo_output = ssds_js_cr_to_string(json_gen);
  ssds_log(logDEBUG, "Message generated.\n\n%s\n\n", repo_output);

  /***********************************************************/
  /* Sending repo info to server                             */
  /***********************************************************/
  ssds_log(logMESSAGE, "Sending message with repo info to server.\n");
  write(comm_sock, repo_output, strlen(repo_output));
  ssds_log(logDEBUG, "Message sent.\n");

  return OK;
}

int ssds_check_repo(int socket, char **message)
{
  char *buffer = sock_recv(socket);
  SsdsJsonRead *json = ssds_js_rd_init();
  int rc = -1;

  ssds_js_rd_parse(buffer, json);

  rc = ssds_js_rd_get_code(json);
      
  if(rc != ANSWER_OK)
  {
     *message = ssds_js_rd_get_message(json);
  }

  return rc;
}

int ssds_answer_process(int socket, int action)
{
  ssds_log(logMESSAGE, "Reading answer from server.\n");

  char *buf = sock_recv(socket);
  SsdsJsonRead *json = ssds_js_rd_init();
  int rc = -1;

  ssds_log(logDEBUG, "Checking answer.\n");

  if(buf == NULL)
  {
    ssds_log(logERROR, "Error while recieving data\n");
    return NETWORKING_ERROR;
  }
  ssds_log(logDEBUG, "Answer is OK.\n\n%s\n\n", buf);

  // parse response
  ssds_log(logDEBUG, "Parsing answer.\n");
  
  if(!ssds_js_rd_parse(buf, json))
  {
     ssds_log(logERROR, "Error while parsing answer from the server\n");
     return JSON_ERROR;
  }
  
  rc = ssds_js_rd_get_code(json);

  if(rc == ANSWER_WARNING)
  {
     ssds_log(logWARNING,"%s\n", ssds_js_rd_get_message(json));
     return rc;
  }

  if(rc == ANSWER_ERROR)
  {
     ssds_log(logERROR,"%s\n", ssds_js_rd_get_message(json));
     return rc;
  }

  int num_pkg;

  switch(action)
  {
     case GET_INSTALL: num_pkg = ssds_js_rd_get_count(json, "install_pkgs");
		       break;
     case GET_UPDATE:  num_pkg = ssds_js_rd_get_count(json, "update_pkgs");
                       break;
     case GET_ERASE:   num_pkg = ssds_js_rd_get_count(json, "erase_pkgs");
                       break;
     default: ssds_log(logWARNING, "Unsupported type of action.\n");
	      return ACTION_ERROR;
  }

  for(int id_app = 0; id_app < num_pkg; id_app++){

        ssds_log(logDEBUG, "Parse init.\n");
        SsdsJsonAnswer* answer_from_srv = ssds_js_rd_answer_init();
        ssds_log(logDEBUG, "Parse answer.\n");

        ssds_js_rd_parse_answer(answer_from_srv, json, id_app);

        ssds_log(logDEBUG, "Answer parsed.\n");

        
	GSList *package_install_list = NULL,
               *package_update_list = NULL,
               *package_erase_list = NULL;

	if(action != GET_ERASE)
        {
          /***********************************************************/
          /* Downloading packages part                               */
          /***********************************************************/

          ssds_log(logDEBUG, "Begin downloading part.\n");
          ssds_log(logMESSAGE, "Working on package: %s.\n", answer_from_srv->name);

          // required variables for downloading
          gboolean return_status;
          LrHandle *handler;
          LrPackageTarget *target;
          GError *error = NULL;

          for(guint i = 0; i < g_slist_length(answer_from_srv->pkgList); i++){
             SsdsJsonInstall* inst = (SsdsJsonInstall*)g_slist_nth_data(answer_from_srv->pkgList, i);
             ssds_log(logMESSAGE, "Downloading preparation for package: %s\n", inst->pkg_name);

             ssds_log(logDEBUG, "Downloading preparation.\n");
             handler = lr_handle_init();
             ssds_log(logDEBUG, "Download handler initied.\n");
             lr_handle_setopt(handler, NULL, LRO_METALINKURL, inst->metalink);
             ssds_log(logDEBUG, "Array of URLs is set.\n");
             lr_handle_setopt(handler, NULL, LRO_REPOTYPE, LR_YUMREPO);
             ssds_log(logDEBUG, "Repo type is set.\n");
             lr_handle_setopt(handler, NULL, LRO_PROGRESSCB, progress_callback);
             ssds_log(logDEBUG, "Progress callback is set.\n");

             // Prepare list of target
             target = lr_packagetarget_new_v2(handler, inst->pkg_loc, DOWNLOAD_TARGET_INSTALL,
                                              LR_CHECKSUM_UNKNOWN, NULL, 0, inst->base_url, TRUE,
                                              progress_callback, inst->pkg_name, end_callback, NULL, &error);
             package_install_list = g_slist_append(package_install_list, target);
          }

          // Download all packages        
          ssds_log(logMESSAGE, "Downloading packages.\n");
          return_status = lr_download_packages(package_install_list, LR_PACKAGEDOWNLOAD_FAILFAST, &error);
          
          if(!return_status || error != NULL){
            ssds_log(logERROR, "%d: %s\n", error->code, error->message);
            g_error_free(error);
            return DOWNLOAD_ERROR;
          }
        
          ssds_log(logMESSAGE, "All packages were downloaded successfully.\n");
        }
	/*********************************************************/
        /* Installing / Updating / Erasing packages              */
        /*********************************************************/

        // required variables for rpmlib
        rpmts ts;

        rpmReadConfigFiles(NULL, NULL);
        ts = rpmtsCreate();
        rpmtsSetRootDir(ts, NULL);


        if(package_install_list != NULL)
        {
          ssds_log(logMESSAGE, "Installing packages.\n");
          for(GSList *elem = package_install_list; elem; elem = g_slist_next(elem))
          {
              LrPackageTarget *target = (LrPackageTarget *)elem->data;

              if(!target->err)
              {
                ssds_add_to_transaction(ts, target->local_path, SSDS_INSTALL);
              }else{
                  ssds_log(logERROR, "Package Error: %s\n", target->err);
		  rc = INSTALL_ERROR;
		  goto rpmEnd;
              }
          }
        } 

	if(package_update_list != NULL) 
        {
          ssds_log(logMESSAGE, "Updating packages.\n");
          for(GSList *elem = package_update_list; elem; elem = g_slist_next(elem))
          {
              LrPackageTarget *target = (LrPackageTarget *)elem->data;

              if(!target->err)
              {
                ssds_add_to_transaction(ts, target->local_path, SSDS_UPDATE);
              }else{
                  ssds_log(logERROR, "Package Error: %s\n", target->err);
		  rc = UPDATE_ERROR;
		  goto rpmEnd;
              }
          }
        }

        if(package_erase_list != NULL)
        {
          ssds_log(logMESSAGE, "Erasing packages.\n");
          for(GSList *elem = package_erase_list; elem; elem = g_slist_next(elem))
          {
              rc = ssds_add_to_erase(ts, (char *)elem);
              if(rc != OK){
		ssds_log(logERROR, "Unable to erase requested package.\n");
                rc = ERASE_ERROR;
		goto rpmEnd;
              }
          }
        }
        rpmprobFilterFlags flag = 0;

        int nf = 0;

        nf |= INSTALL_LABEL | INSTALL_HASH;
        rpmtsSetNotifyCallback(ts, rpmShowProgress,(void *) nf);

        rc = rpmtsRun(ts, NULL, flag);
        if(rc == OK){
	  switch(action){

		case GET_INSTALL:
                     ssds_log(logMESSAGE, "All packages was installed correctly.\n\n\tPackage %s is ready to use.\n\n",answer_from_srv->name);
		     break;

                case GET_UPDATE:
                     ssds_log(logMESSAGE, "All packages was updated correctly.\n\n\tPackage %s is ready to use.\n\n",answer_from_srv->name);
                     break;
                case GET_ERASE:
                     ssds_log(logMESSAGE, "All packages was erased correctly.\n");
                     break;

          }

        }else{
	  switch(action){

                case GET_INSTALL:        
                     ssds_log(logWARNING, "Installation of package %s end with code %d.\n",answer_from_srv->name, rc);
                     break;

                case GET_UPDATE:
                     ssds_log(logWARNING, "Updating of package %s end with code %d.\n",answer_from_srv->name, rc);
                     break;
                case GET_ERASE:
                     ssds_log(logWARNING, "Erasing of package %s end with code %d.\n",answer_from_srv->name, rc);
                     break;

          }
        }

  rpmEnd:
        rpmtsClean(ts);
        rpmtsFree(ts);
        g_slist_free_full(package_install_list, (GDestroyNotify) lr_packagetarget_free);
        g_slist_free_full(package_update_list, (GDestroyNotify) lr_packagetarget_free);
	//g_slist_free_full(package_erase_list, (GDestroyNotify) ssds_free);
        ssds_free(answer_from_srv);
  }

  return rc;
}
