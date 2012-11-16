// Interface for 2D Canvas elements
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef CANVAS_ELEMENT_HXX_
#define CANVAS_ELEMENT_HXX_

#include <simgear/canvas/canvas_fwd.hxx>
#include <simgear/props/props.hxx>
#include <simgear/misc/stdint.hxx> // for uint32_t
#include <osg/BoundingBox>
#include <osg/MatrixTransform>

#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace osg
{
  class Drawable;
}

namespace simgear
{
namespace canvas
{

  class MouseEvent;
  class Element:
    public SGPropertyChangeListener
  {
    public:
      typedef boost::function<void(const SGPropertyNode*)> StyleSetter;
      typedef std::map<std::string, StyleSetter> StyleSetters;

      /**
       * Remove the property listener of the element.
       *
       * You will need to call the appropriate methods (#childAdded,
       * #childRemoved, #valueChanged) yourself to ensure the element still
       * receives the needed events.
       */
      void removeListener();

      /**
       *
       */
      virtual ~Element() = 0;

      /**
       * Called every frame to update internal state
       *
       * @param dt  Frame time in seconds
       */
      virtual void update(double dt);

      SGConstPropertyNode_ptr getProps() const;
      SGPropertyNode_ptr getProps();

      /**
       * Handle mouse event (transforms coordinates to local coordinate frame
       * and forwards event to #handleLocalMouseEvent)
       */
      virtual bool handleMouseEvent(const canvas::MouseEvent& event);

      osg::ref_ptr<osg::MatrixTransform> getMatrixTransform();

      virtual void childAdded( SGPropertyNode * parent,
                               SGPropertyNode * child );
      virtual void childRemoved( SGPropertyNode * parent,
                                 SGPropertyNode * child );
      virtual void valueChanged(SGPropertyNode * child);

      /**
       * Write the given bounding box to the property tree
       */
      void setBoundingBox(const osg::BoundingBox& bb);

    protected:

      enum Attributes
      {
        LAST_ATTRIBUTE  = 0x0001
      };

      enum TransformType
      {
        TT_NONE,
        TT_MATRIX,
        TT_TRANSLATE,
        TT_ROTATE,
        TT_SCALE
      };

      CanvasWeakPtr _canvas;
      uint32_t _attributes_dirty;

      bool _transform_dirty;
      osg::ref_ptr<osg::MatrixTransform>    _transform;
      std::vector<TransformType>            _transform_types;

      SGPropertyNode_ptr                _node;
      Style                             _style;
      StyleSetters                      _style_setters;
      std::vector<SGPropertyNode_ptr>   _bounding_box;

      Element( const CanvasWeakPtr& canvas,
               const SGPropertyNode_ptr& node,
               const Style& parent_style );

      template<typename T, class C1, class C2>
      Element::StyleSetter
      addStyle(const std::string& name, void (C1::*setter)(T), C2 instance)
      {
        return _style_setters[ name ] =
                 bindStyleSetter<T>(name, setter, instance);
      }

      template<typename T1, typename T2, class C1, class C2>
      Element::StyleSetter
      addStyle(const std::string& name, void (C1::*setter)(T2), C2 instance)
      {
        return _style_setters[ name ] =
                 bindStyleSetter<T1>(name, setter, instance);
      }

      template<class C1, class C2>
      Element::StyleSetter
      addStyle( const std::string& name,
                void (C1::*setter)(const std::string&),
                C2 instance )
      {
        return _style_setters[ name ] =
                 bindStyleSetter<const char*>(name, setter, instance);
      }

      template<typename T1, typename T2, class C1, class C2>
      Element::StyleSetter
      bindStyleSetter( const std::string& name,
                       void (C1::*setter)(T2),
                       C2 instance )
      {
        return boost::bind(setter, instance, boost::bind(&getValue<T1>, _1));
      }

      virtual bool handleLocalMouseEvent(const canvas::MouseEvent& event);

      virtual void childAdded(SGPropertyNode * child)  {}
      virtual void childRemoved(SGPropertyNode * child){}
      virtual void childChanged(SGPropertyNode * child){}

      void setDrawable(osg::Drawable* drawable);

      void setupStyle();
      bool setStyle(const SGPropertyNode* child);

    private:

      osg::ref_ptr<osg::Drawable> _drawable;

      Element(const Element&);// = delete
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_ELEMENT_HXX_ */
