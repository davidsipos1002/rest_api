#pragma once

#include <type_traits>

#include <Field.hpp>

template <std::size_t i, typename T, typename... Types>
struct GetType
{
    using type = GetType<i - 1, Types...>::type;
};

template <typename T, typename... Types>
struct GetType<0, T, Types...>
{
    using type = T;
};

template <std::size_t i, FieldConcept F>
class EntityFieldNode
{
public:
    EntityFieldNode() : value() {}
    EntityFieldNode(const F &value) : value(value) {}
    EntityFieldNode(F &&value) : value(std::move(value)) {}

    inline F &get()
    {
        return value;
    }

    inline const F &operator()() const
    {
        return value;
    }

private:
    F value;
};

template <std::size_t i, FieldConcept... Fields>
class EntityFieldList
{
};

template <std::size_t i, FieldConcept F, FieldConcept... Fields>
class EntityFieldList<i, F, Fields...> : public EntityFieldNode<i, typename std::remove_reference<F>::type>,
                                         public EntityFieldList<i + 1, Fields...>
{
public:
    EntityFieldList(){};
    template <FieldConcept U, FieldConcept... Args>
    EntityFieldList(U &&value, Args &&...args) : EntityFieldNode<i, typename std::remove_reference<U>::type>(std::forward<U>(value)),
                                                 EntityFieldList<i + 1, Fields...>(std::forward<Args>(args)...){};
};

class EntityBase
{
};

template <typename T>
concept EntityConcept = std::is_base_of<EntityBase, T>::value;

template <FieldConcept F, FieldConcept... Fields>
class Entity : public EntityBase, public EntityFieldList<0, F, Fields...>
{
public:
    template <FieldConcept... Args>
    Entity(Args &&...args) : EntityFieldList<0, F, Fields...>(std::forward<Args>(args)...){};

    static inline constexpr std::size_t count()
    {
        return sizeof...(Fields) + 1;
    }
};

template <std::size_t i, FieldConcept F, FieldConcept... Fields>
struct GetColumnName
{
    static inline constexpr const ColumnName name = GetColumnName<i - 1, Fields...>::name;
};

template <FieldConcept F, FieldConcept... Fields>
struct GetColumnName<0, F, Fields...>
{
    static inline constexpr const ColumnName name = F::columnName;
};

template <std::size_t i, FieldConcept... Fields>
auto &getField(Entity<Fields...> &entity)
{
    return (static_cast<EntityFieldNode<i, typename GetType<i, Fields...>::type> &>(entity)).get();
}

template <std::size_t i, FieldConcept... Fields>
const auto &getField(const Entity<Fields...> &entity)
{
    return (static_cast<const EntityFieldNode<i, typename GetType<i, Fields...>::type> &>(entity))();
}

template <std::size_t i, FieldConcept... Fields>
bool compareEntities(Entity<Fields...> &a, Entity<Fields...> &b)
{
    if constexpr (i == 0)
        return getField<0>(a) == getField<0>(b);
    else
        return getField<i>(a) == getField<i>(b) && compareEntities<i - 1>(a, b);
}

template <FieldConcept... Fields>
bool operator==(Entity<Fields...> &a, Entity<Fields...> &b)
{
    return compareEntities<sizeof...(Fields) - 1>(a, b);
}

template <std::size_t i, FieldConcept... Fields>
struct GetFieldType
{
    using type = std::remove_const_t<typename std::remove_reference_t<typename GetType<i, Fields...>::type::FieldType>>;
};
