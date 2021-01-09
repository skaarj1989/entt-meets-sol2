#pragma once

#include "entt/entt.hpp"
#include "sol/sol.hpp"
#include "../common/meta_helper.hpp"

template <typename Event>
auto register_listener(entt::dispatcher &dispatcher, const sol::function &f,
                       const sol::this_state &s) {
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
  return sol::make_object(s, listener);
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

  // clang-format off
  entt_module.new_usertype<entt::dispatcher>("dispatcher",
    sol::meta_function::construct,
    sol::factories([] { return entt::dispatcher{}; }),

    "trigger",
      [](entt::dispatcher &self, const sol::table &evt) {
        invoke_meta_func(
          get_type_id(evt), "trigger_event"_hs, std::ref(self), evt);
      },
    "enqueue",
      [](entt::dispatcher &self, const sol::table &evt) {
        invoke_meta_func(
          get_type_id(evt), "enqueue_event"_hs, std::ref(self), evt);
      },
    "clear",
      sol::overload(
        &entt::dispatcher::clear<>,
        [](entt::dispatcher &self, const sol::object &type_or_id) {
          invoke_meta_func(
            deduce_type(type_or_id), "clear_event"_hs, std::ref(self));
        }
      ),
    "update",
      sol::overload(
        &entt::dispatcher::update,
        [](entt::dispatcher &self, const sol::object &type_or_id) {
          invoke_meta_func(
            deduce_type(type_or_id), "update_event"_hs, std::ref(self));
        }
      ),
    "connect",
      [](entt::dispatcher &self, const sol::object &type_or_id,
         const sol::function &listener, sol::this_state &s) {
        if (!listener.valid()) sol::lua_nil_t{};
        auto maybe_any = invoke_meta_func(deduce_type(type_or_id),
          "register_listener"_hs, std::ref(self), listener, s);
        return maybe_any ? maybe_any.cast<sol::object>() : sol::lua_nil_t{};
      }
  );
  // clang-format off

  return entt_module;
}