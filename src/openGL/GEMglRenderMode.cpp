////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Implementation file
//
// Copyright (c) 2002 IOhannes m zmoelnig. forum::f�r::uml�ute. IEM
//	zmoelnig@iem.kug.ac.at
//  For information on usage and redistribution, and for a DISCLAIMER
//  *  OF ALL WARRANTIES, see the file, "GEM.LICENSE.TERMS"
//
//  this file has been generated...
////////////////////////////////////////////////////////

#include "GEMglRenderMode.h"

CPPEXTERN_NEW_WITH_GIMME ( GEMglRenderMode )

/////////////////////////////////////////////////////////
//
// GEMglViewport
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglRenderMode :: GEMglRenderMode	(int argc, t_atom*argv) : 
  mode(0) 
{
  if(1==argc)modeMess(argv[0]); else if(argc) throw(GemException("invalid number of arguments"));

  m_inlet = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float, gensym("mode"));
  m_outlet=outlet_new(this->x_obj, 0);
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglRenderMode :: ~GEMglRenderMode () {
   inlet_free(m_inlet);
   outlet_free(m_outlet);
}

/////////////////////////////////////////////////////////
// Render
//
void GEMglRenderMode :: render(GemState *state) {
  GLint i = glRenderMode (mode);
  outlet_float(m_outlet, (t_float)i);
}

/////////////////////////////////////////////////////////
// Variables
//
void GEMglRenderMode :: modeMess (t_atom arg) {	// FUN
  mode = (GLenum)getGLdefine(&arg);
  setModified();
}


/////////////////////////////////////////////////////////
// static member functions
//

void GEMglRenderMode :: obj_setupCallback(t_class *classPtr) {
  class_addmethod(classPtr, (t_method)&GEMglRenderMode::modeMessCallback, gensym("mode"), A_GIMME, A_NULL);
}

void GEMglRenderMode :: modeMessCallback (void* data, t_symbol*s, int argc, t_atom*argv){
	if(1==argc)GetMyClass(data)->modeMess (argv[0]);
}
