#pragma once

#include <tuple>
#include <type_traits>
#include <algorithm>
#include <array>

#include <sqlite3.h>

#include "kitty/util/string.h"
#include "kitty/util/utility.h"
#include "kitty/util/optional.h"
#include "kitty/util/template_helper.h"

namespace sqlite {
  typedef util::safe_ptr_v2<sqlite3, int, sqlite3_close> sDatabase;
  typedef util::safe_ptr_v2<sqlite3_stmt, int, sqlite3_finalize> sStatement;
  
  sDatabase  mk_db(const char *dbName);
  sStatement mk_statement(sqlite3 *db, const char *sql);
  
  enum comp_t : int {
    eq,
    nt_eq,
    gt,
    st,
    st_eq,
    gt_eq
  };
  
  extern const char *comp_to_string[];
  
  template<class ...Args>
  class Filter {
  public:
    typedef std::tuple<util::Optional<std::pair<comp_t, Args>>...> tuple_optional;
    
    Filter() = default;
    Filter(Filter &&) = default;
    
    tuple_optional tuple;
    
    template<std::size_t elem, class ...ConstructArgs>
    Filter &set(comp_t comp, ConstructArgs &&... args) {
      // Get the undelying element type from the optional
      typedef typename std::tuple_element<elem, tuple_optional>::type::elem_t elem_t;
      
      std::get<elem>(tuple) = elem_t { comp, { std::forward<ConstructArgs>(args)... } };
      
      return *this;
    }
  };
  
  // Add room for id
  template<class Tuple>
  using model_t = std::decay_t<decltype(std::tuple_cat(std::declval<Tuple>(), std::make_tuple(std::declval<sqlite3_int64>())))>;
  
  // Type aliasing without make std::is_same<T, Unique<T>> true
  template<class T, class V = void>
  class Unique : public T {
  public:
    typedef T elem_t;
    using T::T;
    
    Unique() : T() {}
    Unique(const Unique &) = default;
    Unique(Unique &&) = default;
    
    Unique &operator =(const Unique&) = default;
    Unique &operator =(Unique&&) = default;
    
    Unique(const T&other) :  T(other) {}
    Unique(T&&other) : T(std::move(other)) {}
    
    Unique &operator =(const T& other) {
      _this() = other;
      
      return *this;
    }
    
    Unique &operator =(T&& other) {
      _this() = std::move(other);
      
      return *this;
    }
    
  private:
    T &_this() { return *reinterpret_cast<T*>(this); }
  };
  
  // Type aliasing without make std::is_same<T, Unique<T>> true
  template<class T>
  class Unique<T, std::enable_if_t<!std::is_class<T>::value>> {
    T _t;
  public:
    typedef T elem_t;
    
    Unique() = default;
    Unique(const Unique &) = default;
    Unique(Unique &&) = default;
    
    Unique &operator =(const Unique&) = default;
    Unique &operator =(Unique&&) = default;
    
    Unique(const T&other) :  _t(other) {}
    Unique(T&&other) : _t(std::move(other)) {}
    
    operator elem_t& () { return _t; }
    operator const elem_t& () const { return _t; }
  };
  
  template<class T>
  using OptionalUnique = util::Optional<Unique<T>>;
  
  template<class ...Args>
  class Model {
  public:
    typedef std::tuple<Args...> tuple_t;
    typedef Filter<Args...> filter_t;
    
    static constexpr std::size_t tuple_size = std::tuple_size<tuple_t>::value;
  private:
    std::string _table;
    std::array<std::string, tuple_size> _columns;
    
    sStatement _insert;
    sStatement _update;
    sStatement _delete;
  public:
    Model(std::string &&table, std::array<std::string, tuple_size> &&columns, sqlite3 *db)
    : _table(std::move(table)), _columns(std::move(columns))
    {
      char *buf { nullptr };
      sqlite3_exec(db, _sql_init().c_str(), nullptr, nullptr, &buf);
      if(buf) {
        err::set(buf);
        
        sqlite3_free(buf);
        
        return;
      }
      
      _insert = sStatement(mk_statement(db, _sql_insert().c_str()));
      _update = sStatement(mk_statement(db, _sql_update().c_str()));
      _delete = sStatement(mk_statement(db, _sql_delete().c_str()));
    }
    

    template<class... BuildArgs>
    util::Optional<model_t<tuple_t>> build(BuildArgs && ... args) {
      tuple_t tuple { std::forward<BuildArgs>(args)... };
      if(_input(_insert, tuple)) {
        return {};
      }
      
      sqlite3_int64 id = sqlite3_last_insert_rowid(sqlite3_db_handle(_insert.get()));
      
      return std::tuple_cat(std::move(tuple), std::make_tuple(id));
    }
    
