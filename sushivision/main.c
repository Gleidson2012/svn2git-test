/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include <pthread.h>
#include <dlfcn.h>
#include "internal.h"
#include "sushi-gtkrc.h"

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mc = PTHREAD_COND_INITIALIZER;
sig_atomic_t _sushiv_exiting=0;
static int wake_pending = 0;
static int num_threads;

static int instances=0;
static sushiv_instance_t **instance_list;

void _sushiv_wake_workers(){
  if(instances){
    pthread_mutex_lock(&m);
    wake_pending = num_threads;
    pthread_cond_broadcast(&mc);
    pthread_mutex_unlock(&m);
  }
}

void _sushiv_clean_exit(int sig){
  _sushiv_exiting = 1;
  _sushiv_wake_workers();

  //signal(sig,SIG_IGN);
  if(sig!=SIGINT)
    fprintf(stderr,
            "\nTrapped signal %d; exiting!\n",sig);

  gtk_main_quit();
}

static int num_proccies(){
  FILE *f = fopen("/proc/cpuinfo","r");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  int num=1,arg;

  if (f == NULL) return 1;
  while ((read = getline(&line, &len, f)) != -1) {
    if(sscanf(line,"processor : %d",&arg)==1)
      if(arg+1>num)num=arg+1;
  }
  if (line)
    free(line);

  fprintf(stderr,"Number of processors: %d\n",num);
  fclose(f);
  return num;
}

static void *worker_thread(void *dummy){
  /* set up temporary working space for function rendering; this saves
     continuously recreating it in the loop below */
  _sushiv_bythread_cache **c; // [instance][panel]
  int i,j;
  
  c = calloc(instances,sizeof(*c));
  for(j=0;j<instances;j++){
    sushiv_instance_t *s = instance_list[j];
    c[j] = calloc(s->panels,sizeof(**c));
  }

  while(1){
    if(_sushiv_exiting)break;
    
    // look for work
    {
      int flag=0;
      // by instance
      for(j=0;j<instances;j++){
	sushiv_instance_t *s = instance_list[j];
				 
	for(i=0;i<s->panels;i++){
	  sushiv_panel_t *p = s->panel_list[i];

	  if(_sushiv_exiting)break;

	  // pending remap work?
	  gdk_threads_enter();
	  if(p && p->private && p->private->realized && p->private->graph){

	    // pending computation work?
	    if(p->private->plot_active){
	      spinner_set_busy(p->private->spinner);
	      flag |= p->private->compute_action(p,&c[j][i]); // may drop lock internally
	    }
	    
	    if(p->private->map_active){
	      int ret = 1;
	      while(ret){ // favor completing remaps over other ops
		spinner_set_busy(p->private->spinner);
		flag |= ret = p->private->map_action(p,&c[j][i]); // may drop lock internally
		if(!p->private->map_active)
		  set_map_throttle_time(p);
	      }
	    }
	    
	    // pending legend work?
	    if(p->private->legend_active){
	      spinner_set_busy(p->private->spinner);
	      flag |= p->private->legend_action(p); // may drop lock internally
	    }
	    
	    if(!p->private->plot_active &&
	       !p->private->legend_active &&
	       !p->private->map_active)
	      spinner_set_idle(p->private->spinner);
	  }
	  gdk_threads_leave ();
	}
      }
      if(flag==1)continue;
    }
    
    // nothing to do, wait
    pthread_mutex_lock(&m);
    if(!wake_pending)
      pthread_cond_wait(&mc,&m);
    else
      wake_pending--;
    pthread_mutex_unlock(&m);
  }
  
  pthread_mutex_unlock(&m);
  return 0;
}

static char * gtkrc_string(){
  return _SUSHI_GTKRC_STRING;
}

static void sushiv_realize_instance(sushiv_instance_t *s){
  int i;
  for(i=0;i<s->panels;i++)
    _sushiv_realize_panel(s->panel_list[i]);
  for(i=0;i<s->panels;i++)
    if(s->panel_list[i])
      s->panel_list[i]->private->request_compute(s->panel_list[i]);
}

static void sushiv_realize_all(void){
  int i;
  for(i=0;i<instances;i++)
    sushiv_realize_instance(instance_list[i]);
}

/* externally visible interface */

