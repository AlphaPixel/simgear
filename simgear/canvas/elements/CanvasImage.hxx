// An image on the Canvas
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

#ifndef CANVAS_IMAGE_HXX_
#define CANVAS_IMAGE_HXX_

#include "CanvasElement.hxx"

#include <simgear/canvas/canvas_fwd.hxx>
#include <simgear/math/Rect.hxx>
#include <osg/Texture2D>

namespace simgear
{
namespace canvas
{

  class Image:
    public Element
  {
    public:
      /**
       * @param node    Property node containing settings for this image:
       *                  rect/[left/right/top/bottom]  Dimensions of source
       *                                                rect
       *                  size[0-1]                     Dimensions of rectangle
       *                  [x,y]                         Position of rectangle
       */
      Image( const CanvasWeakPtr& canvas,
             const SGPropertyNode_ptr& node,
             const Style& parent_style );
      virtual ~Image();

      virtual void update(double dt);

      void setSrcCanvas(CanvasPtr canvas);
      CanvasWeakPtr getSrcCanvas() const;

      void setImage(osg::Image *img);
      void setFill(const std::string& fill);

      const Rect<float>& getRegion() const;

    protected:

      enum ImageAttributes
      {
        SRC_RECT       = LAST_ATTRIBUTE << 1, // Source image rectangle
        DEST_SIZE      = SRC_RECT << 1        // Element size
      };

      virtual void childChanged(SGPropertyNode * child);

      void setupDefaultDimensions();
      Rect<int> getTextureDimensions() const;

      osg::ref_ptr<osg::Texture2D> _texture;
      // TODO optionally forward events to canvas
      CanvasWeakPtr _src_canvas;

      osg::ref_ptr<osg::Geometry>  _geom;
      osg::ref_ptr<osg::Vec3Array> _vertices;
      osg::ref_ptr<osg::Vec2Array> _texCoords;
      osg::ref_ptr<osg::Vec4Array> _colors;

      SGPropertyNode *_node_src_rect;
      Rect<float>     _src_rect,
                      _region;
  };

} // namespace canvas
} // namespace canvas

#endif /* CANVAS_IMAGE_HXX_ */
