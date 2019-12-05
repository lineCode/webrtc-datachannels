#pragma once

#include <string>

namespace gloer {
namespace net {
namespace ws {

class SessionGUID {
public:
  SessionGUID() = delete;

  explicit SessionGUID(const std::string& id)
    : id_(id) {};

  explicit SessionGUID(const std::string&& id)
    : id_(std::move(id)) {};

  ~SessionGUID() {};

  operator std::string() const {
    return id_;
  };

  operator const std::string&() const {
      return id_;
  }

  operator std::string&&() = delete;

  bool operator==(const SessionGUID& r) const
  {
      return id_ == r.id_;
  }

  bool operator<(const SessionGUID& r) const {
      return id_ < r.id_;
  }

  bool operator==(const std::string& r) const {
      return id_ == r;
  }

  bool operator<(const std::string& r) const {
      return id_ < r;
  }

  friend bool operator==(const std::string& l, const SessionGUID& r) {
      return l == r.id_;
  }

  friend bool operator<(const std::string& l, const SessionGUID& r) {
      return l < r.id_;
  }

private:
  const std::string id_;
};

} // namespace ws
} // namespace net
} // namespace gloer

namespace std {
  template <> struct hash<gloer::net::ws::SessionGUID>
  {
    size_t operator()(const gloer::net::ws::SessionGUID& x) const
    {
      return hash<std::string>()(
        static_cast<std::string>(x));
    }
  };
} // namespace std
