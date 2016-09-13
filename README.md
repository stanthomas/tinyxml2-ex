# tinyxml2-extension

A set of add-on classes and helper functions bringing basic XPath element selection and C++11/14 features such as iterators, strings and exceptions to tinyxml2.

Allows very concise C++ code to access a selection of elements in an XML document.
E.g. given an XML document as a string:
```
auto xml = R"-(<?xml version="1.0" encoding="utf-8"?>
<panagram><part>The quick brown fox </part><part>jumps over the lazy dog.</part></panagram>)-"
```
then
```
auto doc = tinyxml2::load_document (xml);
for (auto part : selection (*doc, "panagram/part"))
   cout << text (part);
```
outputs:
```
The quick brown fox jumps over the lazy dog.
```

## Description
tinyxml2-ex is a header only add-on for tinyxml2; simply include "tixml2ex.h" in your source.
It comprises a number of add-on classes and helper functions to provide iterators, string access and exceptions.
It uses only the public interfaces of TinyXML2 and is completely interoperable so you can mix 'n match extension calls and raw TinyXML2 as required.
It is written to C++14 and should work with any standards conforming compiler. It has been tested with Visual C++ (2015) and GCC (4.9.2).

tinyxml2-ex has its own namespace, tixml2ex, rather cheekily injected into the tinyxml2 namespace.
By virtue of ADL, in many cases there is no need to qualify the namespace.

Elements can be found, either directly or via an iterator, using a subset of XPath syntax which
supports matching elements along a path by element name (type) and attribute name and value.
Currently, attribute values do not fully conform to XPath in that they are not required to be enclosed in quotes;
single quotes may be used but are ignored and as a consequence attribute values containing any of the
special characters `@ [ ] = '` cannot be matched.

See USAGE.md and Wiki page for information on using tinyxml2-ex.

### Background
TinyXML {http://www.grinninglizard.com/tinyxml} is an easy to use, small and efficient XML parser for C++.
I've used it, or rather TinyXML++, aka TiCPP {https://github.com/rjpcomputing/ticpp}, for some years.
TiCPP is a wrapper for TinyXML that adds familiar C++ features, including a rather novel interpretation of iterators.

TinyXML has been superceeded by TinyXML2 {http://www.grinninglizard.com/tinyxml2/index.html} which is smaller, faster and the focus of current development. However, TinyXML2 eschews the STL and several aspects of modern C++, in the interests, presumably, of the widest possible application.

The purpose of this project, tinyxml2-extension, is to bring TiCPP functionality to TinyXML2 but go further *and* conform to modern C++ style while adding little or no overhead. In keeping with the philosophy of TinyXML2, the extension adds enough functionality to be useful without attempting to implement every option - there are other, much larger and more complex XML libraries available for this.

The initial release focusses on reading and processing XML data generated elsewhere (by TiCPP in my case).
It will progress to updating and generating XML as I move to TinyXML2 across my applications.
And stream i/o (since I often want to get/put XML via a socket).

