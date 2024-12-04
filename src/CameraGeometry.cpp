#include "CameraGeometry.hh"
#include "spdlog/fmt/fmt.h"

CameraGeometry::CameraGeometry(std::string camera_name, int num_pixels, double* pix_x, double* pix_y, double* pix_area, int* pix_type, double cam_rotation):
    camera_name(camera_name), num_pixels(num_pixels), cam_rotation(cam_rotation)
{
    this->pix_id = Eigen::VectorXi::LinSpaced(num_pixels, 0, num_pixels-1);
    this->pix_x = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(pix_x, num_pixels));
    this->pix_y = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(pix_y, num_pixels));
    this->pix_area = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(pix_area, num_pixels));
    this->pix_type = Eigen::VectorXi(Eigen::Map<Eigen::VectorXi>(pix_type, num_pixels));
}
const string CameraGeometry::print() const
{
    return fmt::format("CameraGeometry(\n"
    "    camera_name: {}\n"
    "    num_pixels: {}\n"
    "    cam_rotation: {:.3f} deg\n"
    ")", camera_name, num_pixels, cam_rotation);
}