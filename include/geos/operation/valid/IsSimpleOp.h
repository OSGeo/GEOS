/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 * Copyright (C) 2009 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2005-2006 Refractions Research Inc.
 * Copyright (C) 2021 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************/

#pragma once

#include <geos/algorithm/LineIntersector.h>
#include <geos/algorithm/BoundaryNodeRule.h>
#include <geos/noding/SegmentIntersector.h>


// Forward declarations
namespace geos {
namespace noding {
class SegmentString;
class BasicSegmentString;
}
namespace algorithm {
class BoundaryNodeRule;
}
namespace geom {
class LineString;
class LinearRing;
class MultiLineString;
class MultiPoint;
class Geometry;
class Polygon;
class GeometryCollection;
}
}


namespace geos {      // geos
namespace operation { // geos.operation
namespace valid {     // geos.operation.valid


class GEOS_DLL IsSimpleOp {

private:

    const geom::Geometry& inputGeom;
    bool isClosedEndpointsInInterior = true;
    bool isFindAllLocations = false;
    bool isSimpleResult = false;
    std::vector<geom::Coordinate> nonSimplePts;
    bool computed = false;

    void compute();

    bool computeSimple(const geom::Geometry& geom);

    bool isSimpleMultiPoint(const geom::MultiPoint& mp);

    /**
    * Computes simplicity for polygonal geometries.
    * Polygonal geometries are simple if and only if
    * all of their component rings are simple.
    *
    * @param geom a Polygonal geometry
    * @return true if the geometry is simple
    */
    bool isSimplePolygonal(const geom::Geometry& geom);

    /**
    * Semantics for GeometryCollection is
    * simple iff all components are simple.
    *
    * @param geom
    * @return true if the geometry is simple
    */
    bool isSimpleGeometryCollection(const geom::Geometry& geom);

    bool isSimpleLinearGeometry(const geom::Geometry& geom);

    static std::vector<std::unique_ptr<noding::SegmentString>>
        extractSegmentStrings(const geom::Geometry& geom);


    class NonSimpleIntersectionFinder : public noding::SegmentIntersector
    {

    private:

        bool isClosedEndpointsInInterior;
        bool isFindAll = false;

        std::vector<geom::Coordinate>& intersectionPts;
        algorithm::LineIntersector li;

        bool hasInteriorInt;
        bool hasInteriorVertexInt;
        bool hasEqualSegments;
        bool hasInteriorEndpointInt;

        bool findIntersection(
            noding::SegmentString* ss0, std::size_t segIndex0,
            noding::SegmentString* ss1, std::size_t segIndex1,
            const geom::Coordinate& p00, const geom::Coordinate& p01,
            const geom::Coordinate& p10, const geom::Coordinate& p11);

        /**
        * Tests whether an intersection vertex is an endpoint of a segment string.
        *
        * @param ss the segmentString
        * @param ssIndex index of segment in segmentString
        * @param li the line intersector
        * @param liSegmentIndex index of segment in intersector
        * @return true if the intersection vertex is an endpoint
        */
        bool isIntersectionEndpoint(
            const noding::SegmentString* ss, std::size_t ssIndex,
            const algorithm::LineIntersector& li, std::size_t liSegmentIndex) const;

        /**
        * Finds the vertex index in a segment of an intersection
        * which is known to be a vertex.
        *
        * @param li the line intersector
        * @param segmentIndex the intersection segment index
        * @return the vertex index (0 or 1) in the segment vertex of the intersection point
        */
        std::size_t intersectionVertexIndex(
            const algorithm::LineIntersector& li, std::size_t segmentIndex) const;

    public:

        NonSimpleIntersectionFinder(
            bool p_isClosedEndpointsInInterior,
            bool p_isFindAll,
            std::vector<geom::Coordinate>& p_intersectionPts)
        : isClosedEndpointsInInterior(p_isClosedEndpointsInInterior)
        , isFindAll(p_isFindAll)
        , intersectionPts(p_intersectionPts)
        {};

        /**
        * Tests whether an intersection was found.
        *
        * @return true if an intersection was found
        */
        bool hasIntersection() const;

        void processIntersections(
            noding::SegmentString* ss0, std::size_t segIndex0,
            noding::SegmentString* ss1, std::size_t segIndex1) override;

        bool isDone() const override;

    }; // NonSimpleIntersectionFinder


public:

    IsSimpleOp(const geom::Geometry* geom)
        : IsSimpleOp(*geom)
        {};

    /**
    * Creates a simplicity checker using the default SFS Mod-2 Boundary Node Rule
    *
    * @param geom the geometry to test
    */
    IsSimpleOp(const geom::Geometry& geom)
        : IsSimpleOp(geom, algorithm::BoundaryNodeRule::getBoundaryRuleMod2())
        {};

    /**
    * Creates a simplicity checker using a given {@link BoundaryNodeRule}
    *
    * @param geom the geometry to test
    * @param boundaryNodeRule the boundary node rule to use.
    */
    IsSimpleOp(const geom::Geometry& geom, const algorithm::BoundaryNodeRule& p_boundaryNodeRule)
        : inputGeom(geom)
        , isClosedEndpointsInInterior(! p_boundaryNodeRule.isInBoundary(2))
        , isFindAllLocations(false)
        , computed(false)
        {};

    /**
    * Tests whether a geometry is simple.
    *
    * @param geom the geometry to test
    * @return true if the geometry is simple
    */
    static bool isSimple(const geom::Geometry& geom);

    static bool isSimple(const geom::Geometry* geom) {
        if (!geom) return false;
        return isSimple(*geom);
    }

    /**
    * Gets a non-simple location in a geometry, if any.
    *
    * @param geom the input geometry
    * @return a non-simple location, or null if the geometry is simple
    */
    geom::Coordinate getNonSimpleLocation(const geom::Geometry& geom);

    /**
    * Sets whether all non-simple intersection points
    * will be found.
    *
    * @param isFindAll whether to find all non-simple points
    */
    void setFindAllLocations(bool isFindAll);

    /**
    * Tests whether the geometry is simple.
    *
    * @return true if the geometry is simple
    */
    bool isSimple();

    /**
    * Gets the coordinate for an location where the geometry
    * fails to be simple.
    * (i.e. where it has a non-boundary self-intersection).
    *
    * @return a coordinate for the location of the non-boundary self-intersection
    * or null if the geometry is simple
    */
    geom::Coordinate getNonSimpleLocation();

    /**
    * Gets all non-simple intersection locations.
    *
    * @return a list of the coordinates of non-simple locations
    */
    const std::vector<geom::Coordinate>& getNonSimpleLocations();



}; // IsSimpleOp


} // namespace geos.operation.valid
} // namespace geos.operation
} // namespace geos
