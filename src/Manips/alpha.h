/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Turn on alpha blending

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef _INCLUDE__GEM_MANIPS_ALPHA_H_
#define _INCLUDE__GEM_MANIPS_ALPHA_H_

#include "Base/GemBase.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    alpha

    Turn on alpha blending

DESCRIPTION

    "alphastate" - whether to use alpha blending

-----------------------------------------------------------------*/

// need to get some function pointer at runtime
typedef void(*glBlendFunciProc)(GLuint buf, GLenum sfactor, GLenum dfactor);

class GEM_EXTERN alpha : public GemBase
{
    CPPEXTERN_HEADER(alpha, GemBase);

    public:

        //////////
        // Constructor
    	alpha(t_floatarg);

    protected:

    	//////////
    	// Destructor
    	virtual ~alpha();

    	//////////
    	// Do the rendering
    	virtual void 	render(GemState *state);

    	//////////
    	// Turn off alpha blending
    	virtual void 	postrender(GemState *state);

    	//////////
    	// alpha blending state
    	int	    	    m_alphaState;

    	//////////
    	// alpha test state
    	int	    	    m_alphaTest;

	/////////
	// depthtest
	int                 m_depthtest;

	//////////
	// the blending function
	GLenum              m_function;
    GLenum              m_src_function;
    GLenum              m_src_fun_separate[4];
    GLenum              m_dst_fun_separate[4];
	    bool 				m_separate;

    	void	    	funMess(int fun);
        GLenum          int2GLenum(int i);


    	//////////
    	// Alpha state changed
    	void	    	alphaMess(int alphaState);

     	//////////
    	// Alpha test changed
    	void	    	testMess(int alphaTest);

     	//////////
    	// Disable Depthtest
    	void	    	depthtestMess(int i);

    	/////////
    	// Use separable blending function
    	void 			separateMess(bool b);

    	/////////
    	// Set function to use on each channel
    	void separateFuncMess(t_symbol *s, int argc, t_atom* argv);

    	t_inlet *m_inlet;

        glBlendFunciProc glBlendFunciFn;
};

#endif	// for header file
