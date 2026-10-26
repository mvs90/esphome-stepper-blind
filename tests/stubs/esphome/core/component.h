#pragma once
#include <algorithm>
#include <cstdint>
namespace esphome {
inline uint32_t test_micros = 0;
inline uint32_t millis() { return test_micros / 1000; }
inline uint32_t micros() { return test_micros; }
template<class T> T clamp(T v, T a, T b) { return std::clamp(v, a, b); }
namespace setup_priority { constexpr float LATE = -100; }
class Component { public: virtual ~Component() = default; virtual void setup() {} virtual void loop() {} virtual void on_shutdown() {} virtual float get_setup_priority() const { return 0; } };
}
