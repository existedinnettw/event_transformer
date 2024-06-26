#include <iostream>
#include <memory>
#include <stdio.h>
#include <type_traits>
#include <typeinfo>
#include <vector>
// #include <source_location>
// #include <string_view>
#include <chrono>
#include <event_transformer.h>
#include <gtest/gtest.h>
#include <so_5/all.hpp>
#include <string>
#include <typeinfo>

#define TYPE_NAME(x) #x

namespace {
using namespace std;
using namespace ::testing;

/**
 * original required by `events` lib
 */
class BaseEvent
{};

struct AEvent
{
  // const string name = TYPE_NAME(AEvent); //not stable
  double speed; // parameter of event
};
struct BEvent : public BaseEvent
{
  // const string name = TYPE_NAME(BEvent); //not stable
};
struct DEvent : public AEvent
{}; // Events can be arbitrarily derived.

class Foo : public EventList<AEvent, BEvent>
{
public:
  void dispatch(const AEvent& evt) override
  {
    cout << "Foo is handling AEvent" << endl;
  }
  void dispatch(const BEvent& evt) override
  {
    cout << "Foo is handling BEvent" << endl;
  }
};


class Foo_pp : public Foo
{
  string actor = "foo";

public:
  // for c++ 17
  template<typename EVT>
  void dispatch(EVT evt)
  {
    // cout << "this is additional wrapper from event name: " << evt.name << "\n";
    cout << "this is additional wrapper from event name: "
         << "\n";
    static_cast<EventList<decltype(evt)>&>(*this).dispatch(evt);
  }
};


class Evt_trans_test : public Test
{
protected:
  const AEvent a_ev = AEvent();
  const BEvent b_ev = BEvent();
};
TEST_F(Evt_trans_test, create_test)
{
  Foo foo;
  // static_cast<EventList<AEvent>>(foo).dispatch(a_ev);
  // static_cast<Foo>(foo).dispatch(a_ev);
  static_cast<EventList<AEvent>&>(foo).dispatch(a_ev);
  dispatch(foo, a_ev);
  static_cast<EventList<BEvent>&>(foo).dispatch(b_ev);
  // foo.dispatch(a_ev);
  foo.dispatch(std::ref(a_ev));

  printf("\n=======================splitter=======================\n\n");
  Foo_pp foo_pp;
  // static_cast<EventList<AEvent>&>(foo).dispatch(a_ev);
  foo_pp.dispatch(b_ev);
};

/********************************************************************************************************/
#include <any>
#include <map>
#include <typeindex>
#include <typeinfo>

/**
 * @brief
 * as an interface for fake module process
 */
class Fake_module_process_ev : virtual public EventList<AEvent, BEvent>
{}; // end of Fake_module_process_ev

/**
 * 用friend 會變成 `dispatch(foo, a_event);` 而非 `foo.dispatch(a_event);`
 */

/**
 * @brief
 * real implement
 */
// template<typename Functor>
class So5_fake_module_process_ev : virtual public Fake_module_process_ev
{
  so_5::mbox_t target;

public:
  So5_fake_module_process_ev(so_5::mbox_t target_)
    : target(target_)
  {
  }

  void dispatch(const AEvent& evt) override
  {
    cout << "handling AEvent" << endl;
    so_5::send<AEvent>(target, evt);
  }

  void dispatch(const BEvent& evt) override
  {
    cout << "handling BEvent" << endl;
    so_5::send<BEvent>(target, evt);
  }
};

/**
 * simulate module process which require event interface
 */
void
fake_module_process(Fake_module_process_ev* evt_li_ptr)
// fake_module_process(EventList<AEvent, BEvent>* evt_li_ptr)
{
  std::cout << "fake module called\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  const AEvent a_ev = AEvent{ .speed = 5.0 };
  // evt_li_ptr->dispatch(std::ref(a_ev));
  // static_cast<EventList<AEvent>&>(*evt_li_ptr).dispatch(a_ev);
  dispatch(evt_li_ptr, a_ev);
  std::cout << "fake module exist\n";
  //
}
class pinger final : public so_5::agent_t
{
  so_5::mbox_t ponger_;

  void on_pong(mhood_t<AEvent> cmd)
  {
    std::cout << "actor pinger: received a A event\n";
  }

public:
  pinger(context_t ctx)
    : so_5::agent_t{ std::move(ctx) }
  {
  }

  void set_ponger(const so_5::mbox_t mbox)
  {
    ponger_ = mbox;
  }

  void so_define_agent() override
  {
    so_subscribe_self().event(&pinger::on_pong);
  }
};
TEST_F(Evt_trans_test, so5_fake_module_test)
{
  // using namespace std::literals;
  using namespace std::chrono_literals;

  so_5::launch([](so_5::environment_t& env) {
    so_5::mbox_t m;
    env.introduce_coop([&m](so_5::coop_t& coop) {
      auto pinger_actor = coop.make_agent<pinger>();
      m = pinger_actor->so_direct_mbox();
    });
    So5_fake_module_process_ev ev_sys(m);
    fake_module_process(&ev_sys);
    // pause(200ms);
    std::this_thread::sleep_for(200ms);

    std::cout << "Stopping..." << std::endl;
    env.stop();
  }); // end of so5 launch
}
}