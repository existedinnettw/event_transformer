#pragma once

#include <any>
#include <functional>
#include <map>
#include <typeindex>
#include <typeinfo>

/**
 * @details
 * variadic template + template specialization
 */
template<class... EventTypes>
class EventList
{
  // no function here will be expose to outer.
  //  void hello()
  //  {
  //    printf("hello");
  //  }
};

/**
 * base of EventList
 */
template<>
class EventList<>
{
public:
  template<typename EvntLists, typename EVT>
  friend void dispatch(EvntLists* evnt_list_ptr, EVT evt);

  /**
   * @todo create templated setting functor meta function.
   */
  template<typename EvntLists, template<typename Ty> class FunctorT, typename EVT>
  friend void set_meta(EvntLists* evnt_list_ptr);

  // protected:
  // std::optional
  std::map<std::type_index, std::any> fmap;
};

// class Dispatcher
// {
//   Dispatcher(Event)
// };
template<typename EvntLists, typename EVT>
void
dispatch(EvntLists* evnt_list_ptr, EVT evt)
{
  auto base = static_cast<EventList<>*>(evnt_list_ptr); //->dispatch(evt);
  auto any_func = base->fmap.at(std::type_index(typeid(EVT)));
  if (any_func.has_value()) {
    auto func = std::any_cast<std::function<void(EVT&)>>(any_func);
    func(evt); // dispatch
    return;
  }
  // else, call its own private dispatch function.
  dynamic_cast<EventList<EVT>*>(evnt_list_ptr)->dispatch(evt); // error: 'EventList<>' is not polymorphic, so I need to
  // create seperate func.
}
template<typename EvntLists, template<typename Ty> class FunctorT, typename EVT>
void
set_meta(EvntLists* evnt_list_ptr)
{
  printf("set meta called\n");
  auto base = static_cast<EventList<>*>(evnt_list_ptr);
  base->fmap.at(std::type_index(typeid(EVT))) = FunctorT<EVT>();
}

/**
 * The second event list, connect between event and base
 * @todo addition type but without trigger ambiguous error.
 */
template<class Event>
class EventList<Event> : public virtual EventList<>
{
public:
  EventList<Event>()
  {
    printf("normal constructor called\n");
    fmap.insert(std::pair<std::type_index, std::any>(std::type_index(typeid(Event)), std::any()));
    // auto any_func = fmap.at(std::type_index(typeid(Event)));
    // printf("any val:%d\n", any_func.has_value());
  }
  virtual void dispatch(const Event&){}; //=0
  // virtual void dispatch(Event&)
  // {
  //   cout << "default cb\n";
  // };
  // virtual void dispatch(const Event)
  // {
  //   cout << "default cb\n";
  // };
};

/**
 * intermediate, and hierachy
 */
template<class Event, class... Others>
class EventList<Event, Others...>
  : public EventList<Event>
  , public EventList<Others...>
{
  // EventList<Event, Others...>()
  // {
  //   printf("intermediate constructor called\n");
  // }
};

/************************************************/

template<template<typename Event> class FunctorT, class Event>
class EventList<FunctorT<Event>, Event> : public virtual EventList<>
{
public:
  EventList<FunctorT<Event>, Event>()
  {
    printf("constructor with functor\n");
    set_meta<EventList<FunctorT<Event>, Event>, FunctorT, Event>(this);
  }
};

template<template<typename Ty> class FunctorT, class Event, class... Others>
class EventList<FunctorT<Event>, Event, Others...>
  : public EventList<FunctorT<Event>, Event>
  , public EventList<Others...>
{
public:
  EventList<FunctorT<Event>, Event, Others...>()
  {
    printf("outer, constructor with functor\n");
  }
};