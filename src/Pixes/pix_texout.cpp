////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-1998 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    Copyright (c) 2015 Antoine Villeret
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "Gem/GemConfig.h"

#ifndef GEM_TEXOUTBACKEND

#include "pix_texout.h"
#include "Gem/State.h"
#include "Gem/Exception.h"
#include "plugins/PluginFactory.h"

#include "RTE/Symbol.h"

#include <algorithm>

CPPEXTERN_NEW_WITH_GIMME(pix_texout);


#if 0
# define MARK std::cerr << __FILE__<<":"<<__LINE__<<" ("<<__FUNCTION__<<")"<<std::endl
#else
# define MARK
#endif

/////////////////////////////////////////////////////////
//
// pix_texout
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_texout :: pix_texout(int argc, t_atom*argv) :
  m_texoutHandle(NULL), m_driver(-1), m_running(UNKNOWN), m_infoOut(NULL)
{
  gem::PluginFactory<gem::plugins::texout>::loadPlugins("texout");
  std::vector<std::string>ids=gem::PluginFactory<gem::plugins::texout>::getIDs();

  addHandle(ids, "v4l2");
  addHandle(ids, "v4l");
  addHandle(ids, "dv4l");
  MARK;
  addHandle(ids);

  MARK;
  m_infoOut = outlet_new(this->x_obj, 0);

  /*
   * calling driverMess() would immediately startTransfer();
   * we probably don't want this in initialization phase
   */
  MARK;
  if(m_texoutHandles.size()>0) {
    m_driver=-1;
  } else {
    error("no texout backends found!");
  }
MARK;
  std::string dev=gem::RTE::Symbol(argc, argv);
MARK;
  if(!dev.empty())
    deviceMess(dev);
MARK;

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_texout :: ~pix_texout(){
  /* clean up all texout handles;
   * the texout-handles have to stop the transfer themselves
   */
  unsigned int i=0;
  for(i=0; i<m_texoutHandles.size(); i++) {
    delete m_texoutHandles[i];
    m_texoutHandles[i]=NULL;
  }

  outlet_free(m_infoOut); m_infoOut=NULL;
}


/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
void pix_texout :: render(GemState *state){
  if (m_texoutHandle) {
    pixBlock*frame=m_texoutHandle->getFrame();
    //post("got frame: %p", frame);
    state->set(GemState::_PIX, frame);
  }
}

/////////////////////////////////////////////////////////
// startRendering
//
/////////////////////////////////////////////////////////
void pix_texout :: startRendering(){
  if(UNKNOWN==m_running)
    m_running=STARTED;

  if(m_texoutHandles.size()<1) {
    error("do texout for this OS");
    return;
  }

  if (!m_texoutHandle) {
    if(!restart()) {
      error("no valid texout backend found");
      return;
    }
    return;
  }

  verbose(1, "starting transfer");
  m_texoutHandle->start();
}

/////////////////////////////////////////////////////////
// stopRendering
//
/////////////////////////////////////////////////////////
void pix_texout :: stopRendering(){
  if (m_texoutHandle)m_texoutHandle->stop();
}

/////////////////////////////////////////////////////////
// postrender
//
/////////////////////////////////////////////////////////
void pix_texout :: postrender(GemState *state){
  state->set(GemState::_PIX, static_cast<pixBlock*>(NULL));

  if (m_texoutHandle)m_texoutHandle->releaseFrame();
}


/////////////////////////////////////////////////////////
// add backends
//
/////////////////////////////////////////////////////////
bool pix_texout :: addHandle( std::vector<std::string>available, std::string ID)
{
  unsigned int i=0;
  int count=0;

  std::vector<std::string>id;
  if(!ID.empty()) {
    // if requested 'cid' is in 'available' add it to the list of 'id's
    if(std::find(available.begin(), available.end(), ID)!=available.end()) {
      id.push_back(ID);
    } else {
      // request for an unavailable ID
      verbose(2, "backend '%s' unavailable", ID.c_str());
      return false;
    }
  } else {
    // no 'ID' given: add all available IDs
    id=available;
  }

  for(i=0; i<id.size(); i++) {
    MARK;
    std::string key=id[i];
    verbose(2, "trying to add '%s' as backend (%d)", key.c_str(), id.size());
  MARK;
    if(std::find(m_ids.begin(), m_ids.end(), key)==m_ids.end()) {
      // not yet added, do so now!
      gem::plugins::texout         *handle=NULL;
      startpost("backend #%d='%s'\t", m_texoutHandles.size(), key.c_str());
      try {
        handle=gem::PluginFactory<gem::plugins::texout>::getInstance(key);
      } catch (GemException&) {
      handle=NULL;
      }
      if(NULL==handle) {
        post("<--- DISABLED");
        continue;
      }
      std::vector<std::string>devs=handle->provides();
      if(devs.size()>0) {
        startpost(": ");
        unsigned int i=0;
        for(i=0; i<devs.size(); i++) {
          startpost("%s ", devs[i].c_str());
        }
      }
      endpost();

      m_ids.push_back(key);
      m_texoutHandles.push_back(handle);
      count++;
    MARK;
      verbose(2, "added backend#%d '%s' @ 0x%x", m_texoutHandles.size()-1, key.c_str(), handle);
    MARK;
  }
  MARK;
  }
MARK;
  return (count>0);
}


bool pix_texout::restart(void) {
  verbose(1, "restart");
  if(m_texoutHandle) {
    m_texoutHandle->stop();
    m_texoutHandle->close();
  }

  if(m_driver<0) {
    // auto mode
    unsigned int i=0;
    verbose(1, "trying to start driver automatically (%d)", m_running);
    for(i=0; i<m_texoutHandles.size(); i++) {
      if(m_texoutHandles[i]->open(m_writeprops)) {
        m_texoutHandle=m_texoutHandles[i];

        enumPropertyMess();

        if(STARTED==m_running) {
          m_texoutHandle->start();
        }
        return true;
      }
    }
  } else {
    // enforce selected driver
    verbose(1, "trying to start driver#%d (%d)", m_driver, m_running);
    m_texoutHandle=m_texoutHandles[m_driver];
    if(m_texoutHandle->open(m_writeprops)) {
      enumPropertyMess();
      if(STARTED==m_running)
  m_texoutHandle->start();
      return true;
    }
  }

  m_texoutHandle=NULL;
  return false;
}




#define WITH_TEXOUTHANDLES_DO(x) do {unsigned int _i=0; for(_i=0; _i<m_texoutHandles.size(); _i++)m_texoutHandles[_i]->x;} while(false)

/////////////////////////////////////////////////////////
// driverMess
//
/////////////////////////////////////////////////////////
void pix_texout :: driverMess(std::string s)
{
  if("auto"==s) {
    driverMess(-1);
    return;
  } else {
    unsigned int dev;
    for(dev=0; dev<m_texoutHandles.size(); dev++) {
      if(m_texoutHandles[dev]->provides(s)) {
        driverMess(dev);
        return;
      }
    }
  }
  error("could not find a backend for driver '%s'", s.c_str());
}
void pix_texout :: driverMess(int dev)
{
  if(dev>=0) {
    unsigned int udev=(unsigned int)dev;
  if(udev>=m_texoutHandles.size()){
    error("driverID (%d) must not exceed %d", udev, m_texoutHandles.size());
    return;
  }

    if(m_texoutHandle){
      m_texoutHandle->stop();
      m_texoutHandle->close();
    }
    m_texoutHandle=m_texoutHandles[udev];
    if(m_texoutHandle){
      if(m_texoutHandle->open(m_writeprops)) {
        enumPropertyMess();
        if(STARTED==m_running)
         m_texoutHandle->start();
      }
    }
  } else {
    post("automatic driver selection");
  }
  m_driver=dev;
}

void pix_texout :: driverMess() {
  // a little bit of info
  t_atom at;
  t_atom*ap=&at;
  if(m_texoutHandle) {
    post("current driver: '%s'", m_texoutHandle->getName().c_str());

    SETSYMBOL(ap+0, gensym(m_texoutHandle->getName().c_str()));
    outlet_anything(m_infoOut, gensym("currentdriver"), 1, ap);
  }
  if(m_texoutHandles.size()>0) {
    unsigned int i=0;
    SETFLOAT(ap+0, m_texoutHandles.size());
    outlet_anything(m_infoOut, gensym("drivers"), 1, ap);
    post("available drivers:");
    for(i=0; i<m_texoutHandles.size(); i++) {
      gem::plugins::texout*handle= m_texoutHandles[i];
      if(NULL==handle)continue;
      startpost("\t'%s' provides ", handle->getName().c_str());
      std::vector<std::string>backends=handle->provides();
      unsigned int j=0;

      unsigned int asize=0;
      if(backends.size()>0) {
        asize=backends.size();
        ap=new t_atom[asize];
      } else {
        asize=1;
        ap=new t_atom[1];
        SETSYMBOL(ap, gensym(handle->getName().c_str()));
      }

      for(j=0; j<backends.size(); j++) {
        startpost("'%s' ", backends[j].c_str());
        SETSYMBOL(ap+j, gensym(backends[j].c_str()));
      }
      if(j==0)startpost("<nothing>");
      endpost();

      outlet_anything(m_infoOut, gensym("driver"), asize, ap);
      delete[]ap;ap =NULL;
      asize=0;
    }
  }
}

/////////////////////////////////////////////////////////
// deviceMess
//
/////////////////////////////////////////////////////////
void pix_texout :: deviceMess(int dev)
{
  WITH_TEXOUTHANDLES_DO(setDevice(dev));
  restart();
}
void pix_texout :: deviceMess(std::string s)
{
  WITH_TEXOUTHANDLES_DO(setDevice(s));
  restart();
}

void pix_texout :: closeMess()
{
  if(m_texoutHandle) {
    m_texoutHandle->stop();
    m_texoutHandle->close();
  }
  m_texoutHandle=NULL;
}

/////////////////////////////////////////////////////////
// dimenMess
//
/////////////////////////////////////////////////////////
void pix_texout :: dimenMess(int x, int y, int leftmargin, int rightmargin,
             int topmargin, int bottommargin)
{
  m_writeprops.set("width", x);
  m_writeprops.set("height", y);

  m_writeprops.set("leftmargin", leftmargin);
  m_writeprops.set("rightmargin", rightmargin);
  m_writeprops.set("topmargin", topmargin);
  m_writeprops.set("bottommargin", bottommargin);

  if(m_texoutHandle)
    m_texoutHandle->setProperties(m_writeprops);
}

/////////////////////////////////////////////////////////
// channelMess
//
/////////////////////////////////////////////////////////
void pix_texout :: channelMess(int channel, t_float freq)
{
  m_writeprops.set("channel", channel);
  m_writeprops.set("frequency", freq);

  if(m_texoutHandle)
    m_texoutHandle->setProperties(m_writeprops);
}
/////////////////////////////////////////////////////////
// normMess
//
/////////////////////////////////////////////////////////
void pix_texout :: normMess(std::string s)
{
  m_writeprops.set("norm", std::string(s));

  if(m_texoutHandle)
    m_texoutHandle->setProperties(m_writeprops);
}
/////////////////////////////////////////////////////////
// colorMess
//
/////////////////////////////////////////////////////////
void pix_texout :: colorMess(t_atom*a)
{
  int format=0;
  if (a->a_type==A_SYMBOL){
      char c =*atom_getsymbol(a)->s_name;
      // we only have 3 colour-spaces: monochrome (GL_LUMINANCE), yuv (GL_YCBCR_422_GEM), and rgba (GL_RGBA)
      // if you don't need colour, i suggest, take monochrome
      // if you don't need alpha,  i suggest, take yuv
      // else take rgba
      switch (c){
      case 'g': case 'G': format=GL_LUMINANCE; break;
      case 'y': case 'Y': format=GL_YCBCR_422_GEM; break;
      case 'r': case 'R':
      default: format=GL_RGBA_GEM;
      }
  } else format=atom_getint(a);

  WITH_TEXOUTHANDLES_DO(setColor(format));
}

/////////////////////////////////////////////////////////
// enumerate devices
//
/////////////////////////////////////////////////////////
void pix_texout :: enumerateMess()
{
  std::vector<std::string>data;
  std::vector<std::string>backends;
  unsigned int i=0;
  for(i=0; i<m_texoutHandles.size(); i++) {
    //    a.insert(a.end(), b.begin(), b.end());
    if(m_texoutHandles[i]) {
      std::string name=m_texoutHandles[i]->getName();
      verbose(1, "enumerating: %s", name.c_str());
      std::vector<std::string>temp=m_texoutHandles[i]->enumerate();
      unsigned int i=0;
      for(i=0; i<temp.size(); i++) {
        backends.push_back(name);
        data.push_back(temp[i]);
      }
    }
  }
  if(data.size()<=0) {
    error("no devices found");
  }

  t_atom ap[2];
  SETFLOAT(ap, data.size());
  outlet_anything(m_infoOut, gensym("devices"), 1, ap);
  for(i=0; i<data.size(); i++) {
    SETSYMBOL(ap+0, gensym(data[i].c_str()));
    SETSYMBOL(ap+1, gensym(backends[i].c_str()));
    outlet_anything(m_infoOut, gensym("device"), 2, ap);
    //    post("%d: %s", i, data[i].c_str());
  }
}
/////////////////////////////////////////////////////////
// dialog
//
/////////////////////////////////////////////////////////
/*
void pix_texout :: dialogMess(int argc, t_atom*argv)
{
  if(m_texoutHandle) {
    std::vector<std::string>data;
    while(argc>0) {
      data.push_back(std::string(atom_getsymbol(argv)->s_name));
      argv++;argc--;
    }

    if(!m_texoutHandle->dialog(data)) {
      post("no dialog for current backend");
    }
  }
}
*/

/////////////////////////////////////////////////////////
// set properties
//
// example: "set width 640, set name foo, set"
//   will first set the properties "width" to 640 annd "name" to "foo"
//   and then will apply these properties to the currently opened device
//
/////////////////////////////////////////////////////////

static gem::any atom2any(t_atom*ap) {
  gem::any result;
  if(ap) {
    switch(ap->a_type) {
    case A_FLOAT:
      result=atom_getfloat(ap);
      break;
    case A_SYMBOL:
      result=atom_getsymbol(ap)->s_name;
      break;
    default:
      result=ap->a_w.w_gpointer;
    }
  }
  return result;
}
static void addProperties(gem::Properties&props, int argc, t_atom*argv)
{
  if(!argc)return;

    if(argv->a_type != A_SYMBOL) {
      error("no key given...");
      return;
    }
    std::string key=std::string(atom_getsymbol(argv)->s_name);
    std::vector<gem::any> values;
    argc--; argv++;
    while(argc-->0) {
      values.push_back(atom2any(argv++));
    }
    switch(values.size()) {
    default:
      props.set(key, values);
      break;
    case 1:
      props.set(key, values[0]);
      break;
    case 0:
      {
  gem::any dummy;
  props.set(key, dummy);
      }
      break;
    }
}

void pix_texout :: setPropertyMess(int argc, t_atom*argv)
{
  if(!argc) {
    error("no property specified!");
    return;
  }
  addProperties(m_writeprops, argc, argv);

  if(m_texoutHandle) {
    m_texoutHandle->setProperties(m_writeprops);
  }
}

void pix_texout :: getPropertyMess(int argc, t_atom*argv)
{
  if(argc) {
    int i=0;
    m_readprops.clear();

    for(i=0; i<argc; i++) {
      addProperties(m_readprops, 1, argv+i);
    }

  } else {
    /* LATER: read all properties */
  }

  t_atom ap[4];
  if(m_texoutHandle) {
    m_texoutHandle->getProperties(m_readprops);
    std::vector<std::string>keys=m_readprops.keys();
    unsigned int i=0;
    for(i=0; i<keys.size(); i++) {
      std::string key=keys[i];
      SETSYMBOL(ap+0, gensym(key.c_str()));
      int ac=0;
      switch(m_readprops.type(key)) {
      default:
      case gem::Properties::UNSET:
        ac=0;
        break;
      case gem::Properties::NONE:
        ac=1;
        break;
      case gem::Properties::DOUBLE:
        do {
          double d=0;
          if(m_readprops.get(key, d)) {
            ac=2;
            SETFLOAT(ap+1, d);
          }
        } while(0);
        break;
      case gem::Properties::STRING:
        do {
          std::string s;
          if(m_readprops.get(key, s)) {
            ac=2;
            SETSYMBOL(ap+1, gensym(s.c_str()));
          }
        } while(0);
        break;
      }
      if(ac) {
        outlet_anything(m_infoOut, gensym("prop"), ac, ap);
      } else {
        post("oops: %s", key.c_str());
      }
    }
  } else {
    verbose(1, "no open texoutdevice...remembering properties...");
  }
}

void pix_texout :: enumPropertyMess()
{
  if(m_texoutHandle) {
    gem::Properties readable, writeable;
    std::vector<std::string>readkeys, writekeys;
    t_atom ap[4];
    int ac=3;


    m_texoutHandle->enumProperties(readable, writeable);

    readkeys=readable.keys();

    SETSYMBOL(ap+0, gensym("numread"));
    SETFLOAT(ap+1, readkeys.size());
    outlet_anything(m_infoOut, gensym("proplist"), 2, ap);

    SETSYMBOL(ap+0, gensym("read"));
    unsigned int i=0;
    for(i=0; i<readkeys.size(); i++) {
      ac=3;
      std::string key=readkeys[i];
      SETSYMBOL(ap+1, gensym(key.c_str()));
      switch(readable.type(key)) {
      case gem::Properties::NONE:
        SETSYMBOL(ap+2, gensym("bang"));
        break;
      case gem::Properties::DOUBLE: {
        double d=-1;
        SETSYMBOL(ap+2, gensym("float"));
        /* LATER: get and show ranges */
        if(readable.get(key, d)) {
          ac=4;
          SETFLOAT(ap+3, d);
        }
      }
        break;
      case gem::Properties::STRING: {
        SETSYMBOL(ap+2, gensym("symbol"));
        std::string s;
        if(readable.get(key, s)) {
          ac=4;
          SETSYMBOL(ap+3, gensym(s.c_str()));
        }
      }
        break;
      default:
        SETSYMBOL(ap+2, gensym("unknown"));
        break;
      }
      outlet_anything(m_infoOut, gensym("proplist"), ac, ap);
    }


    writekeys=writeable.keys();

    SETSYMBOL(ap+0, gensym("numwrite"));
    SETFLOAT(ap+1, writekeys.size());
    outlet_anything(m_infoOut, gensym("proplist"), 2, ap);

    SETSYMBOL(ap+0, gensym("write"));
    for(i=0; i<writekeys.size(); i++) {
      ac=3;
      std::string key=writekeys[i];
      SETSYMBOL(ap+1, gensym(key.c_str()));
      switch(writeable.type(key)) {
      case gem::Properties::NONE:
        SETSYMBOL(ap+2, gensym("bang"));
        break;
      case gem::Properties::DOUBLE: {
        double d=-1;
        SETSYMBOL(ap+2, gensym("float"));
        /* LATER: get and show ranges */
        ac=4;
        if(writeable.get(key, d)) {
          SETFLOAT(ap+3, d);
        }
      }
        break;
      case gem::Properties::STRING: {
        ac=4;
        SETSYMBOL(ap+2, gensym("symbol"));
        std::string s;
        if(writeable.get(key, s)) {
          SETSYMBOL(ap+3, gensym(s.c_str()));
        }
      }
        break;
      default:
        SETSYMBOL(ap+2, gensym("unknown"));
        break;
      }
      outlet_anything(m_infoOut, gensym("proplist"), ac, ap);
    }
  } else {
    error("cannot enumerate properties without a valid texout-device");
  }
}

void pix_texout :: setPropertiesMess(int argc, t_atom*argv)
{
  addProperties(m_writeprops, argc, argv);
}

void pix_texout :: applyPropertiesMess()
{
  if(m_texoutHandle) {
    m_texoutHandle->setProperties(m_writeprops);
  } else {
    verbose(1, "no open texoutdevice...remembering properties...");
  }
}

void pix_texout :: clearPropertiesMess()
{
  m_writeprops.clear();
}





void pix_texout :: asynchronousMess(bool state)
{
  unsigned int i;
  for(i=0; i<m_texoutHandles.size(); i++) {
    if(m_texoutHandles[i])
      m_texoutHandles[i]->grabAsynchronous(state);
  }
}

/////////////////////////////////////////////////////////
// qualityMess
//
/////////////////////////////////////////////////////////
void pix_texout :: qualityMess(int q) {
  m_writeprops.set("quality", q);

  if(m_texoutHandle)
    m_texoutHandle->setProperties(m_writeprops);
}


void pix_texout :: resetMess(void) {
  if(m_texoutHandle)
    m_texoutHandle->reset();
  else
      WITH_TEXOUTHANDLES_DO(reset());
}


/////////////////////////////////////////////////////////
// transferMess
//
/////////////////////////////////////////////////////////
void pix_texout :: runningMess(bool state) {
  m_running=state?STARTED:STOPPED;
  if(m_texoutHandle) {
    if(STARTED==m_running)
      m_texoutHandle->start();
    else
      m_texoutHandle->stop();
  }
}



/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_texout :: obj_setupCallback(t_class *classPtr)
{
    CPPEXTERN_MSG0(classPtr, "enumerate", enumerateMess);

    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::driverMessCallback),
          gensym("driver"), A_GIMME, A_NULL);
    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::deviceMessCallback),
          gensym("device"), A_GIMME, A_NULL);

    CPPEXTERN_MSG0(classPtr, "close", closeMess);

    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::openMessCallback),
      gensym("open"), A_GIMME, A_NULL);

    CPPEXTERN_MSG1(classPtr, "float", runningMess, bool);

