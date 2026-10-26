#pragma once
#include <optional>
namespace esphome::cover {
enum CoverOperation { COVER_OPERATION_IDLE, COVER_OPERATION_OPENING, COVER_OPERATION_CLOSING };
class CoverTraits { public: void set_supports_position(bool) {} void set_supports_stop(bool) {} void set_is_assumed_state(bool) {} };
class CoverCall { public: bool stop = false; std::optional<float> pos; bool get_stop() const { return stop; } const std::optional<float>& get_position() const { return pos; } };
class Cover { public: virtual ~Cover() = default; float position = 1; CoverOperation current_operation = COVER_OPERATION_IDLE; unsigned publishes = 0; virtual CoverTraits get_traits() = 0; void publish_state(bool) { ++publishes; } protected: virtual void control(const CoverCall&) = 0; };
}
