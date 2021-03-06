#pragma once

#include <mbgl/util/constants.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/geometry.hpp>
#include <mbgl/math/clamp.hpp>

namespace mbgl {

class ProjectedMeters {
private:
    double _northing; // Distance measured northwards.
    double _easting;  // Distance measured eastwards.

public:
    ProjectedMeters(double n_ = 0, double e_ = 0)
        : _northing(n_), _easting(e_) {
        if (std::isnan(_northing)) {
            throw std::domain_error("northing must not be NaN");
        }
        if (std::isnan(_easting)) {
            throw std::domain_error("easting must not be NaN");
        }
    }

    double northing() const { return _northing; }
    double easting() const { return _easting; }

    friend bool operator==(const ProjectedMeters& a, const ProjectedMeters& b) {
        return a._northing == b._northing && a._easting == b._easting;
    }

    friend bool operator!=(const ProjectedMeters& a, const ProjectedMeters& b) {
        return !(a == b);
    }
};

// Spherical Mercator projection
// http://docs.openlayers.org/library/spherical_mercator.html
class Projection {
public:
    // Map pixel width at given scale.
    static double worldSize(double scale) {
        return scale * util::tileSize;
    }

    static double getMetersPerPixelAtLatitude(double lat, double zoom) {
        const double constrainedZoom = util::clamp(zoom, util::MIN_ZOOM, util::MAX_ZOOM);
        const double constrainedScale = std::pow(2.0, constrainedZoom);
        const double constrainedLatitude = util::clamp(lat, -util::LATITUDE_MAX, util::LATITUDE_MAX);
        return std::cos(constrainedLatitude * util::DEG2RAD) * util::M2PI * util::EARTH_RADIUS_M / worldSize(constrainedScale);
    }

    static ProjectedMeters projectedMetersForLatLng(const LatLng& latLng) {
        const double constrainedLatitude = util::clamp(latLng.latitude(), -util::LATITUDE_MAX, util::LATITUDE_MAX);
        const double constrainedLongitude = util::clamp(latLng.longitude(), -util::LONGITUDE_MAX, util::LONGITUDE_MAX);

        const double easting  = util::EARTH_RADIUS_M * constrainedLongitude * util::DEG2RAD;
        const double northing  = util::EARTH_RADIUS_M * constrainedLatitude * util::DEG2RAD;

        return ProjectedMeters(northing, easting);
    }

    static LatLng latLngForProjectedMeters(const ProjectedMeters& projectedMeters) {
        double latitude = projectedMeters.northing() * util::RAD2DEG / util::EARTH_RADIUS_M;
        double longitude = projectedMeters.easting() * util::RAD2DEG / util::EARTH_RADIUS_M;

        latitude = util::clamp(latitude, -util::LATITUDE_MAX, util::LATITUDE_MAX);
        longitude = util::clamp(longitude, -util::LONGITUDE_MAX, util::LONGITUDE_MAX);

        return LatLng(latitude, longitude);
    }

    static Point<double> project(const LatLng& latLng, double scale) {
        return project_(latLng, worldSize(scale));
    }

    //Returns point on tile
    static Point<double> project(const LatLng& latLng, int32_t zoom) {
        return project_(latLng, 1 << zoom);
    }

    static LatLng unproject(const Point<double>& p, double scale, LatLng::WrapMode wrapMode = LatLng::Unwrapped) {
        auto p2 = p * util::DEGREES_MAX / worldSize(scale);
        return LatLng {
            util::clamp(util::LATITUDE_MAX - p2.y, -util::LATITUDE_MAX, util::LATITUDE_MAX),
            p2.x - util::LONGITUDE_MAX,
            wrapMode
        };
    }

private:
    static Point<double> project_(const LatLng& latLng, double worldSize) {
        const double latitude = util::clamp(latLng.latitude(), -util::LATITUDE_MAX, util::LATITUDE_MAX);
        return Point<double> {
            util::LONGITUDE_MAX + latLng.longitude(),
            util::LATITUDE_MAX - latitude
        } * worldSize / util::DEGREES_MAX;
    }
};

} // namespace mbgl
