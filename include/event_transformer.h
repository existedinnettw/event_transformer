#pragma once

using namespace std;

template<class... EventTypes>
class EventList;

template<>
class EventList<>
{};

/**
 * @todo addition type but without trigger ambiguous error.
 */
template<class Event>
class EventList<Event> : public virtual EventList<>
{
public:
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

template<class Event, class... Others>
class EventList<Event, Others...>
  : public EventList<Event>
  , public EventList<Others...>
{};