/**
 * @file CoordFrames.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief define the coordinate frames in the pylast
 * @version 0.1
 * @date 2025-03-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include "Coordinates.hh"
#include <type_traits>

// Forward declare all frame classes
class AltAzFrame;
class TelescopeFrame;
class CartesianFrame;
class TiltedGroundFrame;

// Define the representation trait first
template <typename FrameType>
struct FrameRepresentation;

// Specialize for AltAzFrame
template <>
struct FrameRepresentation<AltAzFrame> {
    using type = SphericalRepresentation;
};

// Specialize for TelescopeFrame
template <>
struct FrameRepresentation<TelescopeFrame> {
    using type = Point2D;
};

// Helper to get the representation type cleanly (handles const and references)
template <typename FrameType>
using RepresentationType = typename FrameRepresentation<std::remove_cv_t<std::remove_reference_t<FrameType>>>::type;

// Define the Frame template class
template<typename Derived>
class Frame
{
public:
    virtual ~Frame() = default;
    template<typename target_frame>
    auto transform_to(const RepresentationType<Derived>& point, const target_frame& target) const
    {
        const Derived& derived = static_cast<const Derived&>(*this);
        return derived.transform_to_imp(point, target);
    }
    friend Derived;
};

// Now fully define AltAzFrame
class AltAzFrame: public Frame<AltAzFrame>
{
public:
    // This is still useful for clarity, but not required for the Frame template
    using representation_type = SphericalRepresentation;
    auto transform_to_imp(const representation_type& point, const TelescopeFrame& target) const -> RepresentationType<TelescopeFrame>;
};

class TelescopeFrame: public Frame<TelescopeFrame>
{
public:
    TelescopeFrame(double azimuth, double altitude)
    : pointing_direction(azimuth, altitude) {
        auto rotation_azimuth = Eigen::AngleAxisd(azimuth, Eigen::Vector3d::UnitZ());
        // Sim_telarray Coordinates: X to the north, Y to the west, Z to the Up
        auto rotation_altitude = Eigen::AngleAxisd(-(M_PI/2 - altitude), Eigen::Vector3d::UnitY());
        rotation_matrix = (rotation_altitude * rotation_azimuth).toRotationMatrix();
    }
    TelescopeFrame(SphericalRepresentation pointing_direction):TelescopeFrame(pointing_direction.azimuth, pointing_direction.altitude) {};
    TelescopeFrame(const TelescopeFrame& other) = default;
    TelescopeFrame(TelescopeFrame&& other) = default;
    using representation_type = Point2D;
    auto transform_to_imp(const representation_type& point, const AltAzFrame& target) const -> RepresentationType<AltAzFrame>;

    SphericalRepresentation pointing_direction;
    Eigen::Matrix3d rotation_matrix; // Transform to the pointing direction
};

template<typename FrameType>
class SkyDirection
{
public:
    template<typename... Args>
    SkyDirection(FrameType frame, Args&&... args)
    : position(std::forward<Args>(args)...), frameRef(std::make_unique<FrameType>(std::move(frame))) {
    }
    RepresentationType<FrameType> position;
    
    std::unique_ptr<FrameType> frameRef;

    template<typename target_frame>
    auto transform_to(const target_frame& target) const -> SkyDirection<target_frame>
    {
        return SkyDirection<target_frame>(target, frameRef->transform_to(position, target));
    }
    
    // Forward any method calls to the position object
    template<typename... Args>
    auto operator->() {
        return &position;
    }
    
    template<typename... Args>
    auto operator->() const {
        return &position;
    }
    
    template<typename R, typename... Args>
    auto operator()(R (RepresentationType<FrameType>::*method)(Args...), Args&&... args) {
        return (position.*method)(std::forward<Args>(args)...);
    }
    
    template<typename R, typename... Args>
    auto operator()(R (RepresentationType<FrameType>::*method)(Args...) const, Args&&... args) const {
        return (position.*method)(std::forward<Args>(args)...);
    }
};

class TiltedGroundFrame: public TelescopeFrame
{
   public:
      TiltedGroundFrame(double azimuth, double altitude)
      : TelescopeFrame(azimuth, altitude) {
      }
      TiltedGroundFrame(SphericalRepresentation pointing_direction)
      : TelescopeFrame(pointing_direction.azimuth, pointing_direction.altitude) {
      }
      TiltedGroundFrame(SkyDirection<AltAzFrame> pointing_direction)
      : TelescopeFrame(pointing_direction->azimuth, pointing_direction->altitude) {
      }

};

class CartesianPoint
{
   public:
      CartesianPoint(double x, double y, double z)
      : point(x, y, z) {
      }
      CartesianPoint(std::array<double, 3> point)
      : point(point[0], point[1], point[2]) {
      }
      Eigen::Vector3d point;
      Eigen::Vector3d transform_to_tilted(const TiltedGroundFrame& target)
      {
         return target.rotation_matrix * point;
      }
      Eigen::Vector3d transform_to_ground(const TiltedGroundFrame& target)
      {
         return target.rotation_matrix.transpose() * point;
      }
};