#include "VslinkParser.hpp"
#include "reference_commands.hpp"
#include "ConnectionManager.hpp"
#include "ControllerProcessor.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
using namespace vslink;
unsigned checks=0;
#define CHECK(x) do { ++checks; if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(0)
struct FakeUsb : GamepadOutput {
  std::vector<GamepadState> reports;
  bool ready=true;
  bool send(const GamepadState &s) override { if (!ready) return false; reports.push_back(s); return true; }
};
int main() {
  GamepadStateManager m;
  for (auto command:reference_xbox) CHECK(m.receive(command));
  CHECK(m.receive("Change gamepad type to: PS"));
  for (auto command:reference_ds4) CHECK(m.receive(command));
  CHECK(m.receive("Change gamepad type to: XBOX"));
  const std::pair<const char *,unsigned> xbox[]={
    {"A",4096},{"B",8192},{"X",16384},{"Y",32768},{"LEFT_BUMPER",256},{"LB",256},
    {"RIGHT_BUMPER",512},{"RB",512},{"BACK",32},{"START",16},{"LEFT_THUMB",64},
    {"LS",64},{"RIGHT_THUMB",128},{"RS",128},{"HOME",1024},{"GUIDE",1024},
    {"XBOX_BUTTON",1024},{"LEFT_SHOULDER",256},{"RIGHT_SHOULDER",512},
    {"DPAD_UP",1},{"DPAD_DOWN",2},{"DPAD_LEFT",4},{"DPAD_RIGHT",8}};
  for (auto &b:xbox) {
    m.reset(); CHECK(m.receive(std::string(b.first)+"_RELEASE")); CHECK(m.state().buttons==0);
    CHECK(m.receive(std::string(b.first)+"_PRESS")); CHECK(m.receive(std::string(b.first)+"_PRESS"));
    CHECK(m.state().buttons==b.second);
    CHECK(m.receive(std::string(b.first)+"_RELEASE")); CHECK(m.state().buttons==0);
  }
  CHECK(m.receive("A_PRESS")); CHECK(m.receive("B_PRESS")); CHECK(m.receive("DPAD_UP_PRESS"));
  CHECK(m.state().buttons==(4096|8192|1)); CHECK(m.receive("A_RELEASE")); CHECK(m.state().buttons==(8192|1));
  CHECK(m.receive("RESET")); CHECK(m.state().buttons==0);
  for (const char *side:{"LEFT","RIGHT"}) {
    std::string p=std::string(side)+"_JOYSTICK_";
    bool r=side[0]=='R';
    CHECK(m.receive(p+"-1.0_1.0")); CHECK((r?m.state().rx:m.state().lx)==-32767); CHECK((r?m.state().ry:m.state().ly)==32767);
    CHECK(m.receive(p+"0.5_-0.5")); CHECK((r?m.state().rx:m.state().lx)==16384); CHECK((r?m.state().ry:m.state().ly)==-16384);
    CHECK(m.receive(p+"-7_9")); CHECK((r?m.state().rx:m.state().lx)==-32767); CHECK((r?m.state().ry:m.state().ly)==32767);
    for (auto d:{"LEFT","RIGHT","UP","DOWN","RESET"}) {
      CHECK(m.receive(p+d));
      CHECK((r?m.state().rx:m.state().lx)==(std::string(d)=="LEFT"?-32767:std::string(d)=="RIGHT"?32767:0));
      CHECK((r?m.state().ry:m.state().ly)==(std::string(d)=="DOWN"?-32767:std::string(d)=="UP"?32767:0));
    }
  }
  for (auto alias:{"LEFT_TRIGGER","LT","L2","RIGHT_TRIGGER","RT","R2"}) {
    bool r=alias[0]=='R'; std::string p=std::string(alias)+"_";
    for (auto sample:{std::pair<const char *,int>{"0.0",0},{"0.5",128},{"1.0",255},{"-3",0},{"5",255}}) {
      CHECK(m.receive(p+sample.first)); CHECK((r?m.state().right_trigger:m.state().left_trigger)==sample.second);
    }
  }
  for (auto alias:{"LEFT_TRIGGER","LT","RIGHT_TRIGGER","RT"}) {
    CHECK(m.receive(std::string(alias)+"_PRESS")); CHECK(m.receive(std::string(alias)+"_RELEASE"));
  }
  m.reset(); CHECK(m.receive("A_PRESS"));
  for (auto s:{""," ","a_PRESS","A_RELEASE_EXTRA","LEFT_JOYSTICK_nan_0","LEFT_JOYSTICK_0_inf",
       "LEFT_JOYSTICK_1_","LEFT_JOYSTICK_0_0_0","LEFT_TRIGGER_0.5x","LEFT_TRIGGER_nan",
       "Change gamepad type to: INVALID","LEFT_JOYSTICK_0x1_0","A_PRESS\nB_PRESS"}) {
    CHECK(!m.receive(s)); CHECK(m.state().buttons==4096);
  }
  CHECK(!m.receive(std::string(513,'a'))); CHECK(!m.receive(std::string_view("A_PRESS\0",8)));
  CHECK(!m.receive("\xC3\xA9")); CHECK(m.receive(" \tA_RELEASE\r\n")); CHECK(m.state().buttons==0);
  CHECK(m.receive("Change gamepad type to: ps")); CHECK(m.type()==ControllerType::PlayStation);
  for (auto b:{"CROSS","CIRCLE","SQUARE","TRIANGLE","L1","R1","SHARE","OPTIONS","L3","R3","PS_BUTTON","TOUCHPAD"}) {
    CHECK(m.receive(std::string(b)+"_PRESS")); CHECK(m.state().buttons!=0);
    CHECK(m.receive(std::string(b)+"_RELEASE")); CHECK(m.state().buttons==0);
  }
  CHECK(!m.receive("A_PRESS")); CHECK(m.receive("L2_PRESS")); CHECK(m.state().left_trigger==255);
  CHECK(m.receive("R2_PRESS")); CHECK(m.state().right_trigger==255);
  CHECK(m.receive("L2_RELEASE")); CHECK(m.receive("R2_RELEASE"));
  CHECK(m.receive("LEFT_JOYSTICK_UP")); CHECK(m.state().ly==32767);
  CHECK(m.receive("LEFT_JOYSTICK_0_1")); CHECK(m.state().ly==-32767);
  CHECK(m.receive("DPAD_UP_PRESS")); CHECK(m.receive("DPAD_RIGHT_PRESS")); CHECK(m.state().buttons==8);
  CHECK(m.receive("DPAD_UP_RELEASE")); CHECK(m.state().buttons==0);
  CHECK(m.receive("Change gamepad type to:XbOx")); CHECK(m.type()==ControllerType::Xbox); CHECK(m.state().ly==0);
  ConnectionManager<2> q; ConnectionManager<2>::Event e; bool fresh=false;
  CHECK(!q.enqueue("A_PRESS",7)); q.connect(); CHECK(q.take_reset(fresh)); CHECK(fresh);
  CHECK(q.enqueue("A_PRESS",7)); CHECK(q.enqueue("B_PRESS",7)); CHECK(q.pop(e));
  CHECK(std::string_view(e.data.data(),e.length)=="A_PRESS");
  CHECK(q.enqueue("A_RELEASE",9)); CHECK(!q.enqueue("B_RELEASE",9)); CHECK(q.overflows()==1);
  CHECK(!q.pop(e)); CHECK(q.take_reset(fresh)); CHECK(!fresh); m.reset(fresh); CHECK(m.state().buttons==0);
  CHECK(!q.pop(e)); CHECK(q.enqueue("X_PRESS",7)); CHECK(q.pop(e)); CHECK(m.receive({e.data.data(),e.length}));
  FakeUsb usb; CHECK(usb.send(m.state())); CHECK(usb.reports.back().buttons==16384);
  CHECK(m.receive("LEFT_JOYSTICK_1_1")); CHECK(m.receive("LEFT_TRIGGER_1"));
  q.disconnect(); CHECK(!q.pop(e)); CHECK(q.take_reset(fresh)); CHECK(fresh); m.reset(fresh);
  CHECK(usb.send(m.state())); CHECK(usb.reports.back().buttons==0); CHECK(usb.reports.back().lx==0); CHECK(usb.reports.back().left_trigger==0);
  CHECK(!q.enqueue("A_PRESS",7)); q.connect(); CHECK(q.take_reset(fresh)); CHECK(q.enqueue("A_PRESS",7)); CHECK(q.pop(e));
  CHECK(m.receive({e.data.data(),e.length})); CHECK(m.state().buttons==4096);
  ControllerProcessor processor;
  FakeUsb bus;
  CHECK(!processor.needs_command()); CHECK(processor.submit(bus,0)); CHECK(processor.needs_command());
  // Idle refresh must never prevent a new command from being dequeued.
  for (unsigned t=1;t<100;++t) { CHECK(processor.submit(bus,t)); CHECK(processor.needs_command()); }
  CHECK(processor.receive("A_PRESS")); bus.ready=false; CHECK(!processor.submit(bus,100));
  CHECK(!processor.needs_command()); CHECK(!processor.receive("A_RELEASE"));
  bus.ready=true; CHECK(processor.submit(bus,101)); CHECK(bus.reports.back().buttons==4096);
  CHECK(processor.receive("A_RELEASE")); CHECK(processor.submit(bus,102)); CHECK(bus.reports.back().buttons==0);
  CHECK(processor.receive("LEFT_JOYSTICK_1_1")); bus.ready=false; CHECK(!processor.submit(bus,103));
  processor.reset(true); CHECK(!processor.submit(bus,104)); CHECK(!processor.needs_command());
  bus.ready=true; CHECK(processor.submit(bus,105)); CHECK(bus.reports.back().buttons==0); CHECK(bus.reports.back().lx==0);
  CHECK(processor.needs_command()); CHECK(processor.receive("B_PRESS")); CHECK(processor.submit(bus,106));
  CHECK(bus.reports.back().buttons==8192);
  std::cout << checks << " checks passed\n";
}
