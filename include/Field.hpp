#pragma once

#include <iostream>
#include <type_traits>

template <std::size_t N>
struct Label
{
    char string[N]{};

    inline consteval Label(const char (&str)[N])
    {
        std::copy_n(str, N, string);
    }

    inline consteval bool operator==(const Label<N> &other) const
    {
        return std::equal(other.string, other.string + N, string);
    }

    template <std::size_t N1>
    inline consteval bool operator==(const Label<N1> &other) const
    {
        return false;
    }

    inline consteval char operator[](std::size_t i) const
    {
        return string[i];
    }

    inline consteval std::size_t size() const
    {
        return N - 1;
    }

    inline operator const char *() const
    {
        return string;
    }

    inline operator std::string() const
    {
        return std::string(string);
    }
};

template <std::size_t N>
struct ColumnName : Label<N>
{
    inline consteval ColumnName(const char (&str)[N]) : Label<N>(str) {}
};

template <std::size_t N>
struct TableName : Label<N>
{
    inline consteval TableName(const char (&str)[N]) : Label<N>(str) {}
};

struct FieldBase
{
};

template <typename T>
concept FieldConcept = std::is_base_of<FieldBase, T>::value;

template <ColumnName col, typename T>
struct Field : public FieldBase
{
    using FieldType = std::remove_reference<T>::type;

    static inline constexpr const ColumnName columnName = col;
    FieldType value;

    Field() : value() {}
    Field(const FieldType &value) : value(value) {}
    Field(const Field &field) : value(field.value) {}
    Field(Field &&field) : value(std::move(field.value)) {}

    inline Field &operator=(const Field &other)
    {
        value = other.value;
        return *this;
    }

    inline Field &operator=(Field &&other)
    {
        value = std::move(other.value);
        return *this;
    }

    template <ColumnName col1, typename U>
    inline bool operator==(const Field<col1, U> &other) const
    {
        return false;
    }

    template <>
    inline bool operator==(const Field<col, T> &other) const
    {
        return other.value == value;
    }
};

template <ColumnName col, typename T>
inline std::ostream &operator<<(std::ostream &os, const Field<col, T> &f)
{
    os << f.columnName << " -> " << f.value;
    return os;
}

// Not used
// template <ColumnName col, TableName join, typename T>
// struct JoinField : public FieldBase
// {
//     using FieldType = std::remove_reference<T>::type;

//     static inline constexpr const ColumnName columnName = col;
//     static inline constexpr const TableName joinTable = join;
//     FieldType value;

//     JoinField(const FieldType &value) : value(value) {}
//     JoinField(const JoinField &field) : value(field.value) {}
//     JoinField(JoinField &&field) : value(std::move(field.value)) {}

//     inline JoinField &operator=(const JoinField &other)
//     {
//         value = other.value;
//         return *this;
//     }

//     inline JoinField &operator=(JoinField &&other)
//     {
//         value = std::move(other);
//         return *this;
//     }

//     template <ColumnName col1, TableName join1, typename U>
//     inline bool operator==(const JoinField<col1, join1, U> &other) const
//     {
//         return false;
//     }

//     template <>
//     inline bool operator==(const JoinField<col, join, T> &other) const
//     {
//         return other.value == value;
//     }
// };

// template <ColumnName col, TableName join, typename T>
// inline std::ostream &operator<<(std::ostream &os, const JoinField<col, join, T> &f)
// {
//     os << f.columnName << " -> " << f.value << " :: join with " << f.joinTable;
//     return os;
// }
