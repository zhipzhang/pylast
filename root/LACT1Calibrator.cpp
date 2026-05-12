#include "LACT1Calibrator.hh"
#include "DL0Event.hh"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include <algorithm>

Eigen::VectorXi tmp_select_gain_channel_by_threshold(
    Eigen::Matrix<double, -1, -1, Eigen::RowMajor> &peak,
    const double threshold) {
    // Matrix is (n_pixels, n_samples)
    // Vector returned is (n_pixels)
    Eigen::VectorXi gain_selector = Eigen::VectorXi::Zero(peak.cols());

    int low_gain = 1;
    for (int i = 0; i < peak.cols(); i++) {
        if (peak(low_gain, i) > threshold) {
            gain_selector(i) = 1;
        }
    }
    return gain_selector;
}
void LACT1Calibrator::operator()(ArrayEvent &event) {
    if (!event.dl0) {
        event.dl0 = DL0Event();
    }
    if (!event.c1) {
        throw std::runtime_error("LACT1Calibrator requires C1 Level!");
    }
    event.c1->tels[0]->pe.resize(2, event.c1->tels[0]->peak.cols());
    DL0Camera dl0_camera;
    auto gain_selector = tmp_select_gain_channel_by_threshold(
        event.c1->tels[0]->peak, threshold);
    dl0_camera.image = Eigen::VectorXd::Zero(event.c1->tels[0]->peak.cols());
    double event_mjd = event.mjd.mjd_int + event.mjd.mjd_double;
    if (!have_calibration_file) {
        for (int i = 0; i < event.c1->tels[0]->peak.cols(); i++) {
            event.c1->tels[0]->pe(0, i) =
                event.c1->tels[0]->peak(0, i) /
                high_gain_mv_to_pe; // High gain channel
            event.c1->tels[0]->pe(1, i) = event.c1->tels[0]->peak(1, i) *
                                          low_gain_mv_to_pe; // Low gain channel
            if (gain_selector(i) == 0) {
                dl0_camera.image(i) = event.c1->tels[0]->peak(0, i) /
                                      high_gain_mv_to_pe; // High gain channel
            } else {
                dl0_camera.image(i) = event.c1->tels[0]->peak(1, i) *
                                      low_gain_mv_to_pe; // Low gain channel
            }
        }
    }
    else
    {
      auto low_gain_calibration_factor = get_low_gain_calibration_factor(event_mjd);
      auto high_gain_calibration_factor = get_high_gain_calibration_factor(event_mjd);
      for(int i = 0; i < event.c1->tels[0]->area.cols(); i++) {
        event.c1->tels[0]->pe(0, i) = event.c1->tels[0]->area(0, i) * high_gain_calibration_factor(i);
        event.c1->tels[0]->pe(1, i) = event.c1->tels[0]->area(1, i) * low_gain_calibration_factor(i);
        if(gain_selector(i) == 0) {
          dl0_camera.image(i) = event.c1->tels[0]->area(0, i) * high_gain_calibration_factor(i);
        }
        else {
          dl0_camera.image(i) = event.c1->tels[0]->area(1, i) * low_gain_calibration_factor(i);
        }
      }

    }
    event.dl0->add_tel(0, std::move(dl0_camera));
}
void LACT1Calibrator::load_calibration_file(
    const std::string &calibration_file) {
    TFile *led_calibration_file = TFile::Open(calibration_file.c_str(), "READ");
    if (!led_calibration_file || !led_calibration_file->IsOpen()) {
        throw std::runtime_error("Failed to open led calibration file: " +
                                 calibration_file);
    }
    auto led_cal_tree =
        static_cast<TTree *>(led_calibration_file->Get("led_cal"));
    if (!led_cal_tree) {
        throw std::runtime_error(
            "Failed to get led calibration tree from file: " +
            calibration_file);
    }
    int n_events = led_cal_tree->GetEntries();
    TTreeReader *led_cal_tree_reader = new TTreeReader(led_cal_tree);
    auto event_time_reader =
        new TTreeReaderValue<double>(*led_cal_tree_reader, "event_time");
    auto low_gain_area_factor_reader =
        new TTreeReaderArray<double>(*led_cal_tree_reader,
                                                  "GainFactor_AreaL");
    auto high_gain_area_factor_reader =
        new TTreeReaderArray<double>(*led_cal_tree_reader,
                                                  "GainFactor_AreaH");
    calibration_time.resize(n_events);
    calibration_low_gain_area.resize(n_events, 1616);
    calibration_high_gain_area.resize(n_events, 1616);
    for (int i = 0; i < n_events; i++) {
        led_cal_tree_reader->Next();
        calibration_time(i) = convert_linux_time_to_mjd(*(*event_time_reader));
        for (int ipix = 0; ipix < 1616; ipix++) {
            calibration_low_gain_area(i, ipix) =
                (*low_gain_area_factor_reader)[ipix];
            calibration_high_gain_area(i, ipix) =
                (*high_gain_area_factor_reader)[ipix];
        }
    }
    have_calibration_file = true;
    led_calibration_file->Close();
    delete led_cal_tree_reader;
    delete event_time_reader;
    delete low_gain_area_factor_reader;
    delete high_gain_area_factor_reader;
}

