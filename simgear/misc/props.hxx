/**
 * \file props.hxx
 * Interface definition for a property list.
 * Started Fall 2000 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * See props.html for documentation [replace with URL when available].
 *
 * $Id$
 */

#ifndef __PROPS_HXX
#define __PROPS_HXX

#include <simgear/compiler.h>

#include <stdio.h>

#include STL_STRING
#include <vector>
#include STL_IOSTREAM

SG_USING_STD(string);
SG_USING_STD(vector);
#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(istream);
SG_USING_STD(ostream);
#endif

#ifdef ALIAS
#pragma warn A sloppy coder has defined ALIAS as a macro!
#undef ALIAS
#endif

#ifdef UNSPECIFIED
#pragma warn A sloppy coder has defined UNSPECIFIED as a macro!
#undef UNSPECIFIED
#endif

#ifdef BOOL
#pragma warn A sloppy coder has defined BOOL as a macro!
#undef BOOL
#endif

#ifdef INT
#pragma warn A sloppy coder has defined INT as a macro!
#undef INT
#endif

#ifdef LONG
#pragma warn A sloppy coder has defined LONG as a macro!
#undef LONG
#endif

#ifdef FLOAT
#pragma warn A sloppy coder has defined FLOAT as a macro!
#undef FLOAT
#endif

#ifdef DOUBLE
#pragma warn A sloppy coder has defined DOUBLE as a macro!
#undef DOUBLE
#endif

#ifdef STRING
#pragma warn A sloppy coder has defined STRING as a macro!
#undef STRING
#endif



////////////////////////////////////////////////////////////////////////
// A raw value.
//
// This is the mechanism that information-providing routines can
// use to link their own values to the property manager.  Any
// SGValue can be tied to a raw value and then untied again.
////////////////////////////////////////////////////////////////////////


/**
 * Abstract base class for a raw value.
 *
 * The property manager is implemented in three layers.  The {@link
 * SGPropertyNode} is the highest and most abstract layer,
 * representing * an LValue/RValue pair: it * records the position
 * of the property in the property tree and * contains facilities
 * for navigation to other nodes.  Each node * may contain an {@link
 * SGValue}, which is guaranteed persistent: the * {@link SGValue}
 * will not change during a session, even if the * property is bound
 * and unbound multiple times.  The SGValue is the * abstraction of
 * an RValue: it allows for conversion among all of the different
 * types, and can be bound to external pointers, functions, methods,
 * or other data sources.  Every SGValue contains an SGRawValue of
 * a specific type.  The SGRawValue (this class) may change frequently
 * during a session as a value is retyped or bound and unbound to
 * various data source, but the abstract SGValue layer insulates
 * the application from those changes.  The raw value contains no
 * facilities for data binding or for type conversion: it is simply
 * the abstraction of a primitive data type (or a compound data
 * type, in the case of a string).
 *
 * The SGValue class always keeps a *copy* of a raw value, not the
 * original one passed to it; if you override a derived class but do
 * not replace the {@link #clone} method, strange things will happen.
 *
 * All raw values must implement {@link #getValue}, {@link #setValue},
 * and {@link #clone} for the appropriate type.
 *
 * @see SGValue
 * @see SGPropertyNode */
template <class T>
class SGRawValue
{
public:

  /**
   * The default underlying value for this type.
   *
   * Every raw value has a default; the default is false for a
   * boolean, 0 for the various numeric values, and "" for a string.
   * If additional types of raw values are added in the future, they
   * may need different kinds of default values (such as epoch for a
   * date type).  The default value is used when creating new values.
   */
  static const T DefaultValue;	// Default for this kind of raw value.


  /**
   * Constructor.
   *
   * Use the default value for this type.
   */
  SGRawValue () {}


  /**
   * Destructor.
   */
  virtual ~SGRawValue () {}


  /**
   * Return the underlying value.
   *
   * @return The actual value for the property.
   * @see #setValue
   */
  virtual T getValue () const = 0;


  /**
   * Assign a new underlying value.
   *
   * If the new value cannot be set (because this is a read-only
   * raw value, or because the new value is not acceptable for
   * some reason) this method returns false and leaves the original
   * value unchanged.
   *
   * @param value The actual value for the property.
   * @return true if the value was set successfully, false otherwise.
   * @see #getValue
   */
  virtual bool setValue (T value) = 0;


