// polar.hxx -- routines to deal with polar math and transformations
//
// Written by Curtis Olson, started June 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#ifndef _POLAR_HXX
#define _POLAR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/fg_constants.h>
#include <Math/point3d.hxx>


// Find the Altitude above the Ellipsoid (WGS84) given the Earth
// Centered Cartesian coordinate vector Distances are specified in
// meters.
double fgGeodAltFromCart(const Point3D& cp);


// Convert a polar coordinate to a cartesian coordinate.  Lon and Lat
// must be specified in radians.  The FG convention is for distances
// to be specified in meters
inline Point3D fgPolarToCart3d(const Point3D& p) {
    double tmp = cos( p.lat() ) * p.radius();

    return Point3D( cos( p.lon() ) * tmp,
		    sin( p.lon() ) * tmp,
		    sin( p.lat() ) * p.radius() );
}


// Convert a cartesian coordinate to polar coordinates (lon/lat
// specified in radians.  Distances are specified in meters.
inline Point3D fgCartToPolar3d(const Point3D& cp) {
    return Point3D( atan2( cp.y(), cp.x() ),
		    FG_PI_2 - 
		    atan2( sqrt(cp.x()*cp.x() + cp.y()*cp.y()), cp.z() ),
		    sqrt(cp.x()*cp.x() + cp.y()*cp.y() + cp.z()*cp.z()) );
}


#endif // _POLAR_HXX


// $Log$
// Revision 1.1  1999/04/05 21:32:33  curt
// Initial revision
//
// Revision 1.5  1999/01/27 04:46:20  curt
// Portability tweaks by Bernie Bright.
//
// Revision 1.4  1998/10/16 19:30:07  curt
// C++-ified the comments.
//
// Revision 1.3  1998/10/16 00:50:30  curt
// Added point3d.hxx to replace cheezy fgPoint3d struct.
//
// Revision 1.2  1998/08/24 20:04:12  curt
// Various "inline" code optimizations contributed by Norman Vine.
//
// Revision 1.1  1998/07/08 14:40:09  curt
// polar3d.[ch] renamed to polar3d.[ch]xx, vector.[ch] renamed to vector.[ch]xx
// Updated fg_geodesy comments to reflect that routines expect and produce
//   meters.
//
// Revision 1.1  1998/05/02 01:50:11  curt
// polar.[ch] renamed to polar3d.[ch]
//
// Revision 1.9  1998/04/25 22:06:23  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.8  1998/04/21 17:03:50  curt
// Prepairing for C++ integration.
//
// Revision 1.7  1998/01/27 00:48:00  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.6  1998/01/22 02:59:39  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.5  1998/01/19 19:27:13  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.4  1997/12/15 23:54:55  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.3  1997/07/31 22:52:28  curt
// Working on redoing internal coordinate systems & scenery transformations.
//
// Revision 1.2  1997/07/23 21:52:21  curt
// Put comments around the text after an #endif for increased portability.
//
// Revision 1.1  1997/07/07 21:02:37  curt
// Initial revision.
//

