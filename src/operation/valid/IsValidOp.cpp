/**********************************************************************
*
* GEOS - Geometry Engine Open Source
* http://geos.osgeo.org
*
* Copyright (C) 2021 Paul Ramsey <pramsey@cleverelephant.ca>
* Copyright (C) 2021 Martin Davis
*
* This is free software; you can redistribute and/or modify it under
* the terms of the GNU Lesser General Public Licence as published
* by the Free Software Foundation.
* See the COPYING file for more information.
*
**********************************************************************/

#include <geos/algorithm/locate/IndexedPointInAreaLocator.h>
#include <geos/geom/Coordinate.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryCollection.h>
#include <geos/geom/LineString.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/Location.h>
#include <geos/geom/MultiPoint.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/geom/Point.h>
#include <geos/geom/Polygon.h>
#include <geos/operation/valid/IsValidOp.h>
#include <geos/operation/valid/IndexedNestedHoleTester.h>
#include <geos/util/UnsupportedOperationException.h>

#include <cmath>

using namespace geos::geom;
using geos::algorithm::locate::IndexedPointInAreaLocator;

namespace geos {      // geos
namespace operation { // geos.operation
namespace valid {     // geos.operation.valid

/* public */
bool
IsValidOp::isValid()
{
    return isValidGeometry(inputGeometry);
}


/* public static */
bool
IsValidOp::isValid(const Coordinate* coord)
{
    if (std::isfinite(coord->x) && std::isfinite(coord->y)) {
        return true;
    }
    else {
        return false;
    }
}


/* public */
const TopologyValidationError *
IsValidOp::getValidationError()
{
    isValidGeometry(inputGeometry);
    return validErr.get();
}


/* private */
void
IsValidOp::logInvalid(int code, const Coordinate* pt)
{
    validErr.reset(new TopologyValidationError(code, *pt));
}


/* private */
bool
IsValidOp::isValidGeometry(const Geometry* g)
{
    validErr.reset(nullptr);

    // empty geometries are always valid
    if (g->isEmpty()) return true;
    switch (g->getGeometryTypeId()) {
        case GEOS_POINT:
            return isValid(static_cast<const Point*>(g));
        case GEOS_MULTIPOINT:
            return isValid(static_cast<const MultiPoint*>(g));
        case GEOS_LINEARRING:
            return isValid(static_cast<const LinearRing*>(g));
        case GEOS_LINESTRING:
            return isValid(static_cast<const LineString*>(g));
        case GEOS_POLYGON:
            return isValid(static_cast<const Polygon*>(g));
        case GEOS_MULTIPOLYGON:
            return isValid(static_cast<const MultiPolygon*>(g));
        case GEOS_MULTILINESTRING:
            return isValid(static_cast<const GeometryCollection*>(g));
        case GEOS_GEOMETRYCOLLECTION:
            return isValid(static_cast<const GeometryCollection*>(g));
    }

    // geometry type not known
    throw util::UnsupportedOperationException(g->getGeometryType());
}


/* private */
bool
IsValidOp::isValid(const Point* g)
{
    checkCoordinateInvalid(g->getCoordinatesRO());
    if (hasInvalidError()) return false;
    return true;
}


/* private */
bool
IsValidOp::isValid(const MultiPoint* g)
{
    for (std::size_t i = 0; i < g->getNumGeometries(); i++) {
        const Point* p = g->getGeometryN(i);
        if (p->isEmpty()) continue;
        if (!isValid(p->getCoordinate())) {
            logInvalid(TopologyValidationError::eInvalidCoordinate,
                       p->getCoordinate());
            return false;;
        }
    }
    return true;
}


/* private */
bool
IsValidOp::isValid(const LineString* g)
{
  checkCoordinateInvalid(g->getCoordinatesRO());
  if (hasInvalidError()) return false;

  checkTooFewPoints(g, MIN_SIZE_LINESTRING);
  if (hasInvalidError()) return false;

  return true;
}


/* private */
bool
IsValidOp::isValid(const LinearRing* g)
{
  checkCoordinateInvalid(g->getCoordinatesRO());
  if (hasInvalidError()) return false;

  checkRingNotClosed(g);
  if (hasInvalidError()) return false;

  checkRingTooFewPoints(g);
  if (hasInvalidError()) return false;

  checkSelfIntersectingRing(g);
  if (hasInvalidError()) return false;

  return true;
}


/* private */
bool
IsValidOp::isValid(const Polygon* g)
{
  checkCoordinateInvalid(g);
  if (hasInvalidError()) return false;

  checkRingsNotClosed(g);
  if (hasInvalidError()) return false;

  checkRingsTooFewPoints(g);
  if (hasInvalidError()) return false;

  PolygonTopologyAnalyzer areaAnalyzer(g, isInvertedRingValid);

  checkAreaIntersections(areaAnalyzer);
  if (hasInvalidError()) return false;

  checkHolesOutsideShell(g);
  if (hasInvalidError()) return false;

  checkHolesNotNested(g);
  if (hasInvalidError()) return false;

  checkInteriorDisconnected(areaAnalyzer);
  if (hasInvalidError()) return false;

  return true;
}


/* private */
bool
IsValidOp::isValid(const MultiPolygon* g)
{
    for (std::size_t i = 0; i < g->getNumGeometries(); i++) {
        const Polygon* p = g->getGeometryN(i);
        checkCoordinateInvalid(p);
        if (hasInvalidError()) return false;

        checkRingsNotClosed(p);
        if (hasInvalidError()) return false;

        checkRingsTooFewPoints(p);
        if (hasInvalidError()) return false;
    }

    PolygonTopologyAnalyzer areaAnalyzer(g, isInvertedRingValid);

    checkAreaIntersections(areaAnalyzer);
    if (hasInvalidError()) return false;

    for (std::size_t i = 0; i < g->getNumGeometries(); i++) {
        const Polygon* p = g->getGeometryN(i);
        checkHolesOutsideShell(p);
        if (hasInvalidError()) return false;
    }
    for (std::size_t i = 0; i < g->getNumGeometries(); i++) {
        const Polygon* p = g->getGeometryN(i);
        checkHolesNotNested(p);
        if (hasInvalidError()) return false;
    }
    checkShellsNotNested(g);
    if (hasInvalidError()) return false;

    checkInteriorDisconnected(areaAnalyzer);
    if (hasInvalidError()) return false;

    return true;
}


/* private */
bool
IsValidOp::isValid(const GeometryCollection* gc)
{
    for (std::size_t i = 0; i < gc->getNumGeometries(); i++) {
        if (! isValidGeometry(gc->getGeometryN(i)))
            return false;
    }
    return true;
}


/* private */
void
IsValidOp::checkCoordinateInvalid(const CoordinateSequence* coords)
{
    for (std::size_t i = 0; i < coords->size(); i++) {
        if (! isValid(coords->getAt(i))) {
            logInvalid(TopologyValidationError::eInvalidCoordinate,
                       &coords->getAt(i));
            return;
        }
    }
}


/* private */
void
IsValidOp::checkCoordinateInvalid(const Polygon* poly)
{
    checkCoordinateInvalid(poly->getExteriorRing()->getCoordinatesRO());
    if (hasInvalidError()) return;
    for (std::size_t i = 0; i < poly->getNumInteriorRing(); i++) {
        checkCoordinateInvalid(poly->getInteriorRingN(i)->getCoordinatesRO());
        if (hasInvalidError()) return;
    }
}


/* private */
void
IsValidOp::checkRingNotClosed(const LinearRing* ring)
{
    if (ring->isEmpty()) return;
    if (! ring->isClosed()) {
        Coordinate pt = ring->getNumPoints() >= 1
                        ? ring->getCoordinateN(0)
                        : Coordinate();
        logInvalid(TopologyValidationError::eRingNotClosed, &pt);
        return;
    }
}


/* private */
void
IsValidOp::checkRingsNotClosed(const Polygon* poly)
{
    checkRingNotClosed(poly->getExteriorRing());
    if (hasInvalidError()) return;
    for (std::size_t i = 0; i < poly->getNumInteriorRing(); i++) {
        checkRingNotClosed(poly->getInteriorRingN(i));
        if (hasInvalidError()) return;
    }
}


/* private */
void
IsValidOp::checkRingsTooFewPoints(const Polygon* poly)
{
    checkRingTooFewPoints(poly->getExteriorRing());
    if (hasInvalidError()) return;
    for (std::size_t i = 0; i < poly->getNumInteriorRing(); i++) {
        checkRingTooFewPoints(poly->getInteriorRingN(i));
        if (hasInvalidError()) return;
    }
}


/* private */
void
IsValidOp::checkRingTooFewPoints(const LinearRing* ring)
{
    if (ring->isEmpty()) return;
    checkTooFewPoints(ring, MIN_SIZE_RING);
}


/* private */
void
IsValidOp::checkTooFewPoints(const LineString* line, std::size_t minSize)
{
    if (! isNonRepeatedSizeAtLeast(line, minSize) ) {
        Coordinate pt = line->getNumPoints() >= 1
                        ? line->getCoordinateN(0)
                        : Coordinate();
        logInvalid(TopologyValidationError::eTooFewPoints, &pt);
    }
}


/* private */
bool
IsValidOp::isNonRepeatedSizeAtLeast(const LineString* line, std::size_t minSize)
{
    std::size_t numPts = 0;
    const Coordinate* prevPt = nullptr;
    for (std::size_t i = 0; i < line->getNumPoints(); i++) {
        if (numPts >= minSize) return true;
        const Coordinate& pt = line->getCoordinateN(i);
        if (prevPt == nullptr || ! pt.equals2D(*prevPt))
            numPts++;
        prevPt = &pt;
    }
    return numPts >= minSize;
}


/* private */
void
IsValidOp::checkAreaIntersections(PolygonTopologyAnalyzer& areaAnalyzer)
{
    if (areaAnalyzer.hasIntersection()) {
         logInvalid(TopologyValidationError::eSelfIntersection,
                   areaAnalyzer.getIntersectionLocation());
        return;
    }
    if (areaAnalyzer.hasDoubleTouch()) {
        logInvalid(TopologyValidationError::eDisconnectedInterior,
                   areaAnalyzer.getIntersectionLocation());
        return;
    }
    if (areaAnalyzer.isInteriorDisconnectedBySelfTouch()) {
        logInvalid(TopologyValidationError::eDisconnectedInterior,
                   areaAnalyzer.getDisconnectionLocation());
        return;
    }
}


/* private */
void
IsValidOp::checkSelfIntersectingRing(const LinearRing* ring)
{
    const Coordinate* intPt = PolygonTopologyAnalyzer::findSelfIntersection(ring);
    if (intPt != nullptr) {
        logInvalid(TopologyValidationError::eRingSelfIntersection,
            intPt);
    }
}


/* private */
void
IsValidOp::checkHolesOutsideShell(const Polygon* poly)
{
    // skip test if no holes are present
    if (poly->getNumInteriorRing() <= 0) return;

    const LinearRing* shell = poly->getExteriorRing();
    bool isShellEmpty = shell->isEmpty();
    IndexedPointInAreaLocator pir(*shell);

    for (std::size_t i = 0; i < poly->getNumInteriorRing(); i++) {
        const LinearRing* hole = poly->getInteriorRingN(i);
        if (hole->isEmpty()) continue;

        const Coordinate* invalidPt = nullptr;
        if (isShellEmpty) {
            invalidPt = hole->getCoordinate();
        }
        else {
            invalidPt = findHoleOutsideShellPoint(pir, hole);
        }
        if (invalidPt != nullptr) {
            logInvalid(
                TopologyValidationError::eHoleOutsideShell,
                invalidPt);
            return;
        }
    }
}


/* private */
const Coordinate *
IsValidOp::findHoleOutsideShellPoint(
    IndexedPointInAreaLocator& shellLocator,
    const LinearRing* hole)
{
    for (std::size_t i = 0; i < hole->getNumPoints() - 1; i++) {
        const Coordinate& holePt = hole->getCoordinateN(i);
        Location loc = shellLocator.locate(&holePt);
        if (loc == Location::BOUNDARY)
            continue;
        if (loc == Location::INTERIOR)
            return nullptr;
        /**
         * Location is EXTERIOR, so hole is outside shell
         */
        return &holePt;
    }
    return nullptr;
}


/* private */
void
IsValidOp::checkHolesNotNested(const Polygon* poly)
{
    // skip test if no holes are present
    if (poly->getNumInteriorRing() <= 0) return;

    IndexedNestedHoleTester nestedTester(poly);
    if (nestedTester.isNested()) {
        logInvalid(TopologyValidationError::eNestedShells,
                   nestedTester.getNestedPoint());
    }
}


/* private */
void
IsValidOp::checkShellsNotNested(const MultiPolygon* mp)
{
    for (std::size_t i = 0; i < mp->getNumGeometries(); i++) {
        const Polygon* p = mp->getGeometryN(i);
        if (p->isEmpty())
            continue;
        const LinearRing* shell = p->getExteriorRing();
        for (std::size_t j = 0; j < mp->getNumGeometries(); j++) {
            if (i == j) continue;
            const Polygon* p2 = mp->getGeometryN(j);
            const Coordinate* invalidPt = findShellSegmentInPolygon(shell, p2);
            if (invalidPt != nullptr) {
            logInvalid(TopologyValidationError::eNestedShells,
                invalidPt);
            return;
            }
        }
    }
}


/* private */
const Coordinate *
IsValidOp::findShellSegmentInPolygon(const LinearRing* shell, const Polygon* poly)
{
    const LinearRing* polyShell = poly->getExteriorRing();
    if (polyShell->isEmpty()) return nullptr;

    //--- if envelope is not covered --> not nested
    if (! poly->getEnvelopeInternal()->covers(shell->getEnvelopeInternal()))
        return nullptr;

    const Coordinate& shell0 = shell->getCoordinateN(0);
    const Coordinate& shell1 = shell->getCoordinateN(1);

    if (! PolygonTopologyAnalyzer::isSegmentInRing(&shell0, &shell1, polyShell))
        return nullptr;

    /**
    * Check if the shell is inside a hole (if there are any).
    * If so this is valid.
    */
    for (std::size_t i = 0; i < poly->getNumInteriorRing(); i++) {
        const LinearRing* hole = poly->getInteriorRingN(i);
        if (hole->getEnvelopeInternal()->covers(shell->getEnvelopeInternal())
            && PolygonTopologyAnalyzer::isSegmentInRing(&shell0, &shell1, hole)) {
            return nullptr;
        }
    }

    /**
    * The shell is contained in the polygon, but is not contained in a hole.
    * This is invalid.
    */
    return &shell0;
}


/* private */
void
IsValidOp::checkInteriorDisconnected(PolygonTopologyAnalyzer& areaAnalyzer)
{
    if (areaAnalyzer.isInteriorDisconnectedByRingCycle())
        logInvalid(TopologyValidationError::eDisconnectedInterior,
                   areaAnalyzer.getDisconnectionLocation());
}


} // namespace geos.operation.valid
} // namespace geos.operation
} // namespace geos