    int save(const model_t<tuple_t> &tuple) {
      if(_input(_update, tuple)) {
        return -1;
      }
      return 0;
    }
    
    template<class Callable>
    int load(Callable &&f) {
      auto statement = mk_statement(sqlite3_db_handle(_insert.get()), _sql_select().c_str());
      return _output(statement, std::forward<Callable>(f));
    }
    
    template<class Callable>
    int load(const std::vector<filter_t> &filters, Callable &&f) {
      auto statement = mk_statement(sqlite3_db_handle(_insert.get()), (_sql_select() + _sql_where(filters)).c_str());
      return _output(statement, filters, std::forward<Callable>(f));
    }
    
    int delete_(const model_t<tuple_t> &tuple) {
      return _input(_delete, std::make_tuple(std::get<tuple_size>(tuple)));
    }
    
    filter_t mk_filter() { return filter_t(); }
    
    template<class ...ConstructArgs>
    std::vector<filter_t> mk_filters(ConstructArgs && ... args) {
      std::vector<filter_t> filters { sizeof...(Args) };
      
      _mk_filters_helper(filters, std::move(args)...);
      
      return filters;
    }
    
  private:
    template<class ...ConstructArgs>
    void _mk_filters_helper(std::vector<filter_t> &filters, filter_t && filter, ConstructArgs && ... args) {
      filters.emplace_back(std::move(filter));
      
      _mk_filters_helper(filters, std::move(args)...);
    }
    
    void _mk_filters_helper(std::vector<filter_t> &filters) {}
    
    template<class Tuple>
    int _input(sStatement &statement, const Tuple &tuple) {
      sqlite3_reset(statement.get());
      sqlite3_clear_bindings(statement.get());
      
      SQL<Tuple, 0>::bind(statement, tuple);
      
      while(true) {
        auto err = sqlite3_step(statement.get());
        
        if(err == SQLITE_ERROR || err == SQLITE_MISUSE) {
          err::set(sqlite3_errmsg(sqlite3_db_handle(statement.get())));
          
          return -1;
        }
        
        if(err == SQLITE_CONSTRAINT) {
          err::set(sqlite3_errmsg(sqlite3_db_handle(statement.get())));
          
          return -1;
        }
        
        if(err != SQLITE_ROW) {
          return 0;
        }
      }
    }
    
    template<class Callable>
    int _output(sStatement &statement, Callable &&f) {
      sqlite3_reset(statement.get());
      sqlite3_clear_bindings(statement.get());
      
      return _exec(statement, std::forward<Callable>(f));
    }
    
    template<class Callable>
    int _output(sStatement &statement, const std::vector<filter_t> &filters, Callable &&f) {
      int bound = 0;
      for(auto &filter : filters) {
        bound = SQL<tuple_t, 0>::optional_bind(bound, statement, filter);
      }
      
      return _exec(statement, std::forward<Callable>(f));
    }
    
    template<class Callable>
    int _exec(sStatement &statement, Callable &&f) {
      while(true) {
        auto err = sqlite3_step(statement.get());
        
        if(err == SQLITE_ERROR || err == SQLITE_MISUSE) {
          err::set(sqlite3_errmsg(sqlite3_db_handle(statement.get())));
          
          return -1;
        }
        
        if(err != SQLITE_ROW) {
          return 0;
        }
        
        auto return_val = f(SQL<model_t<tuple_t>, 0>::row(statement));
        
        if(return_val != err::OK) {
          return return_val == err::BREAK ? 0 : -1;
        }
      }
    }
    
    std::string _sql_init() {
      return "CREATE TABLE IF NOT EXISTS " + _table + " (ID INTEGER PRIMARY KEY, " + SQL<tuple_t, 0>::init(*this);
    }
    
    std::string _sql_insert() {
      std::string sql = "INSERT INTO " + _table + " (";
      std::for_each(_columns.begin(), _columns.end() -1, [&](auto &column) {
        sql.append(column).append(",");
      });
      
      sql.append(*(_columns.end() - 1)).append(") VALUES (");
      
      std::for_each(_columns.begin(), _columns.end() -1, [&](auto &column) {
        sql.append("?,");
      });
      
      sql.append("?);");
      return sql;
    }
    
    std::string _sql_update() {
      std::string sql = "UPDATE " + _table + " SET ";
      
      std::for_each(_columns.begin(), _columns.end() -1, [&](auto &column) {
        sql.append(column).append(" = ?,");
      });
      
      sql.append(*(_columns.end() - 1)).append(" = ?").append(" WHERE ID = ?");
      
      return sql;
    }
    
    std::string _sql_delete() {
      return "DELETE FROM " + _table + " WHERE ID = ?";
    }  
    
