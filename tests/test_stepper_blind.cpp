#include "stepper_blind.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace esphome;
using namespace esphome::stepper_blind;
using namespace esphome::cover;
struct Motor : stepper::Stepper {
 unsigned speed_updates = 0;
 int tick() { test_micros += 4000; return should_step_(); }
 float maximum() const { return max_speed_; }
 void on_update_speed() override { ++speed_updates; }
};
struct Blind : StepperBlind {
 void command(float p) { CoverCall c; c.pos=p; control(c); }
 void stop() { CoverCall c; c.stop=true; control(c); }
};
struct Number : EndstopNumber { using EndstopNumber::control; };
struct Speed : SpeedNumber { using SpeedNumber::control; };

void init(Blind& b, Motor& m, int32_t end, uint32_t key=1, Number* n=nullptr) {
 b.set_stepper(&m); b.set_closed_position(end); b.set_preference_key(key);
 if(n) { n->set_parent(&b); b.set_endstop_number(n); } b.setup();
}
void speed_scenario(int32_t endpoint) {
 records.clear(); test_micros=0; saves=0;
 Motor motor; Blind blind; Speed speed;
 speed.set_parent(&blind); blind.set_speed_number(&speed); blind.set_speed_config(250,150);
 init(blind,motor,endpoint);
 assert(speed.state==150 && motor.maximum()==150 && motor.speed_updates==1);
 assert(motor.current_position==0 && motor.target_position==0);
 const unsigned initial_saves=saves;
 speed.control(100); assert(speed.state==100 && motor.maximum()==100);
 blind.command(0); auto target=motor.target_position;
 speed.control(50); assert(motor.target_position==target && motor.current_position==0);
 for(int i=0;i<200;++i) { motor.tick(); blind.loop(); }
 assert(std::abs(motor.current_position)==40);
 assert(blind.current_operation==COVER_OPERATION_CLOSING);
 speed.control(250); auto start=motor.current_position;
 for(int i=0;i<50;++i) motor.tick();
 // The 4 ms simulated polling grid can quantize the 250 steps/s interval.
 assert(std::abs(motor.current_position-start)>=25 && std::abs(motor.current_position-start)<=50);
 const auto calls=motor.speed_updates;
 for(float invalid : {0.0f,-1.0f,0.5f,250.5f,1000.0f,
                      std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::quiet_NaN()}) {
  speed.control(invalid); assert(speed.state==250 && motor.maximum()==250);
 }
 assert(motor.speed_updates==calls && saves==initial_saves);
 blind.stop(); assert(motor.tick()==0);
 speed.control(1); assert(speed.state==1 && motor.target_position==motor.current_position);
 // Speed deliberately returns to YAML initial value; existing persisted position remains compatible.
 Motor restored_motor; Blind restored; Speed restored_speed;
 restored_speed.set_parent(&restored); restored.set_speed_number(&restored_speed);
 restored.set_speed_config(250,150);
 init(restored,restored_motor,endpoint);
 assert(restored_speed.state==150 && restored_motor.maximum()==150);
 assert(restored_motor.current_position==motor.current_position && restored_motor.has_reached_target());
 std::cout << "PASS speed control and restart endpoint " << endpoint << '\n';
}
struct State { int32_t position, closed; };
State stored(uint32_t key=1) { State s{}; assert(ESPPreferenceObject(key).load(&s)); return s; }
void settle(Blind& b, Motor& m) {
 unsigned timeout=200000;
 while(!m.has_reached_target() && --timeout) { m.tick(); b.loop(); }
 assert(timeout); b.loop(); assert(b.current_operation==COVER_OPERATION_IDLE);
}
void scenario(int32_t end) {
 records.clear(); test_micros=0; saves=0; Motor m; Blind b; Number n;
 init(b,m,end,1,&n);
 assert(m.current_position==0 && m.target_position==0 && b.position==1);
 assert(n.state==end && stored().closed==end);
 unsigned initial=saves; for(int i=0;i<100;++i) b.loop(); assert(saves==initial);
 b.command(0); assert(m.target_position==end && b.current_operation==COVER_OPERATION_CLOSING);
 for(int i=0;i<125;++i) { m.tick(); b.loop(); }
 assert(b.publishes>=3); assert(!b.change_endstop(-end));
 int32_t midway=m.current_position; assert(midway!=0);
 b.command(1); assert(b.current_operation==COVER_OPERATION_OPENING);
 int dir=m.tick(); assert(end>0 ? dir<0 : dir>0);
 b.stop(); assert(m.current_position==m.target_position && b.current_operation==COVER_OPERATION_IDLE);
 int32_t stopped=m.current_position; for(int i=0;i<20;++i) assert(m.tick()==0);
 assert(stored().position==stopped);
 Motor restored_motor; Blind restored; init(restored,restored_motor,-end);
 assert(restored.closed_position()==end && restored_motor.current_position==stopped && restored_motor.target_position==stopped);
 for(float p : {1.0f,0.75f,0.5f,0.25f,0.0f}) {
  b.command(p); assert(m.target_position==std::lround((1.0f-p)*end)); settle(b,m);
  assert(std::fabs(b.position-p)<=1.0f/std::abs(end));
  assert(stored().position==m.current_position);
 }
 // Reproduce arrival-before-control race with a new target equal to current step.
 b.command(1); while(!m.has_reached_target()) m.tick();
 assert(b.current_operation==COVER_OPERATION_OPENING);
 b.command(1); assert(stored().position==0);
 b.command(0); for(int i=0;i<20;++i) m.tick();
 // Retargeting exactly onto the current step is also a completed movement.
 b.command(1.0f-float(m.current_position)/end); assert(m.has_reached_target());
 assert(stored().position==m.current_position);
 auto before=m.target_position;
 b.command(std::numeric_limits<float>::quiet_NaN()); assert(m.target_position==before);
 for(float invalid : {0.0f,0.1f,1.5f,90001.0f,-90001.0f,std::numeric_limits<float>::infinity()}) assert(!b.change_endstop(invalid));
 assert(b.change_endstop(-end)); assert(m.current_position==0 && m.target_position==0 && b.position==1);
 n.control(0); assert(n.state==-end);
 // Even writing the same endpoint resets to open, as in the original YAML.
 b.command(0.5); settle(b,m); n.control(-end); assert(m.current_position==0 && n.state==-end);
 b.command(0); m.tick(); int32_t shutdown_step=m.current_position; b.on_shutdown();
 assert(m.target_position==shutdown_step && stored().position==shutdown_step);
 std::cout << "PASS endpoint " << end << '\n';
}
int main() {
 speed_scenario(10000); speed_scenario(-10000);
 { records.clear(); Motor m; Blind b; Speed speed;
   speed.set_parent(&b); b.set_speed_number(&speed); b.set_speed_config(500,400);
   init(b,m,10000); speed.control(500); assert(m.maximum()==500 && speed.state==500); }

 { records.clear(); Motor m; m.set_max_speed(77); Blind b; init(b,m,10000);
   assert(m.maximum()==77 && m.speed_updates==0 && !b.change_speed(30)); }

 for(auto end : {10000,-10000,90000,-90000,1,-1,12345,-12345}) {
  // Very short tracks use separate exhaustive percentage checks below.
  if(std::abs(end)>125) scenario(end);
  else { records.clear(); Motor m; Blind b; init(b,m,end); for(float p:{0.0f,0.25f,0.5f,0.75f,1.0f}) { b.command(p); settle(b,m); assert(m.current_position==std::lround((1-p)*end)); } }
 }
 // Invalid stored pairs must be rejected together; no partial restore.
 for(State invalid : {State{1,0},State{10001,10000},State{-1,10000},State{1,-10000},State{-90001,-90000},State{0,90001}}) {
  records.clear(); ESPPreferenceObject(1).save(&invalid); Motor m; Blind b; init(b,m,-10000);
  assert(m.current_position==0 && m.target_position==0 && b.closed_position()==-10000);
 }
 records.clear(); Motor m; Blind b; init(b,m,10000);
 b.command(0); while(!m.has_reached_target()) m.tick();
 fail_save=true; b.loop(); assert(stored().position==0);
 fail_save=false; b.loop(); assert(stored().position==10000);
 // Initial endpoint is frozen without a first movement; keys isolate instances.
 Motor m2; Blind b2; init(b2,m2,-10000); assert(b2.closed_position()==10000);
 Motor m3; Blind b3; init(b3,m3,-12345,2); assert(b3.closed_position()==-12345);
 std::cout << "PASS restore validation, save retry, first boot, independent IDs\n";
}
