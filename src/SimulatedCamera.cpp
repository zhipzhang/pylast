#include "SimulatedCamera.hh"

SimulatedCamera::SimulatedCamera(int n_pixels, int* pe_count, double impact_parameter)
    : impact_parameter(impact_parameter, 0.0) {
    true_image = Eigen::VectorXi{Eigen::Map<Eigen::VectorXi>(pe_count, n_pixels)};
    true_image_sum = true_image.sum();
}