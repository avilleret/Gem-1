////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 2015 - Antoine Villeret
//    Copyright (c) 2010-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////
#include "Gem/GemConfig.h"

#include "plugins/texinBase.h"
#include "Gem/RTE.h"
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include <iostream>

#if 0
# define debugPost post
#else
# define debugPost
#endif

#include <pthread.h>

#ifdef _WIN32
# include <winsock2.h>
#endif

using namespace gem::plugins;

/**
 * texin capturing states
 *
 *  state                user-pov            system-pov
 * ----------------------------------------------------
 * is device open?       m_haveTexin         m_haveTexin
 * is device streaming?  m_pimpl->shouldrun  m_capturing
 * is thread running     (opaque)            m_pimpl->running
 *
 */

class texinBase :: PIMPL {
public:
  /* interfaces */
  // the list of provided device-classes
  std::vector<std::string>m_providers;

  /* threading */
  bool threading;
  pthread_t thread;
  pthread_mutex_t**locks;
  unsigned int numlocks;

  bool asynchronous;
  pthread_cond_t*condition_cond;
  pthread_mutex_t*condition_mutex;

  pthread_cond_t*runCondition;
  pthread_mutex_t*runMutex;

  unsigned int timeout;

  bool cont;
  bool running;

  bool shouldrun; /* we should be capturing */

  const std::string name;

  PIMPL(const std::string name_, unsigned int locks_, unsigned int timeout_) :
    threading(locks_>0),
#ifndef HAVE_PTW32_HANDLE_T
    thread(0),
#endif
    locks(NULL),
    numlocks(0),
    asynchronous(true), condition_cond(NULL), condition_mutex(NULL),
    runMutex(NULL), runCondition(NULL),
    timeout(timeout_),
    cont(true),
    running(false),
    shouldrun(false),
    name(name_)
  {
    if(locks_>0) {
      numlocks=locks_;
      locks=new pthread_mutex_t*[numlocks];
      unsigned int i=0;
      for(i=0; i<locks_; i++)
        locks[i]=NULL;

      condition_mutex=new pthread_mutex_t;
      condition_cond =new pthread_cond_t;

      pthread_mutex_init(condition_mutex, NULL);
      pthread_cond_init(condition_cond, NULL);


      runMutex=new pthread_mutex_t;
      runCondition =new pthread_cond_t;

      pthread_mutex_init(runMutex, NULL);
      pthread_cond_init(runCondition, NULL);
    }
  }
  ~PIMPL(void) {
    cont=false;
    lock_delete();
    delete[]locks;
    locks=NULL;

    doThaw();

    if(condition_mutex) {
      pthread_mutex_destroy(condition_mutex);
      delete condition_mutex;
    }
    if(condition_cond) {
      pthread_cond_destroy(condition_cond);
      delete condition_cond;
    }
    if(runMutex) {
      pthread_mutex_destroy(runMutex);
      delete runMutex;
    }
    if(runCondition) {
      pthread_cond_destroy(runCondition);
      delete runCondition;
    }

  }

  void lock(unsigned int i) {
    //    post("lock %d?\t%d", i, numlocks);

    if(i<numlocks && locks[i]) {
      pthread_mutex_lock(locks[i]);
    }
  }
  void unlock(unsigned int i) {
    //      post("unlock %d? %d", i,numlocks);

    if(i<numlocks && locks[i]) {
      pthread_mutex_unlock(locks[i]);
    }
  }
  bool lock_new(void) {
    if(locks) {
      unsigned int i=0;
      for(i=0; i<numlocks; i++) {
        locks[i]=new pthread_mutex_t;
        if ( pthread_mutex_init(locks[i], NULL) < 0 ) {
          lock_delete();
          return false;
        }
      }
      return true;
    }
    return true;
  }
  void lock_delete(void) {
    if(locks) {
      unsigned int i=0;
      for(i=0; i<numlocks; i++) {
        if(locks[i]) {
          pthread_mutex_destroy(locks[i]);
          delete locks[i];
          locks[i]=NULL;
        }
      }
    }
  }

  void doFreeze(void) {
    if(condition_mutex && condition_cond) {
      pthread_mutex_lock  ( condition_mutex );
       pthread_cond_wait  ( condition_cond, condition_mutex );
      pthread_mutex_unlock( condition_mutex );
    }
  }

  void freeze(void) {
    if(asynchronous)return;
    doFreeze();
  }

  void doThaw(void) {
    if(condition_mutex && condition_cond) {
      pthread_mutex_lock  (condition_mutex);
      pthread_cond_signal(condition_cond );
      pthread_mutex_unlock(condition_mutex);
    }
  }

  void thaw(void) {
    if(asynchronous)return;
    doThaw();
  }

