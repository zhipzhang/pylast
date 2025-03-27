#pragma once
#include <Eigen/Dense>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <memory>
#include <algorithm>

namespace eigen_histogram {

// Forward declarations
template<typename Precision = float>
class Histogram1D;

template<typename Precision = float>
class Histogram2D;

// Axis types
enum class AxisType {
    Regular,    // Regular binning with equal width
    Irregular,  // Irregular binning with custom bin edges
    Log         // Logarithmic binning
};

// Base axis class
template<typename Precision = float>
class Axis {
public:
    virtual ~Axis() = default;
    virtual int index(Precision x) const = 0;
    virtual Precision bin_center(int idx) const = 0;
    virtual Precision bin_lower(int idx) const = 0;
    virtual Precision bin_upper(int idx) const = 0;
    virtual int bins() const = 0;
    virtual std::unique_ptr<Axis<Precision>> clone() const = 0;
    virtual Precision get_low_edge() const = 0;
    virtual Precision get_high_edge() const = 0;
};

// Regular axis with equal bin widths
template<typename Precision = float>
class RegularAxis : public Axis<Precision> {
private:
    Precision min_;
    Precision max_;
    int bins_;
    Precision width_;

public:
    RegularAxis(Precision min, Precision max, int bins)
        : min_(min), max_(max), bins_(bins), width_((max - min) / bins) {}

    int index(Precision x) const override {
        if (x < min_ || x >= max_) return -1;
        return static_cast<int>((x - min_) / width_);
    }

    Precision bin_center(int idx) const override {
        return min_ + (idx + 0.5) * width_;
    }

    Precision bin_lower(int idx) const override {
        return min_ + idx * width_;
    }

    Precision bin_upper(int idx) const override {
        return min_ + (idx + 1) * width_;
    }

    int bins() const override {
        return bins_;
    }

    std::unique_ptr<Axis<Precision>> clone() const override {
        return std::make_unique<RegularAxis<Precision>>(min_, max_, bins_);
    }
    
    Precision get_low_edge() const override {
        return min_;
    }
    
    Precision get_high_edge() const override {
        return max_;
    }
};

// Irregular axis with custom bin edges
template<typename Precision = float>
class IrregularAxis : public Axis<Precision> {
private:
    std::vector<Precision> edges_;

public:
    IrregularAxis(const std::vector<Precision>& edges) : edges_(edges) {
        if (edges.size() < 2) {
            throw std::invalid_argument("Axis needs at least 2 edges");
        }
    }

    int index(Precision x) const override {
        if (x < edges_.front() || x >= edges_.back()) return -1;
        auto it = std::upper_bound(edges_.begin(), edges_.end(), x);
        return static_cast<int>(std::distance(edges_.begin(), it) - 1);
    }

    Precision bin_center(int idx) const override {
        return (edges_[idx] + edges_[idx + 1]) / 2;
    }

    Precision bin_lower(int idx) const override {
        return edges_[idx];
    }

    Precision bin_upper(int idx) const override {
        return edges_[idx + 1];
    }

    int bins() const override {
        return static_cast<int>(edges_.size() - 1);
    }

    std::unique_ptr<Axis<Precision>> clone() const override {
        return std::make_unique<IrregularAxis<Precision>>(edges_);
    }
    
    Precision get_low_edge() const override {
        return edges_.front();
    }
    
    Precision get_high_edge() const override {
        return edges_.back();
    }
};

// Logarithmic axis
template<typename Precision = float>
class LogAxis : public Axis<Precision> {
private:
    Precision min_;
    Precision max_;
    int bins_;
    Precision log_min_;
    Precision log_max_;
    Precision log_width_;

public:
    LogAxis(Precision min, Precision max, int bins)
        : min_(min), max_(max), bins_(bins) {
        if (min <= 0 || max <= 0) {
            throw std::invalid_argument("Log axis requires positive bounds");
        }
        log_min_ = std::log(min);
        log_max_ = std::log(max);
        log_width_ = (log_max_ - log_min_) / bins;
    }

    int index(Precision x) const override {
        if (x < min_ || x >= max_) return -1;
        return static_cast<int>((std::log(x) - log_min_) / log_width_);
    }

    Precision bin_center(int idx) const override {
        Precision log_center = log_min_ + (idx + 0.5) * log_width_;
        return std::exp(log_center);
    }

    Precision bin_lower(int idx) const override {
        Precision log_lower = log_min_ + idx * log_width_;
        return std::exp(log_lower);
    }

    Precision bin_upper(int idx) const override {
        Precision log_upper = log_min_ + (idx + 1) * log_width_;
        return std::exp(log_upper);
    }