Eigen::Index
LACT1Calibrator::find_nearest_calibration_index(double event_mjd) const {
    const Eigen::Index n = calibration_time.size();
    if (n == 0) {
        throw std::runtime_error("No calibration times loaded.");
    }

    // Check if event_mjd is strictly within the calibration range
    if (event_mjd < calibration_time[0] || event_mjd > calibration_time[n-1]) {
        throw std::runtime_error("event_mjd (" + std::to_string(event_mjd) +
            ") is outside the calibration time range [" + std::to_string(calibration_time[0]) +
            ", " + std::to_string(calibration_time[n-1]) + "]");
    }

    const double *begin = calibration_time.data();
    const double *end = begin + n;
    const double *it = std::lower_bound(begin, end, event_mjd);
    if (it == begin) {
        return 0;
    }
    if (it == end) {
        return n - 1;
    }
    const Eigen::Index hi = static_cast<Eigen::Index>(it - begin);
    const Eigen::Index lo = hi - 1;
    return (event_mjd - calibration_time[lo] <= calibration_time[hi] - event_mjd)
               ? lo
               : hi;
}

Eigen::Map<Eigen::VectorXd>
LACT1Calibrator::get_low_gain_calibration_factor(const double event_mjd) {
    const Eigen::Index idx = find_nearest_calibration_index(event_mjd);
    return Eigen::Map<Eigen::VectorXd>(calibration_low_gain_area.row(idx).data(),
                                       calibration_low_gain_area.cols());
}

Eigen::Map<Eigen::VectorXd>
LACT1Calibrator::get_high_gain_calibration_factor(const double event_mjd) {
    const Eigen::Index idx = find_nearest_calibration_index(event_mjd);
    return Eigen::Map<Eigen::VectorXd>(calibration_high_gain_area.row(idx).data(),
                                       calibration_high_gain_area.cols());
}
double LACT1Calibrator::convert_linux_time_to_mjd(double linux_time) {
    // Linux/UNIX epoch is 1970-01-01 00:00:00 UTC
    // Modified Julian Date is from 1858-11-17 00:00:00 UTC
    // MJD = JD - 2400000.5
    // UNIX epoch in MJD is 40587.0
    // Linux time is in seconds since UNIX epoch

    // Check for pathological input (e.g. negative times)
    if (linux_time < 0.0) {
        throw std::invalid_argument("linux_time must be non-negative");
    }
    constexpr double seconds_per_day = 86400.0;
    constexpr double unix_epoch_in_mjd = 40587.0;
    return linux_time / seconds_per_day + unix_epoch_in_mjd;
}