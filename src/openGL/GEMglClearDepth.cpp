////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Implementation file
//
// Copyright (c) 2002-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//	zmoelnig@iem.kug.ac.at
//  For information on usage and redistribution, and for a DISCLAIMER
//  *  OF ALL WARRANTIES, see the file, "GEM.LICENSE.TERMS"
//
//  this file has been generated...
////////////////////////////////////////////////////////

#include "GEMglClearDepth.h"

CPPEXTERN_NEW_WITH_ONE_ARG ( GEMglClearDepth , t_floatarg, A_DEFFLOAT);

/////////////////////////////////////////////////////////
//
// GEMglViewport
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglClearDepth :: GEMglClearDepth	(t_floatarg arg0) :
		depth(static_cast<GLclampd>(arg0))
{
	m_inlet[0] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float, gensym("depth"));
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglClearDepth :: ~GEMglClearDepth () {
inlet_free(m_inlet[0]);
}

/////////////////////////////////////////////////////////
// Render
//
void GEMglClearDepth :: render(GemState *state) {

	glClearDepth (static_cast<GLclampd>(depth));

}

/////////////////////////////////////////////////////////
// Variables
//
void GEMglClearDepth :: depthMess (float arg1) {	// FUN

	depth = arg1;
	setModified();
}


/////////////////////////////////////////////////////////
// static member functions
//

void GEMglClearDepth :: obj_setupCallback(t_class *classPtr) {
	 class_addmethod(classPtr, reinterpret_cast<t_method>(&GEMglClearDepth::depthMessCallback),  	gensym("depth"), A_DEFFLOAT, A_NULL);
};

void GEMglClearDepth :: depthMessCallback (void* data, t_float arg0){
	GetMyClass(data)->depthMess ( arg0);
}