    int bins() const override {
        return bins_;
    }

    std::unique_ptr<Axis<Precision>> clone() const override {
        return std::make_unique<LogAxis<Precision>>(min_, max_, bins_);
    }
    
    Precision get_low_edge() const override {
        return min_;
    }
    
    Precision get_high_edge() const override {
        return max_;
    }
};

// Factory functions for creating axes
template<typename Precision = float>
std::unique_ptr<Axis<Precision>> make_regular_axis(Precision min, Precision max, int bins) {
    return std::make_unique<RegularAxis<Precision>>(min, max, bins);
}

template<typename Precision = float>
std::unique_ptr<Axis<Precision>> make_irregular_axis(const std::vector<Precision>& edges) {
    return std::make_unique<IrregularAxis<Precision>>(edges);
}

template<typename Precision = float>
std::unique_ptr<Axis<Precision>> make_log_axis(Precision min, Precision max, int bins) {
    return std::make_unique<LogAxis<Precision>>(min, max, bins);
}

// Base Histogram class
template<typename Precision = float>
class Histogram {
public:
    virtual ~Histogram() = default;
    virtual void reset() = 0;
    virtual void print(std::ostream& os = std::cout) const = 0;
    virtual int get_dimension() const = 0;
    virtual Histogram<Precision>& operator+ (const Histogram<Precision>& other) = 0;
};

// 1D Histogram class
template<typename Precision>
class Histogram1D : public Histogram<Precision> {
private:
    std::unique_ptr<Axis<Precision>> axis_;
    Eigen::Matrix<Precision, Eigen::Dynamic, 1> bins_;
    Precision underflow_;
    Precision overflow_;

public:
    Histogram1D(std::unique_ptr<Axis<Precision>> axis)
        : axis_(std::move(axis)), 
          bins_(Eigen::Matrix<Precision, Eigen::Dynamic, 1>::Zero(axis_->bins())),
          underflow_(0), overflow_(0) {}
    virtual int get_dimension() const override {
        return 1;
    }
    Precision get_low_edge() const
    {
        return axis_->get_low_edge();
    }
    Precision get_high_edge() const
    {
        return axis_->get_high_edge();
    }
    // Copy constructor
    Histogram1D(const Histogram1D& other)
        : axis_(other.axis_->clone()),
          bins_(other.bins_),
          underflow_(other.underflow_),
          overflow_(other.overflow_) {}

    // Move constructor
    Histogram1D(Histogram1D&& other) noexcept = default;

    // Copy assignment
    Histogram1D& operator=(const Histogram1D& other) {
        if (this != &other) {
            axis_ = other.axis_->clone();
            bins_ = other.bins_;
            underflow_ = other.underflow_;
            overflow_ = other.overflow_;
        }
        return *this;
    }

    // Move assignment
    Histogram1D& operator=(Histogram1D&& other) noexcept = default;
    Histogram1D& operator+ (const Histogram<Precision>& other) override {
    const Histogram1D<Precision>* hist1d = dynamic_cast<const Histogram1D<Precision>*>(&other);
    if (!hist1d) {
        throw std::invalid_argument("Cannot add histograms of different types");
    }
        // Check if histograms are compatible
    if (bins_.size() != hist1d->bins_.size() || 
        axis_->get_low_edge() != hist1d->get_low_edge() || 
        axis_->get_high_edge() != hist1d->get_high_edge()) {
        throw std::invalid_argument("Cannot add histograms with different binning");
    }
    
    // Add bin contents
    bins_ += hist1d->bins_;
    underflow_ += hist1d->underflow_;
    overflow_ += hist1d->overflow_;
    
    return *this;
    }
    // Fill a single value
    void fill(Precision x, Precision weight = 1.0) {
        int idx = axis_->index(x);
        if (idx >= 0 && idx < bins_.size()) {
            bins_(idx) += weight;
        } else if (x < axis_->bin_lower(0)) {
            underflow_ += weight;
        } else {
            overflow_ += weight;
        }
    }

    // Fill multiple values efficiently
    void fill(const Eigen::Matrix<Precision, Eigen::Dynamic, 1>& values,
              const Eigen::Matrix<Precision, Eigen::Dynamic, 1>& weights = Eigen::Matrix<Precision, Eigen::Dynamic, 1>()) {
        
        bool use_weights = weights.size() > 0;
        if (use_weights && weights.size() != values.size()) {
            throw std::invalid_argument("Weights array must have the same size as values array");
        }

        for (int i = 0; i < values.size(); ++i) {
            Precision weight = use_weights ? weights(i) : 1.0;
            fill(values(i), weight);
        }
    }

