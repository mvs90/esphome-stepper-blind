#pragma once
#include <algorithm>
#include <cmath>
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/number/number.h"
#include "esphome/components/stepper/stepper.h"

namespace esphome { namespace stepper_blind {
class StepperBlind;
class EndstopNumber : public number::Number {
 public:
  void set_parent(StepperBlind *parent) { parent_ = parent; }
 protected:
  void control(float value) override;
  StepperBlind *parent_{};
};

class StepperBlind : public cover::Cover, public Component {
 public:
  void set_stepper(stepper::Stepper *motor) { motor_ = motor; }
  void set_closed_position(int32_t value) { closed_ = value; }
  void set_preference_key(uint32_t key) { key_ = key; }
  void set_endstop_number(EndstopNumber *entity) { endstop_ = entity; }
  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void loop() override;
  void on_shutdown() override;
  cover::CoverTraits get_traits() override;
  bool change_endstop(float value);
  int32_t closed_position() const { return closed_; }
 protected:
  void control(const cover::CoverCall &call) override;
  void update_state_();
  void save_();
  struct StoredState { int32_t position; int32_t closed; };
  stepper::Stepper *motor_{};
  EndstopNumber *endstop_{};
  ESPPreferenceObject pref_;
  int32_t closed_{10000};
  int32_t saved_position_{0};
  int32_t saved_closed_{0};
  uint32_t key_{};
  uint32_t last_publish_{0};
};
}}  // namespace esphome::stepper_blind
