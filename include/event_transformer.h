#pragma once

using namespace std;

template<class... EventTypes>
class EventList;

template<>
class EventList<>
{
public:
  template<typename EvntLists, typename EVT>
  friend void dispatch(EvntLists* evnt_list_ptr, EVT evt);
  template<typename EvntLists, typename EVT>
  friend void dispatch(EvntLists evnt_list, EVT evt);
};

template<typename EvntLists, typename EVT>
void
dispatch(EvntLists* evnt_list_ptr, EVT evt)
{
  static_cast<EventList<EVT>*>(evnt_list_ptr)->dispatch(evt);
}
template<typename EvntLists, typename EVT>
void
dispatch(EvntLists evnt_list, EVT evt)
{
  static_cast<EventList<EVT>&>(evnt_list).dispatch(evt);
}

/**
 * @todo addition type but without trigger ambiguous error.
 */
template<class Event>
class EventList<Event> : public virtual EventList<>
{
public:
  virtual void dispatch(const Event&) = 0;
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