sushiv_instance_t *sushiv_new_instance(int number, char *name) {
  sushiv_instance_t *ret;

  if(number<0){
    fprintf(stderr,"Instance number must be >= 0\n");
    errno = -EINVAL;
    return NULL;
  }
  
  if(number<instances){
    if(instance_list[number]!=NULL){
      fprintf(stderr,"Instance number %d already exists\n",number);
      errno = -EINVAL;
      return NULL;
    }
  }else{
    if(instances == 0){
      instance_list = calloc (number+1,sizeof(*instance_list));
    }else{
      instance_list = realloc (instance_list,(number+1) * sizeof(*instance_list));
      memset(instance_list + instances, 0, sizeof(*instance_list)*(number+1 - instances));
    }
    instances = number+1; 
  }

  ret = instance_list[number] = calloc(1, sizeof(**instance_list));
  ret->private = calloc(1,sizeof(*ret->private));
  if(name)
    ret->name = strdup(name);

  return ret;
}

char *appname = NULL;
char *filename = NULL;
char *filebase = NULL;
char *dirname = NULL;
char *cwdname = NULL;

int main (int argc, char *argv[]){
  int ret;

  num_threads = num_proccies();

  gtk_mutex_fixup();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  gdk_threads_init ();
  gtk_rc_parse_string(gtkrc_string());
  gtk_rc_add_default_file("sushi-gtkrc");

  ret = sushiv_submain(argc,argv);
  if(ret)return ret;
  
  gdk_threads_enter();
  sushiv_realize_all();
  gtk_button3_fixup();
  
  appname = g_get_prgname ();
  cwdname = getcwd(NULL,0);
  dirname = strdup(cwdname);
  if(argc>1){
    // file to load specified on commandline
    filebase = strdup(argv[argc-1]);
    char *base = strrchr(filebase,'/');
    
    // filebase may include a path; pull it off and apply it toward dirname
    if(base){
      base[0] = '\0';
      char *dirbit = strdup(filebase);
      filebase = base+1;
      if(g_path_is_absolute(dirbit)){
	// replace dirname
	free(dirname);
	dirname = dirbit;
      }else{
	// append to dirname
	char *buf;
	asprintf(&buf,"%s/%s",dirname,dirbit);
	free(dirname);
	dirname = buf;
      }
    }
    
    // load the state file

  }else{
    if(appname){
      char *base = strrchr(appname,'/');
      if(!base) 
	base = appname;
      else
	base++;

      asprintf(&filebase, "%s.sushi",base);
    }else
      filebase = strdup("default.sushi");
  }
  asprintf(&filename,"%s/%s",dirname,filebase);

  {
    pthread_t dummy;
    int threads = num_threads;
    while(threads--)
      pthread_create(&dummy, NULL, &worker_thread,NULL);
  }

  signal(SIGINT,_sushiv_clean_exit);
  //signal(SIGSEGV,_sushiv_clean_exit);


  gtk_main ();
  gdk_threads_leave();
  
  {
    int (*optional_exit)(void) = dlsym(RTLD_DEFAULT, "sushiv_atexit");
    if(optional_exit)
      return optional_exit();
  }

  return 0;
}

static int save_instance(sushiv_instance_t *s, xmlNodePtr root){
  if(!s) return 0;
  char buffer[80];
  int i, ret=0;

  xmlNodePtr instance = xmlNewChild(root, NULL, (xmlChar *) "instance", NULL);

  snprintf(buffer,sizeof(buffer),"%d",s->number);
  xmlNewProp(instance, (xmlChar *)"number", (xmlChar *)buffer);
  if(s->name)
    xmlNewProp(instance, (xmlChar *)"name", (xmlChar *)s->name);
  
  // dimension values are independent of panel
  for(i=0;i<s->dimensions;i++)
    ret|=_save_dimension(s->dimension_list[i], instance);
  
  // objectives have no independent settings

  // panel settings (by panel)
  for(i=0;i<s->panels;i++)
    ret|=_save_panel(s->panel_list[i], instance);

  return ret;
}

int save_main(){
  xmlDocPtr doc = NULL;
  xmlNodePtr root_node = NULL;
  int i, ret=0;

  LIBXML_TEST_VERSION;

  doc = xmlNewDoc((xmlChar *)"1.0");
  root_node = xmlNewNode(NULL, (xmlChar *)appname);
  xmlDocSetRootElement(doc, root_node);

  // save each instance 
  for(i=0;i<instances;i++)
   ret |= save_instance(instance_list[i],root_node);

  xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

  xmlFreeDoc(doc);
  xmlCleanupParser();

  return ret;
}
