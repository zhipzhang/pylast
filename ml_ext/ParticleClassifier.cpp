#include "ParticleClassifier.hh"
#include "ReconstructorFactory.hh"

static bool registered_particle_classifier = []() {
  ReconstructorFactory::instance().register_reconstructor(
      "ParticleClassifier",
      [](const SubarrayDescription &subarray,
         const json &config) -> std::unique_ptr<Reconstructor> {
        return std::make_unique<ParticleClassifier>(config);
      });
  return true;
}();

void ParticleClassifier::operator()(ArrayEvent &event) {
  Reconstructor::operator()(event);
  if (telescopes.size() < 2) {
    particle_reco.is_valid = false;
    particle_reco.hadroness = -99;
    event.dl2->particle[this->name()] = particle_reco;
    return;
  }

  Eigen::VectorXd weights = Eigen::VectorXd::Zero(telescopes.size());
  Eigen::VectorXd tel_scores = Eigen::VectorXd::Zero(telescopes.size());
  
  for (int i = 0; i < telescopes.size(); i++) {
    int tel_id = telescopes[i];
    double score = classifier_model_loader->predict(
        classifier_model_loader->extract_features(event, tel_id));
    tel_scores(i) = score;
    weights(i) =
        event.simulation->tels[tel_id]->image_parameters.hillas.intensity;
  }
  
  double weighted_score =
      (tel_scores.array() * weights.array()).sum() / weights.sum();
  
  particle_reco.hadroness = weighted_score;
  particle_reco.is_valid = true;
  particle_reco.telescopes = telescopes;
  event.dl2->particle[this->name()] = particle_reco;
}
