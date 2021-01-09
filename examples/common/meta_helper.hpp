#pragma once

using namespace entt::literals;

entt::id_type get_type_id(const sol::table &obj) {
  if (auto &f = obj["type_id"]; f.valid())
    return f().get<entt::id_type>();
  return -1;
}

template <typename T>
entt::id_type deduce_type(const T &obj) {
  switch (obj.get_type()) {
  case sol::type::number:
    // registry.has(e, Transform.type_id())
    return obj.as<entt::id_type>();
  case sol::type::table:
    // registry.has(e, Transform) 
    return get_type_id(obj);
  }
  return -1;
}

// @see https://github.com/skypjack/entt/wiki/Crash-Course:-runtime-reflection-system
template <typename... Args>
auto invoke_meta_func(entt::id_type type_id, entt::id_type func_id,
                      Args... args) {
  if (auto meta_type = entt::resolve(type_id); meta_type)
    return meta_type.func(func_id).invoke({}, std::forward<Args>(args)...);
  return entt::meta_any{};
}