#include "stepper_blind.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace esphome;
using namespace esphome::stepper_blind;
using namespace esphome::cover;
struct Motor : stepper::Stepper { int tick() { test_micros += 4000; return should_step_(); } };
struct Blind : StepperBlind {
 void command(float p) { CoverCall c; c.pos=p; control(c); }
 void stop() { CoverCall c; c.stop=true; control(c); }
};
struct Number : EndstopNumber { using EndstopNumber::control; };
void init(Blind& b, Motor& m, int32_t end, uint32_t key=1, Number* n=nullptr) {
 b.set_stepper(&m); b.set_closed_position(end); b.set_preference_key(key);
 if(n) { n->set_parent(&b); b.set_endstop_number(n); } b.setup();
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