  /**
   * Create a new deep copy of this raw value.
   *
   * The copy will contain its own version of the underlying value
   * as well.
   *
   * @return A deep copy of the current object.
   */
  virtual SGRawValue * clone () const = 0;
};


/**
 * An unbound raw value, stored internally.
 *
 * Instances of this class are created automatically, by default,
 * by the SGValue class; ordinarily the application should not
 * need to touch it.
 */
template <class T>
class SGRawValueInternal : public SGRawValue<T>
{
public:

  /**
   * Default constructor.
   *
   * Initialize with the default value for this type.
   */
  SGRawValueInternal () {}

  /**
   * Explicit value constructor.
   *
   * Initialize with the underlying value provided.
   *
   * @param value The initial value for this property.
   */
  SGRawValueInternal (T value) : _value(value) {}

  /**
   * Destructor.
   */
  virtual ~SGRawValueInternal () {}

  /**
   * Get the underlying value.
   */
  virtual T getValue () const { return _value; }

  /**
   * Set the underlying value.
   */
  virtual bool setValue (T value) { _value = value; return true; }

  /**
   * Create a deep copy of this raw value.
   */
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueInternal<T>(_value);
  }

private:
  T _value;
};


/**
 * A raw value bound to a pointer.
 *
 * This is the most efficient way to tie an external value, but also
 * the most dangerous, because there is no way for the supplier to
 * perform bounds checking and derived calculations except by polling
 * the variable to see if it has changed.  There is no default
 * constructor, because this class would be meaningless without a
 * pointer.
 */
template <class T>
class SGRawValuePointer : public SGRawValue<T>
{
public:

  /**
   * Explicit pointer constructor.
   *
   * Create a new raw value bound to the value of the variable
   * referenced by the pointer.
   *
   * @param ptr The pointer to the variable to which this raw value
   * will be bound.
   */
  SGRawValuePointer (T * ptr) : _ptr(ptr) {}

  /**
   * Destructor.
   */
  virtual ~SGRawValuePointer () {}

  /**
   * Get the underlying value.
   *
   * This method will dereference the pointer and return the
   * variable's value.
   */
  virtual T getValue () const { return *_ptr; }

  /**
   * Set the underlying value.
   *
   * This method will dereference the pointer and change the
   * variable's value.
   */
  virtual bool setValue (T value) { *_ptr = value; return true; }

  /**
   * Create a copy of this raw value.
   *
   * The copy will use the same external pointer as the original.
   */
  virtual SGRawValue<T> * clone () const {
    return new SGRawValuePointer<T>(_ptr);
  }

private:
  T * _ptr;
};


/**
 * A value managed through static functions.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class T>
class SGRawValueFunctions : public SGRawValue<T>
{
public:

  /**
   * The template type of a static getter function.
   */
  typedef T (*getter_t)();

  /**
   * The template type of a static setter function.
   */
  typedef void (*setter_t)(T);

  /**
   * Explicit constructor.
   *
   * Create a new raw value bound to the getter and setter supplied.
   *
   * @param getter A static function for getting a value, or 0
   * to read-disable the value.
   * @param setter A static function for setting a value, or 0
   * to write-disable the value.
   */
  SGRawValueFunctions (getter_t getter = 0, setter_t setter = 0)
    : _getter(getter), _setter(setter) {}

  /**
   * Destructor.
   */
  virtual ~SGRawValueFunctions () {}

  /**
   * Get the underlying value.
   *
   * This method will invoke the getter function to get a value.
   * If no getter function was supplied, this method will always
   * return the default value for the type.
   */
  virtual T getValue () const {
    if (_getter) return (*_getter)();
    else return SGRawValue<T>::DefaultValue;
  }

  /**
   * Set the underlying value.
   *
   * This method will invoke the setter function to change the
   * underlying value.  If no setter function was supplied, this
   * method will return false.
   */
  virtual bool setValue (T value) {
    if (_setter) { (*_setter)(value); return true; }
    else return false;
  }

  /**
   * Create a copy of this raw value, bound to the same functions.
   */
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueFunctions<T>(_getter,_setter);
  }

private:
  getter_t _getter;
  setter_t _setter;
};


/**
 * An indexed value bound to static functions.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.  An indexed value is useful for binding one
 * of a list of possible values (such as multiple engines for a
 * plane).  The index is hard-coded at creation time.
 */
