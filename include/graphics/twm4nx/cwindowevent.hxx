/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cwindowevent.hxx
// Shim to manage the interface between NX messages and NxWidgets
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWEVENT_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWEVENT_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <sys/types.h>
#include <cstdbool>
#include <mqueue.h>

#include "graphics/nxwidgets/cwindoweventhandler.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  /**
   * This abstract base class provides add on methods to support dragging
   * of a window.
   */

  class IDragEvent
  {
    public:
      /**
       * A virtual destructor is required in order to override the ITextBox
       * destructor.  We do this because if we delete ITextBox, we want the
       * destructor of the class that inherits from ITextBox to run, not this
       * one.
       */

      virtual ~IDragEvent(void) { }

      /**
       * This function is called when there is any moved of the mouse or
       * touch position that would indicate that the object is being moved.
       *
       * @param pos The current mouse/touch X/Y position in toolbar relative
       *   coordinates.
       * @return True: if the drage event was processed; false it is was
       *   ignored.  The event should be ignored if there is not actually
       *   a drag event in progress
       */

      virtual bool dragEvent(FAR const struct nxgl_point_s &pos) = 0;

      /**
       * This function is called if the mouse left button is released or
       * if the touchscrreen touch is lost.  This indicates that the
       * dragging sequence is complete.
       *
       * @param pos The last mouse/touch X/Y position in toolbar relative
       *   coordinates.
       * @return True: if the drage event was processed; false it is was
       *   ignored.  The event should be ignored if there is not actually
       *   a drag event in progress
       */

      virtual bool dropEvent(FAR const struct nxgl_point_s &pos) = 0;
  };

  /**
   * The class CWindowEvent integrates the widget control with some special
   * handling of mouse and keyboard inputs needs by NxWM.  It use used
   * in place of CWidgetControl whenever an NxWM window is created.
   *
   * CWindowEvent cohabitates with CWidgetControl only because it needs the
   * CWidgetControl as an argument in its messaging.
   */

  class CWindowEvent : public NXWidgets::CWindowEventHandler,
                       public NXWidgets::CWidgetControl
  {
    private:
      FAR CTwm4Nx         *m_twm4nx;           /**< Cached instance of CTwm4Nx */
      mqd_t                m_eventq;           /**< NxWidget event message queue */
      FAR void            *m_object;           /**< Window object (context specific) */
      bool                 m_isBackground;     /**< True if this serves the background window */

      // Dragging

      FAR IDragEvent      *m_dragHandler;      /**< Drag event handlers (may be NULL) */
      bool                 m_dragging;         /**< True:  dragging */
      struct nxgl_point_s  m_dragPos;          /**< Last mouse/touch position */

      /**
       * Send the EVENT_MSG_POLL input event message to the Twm4Nx event loop.
       */

      void sendInputEvent(void);

      // Override CWidgetEventHandler virtual methods ///////////////////////

      /**
       * Handle a NX window redraw request event
       *
       * @param nxRect The region in the window to be redrawn
       * @param more More redraw requests will follow
       */

      void handleRedrawEvent(FAR const nxgl_rect_s *nxRect, bool more);

#ifdef CONFIG_NX_XYINPUT
      /**
       * Handle an NX window mouse input event.
       */

      void handleMouseEvent(FAR const struct nxgl_point_s *pos,
                            uint8_t buttons);
#endif

#ifdef CONFIG_NX_KBD
      /**
       * Handle a NX window keyboard input event.
       */

      void handleKeyboardEvent(void);
#endif

      /**
       * Handle a NX window blocked event
       *
       * @param arg - User provided argument (see nx_block or nxtk_block)
       */

      void handleBlockedEvent(FAR void *arg);

    public:

      /**
       * CWindowEvent Constructor
       *
       * @param twm4nx The Twm4Nx session instance.
       * @param obj Contextual object (Usually 'this' of instantiator)
       * @param isBackground True is this for the background window.
       * @param style The default style that all widgets on this display
       *   should use.  If this is not specified, the widget will use the
       *   values stored in the defaultCWidgetStyle object.
       */

       CWindowEvent(FAR CTwm4Nx *twm4nx, FAR void *obj,
                    bool isBackground = false,
                    FAR const NXWidgets::CWidgetStyle *style =
                    (const NXWidgets::CWidgetStyle *)NULL);

      /**
       * CWindowEvent Destructor.
       */

      ~CWindowEvent(void);

      /**
       * Register an IDragEvent instance to provide callbacks when mouse
       * movement is received.  A mouse movement with the left button down
       * or a touchscreen touch movement are treated as a drag event. 
       * Release of the mouse left button or loss of the touchscreen touch
       * is treated as a drop event.
       *
       * @param cb A reference to the IDragEvent callback interface.
       */

      inline void registerDragEventHandler(FAR IDragEvent *dragHandler)
      {
         m_dragHandler = dragHandler;
      }
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWEVENT_HXX