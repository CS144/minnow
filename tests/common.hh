#pragma once

#include "conversions.hh"
#include "exception.hh"

#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

class ExpectationViolation : public std::runtime_error
{
public:
  static constexpr std::string boolstr( bool b ) { return b ? "true" : "false"; }

  explicit ExpectationViolation( const std::string& msg ) : std::runtime_error( msg ) {}

  template<typename T>
  inline ExpectationViolation( const std::string& property_name, const T& expected, const T& actual );
};

template<typename T>
ExpectationViolation::ExpectationViolation( const std::string& property_name, const T& expected, const T& actual )
  : ExpectationViolation { "The object should have had " + property_name + " = " + to_string( expected )
                           + ", but instead it was " + to_string( actual ) + "." }
{}

template<>
inline ExpectationViolation::ExpectationViolation( const std::string& property_name,
                                                   const bool& expected,
                                                   const bool& actual )
  : ExpectationViolation { "The object should have had " + property_name + " = " + boolstr( expected )
                           + ", but instead it was " + boolstr( actual ) + "." }
{}

template<class T>
struct TestStep
{
  virtual std::string str() const = 0;
  virtual void execute( T& ) const = 0;
  virtual uint8_t color() const = 0;

  TestStep() = default;
  TestStep( const TestStep& other ) = default;
  TestStep( TestStep&& other ) noexcept = default;
  TestStep& operator=( const TestStep& other ) = default;
  TestStep& operator=( TestStep&& other ) noexcept = default;
  virtual ~TestStep() = default;
};

class Printer
{
  bool is_terminal_;

public:
  Printer();

  static constexpr int red = 31;
  static constexpr int green = 32;
  static constexpr int blue = 34;
  static constexpr int def = 39;

  std::string with_color( int color_value, std::string_view str ) const;

  static std::string prettify( std::string_view str, size_t max_length = 32 );

  void diagnostic( std::string_view test_name,
                   const std::vector<std::pair<std::string, int>>& steps_executed,
                   const std::string& failing_step,
                   const std::exception& e ) const;
};

template<class T>
class TestHarness
{
  std::string test_name_;
  T obj_;

  std::vector<std::pair<std::string, int>> steps_executed_ {};
  Printer pr_ {};

protected:
  explicit TestHarness( std::string test_name, std::string_view desc, T&& object )
    : test_name_( std::move( test_name ) ), obj_( std::move( object ) )
  {
    steps_executed_.emplace_back( "Initialized " + demangle( typeid( T ).name() ) + " with " + std::string { desc },
                                  Printer::def );
  }

  const T& object() const { return obj_; }

public:
  void execute( const TestStep<T>& step )
  {
    try {
      step.execute( obj_ );
      steps_executed_.emplace_back( step.str(), step.color() );
    } catch ( const ExpectationViolation& e ) {
      pr_.diagnostic( test_name_, steps_executed_, step.str(), e );
      throw std::runtime_error { "The test \"" + test_name_ + "\" failed." };
    } catch ( const std::exception& e ) {
      pr_.diagnostic( test_name_, steps_executed_, step.str(), e );
      throw std::runtime_error { "The test \"" + test_name_ + "\" made your code throw an exception." };
    }
  }
};

template<class T>
struct Expectation : public TestStep<T>
{
  std::string str() const override { return "Expectation: " + description(); }
  virtual std::string description() const = 0;
  uint8_t color() const override { return Printer::green; }
};

template<class T>
struct Action : public TestStep<T>
{
  std::string str() const override { return "Action: " + description(); }
  virtual std::string description() const = 0;
  uint8_t color() const override { return Printer::blue; }
};

template<class T>
struct ExpectBool : public Expectation<T>
{
  bool value_;
  explicit ExpectBool( bool value ) : value_( value ) {}
  std::string description() const override { return name() + " = " + ExpectationViolation::boolstr( value_ ); }
  virtual std::string name() const = 0;
  virtual bool value( T& ) const = 0;
  void execute( T& obj ) const override
  {
    const bool result = value( obj );
    if ( result != value_ ) {
      throw ExpectationViolation { name(), value_, result };
    }
  }
};

template<class T, typename Num>
struct ExpectNumber : public Expectation<T>
{
  Num num_;
  explicit ExpectNumber( Num num ) : num_( num ) {}
  std::string description() const override { return name() + " = " + to_string( num_ ); }
  virtual std::string name() const = 0;
  virtual Num value( T& ) const = 0;
  void execute( T& obj ) const override
  {
    const Num result { value( obj ) };
    if ( result != num_ ) {
      throw ExpectationViolation { name(), num_, result };
    }
  }
};