    // Get bin content
    Precision at(int idx) const {
        if (idx < 0 || idx >= bins_.size()) {
            throw std::out_of_range("Bin index out of range");
        }
        return bins_(idx);
    }

    // Get bin content with operator()
    Precision operator()(int idx) const {
        return at(idx);
    }

    // Get bin center
    Precision center(int idx) const {
        if (idx < 0 || idx >= bins_.size()) {
            throw std::out_of_range("Bin index out of range");
        }
        return axis_->bin_center(idx);
    }

    // Get all bin contents
    const Eigen::Matrix<Precision, Eigen::Dynamic, 1>& values() const {
        return bins_;
    }

    // Get all bin centers
    Eigen::Vector<Precision, Eigen::Dynamic> centers() const {
        Eigen::Vector<Precision, Eigen::Dynamic> result(bins_.size());
        for (int i = 0; i < bins_.size(); ++i) {
            result(i) = center(i);
        }
        return result;
    }
    std::vector<Precision> vec_centers() const {
        std::vector<Precision> result(bins_.size());
        for (int i = 0; i < bins_.size(); ++i) {
            result[i] = center(i);
        }
        return result;
    }
    Precision get_bin_center(int idx) const {
        return axis_->bin_center(idx);
    }
    Precision get_bin_content(int idx) const {
        return operator()(idx);
    }
    // Get number of bins
    int bins() const {
        return static_cast<int>(bins_.size());
    }

    // Get underflow
    Precision underflow() const {
        return underflow_;
    }

    // Get overflow
    Precision overflow() const {
        return overflow_;
    }

    // Reset the histogram
    void reset() override {
        bins_.setZero();
        underflow_ = 0;
        overflow_ = 0;
    }

    // Print the histogram
    void print(std::ostream& os = std::cout) const override {
        os << "Underflow: " << underflow_ << std::endl;
        for (int i = 0; i < bins_.size(); ++i) {
            os << "[" << axis_->bin_lower(i) << ", " << axis_->bin_upper(i) 
               << "): " << bins_(i) << std::endl;
        }
        os << "Overflow: " << overflow_ << std::endl;
    }
};

// 2D Histogram class
template<typename Precision>
class Histogram2D : public Histogram<Precision> {
private:
    std::unique_ptr<Axis<Precision>> x_axis_;
    std::unique_ptr<Axis<Precision>> y_axis_;
    Eigen::Matrix<Precision, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> bins_;
    Precision underflow_x_;
    Precision overflow_x_;
    Precision underflow_y_;
    Precision overflow_y_;
    Precision underflow_xy_;
    Precision overflow_xy_;

public:
    virtual int get_dimension() const override {
        return 2;
    }
    Precision get_x_low_edge() const
    {
        return x_axis_->get_low_edge();
    }
    Precision get_x_high_edge() const
    {
        return x_axis_->get_high_edge();
    }
    Precision get_y_low_edge() const
    {
        return y_axis_->get_low_edge();
    }
    Precision get_y_high_edge() const
    {
        return y_axis_->get_high_edge();
    }
    Histogram2D(std::unique_ptr<Axis<Precision>> x_axis, std::unique_ptr<Axis<Precision>> y_axis)
        : x_axis_(std::move(x_axis)), y_axis_(std::move(y_axis)),
          bins_(Eigen::Matrix<Precision, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>::Zero(x_axis_->bins(), y_axis_->bins())),
          underflow_x_(0), overflow_x_(0), underflow_y_(0), overflow_y_(0), 
          underflow_xy_(0), overflow_xy_(0) {}

    // Copy constructor
    Histogram2D(const Histogram2D& other)
        : x_axis_(other.x_axis_->clone()),
          y_axis_(other.y_axis_->clone()),
          bins_(other.bins_),
          underflow_x_(other.underflow_x_),
          overflow_x_(other.overflow_x_),
          underflow_y_(other.underflow_y_),
          overflow_y_(other.overflow_y_),
          underflow_xy_(other.underflow_xy_),
          overflow_xy_(other.overflow_xy_) {}