    std::string _sql_select() {
      std::string columns;
      
      for(auto &column : _columns) {
        columns.append(column).append(",");
      }
      
      columns.append("ID");
      
      return "SELECT " + columns + " FROM " + _table;
    }
    
    std::string _sql_where(const std::vector<filter_t> &filters) {
      if(filters.empty()) {
        return std::string();
      }
      
      const filter_t &first = filters[0];
      
      std::string sql = " WHERE " + SQL<tuple_t, 0>::filter(*this, first);
      
      std::for_each(std::begin(filters) +1, std::end(filters), [&](const filter_t &filter) {
        sql.append(" OR " + SQL<tuple_t, 0>::filter(*this, filter));
      });
      
      return sql;
    }
    
    template<class tuple_t, int elem, class NULL_CLASS = void>
    class SQL {
      static constexpr std::size_t tuple_size = std::tuple_size<tuple_t>::value;
      static constexpr int offset = elem;
      
      typedef std::decay_t<decltype(std::get<offset>(std::declval<tuple_t>()))> elem_t;
    public:
      static std::string init(Model &_this) {
        if(elem == tuple_size -1) {
          return _this._columns[offset] + " " + AppendFunc<elem_t>::not_null() + SQL<tuple_t, elem +1>::init(_this);
        }
        else {
          return _this._columns[offset] + " " + AppendFunc<elem_t>::not_null() + "," + SQL<tuple_t, elem +1>::init(_this);
        }
      }
      
      static void bind(sStatement &statement, const tuple_t &tuple) {
        AppendFunc<elem_t>::bind(statement, std::get<offset>(tuple));
        
        SQL<tuple_t, elem +1>::bind(statement, tuple);
      }
      
      template<class ...Columns>
      static tuple_t row(sStatement &statement, Columns && ... columns) {
        return SQL<tuple_t, elem +1>::row(statement, std::forward<Columns>(columns)..., AppendFunc<elem_t>::get(statement));
      }
      
      static std::string filter(Model &_this, const filter_t &filter) {
        auto &optional_value = std::get<elem>(filter.tuple);
        
        std::string sql;
        if(optional_value) {
          if(elem == 0) {
            sql = "(" + _this._columns[elem] + " " + comp_to_string[optional_value->first] + " ? ";
          }
          else {
            sql = "AND " + _this._columns[elem] + " " + comp_to_string[optional_value->first] + " ? ";
          }
        }
        
        return sql + SQL<tuple_t, elem +1>::filter(_this, filter);
      }
      
      static int optional_bind(int bound, sStatement &statement, const filter_t &filter) {
        auto &optional_value = std::get<elem>(filter.tuple);
        
        if(optional_value) {
          _optional_bind_helper(bound, statement, optional_value->second);
          
          return SQL<tuple_t, elem +1>::optional_bind(bound +1, statement, filter);
        }
        else {
          return SQL<tuple_t, elem +1>::optional_bind(bound, statement, filter);
        }
      }
      
      /* very */ private:
      template<class T>
      static void _optional_bind_helper(int bound, sStatement &statement, const T& value) {
        AppendFunc<T>::bind(statement, value, bound);
      }
      
      template<class T, class S = void>
      struct AppendFunc {
        typedef decltype(*std::begin(std::declval<T>())) raw_type;
        typedef std::decay_t<raw_type> base_type;
        
        static std::string not_null() {
          return type() + " NOT NULL";
        }
        
        
        static std::string type() {
          return std::is_same<char, base_type>::value ? "TEXT" : "BLOB";
        }
        
        static void deleter(void *ptr) {
          delete[] reinterpret_cast<base_type*>(ptr);
        }
        
        static void bind(sStatement &statement, const T& value, int element = offset) {
          auto begin = std::begin(value);
          auto end   = std::end(value);
          
          int distance = (int)std::distance(begin, end);
          auto buf = std::unique_ptr<base_type[]>(new base_type[distance +1]);
          
          std::copy(begin, end, buf.get());
          
          // reinterpret_cast is used to ensure it compiles.
          if(std::is_same<char, base_type>::value) {
            sqlite3_bind_text(statement.get(), element +1, reinterpret_cast<const char*>(buf.release()), distance, deleter);
          }
          else {
            sqlite3_bind_blob(statement.get(), element +1, reinterpret_cast<const void*>(buf.release()), distance, deleter);
          }
        }
        
        static T get(sStatement &statement) {
          const base_type *begin = GetFunc<base_type>::get(statement);
          const base_type *end   = begin + sqlite3_column_bytes(statement.get(), offset);
          
          return { begin, end };
        }
        
      private:
        template<class V, class VOID = void>
        struct GetFunc {
          static const V* get(sStatement &statement) {
            return reinterpret_cast<const V*>(sqlite3_column_blob(statement.get(), offset));
          }
        };
        
