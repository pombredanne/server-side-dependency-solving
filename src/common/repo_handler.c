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

#include "repo_handler.h"
#include "log_handler.h"

LocalRepoInfo* repo_parse_init() { //this part uses librepo library to parse .repo files - repoconf module was created by TMlcoch
    LocalRepoInfo* new;
    
    new = (LocalRepoInfo*)malloc(sizeof(LocalRepoInfo));
    new->repoHandler = lr_yum_repoconfs_init(); 
    return new;
}

  
int parse_default_repo(LocalRepoInfo* repo) {
    GError *err = NULL;
    gboolean ret;

    ret = lr_yum_repoconfs_load_dir(repo->repoHandler, "/etc/yum.repos.d/", &err);
    if(!ret) {
        rds_log(logERROR, "Unable to load default repo\n");
        return 0;
    }
    return 1; 
}

void get_repo_urls(LocalRepoInfo* repo, JsonCreate* json, char* arch, char* release) {
    GError* err = NULL;
    GSList* list;

    rds_log(logDEBUG,"Getting repoconfs list.\n");
    list = lr_yum_repoconfs_get_list(repo->repoHandler, &err);
    for(unsigned int i = 0; i < g_slist_length(list); i++) {
        char** url;
        char* name = NULL;
        short type = 0;
        void * val;
        int k = 0;
        LrYumRepoConf * conf;
        
        url = (char **)malloc(1*sizeof(char*));

        rds_log(logDEBUG,"Getting repo configuration.\n");   
        conf = (LrYumRepoConf*)g_slist_nth_data(list, i);

        rds_log(logDEBUG,"Getting repo info.\n"); 
        lr_yum_repoconf_getinfo(conf, &err, LR_YRC_ENABLED, &val);

        if((long int)val) {
            err = NULL;
            if(lr_yum_repoconf_getinfo(conf, &err, LR_YRC_ID, &val) != 0) {
                rds_log(logDEBUG,"Getting repo name.\n"); 
                name = (char*)val;
                rds_log(logDEBUG,"Name: %s.\n",name); 
            }

            err = NULL;
            if(lr_yum_repoconf_getinfo(conf, &err, LR_YRC_MIRRORLIST, &val) != 0) {
                rds_log(logDEBUG,"Getting mirrorlist url.\n"); 
                url[0] = (char*)val;
                k = 1;
                rds_log(logDEBUG,"URL: %s.\n", url[0]); 
                type = MIRRORLIST;
            }

            err = NULL;
            if(lr_yum_repoconf_getinfo(conf, &err, LR_YRC_METALINK, &val) != 0) {
                rds_log(logDEBUG,"Getting metalink url.\n"); 
                url[0] = (char*)val;
                k = 1;
                rds_log(logDEBUG,"URL: %s.\n", url[0]); 
                type = METALINK;
            }

            err = NULL;
            //void ** val2;
            if(lr_yum_repoconf_getinfo(conf, &err, LR_YRC_BASEURL, &val) != 0) {
                rds_log(logDEBUG,"Getting base urls.\n");  
                free(url);
                url = (char**)val; 
                while(url[k])
                k++;
                type = BASEURL;
            }

            LrUrlVars *list;
            char **url_subst_list;
            char* url_copy, *url_subst;
            
            rds_log(logDEBUG,"Setting url var - releaseserver.\n"); 
            list =  lr_urlvars_set(NULL, "releasever", release); //"3.18.6-200.fc21"
            rds_log(logDEBUG,"Setting url var - basearch.\n"); 
            list = lr_urlvars_set(list, "basearch", arch);

            rds_log(logDEBUG,"Getting URL size.\n"); 
            rds_log(logDEBUG,"Size: %d.\n",k); 

            url_subst_list = (char**)malloc((k+1)*sizeof(char*));
            
            rds_log(logDEBUG,"Setting urls into json.\n");      
            for(int j = 0; j < k; j++) { 
                url_copy = strdup(url[j]);
                url_subst = lr_url_substitute(url_copy, list);
                url_subst_list[j] = strdup(url_subst);
                free(url_copy); //created by strdup - not in GC, needs to be freed manually
                free(url_subst); //created by strdup - not in GC, needs to be freed manually
            }
            url_subst_list[k] = NULL;

            js_cr_add_repo(json,url_subst_list, name, type, k);
            free(url);      
            for(int j = 0; j <= k; j++)
                free(url_subst_list[j]); //created by strdup - not in GC, needs to be freed manually

            free(url_subst_list);
        }
    }
}
