#pragma once
#include <compare>
#include <utility>
#include <cstddef>
#include <functional>

namespace util {

/**
 * Универсальный шаблонный класс "Тип с меткой".
 * Позволяет создавать строгие типы на основе других типов.
 * Пример использования:
 *
 *  struct AddressTag{}; // метка для строки-адреса
 *  using Address = util::Tagged<std::string, AddressTag>;
 *
 *  struct NameTag{}; // метка для строки-имени
 *  using Name = util::Tagged<std::string, NameTag>;
 *
 *  struct Person {
 *      Name name;
 *      Address address;
 *  };
 *
 *  Name name{"Harry Potter"s};
 *  Address address{"4 Privet Drive, Little Whinging, Surrey, England"s};
 *
 * Person p1{name, address}; // OK
 * Person p2{address, name}; // Ошибка, Address и Name — разные типы
 */
template <typename Value, typename Tag>
class Tagged {
public:
    using ValueType = Value;
    using TagType = Tag;

    explicit Tagged(Value&& v)
        : value_(std::move(v)) {
    }
    explicit Tagged(const Value& v)
        : value_(v) {
    }

    const Value& operator*() const {
        return value_;
    }

    Value& operator*() {
        return value_;
    }

    // В C++20 можно объявить оператор сравнения Tagged-типов
    // Просто вызывается соответствующий оператор для value_
    auto operator<=>(const Tagged<Value, Tag>&) const = default;

private:
    Value value_;
};

// Хеш-функция для Tagged-типа, чтобы объекты можно было хранить в unordered-контейнерах
template <typename TaggedValue>
struct TaggedHasher {
    size_t operator()(const TaggedValue& value) const {
        // Возвращает хеш значения, хранящегося внутри value
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

}  // namespace util
