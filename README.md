# event_transformer

Simple utility lib to be interface of event system and simplify transform between different evet lib.

Current implementation require you to define(override) all the event specific dispatch function. If implement lack, you won't pass though compiler.

> The original goal of this project is ability to override all event behavior by 1 meta function.
>
> However, it seems dynamic reflection is necessary for implementation, but C++ have no this feature therefore impossible to achieve.

## basic usage

```c++
#include <event_transformer.h>
#include <so_5/all.hpp>

struct AEvent
{
  double speed; // parameter of event
};
struct BEvent
{};
/**
 * @brief
 * as an interface for fake module process
 */
class Fake_module_process_ev : virtual public EventList<AEvent, BEvent>
{}; // end of Fake_module_process_ev

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

void
fake_module_process(Fake_module_process_ev* evt_li_ptr)
{
  //...
  dispatch(evt_li_ptr, a_ev);
  //...
}
```

## build

```bash
conan build . --build=missing
```

## ref

- [MCGallaspy/events](https://github.com/MCGallaspy/events/)
