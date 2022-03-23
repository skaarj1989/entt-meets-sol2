#pragma once

#include "entt/signal/dispatcher.hpp"
#include "meta_helper.hpp"

template <typename Event>
[[nodiscard]] auto register_listener(entt::dispatcher *dispatcher,
                                     sol::function f, sol::this_state s) {
  assert(dispatcher && f.valid());

  struct script_listener {
    script_listener(entt::dispatcher &dispatcher, sol::function &&f)
        : callback{std::move(f)} {
      connection =
        dispatcher.sink<Event>().connect<&script_listener::receive>(*this);
      std::cout << "Registered lua listener: " << callback.pointer()
                << std::endl;
    }
    script_listener(const script_listener &) = delete;
    script_listener(script_listener &&) noexcept = default;
    ~script_listener() {
      connection.release();
      std::cout << "Unregistered lua listener: " << callback.pointer()
                << std::endl;
      callback.abandon();
    }

    script_listener &operator=(const script_listener &) = delete;
    script_listener &operator=(script_listener &&) noexcept = default;

    void receive(const Event &evt) {
      if (connection && callback.valid()) callback(evt);
    }

    sol::function callback;
    entt::connection connection;
  };

  return sol::make_reference(
    s, std::make_unique<script_listener>(*dispatcher, std::move(f)));
}
template <typename Event>
void trigger_event(entt::dispatcher *dispatcher, const sol::table &evt) {
  assert(dispatcher && evt.valid());
  dispatcher->trigger(evt.as<Event>());
}
template <typename Event>
void enqueue_event(entt::dispatcher *dispatcher, const sol::table &evt) {
  assert(dispatcher && evt.valid());
  dispatcher->enqueue(evt.as<Event>());
}
template <typename Event> void clear_event(entt::dispatcher *dispatcher) {
  assert(dispatcher);
  dispatcher->clear<Event>();
}
template <typename Event> void update_event(entt::dispatcher *dispatcher) {
  assert(dispatcher);
  dispatcher->update<Event>();
}

template <typename Event> void register_meta_event() {
  using namespace entt::literals;

  entt::meta<Event>()
    .type()
    .template func<&register_listener<Event>>("register_listener"_hs)
    .template func<&trigger_event<Event>>("trigger_event"_hs)
    .template func<&enqueue_event<Event>>("enqueue_event"_hs)
    .template func<&clear_event<Event>>("clear_event"_hs)
    .template func<&update_event<Event>>("update_event"_hs);
}

[[nodiscard]] sol::table open_dispatcher(sol::this_state s) {
  // To create a dispatcher in a script: entt.dispatcher.new()

  sol::state_view lua{s};
  auto entt_module = lua["entt"].get_or_create<sol::table>();

  using namespace entt::literals;

  // clang-format off
  entt_module.new_usertype<entt::dispatcher>("dispatcher",
    sol::meta_function::construct,
    sol::factories([] { return entt::dispatcher{}; }),

    "trigger",
      [](entt::dispatcher &self, const sol::table &evt) {
      invoke_meta_func(
          get_type_id(evt), "trigger_event"_hs, &self, evt);
      },
    "enqueue",
      [](entt::dispatcher &self, const sol::table &evt) {
        invoke_meta_func(
          get_type_id(evt), "enqueue_event"_hs, &self, evt);
      },
    "clear",
      sol::overload(
        &entt::dispatcher::clear<>,
        [](entt::dispatcher &self, const sol::object &type_or_id) {
          invoke_meta_func(
            deduce_type(type_or_id), "clear_event"_hs, &self);
        }
      ),
      "update", [](entt::dispatcher &self, const sol::object &type_or_id) {
        invoke_meta_func(
          deduce_type(type_or_id), "update_event"_hs, &self);
      },
      "connect", [](entt::dispatcher &self, const sol::object &type_or_id,
        const sol::function &listener, sol::this_state s) -> sol::object {
        if (!listener.valid()) {
          // TODO: warning message
          return sol::lua_nil_t{};
        }
        const auto maybe_any = invoke_meta_func(deduce_type(type_or_id),
          "register_listener"_hs, &self, listener, s);
        return maybe_any ? maybe_any.cast<sol::reference>() : sol::lua_nil_t{};
      }
   
  );
  // clang-format on

  return entt_module;
}