    // Move constructor
    Histogram2D(Histogram2D&& other) noexcept = default;
    Histogram2D& operator+ (const Histogram<Precision>& other) override {
        const Histogram2D<Precision>* hist2d = dynamic_cast<const Histogram2D<Precision>*>(&other);
        if (!hist2d) {
            throw std::invalid_argument("Cannot add histograms of different types");
        }
        if (bins_.rows() != hist2d->bins_.rows() || 
            bins_.cols() != hist2d->bins_.cols() || 
            x_axis_->get_low_edge() != hist2d->get_x_low_edge() || 
            x_axis_->get_high_edge() != hist2d->get_x_high_edge() || 
            y_axis_->get_low_edge() != hist2d->get_y_low_edge() || 
            y_axis_->get_high_edge() != hist2d->get_y_high_edge()) {
            throw std::invalid_argument("Cannot add histograms with different binning");
        }
        bins_ += hist2d->bins_;
        underflow_x_ += hist2d->underflow_x_;
        overflow_x_ += hist2d->overflow_x_;
        underflow_y_ += hist2d->underflow_y_;
        overflow_y_ += hist2d->overflow_y_;
        underflow_xy_ += hist2d->underflow_xy_;
        overflow_xy_ += hist2d->overflow_xy_;
        return *this;
    }

    // Copy assignment
    Histogram2D& operator=(const Histogram2D& other) {
        if (this != &other) {
            x_axis_ = other.x_axis_->clone();
            y_axis_ = other.y_axis_->clone();
            bins_ = other.bins_;
            underflow_x_ = other.underflow_x_;
            overflow_x_ = other.overflow_x_;
            underflow_y_ = other.underflow_y_;
            overflow_y_ = other.overflow_y_;
            underflow_xy_ = other.underflow_xy_;
            overflow_xy_ = other.overflow_xy_;
        }
        return *this;
    }

    // Move assignment
    Histogram2D& operator=(Histogram2D&& other) noexcept = default;

    // Fill a single value
    void fill(Precision x, Precision y, Precision weight = 1.0) {
        int x_idx = x_axis_->index(x);
        int y_idx = y_axis_->index(y);

        bool x_in_range = (x_idx >= 0 && x_idx < bins_.rows());
        bool y_in_range = (y_idx >= 0 && y_idx < bins_.cols());

        if (x_in_range && y_in_range) {
            bins_(x_idx, y_idx) += weight;
        } else if (!x_in_range && !y_in_range) {
            underflow_xy_ += weight;
        } else if (!x_in_range) {
            if (x < x_axis_->bin_lower(0)) {
                underflow_x_ += weight;
            } else {
                overflow_x_ += weight;
            }
        } else { // !y_in_range
            if (y < y_axis_->bin_lower(0)) {
                underflow_y_ += weight;
            } else {
                overflow_y_ += weight;
            }
        }
    }

    // Fill multiple values efficiently
    void fill(const Eigen::Matrix<Precision, Eigen::Dynamic, 1>& x_values,
              const Eigen::Matrix<Precision, Eigen::Dynamic, 1>& y_values,
              const Eigen::Matrix<Precision, Eigen::Dynamic, 1>& weights = Eigen::Matrix<Precision, Eigen::Dynamic, 1>()) {
        
        if (x_values.size() != y_values.size()) {
            throw std::invalid_argument("x and y arrays must have the same size");
        }
        
        bool use_weights = weights.size() > 0;
        if (use_weights && weights.size() != x_values.size()) {
            throw std::invalid_argument("Weights array must have the same size as x and y arrays");
        }

        for (int i = 0; i < x_values.size(); ++i) {
            Precision weight = use_weights ? weights(i) : 1.0;
            fill(x_values(i), y_values(i), weight);
        }
    }

    // Get bin content
    Precision at(int x_idx, int y_idx) const {
        if (x_idx < 0 || x_idx >= bins_.rows() || y_idx < 0 || y_idx >= bins_.cols()) {
            throw std::out_of_range("Bin indices out of range");
        }
        return bins_(x_idx, y_idx);
    }

    // Get bin content with operator()
    Precision operator()(int x_idx, int y_idx) const {
        return at(x_idx, y_idx);
    }

    // Get bin center for x
    Precision x_center(int idx) const {
        if (idx < 0 || idx >= bins_.rows()) {
            throw std::out_of_range("Bin index out of range");
        }
        return x_axis_->bin_center(idx);
    }

    // Get bin center for y
    Precision y_center(int idx) const {
        if (idx < 0 || idx >= bins_.cols()) {
            throw std::out_of_range("Bin index out of range");
        }
        return y_axis_->bin_center(idx);
    }

    // Get all bin contents
    const Eigen::Matrix<Precision, Eigen::Dynamic, Eigen::Dynamic>& values() const {
        return bins_;
    }