        template<class V>
        struct GetFunc<V, std::enable_if_t<std::is_same<char, V>::value>> {
          static const char* get(sStatement &statement) {
            return reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), offset));
          }
        };
      };
      
      template<class T>
      struct AppendFunc<T, std::enable_if_t<std::is_integral<T>::value>> {
        static std::string not_null() {
          return "INTEGER NOT NULL";
        }
        
        static std::string type() {
          return "INTEGER";
        }
        
        static void bind(sStatement &statement, const T& value, int element = offset) {
          sqlite3_bind_int64(statement.get(), element +1, value);
        }
        
        static T get(sStatement &statement) {
          return (T)sqlite3_column_int64(statement.get(), offset);
        }
      };
      
      template<class T>
      struct AppendFunc<T, std::enable_if_t<std::is_floating_point<T>::value>> {
        static std::string not_null() {
          return "REAL NOT NULL";
        }
        
        static std::string type() {
          return "REAL";
        }
        
        static void bind(sStatement &statement, const T& value, int element = offset) {
          sqlite3_bind_double(statement.get(), element +1, value);
        }
        
        static T get(sStatement &statement) {
          return sqlite3_column_double(statement.get(), offset);
        }
      };
      
      template<class T>
      struct AppendFunc<T, std::enable_if_t<std::is_pointer<T>::value>>;
      
      template<class T>
      struct AppendFunc<T, std::enable_if_t<util::instantiation_of<util::Optional, T>::value>> {
        typedef typename T::elem_t elem_t;
        
        static_assert(!util::contains_instantiation_of<util::Optional, elem_t>::value, "Optional<Optional<T>> detected.");
        static std::string not_null() {
          return AppendFunc<elem_t>::type();
        }
        
        static void bind(sStatement &statement, const T &value, int element = offset) {
          if(value) {
            AppendFunc<elem_t>::bind(statement, *value, element);
          }
          else {
            sqlite3_bind_null(statement.get(), element +1);
          }
        }
        
        static T get(sStatement &statement) {
          if(sqlite3_column_type(statement.get(), offset +1) == SQLITE_NULL) {
            return {};
          }
          
          return AppendFunc<elem_t>::get(statement);
        }
      };
      
      template<class T>
      struct AppendFunc<T, std::enable_if_t<util::instantiation_of<Unique, T>::value>> {
        typedef typename T::elem_t elem_t;
        
        static_assert(!util::contains_instantiation_of<util::Optional, elem_t>::value, "Optional should be the top constraint.");
        static_assert(!util::contains_instantiation_of<Unique, elem_t>::value, "Unique<Unique<T>> detected.");
        
        static std::string not_null() {
          return AppendFunc<elem_t>::not_null() + " UNIQUE";
        }
        
        static std::string type() {
          return AppendFunc<elem_t>::type() + " UNIQUE";
        }
        
        static void bind(sStatement &statement, const T &value, int element = offset) {
          AppendFunc<elem_t>::bind(statement, value, element);
        }
        
        static T get(sStatement &statement) {
          return AppendFunc<elem_t>::get(statement);
        }
      };
    };
    
    template<class tuple_t, int elem>
    class SQL<tuple_t, elem, std::enable_if_t<(elem == std::tuple_size<tuple_t>::value)>> {
    public:
      static std::string init(Model &_this) { return ");"; }
      
      static void bind(sStatement &statement, const tuple_t &tuple) {}
      
      template<class ...Columns>
      static tuple_t row(sStatement &statement, Columns && ... columns) {
        return std::make_tuple(std::forward<Columns>(columns)...);
      }
      
      static std::string filter(Model &_this, const filter_t& filter) { return ")"; }
      
      // Return the amount of values bound
      static int optional_bind(int bound, sStatement &statement, const filter_t &filter) { return bound; }
    };
  };

  template<template<typename...> class X, typename Y>
  struct WithArgs {};

  template<template<typename...> class X, typename... Args>
  struct WithArgs<X, X<Args...>> {
    typedef Model<Args...> type;
  };

  template<class Tuple>
  using model_controller_t = typename WithArgs<std::tuple, Tuple>::type;

  class Database {
    sDatabase _db;
  public:
    
    Database(const char *dbName);
    Database(const std::string &dbName);
    
    template<class Tuple, class ...Args>
    auto mk_model(std::string &&table, Args &&... columns) {
      static_assert(sizeof...(Args) >= std::tuple_size<Tuple>::value, "Too few columns");
      static_assert(sizeof...(Args) == std::tuple_size<Tuple>::value, "Too many columns");

      return model_controller_t<Tuple>(std::move(table), { std::forward<Args>(columns)... }, _db.get());
    }
  };
}