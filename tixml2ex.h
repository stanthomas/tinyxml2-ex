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

#include "tinyxml2.h"


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


		class AttributeNameValue
		{
		public:
			AttributeNameValue (std::string name, std::string value) : _name(name), _value(value) {}
			std::string Name() const { return _name; }
			std::string Value() const { return _value; }
		private:
			std::string _name;
			std::string _value;
		};	// AttributeNameValue


		using AttributeList = std::list <AttributeNameValue>;


		class ElementProperties
		{
		public:
			ElementProperties (std::string xProps)
			{
				// parse xProps for element name and attribute filters using simplified XPath syntax
				std::string attributeName, attributeValue;
				for (auto c : xProps)
				{
					if (c == '[')
					{
						if (_state != ParseState::elementName)
							throw XmlException ("ill formed XPath"s);
						_state = ParseState::attributeFilter;
					}
					else if (c == ']')
					{
						if (! (_state == ParseState::attributeFilter || _state == ParseState::attributeName || _state == ParseState::attributeAssignment  || _state == ParseState::attributeValue))
							throw XmlException ("ill formed XPath"s);
						if (_state == ParseState::attributeName || _state == ParseState::attributeAssignment || _state == ParseState::attributeValue)
						{
							_attributes .emplace_back (AttributeNameValue (attributeName, attributeValue));
							attributeName .erase();
							attributeValue .erase();
						}
						_state = ParseState::elementName;
					}
					else if (c == '@')
					{
						if (_state != ParseState::attributeFilter)
							throw XmlException ("ill formed XPath"s);
						_state = ParseState::attributeName;
					}
					else if (c == '=')
					{
						if (_state != ParseState::attributeName)
							throw XmlException ("ill formed XPath"s);
						_state = ParseState::attributeAssignment;
					}
					else if (c == '\'')
					{
						if (!(_state == ParseState::attributeAssignment || _state == ParseState::attributeValue))
							throw XmlException ("ill formed XPath"s);
						// XPath attribute values are wrapped in single quote marks
						// but we don't require them and ignore them if present
						// toggle between two effectively equivalent modes
						_state = _state == ParseState::attributeAssignment ? ParseState::attributeValue : ParseState::attributeAssignment;
					}
					else
					{
						switch (_state)
						{
						case ParseState::elementName:
							_name += c;
							break;
						case ParseState::attributeName:
							attributeName += c;
							break;
						case ParseState::attributeAssignment:
						case ParseState::attributeValue:
							attributeValue += c;
							break;
						case ParseState::attributeFilter:
							/*skip*/;
						}
					}
				}

			}
			ElementProperties() {}	// an empty property set
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
			void Update(XMLElement * element) const
			{
				for (auto const & attr : _attributes)
					element -> SetAttribute (attr .Name() .c_str(), attr .Value() .c_str());
			}
		private:
			std::string _name;
			AttributeList _attributes;
			enum class ParseState { elementName, attributeFilter, attributeName, attributeAssignment, attributeValue } _state {ParseState::elementName};
		};	// ElementProperties


		template <typename XE>
		class ElementPath : public std::list<std::pair<ElementProperties, XE *>>
		{
		public:
			ElementPath (XE * root, std::string xpath)
			{
				if (!root)
					throw XmlException ("null element"s);

				// split the path
				size_t start = 0;
				size_t pos;
				// set element at head of selection branch
				//	if path starts with '/' then it is relative to document, otherwise relative to element passed in
				// first element in selection branch is the root and only children of the root are considered
				// for document-based paths, this works because there can only be one document element
				if (!xpath.empty() && xpath[0] == '/')
				{
					// document is not an element so needs special handling
					// advance to the actual document element
					// note that document element must still appear in path, so we have to step over it
					++start;
					if ((pos = xpath .find ('/', start)) != std::string::npos)
					{
						auto filter = ElementProperties (xpath .substr (start, pos - start));
						auto element = root -> GetDocument() -> RootElement();
						if (element && !filter .Name() .empty())
						{
							if (filter .Name() != std::string (element -> Name()))
								throw XmlException ("document element name mismatch"s);
						}
						this -> emplace_back (std::make_pair (filter, element));
						start = pos + 1;
					}
				}
				else
					this -> emplace_back (std::make_pair (ElementProperties (root -> Name()), root));

				// continue with other elements along path
				while ((pos = xpath .find ('/', start)) != std::string::npos)
				{
					this -> emplace_back (std::make_pair (ElementProperties (xpath .substr (start, pos - start)), nullptr));
					start = pos + 1;
				}
				// and the final element
				this -> emplace_back (std::make_pair (ElementProperties (xpath .substr (start, pos - start)), nullptr));
			}
			ElementPath (XE * e)
			{
				this -> emplace_back (std::make_pair (ElementProperties(), e));	// e can be null
			}
		};


		template <typename XE>
		class ElementIterator
		{
		public:
			// iterator_traits
			using iterator_category = std::input_iterator_tag;
			using value_type = XE;
			using difference_type = std::ptrdiff_t;
			using pointer = XE *;
			using reference = XE &;


		public:
			ElementIterator() : _selection (nullptr) {}
			ElementIterator (XE * e) : _selection (e) {}
			ElementIterator (ElementPath<XE> s)
				: _selection(s)
			{
				if (_selection .empty())
					throw XmlException ("selection xpath is empty - logic error");

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

			XE * operator *() const { return _selection .back() .second; }
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
			using path_iterator = typename ElementPath<XE>::iterator;

			bool descend (path_iterator ixSel)
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

			void traverse (path_iterator ixSel)
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
			ElementPath<XE> _selection;
		};	// ElementIterator


		inline ElementIterator<XMLElement> begin (XMLElement * parent)
		{
			if (!parent)
				throw XmlException ("null element"s);
			return ElementIterator<XMLElement> (parent -> FirstChildElement());
		}


		inline ElementIterator<XMLElement> end (XMLElement *)
		{
			return ElementIterator<XMLElement>();
		}


		inline ElementIterator<const XMLElement> begin (const XMLElement * parent)
		{
			if (!parent)
				throw XmlException ("null element"s);
			return ElementIterator<const XMLElement> (parent -> FirstChildElement());
		}


		inline ElementIterator<const XMLElement> end (const XMLElement *)
		{
			return ElementIterator<const XMLElement>();
		}


		inline ElementIterator<const XMLElement> cbegin (const XMLElement * parent)
		{
			return begin (parent);
		}


		inline ElementIterator<const XMLElement> cend (const XMLElement * e)
		{
			return end (e);
		}


		template <typename XE>
		class Selector
		{
			// select child elements along XPath-style path for iteration
		public:
			Selector (XE * base, const std::string & xpath) : _branch (base, xpath) {}

			ElementIterator<XE> begin() const
			{
				if (!_branch .empty() && _branch .front() .second)
					return ElementIterator<XE> (_branch);
				else
					return end();
			};

			ElementIterator<XE> end() const
			{
				return ElementIterator<XE>();	// an empty iterator that will return a null XMLElement
			};

		private:
			ElementPath<XE> _branch;	// todo: passed to iterator by value, can it be passed by reference? or should it be heap allocated and passed as unique_ptr ?
		};	// Selector


		// helper functions to return appropriate const / non-const Selector
		inline Selector<XMLElement> selection (XMLElement * base, const std::string & xpath)
		{
			return Selector<XMLElement> (base, xpath);
		}

		inline Selector<const XMLElement> selection (const XMLElement * base, const std::string & xpath)
		{
			return Selector<const XMLElement> (base, xpath);
		}

		inline Selector<XMLElement> selection (XMLDocument & doc, const std::string & xpath)
		{
			return Selector<XMLElement> (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}

		inline Selector<const XMLElement> selection (const XMLDocument & doc, const std::string & xpath)
		{
			return Selector<const XMLElement> (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}


		// helper functions to find the first element (if any) below a base element matching the XPath
		inline XMLElement * find_element (XMLElement * base, const std::string & xpath = ""s)
		{
			return *Selector<XMLElement> (base, xpath) .begin();
		}


		inline const XMLElement * find_element (const XMLElement * base, const std::string & xpath = ""s)
		{
			return *Selector<const XMLElement> (base, xpath) .begin();
		}


		inline XMLElement * find_element (XMLDocument & doc, const std::string & xpath = ""s)
		{
			return find_element (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}

		inline const XMLElement * find_element (const XMLDocument & doc, const std::string & xpath = ""s)
		{
			return find_element (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}


		// load XML document from string buffer
		inline std::unique_ptr <XMLDocument> load_document (const std::string & xmlString)
		{
			auto doc = std:: make_unique <XMLDocument>();
			if (doc -> Parse (xmlString .c_str()) != XML_SUCCESS)
				throw XmlException ("error in XML"s);
			return doc;
		}


		// find the first child element of given element (if any) with (option) element type name
		// todo: this is possibly redundant - use find_element()
		inline const XMLElement * first_child_element (const XMLNode * parent, const std::string & name = ""s)
		{
			if (!parent)
				throw XmlException ("null element"s);

			return parent -> FirstChildElement (!name .empty() ? name .c_str() : nullptr);
		}


		// helper function to get atribute value as a string, blank if attribute missing
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


		// helper function to get element text as a string, blank if none
		inline std::string text (const XMLElement * element)
		{
			if (!element)
				throw XmlException ("null element"s);

			if (auto value = element -> GetText())
				return std::string (value);
			else
				return ""s;
		}


		// append / prepend element
		// common method for all append / prepend element insertions
		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, AttributeList attributes, const std::string & text, bool addAtBack)
		{
			XMLElement * element {nullptr};
			bool inserted {false};

			ElementPath<XMLElement> branch {parent, xpath};
			// add all the elements to create new branch
			// first element in branch is the parent, so skip
			for (auto be = ++branch .begin(); be != branch .end(); ++be)
			{
				element = parent -> GetDocument() -> NewElement (be -> first .Name() .c_str());
				if (!element)
					break;
				// and set element attributes from XPath data
				be -> first .Update (element);
				be -> second = element;
				// insert new element into hierarchy
				auto last = parent -> LastChildElement();
				if (addAtBack && last)
					// XMLElement::InsertEndChild puts new element after *all* nodes, including text, which looks odd
					// therefore, add new element immediately after current last element when present, otherwise first
						inserted = parent -> InsertAfterChild (last, element) != nullptr;
				else
					inserted = parent -> InsertFirstChild (element) != nullptr;

				parent = element;	// move along branch as it's built
			}
			if (inserted)
			{
				// set the attributes and text for final element from arguments
				for (auto const & attr : attributes)
					element -> SetAttribute (attr .Name() .c_str(), attr .Value() .c_str());
				if (!text .empty())
					element -> SetText (text .c_str());
				return element;
			}
			else
			{
				// failed, delete any elements we created
				for (auto be = ++branch .begin(); be != branch .end(); ++be)
					parent -> GetDocument() -> DeleteNode (be -> second);
				throw XmlException ("unable to append element"s);
			}
			// always returns valid XMLElement on success, failures are exceptions
		}


		// append family
		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath)
		{
			return append_element (parent, xpath, {}, ""s, true);
		}

		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, AttributeList attributes)
		{
			return append_element (parent, xpath, attributes, ""s, true);
		}

		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, const std::string & text)
		{
			return append_element (parent, xpath, {}, text, true);
		}

		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, AttributeList attributes, const std::string & text)
		{
			return append_element (parent, xpath, attributes, text, true);
		}


		// prepend family
		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath)
		{
			return append_element (parent, xpath, {}, ""s, false);
		}

		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath, AttributeList attributes)
		{
			return append_element (parent, xpath, attributes, ""s, false);
		}

		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath, const std::string & text)
		{
			return append_element (parent, xpath, {}, text, false);
		}

		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath, AttributeList attributes, const std::string & text)
		{
			return append_element (parent, xpath, attributes, text, false);
		}


		inline XMLElement * insert_next_element (XMLElement * sibling, const std::string & name, AttributeList attributes = {}, const std::string & text = ""s)
		{
			if (!sibling)
				throw XmlException ("null element"s);

			auto parent = sibling -> Parent();
			if (!parent)
				throw XmlException ("orphaned element"s);

			XMLElement * element = parent -> GetDocument() -> NewElement (name .c_str());
			if (!element)
				throw XmlException ("unable to create element"s);

			auto inserted = parent -> InsertAfterChild (sibling, element) != nullptr;
			// todo: this block is identical to append_element(), make common code
			if (inserted)
			{
				for (auto const & attr : attributes)
					element -> SetAttribute (attr .Name() .c_str(), attr .Value() .c_str());
				if (!text .empty())
					element -> SetText (text .c_str());
				return element;
			}
			else
			{
				parent -> GetDocument() -> DeleteNode (element);
				throw XmlException ("unable to insert element"s);
			}
			// always returns valid XMLElement on success, failures are exceptions
		}
	}
}
