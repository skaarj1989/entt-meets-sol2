#pragma once

#include "entt/entt.hpp"
#include "sol/sol.hpp"
#include "../common/meta_helper.hpp"

template <typename Event>
auto register_listener(entt::dispatcher &dispatcher, const sol::function &f) {
  assert(f.valid());
  std::cout << "Registered lua listener: " << f.pointer() << std::endl;

  class ScriptListener {
  public:
    ScriptListener(const sol::function &f) : callback{ f } {}
    ~ScriptListener() {
      connection.release();
      std::cout << "Unregistered lua listener: " << callback.pointer()
                << std::endl;
      callback.abandon();
    }
    void receive(const Event &evt) {
      if (connection && callback.valid()) callback(evt);
    }

    sol::function callback;
    entt::connection connection;
  };

  auto listener = std::make_shared<ScriptListener>(f);
  listener->connection =
    dispatcher.sink<Event>().connect<&ScriptListener::receive>(*listener);
  return listener;
}
template <typename Event>
void trigger_event(entt::dispatcher &dispatcher, const sol::table &evt) {
  assert(evt.valid());
  dispatcher.trigger(evt.as<Event>());
}
template <typename Event>
void enqueue_event(entt::dispatcher &dispatcher, const sol::table &evt) {
  assert(evt.valid());
  dispatcher.enqueue(evt.as<Event>());
}
template <typename Event> void clear_event(entt::dispatcher &dispatcher) {
  dispatcher.clear<Event>();
}
template <typename Event> void update_event(entt::dispatcher &dispatcher) {
  dispatcher.update<Event>();
}

template <typename Event> void register_meta_event() {
  entt::meta<Event>().type();
  entt::meta<Event>()
    .template func<&register_listener<Event>>("register_listener"_hs)
    .template func<&trigger_event<Event>>("trigger_event"_hs)
    .template func<&enqueue_event<Event>>("enqueue_event"_hs)
    .template func<&clear_event<Event>>("clear_event"_hs)
    .template func<&update_event<Event>>("update_event"_hs);
}

sol::table open_dispatcher(const sol::this_state &s) {
  sol::state_view lua{ s };
  auto entt_module = lua["entt"].get_or_create<sol::table>();

#define AS_DISPATCHER_REF(self) std::ref(self.as<entt::dispatcher>())

  // clang-format off
  entt_module.new_usertype<entt::dispatcher>("dispatcher",
    sol::meta_function::construct,
    sol::factories([] { return entt::dispatcher{}; }),

    "trigger",
      [](const sol::object &self, const sol::table &evt) {
        invoke_meta_func(get_type_id(evt), "trigger_event"_hs,
          AS_DISPATCHER_REF(self), evt);
      },
    "enqueue",
      [](const sol::object &self, const sol::table &evt) {
        invoke_meta_func(get_type_id(evt), "enqueue_event"_hs,
          AS_DISPATCHER_REF(self), evt);
      },
    "clear",
      sol::overload(
        &entt::dispatcher::clear<>,
        [](const sol::object &self, const sol::object &type_or_id) {
          invoke_meta_func(deduce_type(type_or_id), "clear_event"_hs,
            AS_DISPATCHER_REF(self));
        }
      ),
    "update",
      sol::overload(
        &entt::dispatcher::update,
        [](const sol::object &self, const sol::object &type_or_id) {
          invoke_meta_func(deduce_type(type_or_id), "update_event"_hs,
            AS_DISPATCHER_REF(self));
        }
      ),
    "connect",
      [](const sol::object &self, const sol::object &type_or_id,
         const sol::function &listener) {
        if (!listener.valid()) return entt::meta_any{};
        return invoke_meta_func(deduce_type(type_or_id), "register_listener"_hs,
          AS_DISPATCHER_REF(self), listener);
      }
  );
  // clang-format off

#undef AS_DISPATCHER_REF

  return entt_module;
}