    // Get all x bin centers
    Eigen::Matrix<Precision, Eigen::Dynamic, 1> x_centers() const {
        Eigen::Matrix<Precision, Eigen::Dynamic, 1> result(bins_.rows());
        for (int i = 0; i < bins_.rows(); ++i) {
            result(i) = x_axis_->bin_center(i);
        }
        return result;
    }

    // Get all y bin centers
    Eigen::Matrix<Precision, Eigen::Dynamic, 1> y_centers() const {
        Eigen::Matrix<Precision, Eigen::Dynamic, 1> result(bins_.cols());
        for (int i = 0; i < bins_.cols(); ++i) {
            result(i) = y_axis_->bin_center(i);
        }
        return result;
    }

    // Get number of x bins
    int x_bins() const {
        return bins_.rows();
    }

    // Get number of y bins
    int y_bins() const {
        return bins_.cols();
    }

    // Get underflow and overflow statistics
    Precision underflow_x() const { return underflow_x_; }
    Precision overflow_x() const { return overflow_x_; }
    Precision underflow_y() const { return underflow_y_; }
    Precision overflow_y() const { return overflow_y_; }
    Precision underflow_xy() const { return underflow_xy_; }
    Precision overflow_xy() const { return overflow_xy_; }

    // Reset the histogram
    void reset() override {
        bins_.setZero();
        underflow_x_ = 0;
        overflow_x_ = 0;
        underflow_y_ = 0;
        overflow_y_ = 0;
        underflow_xy_ = 0;
        overflow_xy_ = 0;
    }

    // Print the histogram
    void print(std::ostream& os = std::cout) const override {
        os << "2D Histogram with " << x_bins() << "x" << y_bins() << " bins" << std::endl;
        os << "Underflow x: " << underflow_x_ << ", Overflow x: " << overflow_x_ << std::endl;
        os << "Underflow y: " << underflow_y_ << ", Overflow y: " << overflow_y_ << std::endl;
        os << "Underflow xy: " << underflow_xy_ << ", Overflow xy: " << overflow_xy_ << std::endl;
        os << "Bin contents:" << std::endl;
        os << bins_ << std::endl;
    }
};

// Factory functions for creating histograms
template<typename Precision = float>
Histogram1D<Precision> make_histogram(std::unique_ptr<Axis<Precision>> axis) {
    return Histogram1D<Precision>(std::move(axis));
}

template<typename Precision = float>
Histogram2D<Precision> make_histogram(std::unique_ptr<Axis<Precision>> x_axis, 
                                     std::unique_ptr<Axis<Precision>> y_axis) {
    return Histogram2D<Precision>(std::move(x_axis), std::move(y_axis));
}

// Convenience functions for common histogram types
template<typename Precision = double>
Histogram1D<Precision> make_regular_histogram(Precision min, Precision max, int bins) {
    return make_histogram<Precision>(make_regular_axis<Precision>(min, max, bins));
}

template<typename Precision = float>
Histogram1D<Precision> make_log_histogram(Precision min, Precision max, int bins) {
    return make_histogram<Precision>(make_log_axis<Precision>(min, max, bins));
}

template<typename Precision = float>
Histogram2D<Precision> make_regular_histogram_2d(
    Precision x_min, Precision x_max, int x_bins,
    Precision y_min, Precision y_max, int y_bins) {
    return make_histogram<Precision>(
        make_regular_axis<Precision>(x_min, x_max, x_bins),
        make_regular_axis<Precision>(y_min, y_max, y_bins)
    );
}

template<typename Precision = float>
Histogram2D<Precision> make_log_histogram_2d(
    Precision x_min, Precision x_max, int x_bins,
    Precision y_min, Precision y_max, int y_bins) {
    return make_histogram<Precision>(
        make_log_axis<Precision>(x_min, x_max, x_bins),
        make_log_axis<Precision>(y_min, y_max, y_bins)
    );
}

template<typename Precision = float>
Histogram2D<Precision> make_mixed_histogram_2d(
    Precision x_min, Precision x_max, int x_bins, bool x_log,
    Precision y_min, Precision y_max, int y_bins, bool y_log) {
    
    std::unique_ptr<Axis<Precision>> x_axis = x_log ? 
        make_log_axis<Precision>(x_min, x_max, x_bins) : 
        make_regular_axis<Precision>(x_min, x_max, x_bins);
    
    std::unique_ptr<Axis<Precision>> y_axis = y_log ? 
        make_log_axis<Precision>(y_min, y_max, y_bins) : 
        make_regular_axis<Precision>(y_min, y_max, y_bins);
    
    return make_histogram<Precision>(std::move(x_axis), std::move(y_axis));
}

} // namespace eigen_histogram