/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2019 Daniel Baston <dbaston@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/operation/union/OverlapUnion.h>

#include <geos/geom/Coordinate.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/CoordinateSequenceFilter.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/LineSegment.h>
#include <geos/geom/util/GeometryCombiner.h>
#include <geos/util/TopologyException.h>

namespace geos {
namespace operation {
namespace geounion {

// https://github.com/locationtech/jts/blob/master/modules/core/src/main/java/org/locationtech/jts/operation/union/OverlapUnion.java

// xxx OverlapUnion::union leaks like a seive, check cascadedunion implementation for use of unique_ptr

using namespace geom;
using namespace geom::util;

/* public */
Geometry*
OverlapUnion::doUnion()
{
    Envelope overlapEnv = overlapEnvelope(g0, g1);
    /**
     * If no overlap, can just combine the geometries
     */
    if (overlapEnv.isNull()) {
        Geometry* g0Copy = g0->clone().get();
        Geometry* g1Copy = g1->clone().get();
        return GeometryCombiner::combine(g0Copy, g1Copy).get();
    }

    std::vector<Geometry*> disjointPolys;

    std::unique_ptr<Geometry> g0Overlap = extractByEnvelope(overlapEnv, g0, disjointPolys);
    std::unique_ptr<Geometry> g1Overlap = extractByEnvelope(overlapEnv, g1, disjointPolys);

    // std::out << "# geoms in common: " << intersectingPolys.size() << std::endl;
    std::unique_ptr<Geometry> theUnion(unionFull(g0Overlap.get(), g1Overlap.get()));
    isUnionSafe = isBorderSegmentsSame(theUnion.get(), overlapEnv);
    if (!isUnionSafe) {
        // overlap union changed border segments... need to do full union
        // std::out <<  "OverlapUnion: Falling back to full union" << std::endl;
        return unionFull(g0, g1);
    }
    else {
        // std::out << "OverlapUnion: fast path" << std::endl;
        return combine(theUnion, disjointPolys);
    }
}

/* private static */
Envelope
overlapEnvelope(const Geometry* geom0, const Geometry* geom1)
{
    const Envelope* g0Env = geom0->getEnvelopeInternal();
    const Envelope* g1Env = geom1->getEnvelopeInternal();
    Envelope overlapEnv;
    g0Env->intersection(*g1Env, overlapEnv);
    return overlapEnv;
}

/* private */
Geometry*
OverlapUnion::combine(std::unique_ptr<Geometry>& unionGeom, std::vector<Geometry*>& disjointPolys)
{
    if (disjointPolys.size() <= 0)
        return unionGeom.release();

    disjointPolys.push_back(unionGeom.release());
    return GeometryCombiner::combine(disjointPolys).release();
}

/* private */
std::unique_ptr<Geometry>
OverlapUnion::extractByEnvelope(const Envelope& env, const Geometry* geom, std::vector<Geometry*>& disjointGeoms)
{
    std::vector<Geometry*> intersectingGeoms;
    for (std::size_t i = 0; i < geom->getNumGeometries(); i++) {
        const Geometry* elem = geom->getGeometryN(i);
        if (elem->getEnvelopeInternal()->intersects(env)) {
            Geometry* copy = elem->clone().get();
            intersectingGeoms.push_back(copy);
        }
        else {
            Geometry* copy = elem->clone().get();
            disjointGeoms.push_back(copy);
        }
    }
    std::unique_ptr<Geometry> result(geomFactory->buildGeometry(intersectingGeoms));
    return result;
}

/* private */
Geometry*
OverlapUnion::unionFull(const Geometry* geom0, const Geometry* geom1)
{
    try {
        return geom0->Union(geom1).release();
    }
    catch (geos::util::TopologyException ex) {
        /**
         * If the overlay union fails,
         * try a buffer union, which often succeeds
         */
        return unionBuffer(geom0, geom1);
    }
}

/* private static */
Geometry*
unionBuffer(const Geometry* geom0, const Geometry* geom1)
{
    Geometry* copy0 = geom0->clone().release();
    Geometry* copy1 = geom1->clone().release();
    std::vector<Geometry*> geoms = {copy0, copy1};
    const GeometryFactory* factory = copy0->getFactory();
    std::unique_ptr<GeometryCollection> gColl(factory->createGeometryCollection(&geoms));
    return gColl->buffer(0.0).release();
}

/* private */
bool
OverlapUnion::isBorderSegmentsSame(const Geometry* result, const Envelope& env)
{
    std::vector<LineSegment*> segsBefore = extractBorderSegments(g0, g1, env);
    std::vector<LineSegment*> segsAfter;
    extractBorderSegments(result, env, segsAfter);
    //std::cout << ("# seg before: " << segsBefore.size() << " - # seg after: " << segsAfter.size() << std::endl;
    bool eq = isEqual(segsBefore, segsAfter);

    // Clean up temporary segment arrays
    for (auto seg : segsBefore) delete seg;
    for (auto seg : segsAfter) delete seg;

    return eq;
}

static bool lineSegmentPtrCmp(const LineSegment* a, const LineSegment* b)
{
    return a->compareTo(*b) < 0;
}

/* private */
bool
OverlapUnion::isEqual(std::vector<LineSegment*>& segs0, std::vector<LineSegment*>& segs1)
{
    if (segs0.size() != segs1.size())
        return false;

    std::sort(segs0.begin(), segs0.end(), lineSegmentPtrCmp);
    std::sort(segs1.begin(), segs1.end(), lineSegmentPtrCmp);

    size_t sz = segs0.size();
    for (std::size_t i = 0; i < sz; i++) {
        if (segs0[i]->p0.x != segs1[i]->p0.x ||
            segs0[i]->p0.y != segs1[i]->p0.y ||
            segs0[i]->p1.x != segs1[i]->p1.x ||
            segs0[i]->p1.y != segs1[i]->p1.y)
        {
            return false;
        }
    }

    return true;
}

/* private */
std::vector<LineSegment*>
OverlapUnion::extractBorderSegments(const Geometry* geom0, const Geometry* geom1, const Envelope& env)
{
    std::vector<LineSegment*> segs;
    extractBorderSegments(geom0, env, segs);
    if (geom1 != nullptr)
        extractBorderSegments(geom1, env, segs);
    return segs;
}

/* private static */
bool
intersects(const Envelope& env, const Coordinate& p0, const Coordinate& p1)
{
    return env.intersects(p0) || env.intersects(p1);
}

/* private static */
bool
containsProperly(const Envelope& env, const Coordinate& p)
{
    if (env.isNull()) return false;
    return p.x > env.getMinX() &&
           p.x < env.getMaxX() &&
           p.y > env.getMinY() &&
           p.y < env.getMaxY();
}

/* private static */
bool
containsProperly(const Envelope& env, const Coordinate& p0, const Coordinate& p1)
{
    return containsProperly(env, p0) && containsProperly(env, p1);
}

/* private static */
void
extractBorderSegments(const Geometry* geom, const Envelope& penv, std::vector<LineSegment*>& psegs)
{
    class BorderSegmentFilter : public CoordinateSequenceFilter {

    private:
        std::vector<LineSegment*>* segs;
        const Envelope env;

    public:

        BorderSegmentFilter(const Envelope& penv, std::vector<LineSegment*>* psegs)
            : env(penv),
              segs(psegs) {};

        bool
        isDone() const override { return false; }

        bool
        isGeometryChanged() const override  { return false; }

        void
        filter_ro(CoordinateSequence& seq, std::size_t i)
        {
            if (i <= 0) return;

            // extract LineSegment
            Coordinate p0, p1;
            seq.getAt(i-1, p0);
            seq.getAt(  i, p1);
            bool isBorder = intersects(env, p0, p1) && ! containsProperly(env, p0, p1);
            if (isBorder) {
                segs->push_back(new LineSegment(p0, p1));
            }
        }
    };

    BorderSegmentFilter bsf(penv, &psegs);
    geom->apply_ro(bsf);

}



}
}
}
