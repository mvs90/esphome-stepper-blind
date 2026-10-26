#pragma once
namespace esphome::number {
class Number { public: virtual ~Number() = default; float state = 0; void publish_state(float v) { state = v; } protected: virtual void control(float) = 0; };
}