template <class T>
class SGRawValueFunctionsIndexed : public SGRawValue<T>
{
public:
  typedef T (*getter_t)(int);
  typedef void (*setter_t)(int,T);
  SGRawValueFunctionsIndexed (int index, getter_t getter = 0, setter_t setter = 0)
    : _index(index), _getter(getter), _setter(setter) {}
  virtual ~SGRawValueFunctionsIndexed () {}
  virtual T getValue () const {
    if (_getter) return (*_getter)(_index);
    else return SGRawValue<T>::DefaultValue;
  }
  virtual bool setValue (T value) {
    if (_setter) { (*_setter)(_index, value); return true; }
    else return false;
  }
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueFunctionsIndexed<T>(_index, _getter, _setter);
  }
private:
  int _index;
  getter_t _getter;
  setter_t _setter;
};


/**
 * A value managed through an object and access methods.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class C, class T>
class SGRawValueMethods : public SGRawValue<T>
{
public:
  typedef T (C::*getter_t)() const;
  typedef void (C::*setter_t)(T);
  SGRawValueMethods (C &obj, getter_t getter = 0, setter_t setter = 0)
    : _obj(obj), _getter(getter), _setter(setter) {}
  virtual ~SGRawValueMethods () {}
  virtual T getValue () const {
    if (_getter) { return (_obj.*_getter)(); }
    else { return SGRawValue<T>::DefaultValue; }
  }
  virtual bool setValue (T value) {
    if (_setter) { (_obj.*_setter)(value); return true; }
    else return false;
  }
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueMethods<C,T>(_obj, _getter, _setter);
  }
private:
  C &_obj;
  getter_t _getter;
  setter_t _setter;
};


/**
 * An indexed value managed through an object and access methods.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class C, class T>
class SGRawValueMethodsIndexed : public SGRawValue<T>
{
public:
  typedef T (C::*getter_t)(int) const;
  typedef void (C::*setter_t)(int, T);
  SGRawValueMethodsIndexed (C &obj, int index,
		     getter_t getter = 0, setter_t setter = 0)
    : _obj(obj), _index(index), _getter(getter), _setter(setter) {}
  virtual ~SGRawValueMethodsIndexed () {}
  virtual T getValue () const {
    if (_getter) { return (_obj.*_getter)(_index); }
    else { return SGRawValue<T>::DefaultValue; }
  }
  virtual bool setValue (T value) {
    if (_setter) { (_obj.*_setter)(_index, value); return true; }
    else return false;
  }
  virtual SGRawValue<T> * clone () const {
    return new SGRawValueMethodsIndexed<C,T>(_obj, _index, _getter, _setter);
  }
private:
  C &_obj;
  int _index;
  getter_t _getter;
  setter_t _setter;
};



/**
 * A node in a property tree.
 */
class SGPropertyNode
{

public:


  /**
   * Property value types.
   */
  enum Type {
    NONE,
    ALIAS,
    BOOL,
    INT,
    LONG,
    FLOAT,
    DOUBLE,
    STRING,
    UNSPECIFIED
  };


  /**
   * Access mode attributes.
   *
   * <p>The ARCHIVE attribute is strictly advisory, and controls
   * whether the property should normally be saved and restored.</p>
   */
  enum Attribute {
    READ = 1,
    WRITE = 2,
    ARCHIVE = 4
  };


  /**
   * Default constructor.
   */
  SGPropertyNode ();


  /**
   * Copy constructor.
   */
  SGPropertyNode (const SGPropertyNode &node);


  /**
   * Destructor.
   */
  virtual ~SGPropertyNode ();



  //
  // Basic properties.
  //

  /**
   * Test whether this node contains a primitive leaf value.
   */
  bool hasValue () const { return (_type != NONE); }


  /**
   * Get the node's simple (XML) name.
   */
  const string &getName () const { return _name; }


  /**
   * Get the node's integer index.
   */
  const int getIndex () const { return _index; }


  /**
   * Get a non-const pointer to the node's parent.
   */
  SGPropertyNode * getParent () { return _parent; }


  /**
   * Get a const pointer to the node's parent.
   */
  const SGPropertyNode * getParent () const { return _parent; }


  //
  // Children.
  //


  /**
   * Get the number of child nodes.
   */
  const int nChildren () const { return _children.size(); }


  /**
   * Get a child node by position (*NOT* index).
   */
  SGPropertyNode * getChild (int position);