  static void*threadfun(void*you) {
    texinBase*me=(texinBase*)you;
    pixBlock*pix=NULL;
    me->m_pimpl->cont=true;
    me->m_pimpl->running=true;

    if(me->m_pimpl->runMutex) {
      pthread_mutex_lock(me->m_pimpl->runMutex);
      if(me->m_pimpl->runCondition)
        pthread_cond_signal(me->m_pimpl->runCondition);
      pthread_mutex_unlock(me->m_pimpl->runMutex);
    }

    while(me->m_pimpl->cont) {
      if(!me->grabFrame()) {
        break;
      }
      me->m_pimpl->freeze();
    }
    me->m_pimpl->running=false;

    return NULL;
  }

  bool setAsynchronous(bool cont) {
    bool old=asynchronous;
    asynchronous=cont;
    if(asynchronous){
      doThaw();
    }
    return old;
  }
};


/////////////////////////////////////////////////////////
//
// pix_texinLinux
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
texinBase :: texinBase(const std::string name, unsigned int locks) :
  m_capturing(false), m_haveTexin(false),
  m_width(64), m_height(64),
  m_reqFormat(GL_RGBA_GEM),
  m_devicename(std::string("")), m_devicenum(0),
  m_pimpl(new PIMPL(name.empty()?std::string("<unknown>"):name, locks, 0))
{
  if(!name.empty()) {
    provide(name);
  }
}
texinBase :: texinBase(const std::string name) :
  m_capturing(false), m_haveTexin(false),
  m_width(64), m_height(64),
  m_reqFormat(GL_RGBA_GEM),
  m_devicename(std::string("")), m_devicenum(0),
  m_pimpl(new PIMPL(name.empty()?std::string("<unknown>"):name, 1, 0))
{
  if(!name.empty()) {
    provide(name);
  }
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
texinBase :: ~texinBase()
{
  if(m_pimpl)delete m_pimpl; m_pimpl=NULL;
}
/////////////////////////////////////////////////////////
// open/close
//
/////////////////////////////////////////////////////////
bool texinBase :: open(gem::Properties&props)
{
  debugPost("open: %d -> %d", m_haveTexin, m_capturing);
  if(m_haveTexin)close();
  m_haveTexin=openDevice(props);
  return m_haveTexin;
}
void texinBase :: close()
{
  debugPost("close: %d -> %d", m_capturing, m_haveTexin);
  if(m_capturing)stop();
  if(m_haveTexin)closeDevice();
  m_pimpl->shouldrun=false;
  m_haveTexin=false;
}
/////////////////////////////////////////////////////////
// start/stop
//
/////////////////////////////////////////////////////////
bool texinBase :: start()
{
  debugPost("start: %d -> %d", m_haveTexin, m_capturing);
  if(!m_haveTexin)return false;
  if(m_capturing)stop();
  m_capturing=startTransfer();
  if(m_capturing)
    startThread();

  m_pimpl->shouldrun=true;
  return m_capturing;
}
bool texinBase :: stop()
{
  debugPost("stop(%d): %d -> %d", m_pimpl->shouldrun, m_capturing, m_haveTexin);
  bool running=m_pimpl->shouldrun;
  m_pimpl->shouldrun=false;
  if(!m_haveTexin)return false;
  if(m_capturing) {
    stopThread();
    stopTransfer();
  }

  m_capturing=false;
  return running;
}

/////////////////////////////////////////////////////////
// resetDevice
//
/////////////////////////////////////////////////////////
bool texinBase :: reset()
{
  return(false);
}

/////////////////////////////////////////////////////////
// startTransfer
//
/////////////////////////////////////////////////////////
bool texinBase :: restartTransfer()
{
  debugPost("restartTransfer");
  bool running=stop();
  if(running)return start();

  return false;
}

bool texinBase :: startThread() {
  debugPost("startThread %d", m_pimpl->running);
  if(m_pimpl->running) {
    stopThread();
  }

  if(m_pimpl->threading) {
    if(!m_pimpl->lock_new())return false;

    if(m_pimpl->runMutex)
      pthread_mutex_lock(m_pimpl->runMutex);

    pthread_create(&m_pimpl->thread,
                   0,
                   m_pimpl->threadfun,
                   this);
    if(m_pimpl->runMutex) {
      if(m_pimpl->runCondition) {
        pthread_cond_wait(m_pimpl->runCondition, m_pimpl->runMutex);
      }
      pthread_mutex_unlock(m_pimpl->runMutex);
    }
    while(!m_pimpl->running)
      usleep(10);

    return true;
  }
  return false;
}
bool texinBase :: stopThread(int timeout) {
  int i=0;
  if(!m_pimpl->threading)return true;

  debugPost("stopThread: %d", timeout);

  m_pimpl->cont=false;

  m_pimpl->thaw();
  if(timeout<0)timeout=m_pimpl->timeout;

  if(timeout>0) {
    while(m_pimpl->running) {
      usleep(10);
      i+=10;
      if(i>timeout) {
        return false;
      }
    }
  } else {
    while(m_pimpl->running) {
      usleep(10);
      i+=10;
      if(i>1000000) {
        post("waiting for texin grabbing thread to terminate...");
        i=0;
      }
      m_pimpl->thaw();
    }
    //pthread_join(m_pimpl->thread, NULL);
  }

  m_pimpl->lock_delete();
  return true;
}
void texinBase :: lock(unsigned int id) {
  m_pimpl->lock(id);
}
void texinBase :: unlock(unsigned int id) {
  m_pimpl->unlock(id);
}
void texinBase :: usleep(unsigned long usec) {
  struct timeval sleep;
  long usec_ = usec%1000000;
  long sec_=0;
  //  long  sec_ = usec\1000000;
  sleep.tv_sec=sec_;
  sleep.tv_usec=usec_;
  select(0,0,0,0,&sleep);
}

GLuint texinBase :: getFrame(void) {
  GLuint tex=m_textureObj;
  if(!(m_haveTexin && m_capturing))return 0;
  /*
  if(m_pimpl->threading) {
     // get from thread
    if(!m_pimpl->running){
      pix=NULL;
    }
  } else {
    // no thread, grab it directly
    if(!grabFrame()) {
      m_capturing=false;
      pix=NULL;
    }
  }
  */
  lock();
  return tex;
}


void texinBase :: releaseFrame(void) {
  unlock();
  m_pimpl->thaw();
}

/////////////////////////////////////////////////////////
// set the color-space
bool texinBase :: setColor(int d){
  post("setting the color-space is not supported by this OS/device");
  return false;
}

/////////////////////////////////////////////////////////
// open a dialog for the settings
/*
bool texinBase :: dialog(std::vector<std::string>dialognames){
  return false;
}
std::vector<std::string>texinBase :: dialogs(void) {
  std::vector<std::string>result;
  return result;
}
*/
std::vector<std::string>texinBase :: enumerate(void) {
  std::vector<std::string>result;
  return result;
}

////////////////////
// set the texin device
bool texinBase :: setDevice(int d)
{
  m_devicename.clear();
  if (d==m_devicenum)return true;
  m_devicenum=d;
  return true;
}
bool texinBase :: setDevice(const std::string name)
{
  m_devicenum=-1;
  m_devicename=name;
  return true;
}

const std::string texinBase :: getName() {
  return m_pimpl->name;
}




/////////////////////////////////////////////////////////
// query whether this backend provides a certain type of texin decoding, e.g. "dv"
bool texinBase :: provides(const std::string name) {
  if(!m_pimpl)return false;
  unsigned int i;
  for(i=0; i<m_pimpl->m_providers.size(); i++)
    if(name == m_pimpl->m_providers[i])return true;

  return false;
}
std::vector<std::string>texinBase :: provides() {
  std::vector<std::string>result;
  if(m_pimpl) {
    unsigned int i;
    for(i=0; i<m_pimpl->m_providers.size(); i++)
      result.push_back(m_pimpl->m_providers[i]);
  }
  return result;
}


/////////////////////////////////////////////////////////
// remember that this backend provides a certain type of texin decoding, e.g. "dv"
void texinBase :: provide(const std::string name) {
  if(!m_pimpl)return;
  if(!provides(name)) {
    m_pimpl->m_providers.push_back(name);
  }
}

bool texinBase :: enumProperties(gem::Properties&readable,
			     gem::Properties&writeable)
{
  readable.clear();
  writeable.clear();
  return false;
}

void texinBase :: setProperties(gem::Properties&props) {
  // nada

  std::vector<std::string> keys=props.keys();
  unsigned int i=0;
  for(i=0; i<keys.size(); i++) {
    enum gem::Properties::PropertyType typ=props.type(keys[i]);
    std::cerr  << "key["<<keys[i]<<"]: "<<typ<<" :: ";
    switch(typ) {
    case (gem::Properties::NONE):
      props.erase(keys[i]);
      break;
    case (gem::Properties::DOUBLE):
      std::cerr << gem::any_cast<double>(props.get(keys[i]));
      break;
    case (gem::Properties::STRING):
      std::cerr << "'" << gem::any_cast<std::string>(props.get(keys[i])) << "'";
      break;
    default:
      std::cerr << "<unknown:" << props.get(keys[i]).get_type().name() << ">";
      break;
    }
  }
  std::cerr << std::endl;
}

void texinBase :: getProperties(gem::Properties&props) {
  // nada
  std::vector<std::string>keys=props.keys();
  unsigned int i=0;
  for(i=0; i<keys.size(); i++) {
    gem::any unset;
    props.set(keys[i], unset);
  }
}


bool texinBase :: grabAsynchronous(bool fast) {
  return m_pimpl->setAsynchronous(fast);
}

bool texinBase :: isThreadable(void) {
  return (m_pimpl->numlocks>0);
}