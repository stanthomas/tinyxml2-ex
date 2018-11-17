/*
tinyxml2ex - a set of add-on classes and helper functions bringing C++11/14 features, such as iterators, strings and exceptions, to tinyxml2


Copyright (c) 2017 Stan Thomas

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

#define __TINYXML_EX__

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

		class TINYXML2_LIB XmlException : public std::logic_error
		{
		public:
			XmlException (const std::string & description) : logic_error (description) {}
		};


		class TINYXML2_LIB AttributeNameValue
		{
		public:
			AttributeNameValue (std::string name, std::string value) : _name(name), _value(value) {}
			std::string Name() const { return _name; }
			std::string Value() const { return _value; }
		private:
			std::string _name;
			std::string _value;
		};	// AttributeNameValue


		using attribute_list_t = std::list <AttributeNameValue>;


		class TINYXML2_LIB ElementProperties
		{
		public:
            enum class Location {
                locationChildren, locationChildrenNoName, locationAllChildren, locationMyself, locationParent, locationFunction, locationRoot
            };           

			ElementProperties (std::string xProps,size_t pos)
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
							throw XmlException("ill formed XPath"s);
						// XPath attribute values are wrapped in single quote marks
						// but we don't require them and ignore them if present
						// toggle between two effectively equivalent modes
						_state = _state == ParseState::attributeAssignment ? ParseState::attributeValue : ParseState::attributeAssignment;
					}
					else if (c == '.')
					{
						if (_state != ParseState::elementName)
							throw XmlException("ill formed XPath"s);

						if (_location == Location::locationChildren)
						{
							_location = Location::locationMyself;
						}
						else if (_location == Location::locationMyself)
						{
							_location = Location::locationParent;
						}
						else
						{
							throw XmlException("ill formed XPath"s);
						}
					}
					else if (c == '*')
					{
						if (_state != ParseState::elementName)
							throw XmlException("ill formed XPath"s);
						_location = Location::locationChildrenNoName;
					}
					else if (c == '/')
					{
						if (_state != ParseState::elementName)
							throw XmlException("ill formed XPath"s);
						if (pos == 0)
						{
							_location = Location::locationRoot;
						}
						else if (_location == Location::locationChildren)
						{
							_location = Location::locationAllChildren;
						}
						else
						{
							throw XmlException("ill formed XPath"s);
						}
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
            const std::string& Name() const { return _name; }
            const Location& LocationType() const { return  _location; }

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

            XMLElement * Locate(XMLElement *element, XMLElement *lastLocationElement = nullptr)const
            {
               auto ret = Locate(const_cast<const XMLElement *>(element), const_cast<const XMLElement *>(lastLocationElement));
               return const_cast<XMLElement *>(ret);
            }

				//locate the element
				//element = curLocation
				//frontElement=lastLocation(not must parent, example A//B) 
            const XMLElement * Locate(const XMLElement *element,const XMLElement *frontElement)const
            {
                if (frontElement == nullptr) return nullptr;

                const XMLElement * retElement = nullptr;//return element
                switch (_location)
                {
                case Location::locationChildren:
						 if(element == nullptr)
							retElement= frontElement->FirstChildElement(_name.c_str());
						 else
							 retElement = element->NextSiblingElement(_name.c_str());
                    break;
                case Location::locationMyself:
                    retElement = frontElement;
                    break;
                case Location::locationParent:
                    retElement = frontElement->Parent()->ToElement();						  
                    break;
					 case Location::locationChildrenNoName:
						 if (!element)
						 {
							 retElement = frontElement->FirstChildElement();
						 }
						 else
						 {
							 retElement = element->NextSiblingElement();
						 }
						 break;
                case Location::locationAllChildren:
					 {
						 assert(frontElement);
						 std::vector<const XMLElement *> searchEleList;
						 if (element == nullptr)
						 {
							 searchEleList = { frontElement ,frontElement->FirstChildElement() };
						 }
						 else
						 {
							 //get element->xx->xx->lastLocationElement list
							 searchEleList = { element };
							 auto lastpath = element;
							 while (auto parentEle = lastpath->Parent()->ToElement())
							 {
								 searchEleList.push_back(parentEle);

								 if (searchEleList.back() == frontElement)
									 break;

								 lastpath = parentEle;
							 }
							 assert(searchEleList.back() != nullptr);
							 if (searchEleList.back() != frontElement)
								 break;
							 //reverse to lastLocationElement->xx->xx->element list
							 std::reverse(searchEleList.begin(), searchEleList.end());
						 }

						 //search lastLocationElement----->element,all children will be checked
						 const char* eleName = _name.c_str();
						 while (searchEleList.size())
						 {
							 while (searchEleList.back() == nullptr && searchEleList.size())
							 {
								 searchEleList.pop_back();
								 if (searchEleList.empty()) break;
								 searchEleList.back() = searchEleList.back()->NextSiblingElement();
							 }
							 if (searchEleList.empty()) break;

							 auto& lastEle = searchEleList.back();
							 if (lastEle != frontElement && lastEle != element
								 && _name == lastEle->Value())
							 {
								 retElement = lastEle;
								 break;
							 }
							 else if (auto ele = lastEle->FirstChildElement())
							 {
								 searchEleList.push_back(ele);
								 continue;
							 }

							 //search the children where is a target element
							 lastEle = lastEle->NextSiblingElement();
						 }
					 }break;
                case Location::locationFunction:
                    //TODO some function like A[0],A[price<3],A[size()-1]
                    break;
                case Location::locationRoot:
                    retElement = frontElement->GetDocument()->RootElement();
                    break;
                default:
                    break;
                }
					 if (retElement == element)//if it's myself return false
						 retElement = nullptr;

                return retElement;
            }

		private:
			std::string _name;
            attribute_list_t _attributes;
            enum class ParseState { elementName, attributeFilter, attributeName, attributeAssignment, attributeValue };
            
            ParseState _state {ParseState::elementName};
            Location _location = { Location::locationChildren };//use to relocate the level in element
        };	// ElementProperties


		template <typename XE> using element_path_location_t = std::pair<ElementProperties, XE *>;
		template <typename XE> using element_path_t = std::list<element_path_location_t<XE>>;
		template <typename XE> using element_path_iterator_t = typename element_path_t<XE>::iterator;


		template <typename XE> inline element_path_t<XE> element_path_from_xpath (XE * root, const std::string & xpath)
		{
            //if there is //B, list is /B
            //if there is A//B, list is A->/B
            //if there is A//B/C, list is A->/B->C
            //if there is A//B[@p='v']/C, list is A->/B[@p='v']->C
            //if there is /A//B/C, list is A->/B->C

			if (!root || xpath.empty())
				throw XmlException ("null element"s);

			element_path_t<XE> ep;

			// split the path
			size_t start = 0;
			size_t pos = 0;
			// set element at head of selection branch
			//	if path starts with '/' then it is relative to document, otherwise relative to element passed in
			// first element in selection branch is the root and only children of the root are considered
			// for document-based paths, this works because there can only be one document element
			if (xpath.size() >= 2 && xpath[0] == '/' && xpath[1] != '/')
			{
				// document is not an element so needs special handling
				// advance to the actual document element
				// note that document element must still appear in path, so we have to step over it
				start++;
				pos = xpath.find('/', start);

				auto filter = ElementProperties(xpath.substr(start, pos - start), 0);
				auto element = root->GetDocument()->RootElement();
				if (element && !filter.Name().empty())
				{
					if (filter.Name() != std::string(element->Name()))
						throw XmlException("document element name mismatch"s);
				}
				ep.emplace_back(std::make_pair(filter, element));
				start = pos;
			}
			else
			{
				ep.emplace_back(std::make_pair(ElementProperties(root->Name(), 0), root));
			}

			// continue with other elements along path
			while (start != std::string::npos)
			{
				//if (xpath.size()-1 > start && xpath[start]=='/' && xpath[start + 1] == '/')
				//{
				//	//if there is //B, substring is /B
				//	start++;
				//}
				if (xpath[start]=='/')
					start++;
				pos = xpath.find('/', start + 1);

				ep.emplace_back(std::make_pair(ElementProperties(xpath.substr(start, pos - start), start), nullptr));

				start = pos;
			}

			// and the final element,if "//B" is last string, step over it
			if (start < xpath.size())
			{
				ep.emplace_back(std::make_pair(ElementProperties(xpath.substr(start, pos - start), start), nullptr));
			}

			return std::move(ep);
		}


		template <typename XE> inline element_path_t<XE> element_path_from_element (XE * e)
		{
			return {std::make_pair (ElementProperties(), e)};
		}


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
			ElementIterator() : _selectionPath (element_path_from_element (static_cast<XE *>(nullptr))) {}
			ElementIterator (XE * origin) : _selectionPath (element_path_from_element (origin)) {}
			ElementIterator (XE * origin, const std::string & xpath)
				: _selectionPath (element_path_from_xpath (origin, xpath))
			{
				if (_selectionPath .empty())
					throw XmlException ("selection xpath is empty - logic error");

				// _selectionPath must have at least one element which is the origin of all branches considered
				// only children of the origin are considered
				// on construction the origin element must be a valid XMLElement
				// elements in the remainder of the path are initially null
				// descend and initialise first matching branch (if any)
				descend (_selectionPath .begin());
				// remove the origin from the list so that iteration is only over child elements

                /*if(_selectionPath.begin()->first.LocationType() != ElementProperties::Location::locationRoot)
                    _selectionPath .pop_front();*/
			}
			XE * operator *() const { return !_selectionPath .empty() ? _selectionPath .back() .second : nullptr; }
			bool operator == (const ElementIterator & iter) const { return iter.operator*() == this->operator*(); }
			bool operator != (const ElementIterator & iter) const { return ! operator == (iter); }
			ElementIterator & operator ++()
			{
				// to get here we must have found at least one matching element
				// selection branch contains the complete element path
#if !defined (NDEBUG)
				for (auto const & pe : _selectionPath)
					assert (pe .second);
#endif
				// start at the bottom with the current element
				traverse (--_selectionPath .end());
				return *this;
			}

		private:
			bool descend(element_path_iterator_t<XE> parentixSel)
			{
				// recursively descend selection branch of matching elements
				if (!parentixSel->second)
					return false;
				auto parentElement = parentixSel->second;
				if (parentElement == nullptr) return false;

				auto ixSel = parentixSel;
				if (++ixSel == _selectionPath.end())
					return true;	// we've found the first matching element

				const ElementProperties&  eleppt = ixSel->first;

				while (ixSel->second = eleppt.Locate(ixSel->second, parentElement))
				{
					if (ixSel->first.Match(ixSel->second))
					{
						if (descend(ixSel))
							return true;
					}
				}
				return false;
			}

			void traverse(element_path_iterator_t<XE> ixSel)
			{
				// to find next element we can go sideways or up and then down
				// traverse() does the moves across the xml tree, descend() then explores each potential new branch
				// note that this method can only be called once the selection has been initialised
				const ElementProperties&  eleppt = ixSel->first;
				XE* parentElement = nullptr;
				if (ixSel != _selectionPath.begin())
				{
					auto parentixSel = ixSel;
					--parentixSel;
					parentElement = parentixSel->second;
				}

				if (parentElement)
				{
					while (ixSel->second = eleppt.Locate(ixSel->second, parentElement))
					{
						if (ixSel->first.Match(ixSel->second))
						{
							if (descend(ixSel))
							{
								return;
							}
						}
					}
				}
				
				//clear cur element
				ixSel->second = nullptr;

				// no siblings or sibling branches match, go up a level (unless already at origin)
				if (ixSel != _selectionPath.begin())
				{
					traverse(--ixSel);
				}
			}

		private:
			element_path_t<XE> _selectionPath;
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
			Selector (XE * base, std::string xpath) : _base (base), _xpath (xpath) {}

			ElementIterator<XE> begin() const
			{
				if (!_xpath .empty() && _base)
					return ElementIterator<XE> (_base, _xpath);
				else
					return end();
			};

			ElementIterator<XE> end() const
			{
				return ElementIterator<XE>();	// an empty iterator that will return a null XMLElement
			};

		private:
			XE * _base;
			std::string _xpath;
		};	// Selector


		// helper functions to return appropriate const / non-const Selector
		inline Selector<XMLElement> selection (XMLElement * base, std::string xpath)
		{
			return Selector<XMLElement> (base, xpath);
		}

		inline Selector<const XMLElement> selection (const XMLElement * base, std::string xpath)
		{
			return Selector<const XMLElement> (base, xpath);
		}

		inline Selector<XMLElement> selection (XMLDocument & doc, std::string xpath)
		{
			return Selector<XMLElement> (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}

		inline Selector<const XMLElement> selection (const XMLDocument & doc, std::string xpath)
		{
			return Selector<const XMLElement> (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}


		// helper functions to find the first element (if any) below a base element matching the XPath
		inline XMLElement * find_element (XMLElement * base, std::string xpath = ""s)
		{
			return *Selector<XMLElement> (base, xpath) .begin();
		}


		inline const XMLElement * find_element (const XMLElement * base, std::string xpath = ""s)
		{
			return *Selector<const XMLElement> (base, xpath) .begin();
		}


		inline XMLElement * find_element (XMLDocument & doc, std::string xpath = ""s)
		{
			return find_element (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}

		inline const XMLElement * find_element (const XMLDocument & doc, std::string xpath = ""s)
		{
			return find_element (doc .RootElement(), (!xpath.empty() && xpath[0] == '/') ? xpath : '/' + xpath);
		}


		// load XML document from string buffer
		inline std::unique_ptr <XMLDocument> load_document (const std::string & xmlString)
		{
			auto doc = std::make_unique <XMLDocument>();
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
		// todo: consider using std::initializer_list<AttributeNameValue> for attributes parameter
		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, const attribute_list_t & attributes, const std::string & text, bool addAtBack)
		{
			XMLElement * element {nullptr};
			bool inserted {false};

			element_path_t<XMLElement> branch {element_path_from_xpath (parent, xpath)};
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

		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, const attribute_list_t &  attributes)
		{
			return append_element (parent, xpath, attributes, ""s, true);
		}

		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, const std::string & text)
		{
			return append_element (parent, xpath, {}, text, true);
		}

		inline XMLElement * append_element (XMLElement * parent, const std::string & xpath, const attribute_list_t &  attributes, const std::string & text)
		{
			return append_element (parent, xpath, attributes, text, true);
		}


		// prepend family
		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath)
		{
			return append_element (parent, xpath, {}, ""s, false);
		}

		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath, const attribute_list_t &  attributes)
		{
			return append_element (parent, xpath, attributes, ""s, false);
		}

		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath, const std::string & text)
		{
			return append_element (parent, xpath, {}, text, false);
		}

		inline XMLElement * prepend_element (XMLElement * parent, const std::string & xpath, const attribute_list_t &  attributes, const std::string & text)
		{
			return append_element (parent, xpath, attributes, text, false);
		}

		//touch family
		inline XMLElement * touch_element(XMLElement * parent, const std::string & xpath, const attribute_list_t &  attributes = {}, const std::string & text = "")
		{
			auto ele = find_element(parent, xpath);
			if (ele == nullptr) ele = append_element(parent, xpath, attributes, text, true);
			assert(ele);
			return ele;
		}

		inline XMLElement * insert_next_element (XMLElement * sibling, const std::string & name, const attribute_list_t &  attributes = {}, const std::string & text = ""s)
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