  /**
   * Get a const child node by position (*NOT* index).
   */
  const SGPropertyNode * getChild (int position) const;


  /**
   * Get a child node by name and index.
   */
  SGPropertyNode * getChild (const string &name, int index = 0,
			     bool create = false);


  /**
   * Get a const child node by name and index.
   */
  const SGPropertyNode * getChild (const string &name, int index = 0) const;


  /**
   * Get a vector of all children with the specified name.
   */
  vector<SGPropertyNode *> getChildren (const string &name);


  /**
   * Get a vector all all children (const) with the specified name.
   */
  vector<const SGPropertyNode *> getChildren (const string &name) const;


  //
  // Alias support.
  //


  /**
   * Alias this node's leaf value to another's.
   */
  bool alias (SGPropertyNode * target);


  /**
   * Alias this node's leaf value to another's by relative path.
   */
  bool alias (const string &path);


  /**
   * Remove any alias for this node.
   */
  bool unalias ();


  /**
   * Test whether the node's leaf value is aliased to another's.
   */
  bool isAlias () const { return (_type == ALIAS); }


  /**
   * Get a non-const pointer to the current alias target, if any.
   */
  SGPropertyNode * getAliasTarget ();


  /**
   * Get a const pointer to the current alias target, if any.
   */
  const SGPropertyNode * getAliasTarget () const;


  //
  // Path information.
  //


  /**
   * Get the path to this node from the root.
   */
  string getPath (bool simplify = false) const;


  /**
   * Get a pointer to the root node.
   */
  SGPropertyNode * getRootNode ();


  /**
   * Get a const pointer to the root node.
   */
  const SGPropertyNode * getRootNode () const;


  /**
   * Get a pointer to another node by relative path.
   */
  SGPropertyNode * getNode (const string &relative_path, bool create = false);


  /**
   * Get a const pointer to another node by relative path.
   */
  const SGPropertyNode * getNode (const string &relative_path) const;


  //
  // Access Mode.
  //

  /**
   * Check a single mode attribute for the property node.
   */
  bool getAttribute (Attribute attr) const { return (_attr & attr); }


  /**
   * Set a single mode attribute for the property node.
   */
  void setAttribute (Attribute attr, bool state) {
    (state ? _attr |= attr : _attr &= ~attr);
  }


  /**
   * Get all of the mode attributes for the property node.
   */
  int getAttributes () const { return _attr; }


  /**
   * Set all of the mode attributes for the property node.
   */
  void setAttributes (int attr) { _attr = attr; }
  

  //
  // Leaf Value (primitive).
  //


  /**
   * Get the type of leaf value, if any, for this node.
   */
  Type getType () const;


  /**
   * Get a bool value for this node.
   */
  bool getBoolValue () const;


  /**
   * Get an int value for this node.
   */
  int getIntValue () const;


  /**
   * Get a long int value for this node.
   */
  long getLongValue () const;


  /**
   * Get a float value for this node.
   */
  float getFloatValue () const;


  /**
   * Get a double value for this node.
   */
  double getDoubleValue () const;


  /**
   * Get a string value for this node.
   */
  string getStringValue () const;



  /**
   * Set a bool value for this node.
   */
  bool setBoolValue (bool value);


  /**
   * Set an int value for this node.
   */
  bool setIntValue (int value);


  /**
   * Set a long int value for this node.
   */
  bool setLongValue (long value);


  /**
   * Set a float value for this node.
   */
  bool setFloatValue (float value);


  /**
   * Set a double value for this node.
   */
  bool setDoubleValue (double value);


  /**
   * Set a string value for this node.
   */
  bool setStringValue (string value);


  /**
   * Set a value of unspecified type for this node.
   */
  bool setUnspecifiedValue (string value);


  //
  // Data binding.
  //


  /**
   * Test whether this node is bound to an external data source.
   */
  bool isTied () const { return _tied; }


