// waypoint.hxx -- Class to hold data and return info relating to a waypoint
//
// Written by Curtis Olson, started September 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@hfrl.umn.edu
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


#ifndef _WAYPOINT_HXX
#define _WAYPOINT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


class SGWayPoint {

public:

    enum modetype { 
	LATLON = 0,
	XY = 1,
    };

private:

    modetype mode;

    double lon;
    double lat;

public:

    SGWayPoint();
    SGWayPoint( const double _lon, const double _lat );
    SGWayPoint( const double _lon, const double _lat, const modetype _mode );
    ~SGWayPoint();

    inline modetype get_mode() const { return mode; }
    inline double get_lon() const { return lon; }
    inline double get_lat() const { return lat; }
};


#endif // _WAYPOINT_HXX
