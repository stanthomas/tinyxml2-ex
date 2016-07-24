/*
tinyxml2ex - a set of add-on classes and helper functions bringing C++11/14 features, such as iterators, strings and exceptions, to tinyxml2


Copyright (c) 2016 Stan Thomas

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.


tinyxml2 is the work of Lee Thomason (www.grinninglizard.com) and others.
It can be found here: https://github.com/leethomason/tinyxml2 and has it's own licensing terms.

*/

#pragma once

#include <string>
#include <list>
#include <memory>
#include <exception>
#include <cassert>

#include <tinyxml2.h>


namespace tinyxml2
{
	inline namespace tixml2ex
	{
		using namespace std::literals::string_literals;

		class XmlException : public std::logic_error
		{
		public:
			XmlException (const std::string & description) : logic_error (description) {}
		};


		class AttributeFilter
		{
		public:
			AttributeFilter (std::string name, std::string value) : _name(name), _value(value) {}
			std::string Name() const { return _name; }
			std::string Value() const { return _value; }
		private:
			std::string _name;
			std::string _value;
		};	// AttributeFilter


		class ElementFilter
		{
		public:
			ElementFilter (std::string nodeTest)
			{
				// parse nodeTest for element name and attribute filters
				std::string attributeName, attributeValue;
				for (auto c : nodeTest)
				{
					if (c == '[')
					{
						if (_mode != Mode::elementName)
							throw XmlException ("ill formed path"s);
						_mode = Mode::attributeFilter;
					}
					else if (c == ']')
					{
						if (! (_mode == Mode::attributeFilter || _mode == Mode::attributeName || _mode == Mode::attributeAssignment  || _mode == Mode::attributeValue))
							throw XmlException ("ill formed path"s);
						if (_mode == Mode::attributeName || _mode == Mode::attributeAssignment || _mode == Mode::attributeValue)
						{
							_attributes .emplace_back (AttributeFilter (attributeName, attributeValue));
							attributeName .erase();
							attributeValue .erase();
						}
						_mode = Mode::elementName;
					}
					else if (c == '@')
					{
						if (_mode != Mode::attributeFilter)
							throw XmlException ("ill formed path"s);
						_mode = Mode::attributeName;
					}
					else if (c == '=')
					{
						if (_mode != Mode::attributeName)
							throw XmlException ("ill formed path"s);
						_mode = Mode::attributeAssignment;
					}
					else if (c == '\'')
					{
						if (!(_mode == Mode::attributeAssignment || _mode == Mode::attributeValue))
							throw XmlException ("ill formed path"s);
						// XPath attribute values are wrapped in single quote marks
						// but we don't require them and ignore them if present
						// toggle between two effectively equivalent modes
						_mode = _mode == Mode::attributeAssignment ? Mode::attributeValue : Mode::attributeAssignment;
					}
					else
					{
						switch (_mode)
						{
						case Mode::elementName:
							_name += c;
							break;
						case Mode::attributeName:
							attributeName += c;
							break;
						case Mode::attributeAssignment:
						case Mode::attributeValue:
							attributeValue += c;
							break;
						case Mode::attributeFilter:
							/*skip*/;
						}
					}
				}

			}
			ElementFilter() {}	// an empty filter
			std::string Name() const { return _name; }
			bool Match (const XMLElement * element) const
			{
				// n.b. we only match attributes here, not the element name (type)
				for (auto const & attr : _attributes)
				{
					if (!element -> Attribute (attr .Name() .c_str(), !attr .Value() .empty() ? attr .Value() .c_str() : nullptr))
						return false;	// attribute not matched
				}
				return true;
			}
		private:
			std::string _name;
			std::list <AttributeFilter> _attributes;
			enum class Mode { elementName, attributeFilter, attributeName, attributeAssignment, attributeValue } _mode {Mode::elementName};
		};	// ElementFilter


		class ElementIterator
		{
		public:
			using selection = std::list<std::pair<ElementFilter, const XMLElement *>>;

		public:
			ElementIterator() { _selection .emplace_back (std::make_pair (ElementFilter(), nullptr));  }
			ElementIterator (const XMLElement * e) { _selection .emplace_back (std::make_pair (ElementFilter(), e)); }
			ElementIterator (selection s)
				: _selection(s)
			{
				if (_selection .empty())
					throw XmlException ("selection path is empty - logic error");

				// _selection must have at least one element which is the origin of all branches considered
				// only children of the origin are considered
				// on construction the origin element must be a valid XMLElement
				// elements in the remainder of the path are initially null and will be populated with the current branch
				// as iterations progress

				// descend first matching branch (if any)
				descend (_selection .begin());
				// remove the origin from the list so that iteration is only over child elements
				_selection .pop_front();
			}

			const XMLElement * operator *() const { return _selection .back() .second; }
			bool operator != (const ElementIterator & iter) const { return *iter != _selection .back() .second; }
			ElementIterator & operator ++()
			{
				// to get here we must have found at least one matching element
				// selection branch contains the complete element path
#if !defined (NDEBUG)
				for (auto const & pe : _selection)
					assert (pe .second);
#endif
				// start at the bottom with the current element
				traverse (--_selection .end());
				return *this;
			}

		private:
			bool descend (selection::iterator ixSel)
			{
				// recursively descend selection branch of matching elements
				if (!ixSel -> second)
					return false;
				auto parentElement = ixSel -> second;

				if (++ixSel == _selection .end())
					return true;	// we've found the first matching element

				ixSel -> second = parentElement -> FirstChildElement (ixSel -> first .Name() .empty() ? nullptr : ixSel -> first .Name() .c_str());
				while (ixSel -> second)
				{
					if (ixSel -> first .Match (ixSel -> second))
					{
						if (descend (ixSel))
							return true;
					}
					// move sideways
					ixSel -> second = ixSel -> second -> NextSiblingElement (ixSel -> first .Name() .empty() ? nullptr : ixSel -> first .Name() .c_str());
				}
				return false;	// no matching elements at this depth
			}

			void traverse (selection::iterator ixSel)
			{
				// to find next element we can go sideways or up and then down
				// traverse() does the moves across the xml tree, descend() then explores each potential new branch
				// note that this method can only be called once the selection has been initialised
				while (ixSel -> second = ixSel -> second -> NextSiblingElement (ixSel -> first .Name() .empty() ? nullptr : ixSel -> first .Name() .c_str()))
				{
					if (ixSel -> first .Match (ixSel -> second))
					{
						if (descend (ixSel))
							return;
					}
				}
				// no siblings or sibling branches match, go up a level (unless already at origin)
				if (ixSel != _selection .begin())
					traverse (--ixSel);
			}

		private:
			selection _selection;
		};	// ElementIterator


		inline ElementIterator begin (XMLElement * parent)
		{
			if (!parent)
				throw XmlException ("null element"s);
			return ElementIterator (parent -> FirstChildElement());
		}


		inline ElementIterator end (XMLElement *)
		{
			return ElementIterator();
		}


		inline ElementIterator begin (const XMLElement * parent)
		{
			if (!parent)
				throw XmlException ("null element"s);
			return ElementIterator (parent -> FirstChildElement());
		}


		inline ElementIterator end (const XMLElement *)
		{
			return ElementIterator();
		}


		class Selector
		{
			// select child elements along XPath-style path for iteration

		public:
			Selector (const XMLElement * root, std::string path)
			{
				//	parse the path into a selection branch

				if (!root)
					throw XmlException ("null element"s);

				// split the path
				size_t start = 0;
				size_t pos;
				// set element at head of selection branch
				//	if path starts with '/' then it is relative to document, otherwise relative to element passed in
				// first element in selection branch is the root and only children of the root are considered
				// for document-based paths, this works because there can only be one document element
				if (!path.empty() && path[0] == '/')
				{
					// document is not an element so needs special handling
					// advance to the actual document element
					// note that document element must still appear in path, so we have to step over it
					++start;
					if ((pos = path .find ('/', start)) != std::string::npos)
					{
						auto filter = ElementFilter (path .substr (start, pos - start));
						auto element = root -> GetDocument() -> FirstChildElement (filter .Name() != "*"s ? filter .Name() .c_str() : nullptr);
						_branch .emplace_back (std::make_pair (filter, element));
						start = pos + 1;
					}
				}
				else
					_branch .emplace_back (std::make_pair (ElementFilter (root -> Name()), root));

				// continue with other elements along path
				while ((pos = path .find ('/', start)) != std::string::npos)
				{
					_branch .emplace_back (std::make_pair (ElementFilter (path .substr (start, pos - start)), nullptr));
					start = pos + 1;
				}
				// and the final element
				_branch .emplace_back (std::make_pair (ElementFilter (path .substr (start, pos - start)), nullptr));
			}

			Selector (const XMLDocument & doc, std::string path) : Selector (doc .FirstChildElement(), (!path.empty() && path[0] == '/') ? path : '/' + path) {}

			ElementIterator begin()
			{
				if (!_branch .empty() && _branch .front() .second)
					return ElementIterator (_branch);
				else
					return end();
			};

			ElementIterator end()
			{
				return ElementIterator();	// an empty iterator that will return a null XMLElement
			};

		private:
			ElementIterator::selection _branch;
		};	// Selector


		inline const XMLElement * find_element (const XMLElement * root, const std::string & path)
		{
			return *Selector (root, path) .begin();
		}


		inline const XMLElement * find_element (const XMLDocument & doc, const std::string & path)
		{
			return find_element (doc .FirstChildElement(), (!path.empty() && path[0] == '/') ? path : '/' + path);
		}


		inline std::unique_ptr <XMLDocument> load_document (const std::string & xmlString)
		{
			auto doc = std:: make_unique <XMLDocument>();
			if (doc -> Parse (xmlString .c_str()) != XML_SUCCESS)
				throw XmlException ("error in XML"s);
			return doc;
		}


		inline const XMLElement * first_child_element (const XMLNode * parent, const std::string & name)
		{
			if (auto e = parent -> FirstChildElement (name .c_str()))
				return e;
			else
				throw XmlException ("unknown element"s);
		}


		inline std::string attribute_value (const XMLElement * element, const std::string & name, bool throwIfUnknown = false)
		{
			if (!element)
				throw XmlException ("null element"s);

			if (name .empty())
				throw XmlException ("missing attribute name"s);

			if (auto value = element -> Attribute (name .c_str()))
				return std::string (value);

			if (!throwIfUnknown)
				return ""s;
			else
				throw XmlException ("attribute not present"s);
		}


		inline std::string text (const XMLElement * element)
		{
			if (!element)
				throw XmlException ("null element"s);

			if (auto value = element -> GetText())
				return std::string (value);
			else
				return ""s;
		}
	}
}