  /**
   * Bind this node to an external bool source.
   */
  bool tie (const SGRawValue<bool> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external int source.
   */
  bool tie (const SGRawValue<int> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external long int source.
   */
  bool tie (const SGRawValue<long> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external float source.
   */
  bool tie (const SGRawValue<float> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external double source.
   */
  bool tie (const SGRawValue<double> &rawValue, bool useDefault = true);


  /**
   * Bind this node to an external string source.
   */
  bool tie (const SGRawValue<string> &rawValue, bool useDefault = true);


  /**
   * Unbind this node from any external data source.
   */
  bool untie ();


  //
  // Convenience methods using paths.
  // TODO: add attribute methods
  //


  /**
   * Get another node's type.
   */
  Type getType (const string &relative_path) const;


  /**
   * Test whether another node has a leaf value.
   */
  bool hasValue (const string &relative_path) const;


  /**
   * Get another node's value as a bool.
   */
  bool getBoolValue (const string &relative_path,
		     bool defaultValue = false) const;


  /**
   * Get another node's value as an int.
   */
  int getIntValue (const string &relative_path,
		   int defaultValue = 0) const;


  /**
   * Get another node's value as a long int.
   */
  long getLongValue (const string &relative_path,
		     long defaultValue = 0L) const;


  /**
   * Get another node's value as a float.
   */
  float getFloatValue (const string &relative_path,
		       float defaultValue = 0.0) const;


  /**
   * Get another node's value as a double.
   */
  double getDoubleValue (const string &relative_path,
			 double defaultValue = 0.0L) const;


  /**
   * Get another node's value as a string.
   */
  string getStringValue (const string &relative_path,
			 string defaultValue = "") const;


  /**
   * Set another node's value as a bool.
   */
  bool setBoolValue (const string &relative_path, bool value);


  /**
   * Set another node's value as an int.
   */
  bool setIntValue (const string &relative_path, int value);


  /**
   * Set another node's value as a long int.
   */
  bool setLongValue (const string &relative_path, long value);


  /**
   * Set another node's value as a float.
   */
  bool setFloatValue (const string &relative_path, float value);


  /**
   * Set another node's value as a double.
   */
  bool setDoubleValue (const string &relative_path, double value);


  /**
   * Set another node's value as a string.
   */
  bool setStringValue (const string &relative_path, string value);


  /**
   * Set another node's value with no specified type.
   */
  bool setUnspecifiedValue (const string &relative_path, string value);


  /**
   * Test whether another node is bound to an external data source.
   */
  bool isTied (const string &relative_path) const;


  /**
   * Bind another node to an external bool source.
   */
  bool tie (const string &relative_path, const SGRawValue<bool> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external int source.
   */
  bool tie (const string &relative_path, const SGRawValue<int> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external long int source.
   */
  bool tie (const string &relative_path, const SGRawValue<long> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external float source.
   */
  bool tie (const string &relative_path, const SGRawValue<float> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external double source.
   */
  bool tie (const string &relative_path, const SGRawValue<double> &rawValue,
	    bool useDefault = true);


  /**
   * Bind another node to an external string source.
   */
  bool tie (const string &relative_path, const SGRawValue<string> &rawValue,
	    bool useDefault = true);


  /**
   * Unbind another node from any external data source.
   */
  bool untie (const string &relative_path);


protected:


  /**
   * Protected constructor for making new nodes on demand.
   */
  SGPropertyNode (const string &name, int index, SGPropertyNode * parent);


private:


  /**
   * Clear any existing value and set the type to NONE.
   */
  void clear_value ();

  string _name;
  int _index;
  SGPropertyNode * _parent;
  vector<SGPropertyNode *> _children;


  Type _type;
  bool _tied;
  int _attr;

				// The right kind of pointer...
  union {
    SGPropertyNode * alias;
    SGRawValue<bool> * bool_val;
    SGRawValue<int> * int_val;
    SGRawValue<long> * long_val;
    SGRawValue<float> * float_val;
    SGRawValue<double> * double_val;
    SGRawValue<string> * string_val;
  } _value;


};



////////////////////////////////////////////////////////////////////////
// I/O functions.
////////////////////////////////////////////////////////////////////////


/**
 * Read properties from an XML input stream.
 */
bool readProperties (istream &input, SGPropertyNode * start_node,
		     const string &base = "");


/**
 * Read properties from an XML file.
 */
bool readProperties (const string &file, SGPropertyNode * start_node);


/**
 * Write properties to an XML output stream.
 */
bool writeProperties (ostream &output, const SGPropertyNode * start_node);


/**
 * Write properties to an XML file.
 */
bool writeProperties (const string &file, const SGPropertyNode * start_node);


/**
 * Copy properties from one node to another.
 */
bool copyProperties (const SGPropertyNode *in, SGPropertyNode *out);


#endif // __PROPS_HXX

// end of props.hxx
