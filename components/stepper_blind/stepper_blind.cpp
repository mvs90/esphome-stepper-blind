#include "stepper_blind.h"
#include "esphome/core/log.h"

namespace esphome { namespace stepper_blind {
static const char *const TAG = "stepper_blind";

void StepperBlind::setup() {
  pref_ = global_preferences->make_preference<StoredState>(key_, true);
  StoredState stored{};
  int32_t steps = 0;
  const bool restored = pref_.load(&stored) && stored.closed != 0 && stored.closed >= -90000 &&
      stored.closed <= 90000 && stored.position >= std::min(int32_t{0}, stored.closed) &&
      stored.position <= std::max(int32_t{0}, stored.closed);
  if (restored) {
    closed_ = stored.closed;
    steps = stored.position;
  }
  motor_->report_position(steps);
  motor_->set_target(steps);
  if (speed_number_ != nullptr) {
    motor_->set_max_speed(speed_);
    motor_->on_update_speed();
    speed_number_->publish_state(speed_);
  }
  saved_position_ = steps;
  saved_closed_ = restored ? closed_ : 0;
  // Persist the initial YAML endpoint too, even if the blind has never moved.
  save_();
  if (endstop_ != nullptr) endstop_->publish_state(closed_);
  update_state_();
}

cover::CoverTraits StepperBlind::get_traits() {
  cover::CoverTraits traits;
  traits.set_supports_position(true);
  traits.set_supports_stop(true);
  traits.set_is_assumed_state(true);
  return traits;
}

void StepperBlind::update_state_() {
  position = std::max(0.0f, std::min(1.0f, 1.0f - float(motor_->current_position) / closed_));
  if (motor_->has_reached_target()) {
    current_operation = cover::COVER_OPERATION_IDLE;
  } else if (float(motor_->target_position) / closed_ < float(motor_->current_position) / closed_) {
    current_operation = cover::COVER_OPERATION_OPENING;
  } else {
    current_operation = cover::COVER_OPERATION_CLOSING;
  }
  publish_state(false);
  last_publish_ = millis();
}

void StepperBlind::control(const cover::CoverCall &call) {
  // A command may arrive after the last motor step but before our loop observes it.
  if (motor_->has_reached_target()) save_();
  if (call.get_stop()) {
    motor_->set_target(motor_->current_position);
    save_();
  } else if (call.get_position().has_value()) {
    const float requested = *call.get_position();
    if (!std::isfinite(requested)) {
      ESP_LOGW(TAG, "Ungueltige Cover-Position");
      return;
    }
    const float bounded = std::max(0.0f, std::min(1.0f, requested));
    motor_->set_target(int32_t(std::lround((1.0f - bounded) * closed_)));
  }
  if (motor_->has_reached_target()) save_();
  update_state_();
}

void StepperBlind::loop() {
  if (motor_->has_reached_target()) {
    if (current_operation != cover::COVER_OPERATION_IDLE) {
      update_state_();
    }
    // Also retry a failed save, and handle commands targeting the current step.
    save_();
  } else if (millis() - last_publish_ >= 500) {
    update_state_();
  }
}

void StepperBlind::on_shutdown() {
  // Freeze the target before ESPHome tears down networking and flushes preferences.
  motor_->set_target(motor_->current_position);
  save_();
}

void StepperBlind::save_() {
  if (saved_position_ == motor_->current_position && saved_closed_ == closed_) return;
  StoredState state{motor_->current_position, closed_};
  if (pref_.save(&state)) {
    saved_position_ = state.position;
    saved_closed_ = state.closed;
  }
}

bool StepperBlind::change_endstop(float value) {
  if (!std::isfinite(value) || value == 0 || value < -90000 || value > 90000 ||
      std::trunc(value) != value ||
      !motor_->has_reached_target()) {
    ESP_LOGW(TAG, "Endposition ungueltig oder Motor faehrt noch");
    return false;
  }
  int32_t next = int32_t(std::lround(value));
  if (next == 0) return false;
  closed_ = next;
  motor_->report_position(0);
  motor_->set_target(0);
  save_();
  update_state_();
  return true;
}

void EndstopNumber::control(float value) {
  parent_->change_endstop(value);
  publish_state(parent_->closed_position());
}

bool StepperBlind::change_speed(float value) {
  if (speed_number_ == nullptr || !std::isfinite(value) || value < 1.0f || value > max_speed_) {
    ESP_LOGW(TAG, "Geschwindigkeit ausserhalb des konfigurierten Bereichs");
    return false;
  }
  speed_ = value;
  motor_->set_max_speed(value);
  // Match ESPHome's stepper.set_speed action, including driver notification.
  motor_->on_update_speed();
  return true;
}

void SpeedNumber::control(float value) {
  parent_->change_speed(value);
  publish_state(parent_->speed());
}
}}  // namespace esphome::stepper_blind