/*
    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::dialogMessCallback),
        gensym("dialog"), A_GIMME, A_NULL);
*/
    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::setPropertyMessCallback),
        gensym("set"), A_GIMME, A_NULL);
    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::getPropertyMessCallback),
        gensym("get"), A_GIMME, A_NULL);

    CPPEXTERN_MSG0(classPtr, "enumProps", enumPropertyMess);
    class_addmethod(classPtr,
        reinterpret_cast<t_method>(&pix_texout::  setPropertiesMessCallback),
        gensym("setProps"), A_GIMME, A_NULL);
    CPPEXTERN_MSG0(classPtr, "applyProps", applyPropertiesMess);
    CPPEXTERN_MSG0(classPtr, "clearProps", clearPropertiesMess);

    CPPEXTERN_MSG1(classPtr, "async", asynchronousMess, bool);


    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::colorMessCallback),
          gensym("colorspace"), A_GIMME, A_NULL);

    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::dimenMessCallback),
          gensym("dimen"), A_GIMME, A_NULL);
    CPPEXTERN_MSG1(classPtr, "norm", normMess, std::string);

    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::channelMessCallback),
          gensym("channel"), A_GIMME, A_NULL);
    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::modeMessCallback),
          gensym("mode"), A_GIMME, A_NULL);
    class_addmethod(classPtr, reinterpret_cast<t_method>(&pix_texout::colorMessCallback),
          gensym("color"), A_GIMME, A_NULL);
    CPPEXTERN_MSG1(classPtr, "quality", qualityMess, int);

  CPPEXTERN_MSG0(classPtr, "reset", resetMess);

}
void pix_texout :: dimenMessCallback(void *data, t_symbol *s, int ac, t_atom *av)
{
  GetMyClass(data)->dimenMess(static_cast<int>(atom_getfloatarg(0, ac, av)),
            static_cast<int>(atom_getfloatarg(1, ac, av)),
            static_cast<int>(atom_getfloatarg(2, ac, av)),
            static_cast<int>(atom_getfloatarg(3, ac, av)),
            static_cast<int>(atom_getfloatarg(4, ac, av)),
            static_cast<int>(atom_getfloatarg(5, ac, av)) );
}
void pix_texout :: channelMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  if (argc!=1&&argc!=2)return;
  int chan = atom_getint(argv);
  t_float freq = (argc==1)?0:atom_getfloat(argv+1);
  GetMyClass(data)->channelMess(static_cast<int>(chan), freq);

}
void pix_texout :: modeMessCallback(void *data, t_symbol* nop, int argc, t_atom *argv)
{
  switch (argc){
  case 1:
    if      (A_FLOAT ==argv->a_type)GetMyClass(data)->channelMess(atom_getint(argv));
    else if (A_SYMBOL==argv->a_type)GetMyClass(data)->normMess(atom_getsymbol(argv)->s_name);
    else goto mode_error;
    break;
  case 2:
    if (A_SYMBOL==argv->a_type && A_FLOAT==(argv+1)->a_type){
      GetMyClass(data)->normMess(atom_getsymbol(argv)->s_name);
      GetMyClass(data)->channelMess(atom_getint(argv+1));
    } else goto mode_error;
    break;
  default:
  mode_error:
    ::post("invalid arguments for message \"mode [<norm>] [<channel>]\"");
  }
}
void pix_texout :: colorMessCallback(void *data, t_symbol* nop, int argc, t_atom *argv){
  if (argc==1)GetMyClass(data)->colorMess(argv);
  else GetMyClass(data)->error("invalid number of arguments (must be 1)");
}

