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
// Define a type trait to get the representation type.

// Forward declare all frame classes
class AltAzFrame;
class TelescopeFrame;
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
         auto rotation_altitude = Eigen::AngleAxisd(-(M_PI/2 - altitude), Eigen::Vector3d::UnitY());
         rotation_matrix = (rotation_altitude * rotation_azimuth).toRotationMatrix();
      }
      using representation_type = Point2D;
      auto transform_to_imp(const representation_type& point, const AltAzFrame& target) const -> RepresentationType<AltAzFrame>;

      SphericalRepresentation pointing_direction;
      Eigen::Matrix3d rotation_matrix; // Transform to the pointing direction
};

class SkyDirection
{
   public:
   SkyDirection(double azimuth, double altitude)
   : direction(azimuth, altitude) {
       frame = std::make_unique<AltAzFrame>();
   };
   SphericalRepresentation direction;

   std::unique_ptr<AltAzFrame> frame;
   template<typename target_frame>
   auto transform_to(const target_frame& target) const
   {
      return frame->transform_to(direction, target);
   }
};

