/**
 * @file Coordinates.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  describe the different coordinate systems
 * @version 0.1
 * @date 2025-01-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once
#include <memory>
#include <cmath>
#include <stdexcept>
#include <Eigen/Dense>
#include <optional>
class SphericalRepresentation;
struct CartesianRepresentation
{
    CartesianRepresentation(double x, double y, double z) : direction(x, y, z) {};
    CartesianRepresentation(Eigen::Vector3d direction) : direction(direction) {};
    void normalize()
    {
        direction.normalize();
    }
    double dot(const CartesianRepresentation& other) const
    {
        return direction.dot(other.direction);
    }
    SphericalRepresentation transform_to_spherical() const;
    Eigen::Vector3d direction;
};

struct SphericalRepresentation
{
    SphericalRepresentation() = default;
    SphericalRepresentation(double azimuth, double altitude) : azimuth(azimuth), altitude(altitude) {};
    SphericalRepresentation(const SphericalRepresentation& other) = default;
    SphericalRepresentation(SphericalRepresentation&& other) = default;
    SphericalRepresentation& operator=(const SphericalRepresentation& other) = default;
    SphericalRepresentation& operator=(SphericalRepresentation&& other) = default;
    double azimuth;
    double altitude;
    CartesianRepresentation transform_to_cartesian() const
    {
        return CartesianRepresentation(
            std::cos(azimuth) * std::cos(altitude),
            -std::sin(azimuth) * std::cos(altitude),
            std::sin(altitude)
        );
    }
    double angle_separation(const SphericalRepresentation& other) const
    {
        auto unit_this = this->transform_to_cartesian();
        auto unit_other = other.transform_to_cartesian();
        return std::acos(unit_this.dot(unit_other));
    }
};

class Line2D;
struct Point2D
{
    Point2D(Eigen::Vector2d point) : point(point) {};
    Point2D(double x, double y) : point(x, y) {};
    Point2D(Point2D&& other) = default;
    Point2D(const Point2D& other) = default;
    Point2D& operator=(const Point2D& other) = default;
    Point2D& operator=(Point2D&& other) = default;
    Point2D(std::initializer_list<double> point)
    {
        if(point.size() != 2)
        {
            throw std::invalid_argument("Point must be 2D vector");
        }
        this->point = Eigen::Map<const Eigen::Vector2d>(point.begin());
    }
    bool operator==(const Point2D& other) const
    {
        return point == other.point;
    }
    double distance(const Line2D& line) const;
    double x() const { return point.x(); }
    double y() const { return point.y(); }
    Eigen::Vector2d point;
};

struct Line2D
{
    Line2D(Eigen::Vector2d point, Eigen::Vector2d direction) : line(point, direction) {};
    Line2D(Point2D point, Eigen::Vector2d direction) : line(point.point, direction) {};
    Line2D(std::initializer_list<double> point, std::initializer_list<double> direction)
    {
        if(point.size() != 2 || direction.size() != 2)
        {
            throw std::invalid_argument("Point and direction must be 2D vectors");
        }
        // Normalize the direction vector
        Eigen::Vector2d normalized_direction = Eigen::Map<const Eigen::Vector2d>(direction.begin()).normalized();
        line = Eigen::ParametrizedLine<double, 2>(Eigen::Map<const Eigen::Vector2d>(point.begin()), normalized_direction);
    }
    double distance(const Point2D& point) const
    {
        return line.distance(point.point);
    }
    std::optional<Point2D> intersection(const Line2D& other) const
    {
        Eigen::Hyperplane<double, 2> otherplane(other.line);
        auto intersection_point = line.intersectionPoint(otherplane);
        if(intersection_point.allFinite())
        {
            return Point2D(intersection_point);
        }
        else
        {
            return std::nullopt;
        }
    }
    Eigen::ParametrizedLine<double, 2> line;
};

struct Point3D
{
    Point3D(Eigen::Vector3d point) : point(point) {};
    Point3D(double x, double y, double z) : point(x, y, z) {};
    Eigen::Vector3d point;
};

struct CameraPoint: public Point2D
{
    CameraPoint(std::initializer_list<double> point) : Point2D(point) {};
    CameraPoint(Eigen::Vector2d point) : Point2D(point) {};
};
