#include "CameraGeometry.hh"
#include "spdlog/fmt/fmt.h"
#include "spdlog/spdlog.h"
#include "nanoflann.hpp"
#include <cstddef>
#include <algorithm>
CameraGeometry::CameraGeometry(std::string camera_name, int num_pixels, double* pix_x, double* pix_y, double* pix_area, int* pix_type, double cam_rotation):
    camera_name(camera_name), num_pixels(num_pixels), cam_rotation(cam_rotation)
{
    this->pix_id = Eigen::VectorXi::LinSpaced(num_pixels, 0, num_pixels-1);
    this->pix_x = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(pix_x, num_pixels));
    this->pix_y = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(pix_y, num_pixels));
    this->pix_area = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(pix_area, num_pixels));
    this->pix_type = Eigen::VectorXi(Eigen::Map<Eigen::VectorXi>(pix_type, num_pixels));
    compute_neighbor_matrix(false);
}
void CameraGeometry::compute_neighbor_matrix(bool diagnal )
{
    // Create a matrix of 2D points from pixel coordinates
    Eigen::Matrix<double, Eigen::Dynamic, 2, Eigen::RowMajor> points(num_pixels, 2);
    points.col(0) = pix_x;
    points.col(1) = pix_y;
    // Create KD-tree adapter for the points matrix
    using KDTreeAdapter = nanoflann::KDTreeEigenMatrixAdaptor<Eigen::Matrix<double, Eigen::Dynamic, 2, Eigen::RowMajor>>;
    KDTreeAdapter kdtree(2, std::cref(points), 20);
    int min_neighbors;
    double radius ;
    if(pix_type[0] == 0 || pix_type[0] == 1)
    {
        min_neighbors = 6;
        radius = 1.4;
    }
    else if(pix_type[0] == 2)
    {
        min_neighbors = 4;
        radius = 1.4;
        if(diagnal)
        {
            min_neighbors = 8;
            radius = 1.99;
        }
    }
    else {
        throw std::runtime_error("Invalid pixel type");
    }
    neigh_matrix.resize(num_pixels, num_pixels);
    std::vector<size_t> indices(min_neighbors + 1);
    std::vector<double> distances(min_neighbors + 1);
    for(int i = 0; i < num_pixels; i++)
    {
        nanoflann::KNNResultSet<double> resultSet(min_neighbors + 1);
        resultSet.init(&indices[0], &distances[0]);
        kdtree.index_->findNeighbors(resultSet, points.row(i).data());
        double min_squared_dist = std::numeric_limits<double>::max();
        for(int j = 0; j < min_neighbors + 1; j++)
        {
            if(indices[j] != static_cast<size_t>(i) && distances[j] < min_squared_dist){
                min_squared_dist = distances[j];
            }
        }
        
        for(int j = 0; j < min_neighbors + 1; j++)
        {
            if(indices[j] == static_cast<size_t>(i)){
                continue;
            }
            if(distances[j] < radius * radius * min_squared_dist)
            {
                neigh_matrix.coeffRef(i, indices[j]) = 1;
            }
        }
    }
    neigh_matrix.makeCompressed();
}
Eigen::Vector<bool, -1> CameraGeometry::get_border_pixel_mask(int width)
{
    try{
        return border_pixel_mask.at(width);
    }
    catch(const std::out_of_range& e)
    {
        spdlog::debug("Computing border pixel mask for width {}", width);
        Eigen::VectorXi neighbor_number = neigh_matrix * Eigen::VectorXi::Ones(neigh_matrix.cols());
        double max_neighbor_number = neighbor_number.maxCoeff();
        Eigen::Vector<bool, -1> outermost_pixel_mask = neighbor_number.array() < max_neighbor_number;
        if(width == 1)
        {
            border_pixel_mask[width] = outermost_pixel_mask;
            return outermost_pixel_mask;
        }
        else
        {
            Eigen::Vector<bool, -1> border_mask;
            Eigen::Vector<bool, -1> previous_border_mask = outermost_pixel_mask;
            for(int i = 0; i < width - 1; i++)
            {
                border_mask = (neigh_matrix * previous_border_mask.cast<int>()).array() > 0;
                border_mask = (border_mask.array() || previous_border_mask.array());
                previous_border_mask = border_mask;
            }
            this->border_pixel_mask[width] = border_mask;
            return border_mask;
        }
        
    }
}
const string CameraGeometry::print() const
{
    return fmt::format("CameraGeometry(\n"
    "    camera_name: {}\n"
    "    num_pixels: {}\n"
    "    cam_rotation: {:.3f} deg\n"
    ")", camera_name, num_pixels, cam_rotation);
}