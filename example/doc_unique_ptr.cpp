//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/config_begin.hpp>
#include <cassert>

//[unique_ptr_basic_example
#include <boost/move/unique_ptr.hpp>
#include <boost/move/make_unique.hpp>

namespace bml = ::boost::movelib;

class widget
{
   public:
   explicit widget(int v) : value(v) {}
   int value;
};

//A factory: the caller owns the returned object
bml::unique_ptr<widget> make_widget(int v)
{
   return bml::make_unique<widget>(v);
}

//A sink: the function takes the ownership of the object
int consume_widget(bml::unique_ptr<widget> w)
{
   return w->value;
}  //The widget is deleted here

void basic_example()
{
   bml::unique_ptr<widget> p = make_widget(1);  //p owns a widget
   assert(p && p->value == 1);

   bml::unique_ptr<widget> q(boost::move(p));   //The ownership goes from p to q
   assert(!p && q->value == 1);

   q.reset(new widget(2));                      //Deletes the first widget
   assert(q->value == 2);

   widget *raw = q.release();                   //q does not own the widget now
   assert(!q);
   delete raw;

   q = make_widget(3);
   assert(consume_widget(boost::move(q)) == 3); //The sink owns and deletes it
   assert(!q);
}
//]

//[unique_ptr_array_example
void array_example()
{
   //new[] is paired with delete[]
   bml::unique_ptr<int[]> a(new int[3]);
   a[0] = 1;
   a[1] = 2;
   a[2] = 3;
   assert(a[1] == 2);

   //Value-initialized elements: all are zero
   bml::unique_ptr<int[]> z = bml::make_unique<int[]>(10);
   assert(z[0] == 0 && z[9] == 0);

   //Default-initialized elements: the values are not set
   bml::unique_ptr<int[]> d = bml::make_unique_definit<int[]>(10);
   d[0] = 1;
   assert(d[0] == 1);
}
//]

//[unique_ptr_deleter_example
//A C-style interface that creates and destroys a resource
struct resource { int handle; };
int destroyed_resources = 0;

resource *open_resource(int h)
{
   resource *r = new resource;
   r->handle = h;
   return r;
}

void close_resource(resource *r)
{
   ++destroyed_resources;
   delete r;
}

//A deleter without state: it adds no size to unique_ptr
struct resource_closer
{
   void operator()(resource *r) const {  close_resource(r);  }
};

void deleter_example()
{
   {
      bml::unique_ptr<resource, resource_closer> r(open_resource(1));
      assert(r->handle == 1);
      //The empty deleter uses no storage
      assert(sizeof(r) == sizeof(resource*));
   }  //close_resource is called here
   assert(destroyed_resources == 1);

   {
      //A function pointer as deleter: it is stored in the unique_ptr
      bml::unique_ptr<resource, void(*)(resource*)> r(open_resource(2), &close_resource);
      assert(r.get_deleter() == &close_resource);
   }
   assert(destroyed_resources == 2);

   {
      //A reference to a deleter: the unique_ptr uses the deleter object of the caller
      resource_closer closer;
      bml::unique_ptr<resource, resource_closer&> r(open_resource(3), closer);
      assert(&r.get_deleter() == &closer);
   }
   assert(destroyed_resources == 3);
}
//]

//[unique_ptr_conversion_example
struct base
{
   virtual ~base() {}
   virtual int id() const { return 0; }
};

struct derived : base
{
   virtual int id() const { return 1; }
};

void conversion_example()
{
   bml::unique_ptr<derived> d = bml::make_unique<derived>();
   //A unique_ptr to a derived class converts to a unique_ptr to a base class
   bml::unique_ptr<base> b(boost::move(d));
   assert(!d && b->id() == 1);

   //A unique_ptr to an array converts to a unique_ptr to an array of more cv-qualified elements
   bml::unique_ptr<int[]> a(new int[2]);
   bml::unique_ptr<const int[]> ca(boost::move(a));
   assert(!a && ca);
}
//]

//[unique_ptr_container_example
#include <boost/container/vector.hpp>

void container_example()
{
   boost::container::vector< bml::unique_ptr<widget> > v;
   v.push_back(bml::make_unique<widget>(1));     //A temporary is moved
   bml::unique_ptr<widget> w = bml::make_unique<widget>(2);
   v.push_back(boost::move(w));                  //A named object must be moved explicitly
   assert(v.size() == 2 && !w);
   assert(v[0]->value == 1 && v[1]->value == 2);
}  //The vector deletes the widgets
//]

int main()
{
   basic_example();
   array_example();
   deleter_example();
   conversion_example();
   container_example();
   return 0;
}

#include <boost/move/detail/config_end.hpp>