void pix_texout :: deviceMessCallback(void *data, t_symbol*,int argc, t_atom*argv)
{
  if(argc==1){
    switch(argv->a_type){
    case A_FLOAT:
      GetMyClass(data)->deviceMess(atom_getint(argv));
      break;
    case A_SYMBOL:
      GetMyClass(data)->deviceMess(atom_getsymbol(argv)->s_name);
      break;
    default:
      GetMyClass(data)->error("device must be integer or symbol");
    }
  } else {
    GetMyClass(data)->error("can only set to 1 device at a time");
  }
}
void pix_texout :: driverMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  if(!argc) {
    GetMyClass(data)->driverMess();
    return;
  }
  if(argc!=1) {
    GetMyClass(data)->error("'driver' takes a single numeric or symbolic driver ID");
  } else if (argv->a_type == A_FLOAT) {
    GetMyClass(data)->driverMess(atom_getint(argv));
  } else if (argv->a_type == A_SYMBOL) {
    GetMyClass(data)->driverMess(atom_getsymbol(argv)->s_name);
  } else {
    GetMyClass(data)->error("'driver' takes a single numeric or symbolic driver ID");
  }
}
/*
void pix_texout :: dialogMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  GetMyClass(data)->dialogMess(argc, argv);
}
*/
void pix_texout :: getPropertyMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  GetMyClass(data)->getPropertyMess(argc, argv);
}
void pix_texout :: setPropertyMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  GetMyClass(data)->setPropertyMess(argc, argv);
}

void pix_texout :: setPropertiesMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  GetMyClass(data)->setPropertiesMess(argc, argv);
}

void pix_texout :: openMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  if(argc)driverMessCallback(data, s, argc, argv);
  else
    GetMyClass(data)->startRendering();
}
#endif /* no OS-specific GEM_TEXOUTBACKEND */