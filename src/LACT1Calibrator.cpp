#include "LACT1Calibrator.hh"
#include "DL0Event.hh"

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
  auto gain_selector =
      tmp_select_gain_channel_by_threshold(event.c1->tels[0]->peak, threshold);
  dl0_camera.image = Eigen::VectorXd::Zero(event.c1->tels[0]->peak.cols());
  for (int i = 0; i < event.c1->tels[0]->peak.cols(); i++) {
    event.c1->tels[0]->pe(0, i) =
        event.c1->tels[0]->peak(0, i) / high_gain_mv_to_pe; // High gain channel
    event.c1->tels[0]->pe(1, i) =
        event.c1->tels[0]->peak(1, i) * low_gain_mv_to_pe; // Low gain channel
    if (gain_selector(i) == 0) {
      dl0_camera.image(i) = event.c1->tels[0]->peak(0, i) /
                            high_gain_mv_to_pe; // High gain channel
    } else {
      dl0_camera.image(i) =
          event.c1->tels[0]->peak(1, i) * low_gain_mv_to_pe; // Low gain channel
    }
    dl0_camera.peak_time =
        Eigen::VectorXd::Zero(event.c1->tels[0]->peak.cols());
  }
  event.dl0->add_tel(0, std::move(dl0_camera));
}