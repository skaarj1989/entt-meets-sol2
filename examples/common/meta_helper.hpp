#pragma once

entt::id_type get_type_id(const sol::table &obj) {
  auto &f = obj["type_id"];
  return f.valid() ? f().get<entt::id_type>() : -1;
}

template <typename T> entt::id_type deduce_type(const T &obj) {
  switch (obj.get_type()) {
  // in lua: registry:has(e, Transform.type_id())
  case sol::type::number:
    return obj.as<entt::id_type>();
  // in lua: registry:has(e, Transform)
  case sol::type::table:
    return get_type_id(obj);
  }
  assert(false);
  return -1;
}

// @see
// https://github.com/skypjack/entt/wiki/Crash-Course:-runtime-reflection-system
template <typename... Args>
auto invoke_meta_func(entt::id_type type_id, entt::id_type func_id,
                      Args &&...args) {
  if (const auto meta_type = entt::resolve(type_id); meta_type)
    return meta_type.func(func_id).invoke({}, std::forward<Args>(args)...);
  return entt::meta_any{};
}