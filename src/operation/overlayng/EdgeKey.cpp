/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2020 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/operation/overlayng/EdgeKey.h>
#include <geos/geom/Dimension.h>
#include <geos/geom/CoordinateSequence.h>


namespace geos {      // geos
namespace operation { // geos.operation
namespace overlayng { // geos.operation.overlayng

EdgeKey::EdgeKey(const Edge* edge)
{
    initPoints(edge);
}

} // namespace geos.operation.overlayng
} // namespace geos.operation
} // namespace geos

#ifndef GEOS_INLINE
#include "geos/operation/overlayng/EdgeKey.inl"
#endif
