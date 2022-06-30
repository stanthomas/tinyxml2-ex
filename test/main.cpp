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


#include <string>
#include <iostream>
#include <algorithm>
#include <conio.h>


// include the header for tinyxml2ex which includes tinyxml2, remember to put them on your include path
#include <tixml2cx.h>
#include <map>
#include <assert.h>

using namespace std;
using namespace std::literals::string_literals;


int main()
{
	// a simple XML document
	string testXml {R"-(
<?xml version="1.0" encoding="UTF-8"?>
<A>
	<B id="one">
		<C code="1234">
			A-B(one)-C.1234
		</C>
		<C code="5678">
			<![CDATA[A-B(one)-C.5678]]>
		</C>
		<C code="9ABC"> A-B{one)-C.9ABC</C>
		<D code="9ABC" id="d1" />
	</B>
	<B id="two">
		<D id="d2" />
	</B>
	<B id="three" org="{extern}">
		<C code="1234">
			A-B(three)-C.1234
		</C>
		<C code="9ABC">A-B(three)-C.9ABC</C>
		<D id="d3" description="A-B(three)-D.9ABC" />
	</B>
	<B id="four">
			one {B4} two {B4}
	</B>
</A>
)-"s};


	// these three blocks are equivalent and demonstrate different ways to iterate over
	// all <C> element children of <B> element children of the document element <A>
	// we only want to inspect the elements and attributes, so treat the XMLElements as const
	// 1) native TinyXML2 : 21 lines of code
	{
		printf ("\n1)   <C> element children of <B> element children of the document element <A>\nnative TinyXML2\n");
		tinyxml2::XMLDocument doc;
		if (doc .Parse (testXml .c_str()) == 0/*XML_NO_ERORR*/)
		{
			const tinyxml2::XMLElement * eA = doc .FirstChildElement();
			if (eA)
			{
				const tinyxml2::XMLElement * eB = eA -> FirstChildElement("B");
				while (eB)
				{
					const tinyxml2::XMLElement * eC = eB -> FirstChildElement ("C");
					while (eC)
					{
						printf ("%s = %s\n", eC -> Name(), eC -> GetText());
						eC = eC -> NextSiblingElement ("C");
					}
					eB = eB -> NextSiblingElement ("B");
				}
			}
			else
				printf ("unable to load XML document\n");
		}

		std::map<std::string, size_t> xpathElementCount = {
			{ R"(A)",1 }
			,{ R"(A/B/C)",3 }
			,{ R"(A//B/C)",3 }
			,{ R"(A/B//C)",3 }
			,{ R"(A//B//C)",3 }
			,{ R"(/A//B//C)",3 }
			,{R"(//A//B//C)",3}
			,{ R"(A/B[@code='1'])",2 }
			,{ R"(A/B[@code='1']/C)",3 }
			,{ R"(/A/../B[@code='1']/C)",4 }
		};

		for (auto& it : xpathElementCount)
		{
			auto element_path = tinyxml2::element_path_from_xpath(doc.RootElement(), it.first);

			auto& xpath = it.first;
			auto expectCount = it.second;
			//if it's not root
			if (!(xpath.size() >= 2 && xpath[0] == '/' && xpath[1] != '/'))
			{
				expectCount++;
			}

			assert(element_path.size() == expectCount);
		}
	}
	cout << "----" << endl << endl;




	// 2) tixml2ex XPath selector : 10 lines of code
	cout << "2)   <C> element children of <B> element children of the document element <A>" << endl
		<< "tixml2ex XPath selector" << endl;
	try
	{
		auto doc = tinyxml2::load_document (testXml);
		// n.b. the static_cast makes the XMLDocument const and hence all XMLElements returned are also const
		for (auto eC : tinyxml2::selection (static_cast <const tinyxml2::XMLDocument &> (*doc), "A/B/C"))
			cout << eC -> Name() << " = " << text (eC) << endl;

		cout << "=================================================" << endl << endl;

		size_t count = 0;

		for (auto eC : tinyxml2::selection(static_cast <const tinyxml2::XMLDocument &> (*doc), "A//C"))
		{
			++count;
		}
		assert(count== 5);
		if (count != 5)
			cout << "A//C expect 5  but actrul equal to "<<count  << endl;

		count = 0;
		for(auto it : tinyxml2::selection(doc->RootElement(),"*/B/C"s))
			++count;
		assert(count == 0);

		count = 0;
		for (auto it : tinyxml2::selection(doc->RootElement(), "*/C"s))
			++count;
		assert(count == 5);

		count = 0;
		for (auto it : tinyxml2::selection(doc->RootElement(), "/A/*/C"s))
			++count;
		assert(count == 5);

		count = 0;
		for (auto it : tinyxml2::selection(doc->RootElement(), "/A/B[@id='one']/../B[@id='three']/C"s))
			++count;
		assert(count == 2);

		count = 0;
		for (auto it : tinyxml2::selection(doc->RootElement(), "/A/B/.[@id='one']/C"s))
			++count;
		assert(count == 3);

		for (auto eC : tinyxml2::selection(doc->RootElement(), "//B[@id='one']"s))
			cout << eC->Name() << " id = " << attribute_value(eC,"id") << endl;

		cout << "=================================================" << endl << endl;
	}
	catch (tinyxml2::XmlException & e)
	{
		cout << "XmlException caught" << e .what() << endl;
	}
	cout << "----" << endl << endl;


	// 3) simple tixml2ex element iterator : 18 lines of code
	cout << endl << "3)   <C> element children of <B> element children of the document element <A>" << endl
		<< "simple tixml2ex element iterator" << endl;
	try
	{
		auto doc = tinyxml2::load_document (testXml);
		auto eA = doc -> FirstChildElement();
		if (eA)
			for (auto eB : eA)
			{
				// just for fun, use standard algorithm for_each to iterate over the children of <B>
				for_each (cbegin (eB), cend (eB),
					[](auto e)
				{
						// simple iterators are just that, they iterate over all children
						// therefore we must test element name (type) to examine only <C> elements
					if (strcmp (e -> Name(), "C") == 0) cout << e -> Name() << " = " << text (e) << endl;
				});
			}
	}
	catch (tinyxml2::XmlException & e)
	{
		cout << "XmlException caught" << e .what() << endl;
	}

	cout << "=================================================" << endl << endl;


	// additional element selection and iteration using XPath syntax
	// illustrating helper functions
	try
	{
		auto doc = tinyxml2::load_document (testXml);

		// find first matching an element in the document
		cout << "find an element by attribute value" << endl;
		auto bThree = find_element (static_cast<const tinyxml2::XMLDocument &>(*doc), "A/B[@id='three']"s);
		cout << attribute_value (bThree, "id"s) << " - " << attribute_value (bThree, "org"s) << endl;
		cout << "=================================================" << endl << endl;


		// find the first instance of a child element of the selected type
		cout << "get description attribute of <D> element" << endl;
		if (auto ch1 = find_element (bThree, "D"s))
			cout << attribute_value (ch1, "description"s) << text (ch1) << endl << endl;
		cout << "=================================================" << endl << endl;

		// iterate over all <C> children of selected <B>
		cout << "iterate over all <C> children of selected <B>" << endl;
		for (auto cc : tinyxml2::selection (bThree, "C"s))
		{
			cout << cc -> Parent() -> ToElement() -> Name() << "[" << attribute_value (cc -> Parent() -> ToElement(), "id") << "] / " << cc -> Name() << "[@code='";
			if (!attribute_value (cc, "code") .empty())
				cout << attribute_value (cc, "code");
			else
				cout << "**element has no attribute - code**";
			cout << "']" << endl;
		}
		cout << "=================================================" << endl << endl;


		// iterate over all elements in the document along the selected path by starting with '/'
		// note that because path starts from the document, bThree is used only as an element in the document not the root of the search
		cout << "iterate over all <C> children : /A/B[@id='three']/C" << endl;
		int nC = 0;
		for (auto cc : tinyxml2::selection (bThree, "/A/B[@id='three']/C"s))
		{
			++nC;
			cout << cc -> Parent() -> ToElement() -> Name() << "[" << attribute_value (cc -> Parent() -> ToElement(), "id") << "] / " << cc -> Name() << "[@code='";
			if (!attribute_value (cc, "code") .empty())
				cout << attribute_value (cc, "code");
			else
				cout << "**element has no attribute - code**";
			cout << "']" << endl;
		}
		cout << nC << " Cs in B[@id='three']" << endl << endl;
		cout << "=================================================" << endl << endl;

		// iterate over all children, any name (type), with code attribute value
		cout << "iterate over all children of any name (type) : /A/B/[@code='9ABC']" << endl;
		for (auto cc : tinyxml2::selection (bThree, "/A/B/[@code='9ABC']"s))
		{
			cout << cc -> Parent() -> ToElement() -> Name() << "[" << attribute_value (cc -> Parent() -> ToElement(), "id") << "] / " << cc -> Name() << "[@code='";
			if (!attribute_value (cc, "code") .empty())
				cout << attribute_value (cc, "code");
			else
				cout << "**element has no attribute - code**";
			cout << "']" << endl;
		}
		cout << "=================================================" << endl << endl;


		// find the first instance of a child element of the selected type with a matching attribute value
		cout << "find C[@code='9ABC'] within B[@id='three']" << endl;
		if (auto cc = find_element (bThree, "C[@code='9ABC']"s))
			cout << text (cc) << " , " << attribute_value (cc, "description") << endl << endl;
		else
			cout << "could not find C[@code='9ABC'] in B" << endl;
		cout << "=================================================" << endl << endl;

		// find the same element within document
		cout << "find B[@id='three']/C[@code='9ABC']" << endl;
		if (auto cc = find_element (*doc, "/A/B[@id='three']/C[@code='9ABC']"s))
			cout << text (cc) << " , " << attribute_value (cc, "description") << endl << endl;
		else
			cout << "could not find A/B[@id='three']/C[@code='9ABC'] in document" << endl;
		cout << "=================================================" << endl << endl;


		// iterate over all children, any name (type), of <B> elements which are children of the document element
		cout << "iterate over all children, any name (type), of <B> elements which are children of the document element" << endl;
		auto eA = doc -> FirstChildElement();
		for (auto cd : tinyxml2::selection (eA, "B/*"))
			cout << cd -> Name() << " = " << text (cd) << " id=" << attribute_value (cd, "id") << endl;
		cout << "=================================================" << endl << endl;
	}
	catch (tinyxml2::XmlException & e)
	{
		cout << e .what() << endl;
	}


	/////////////////////// modify the document


	try
	{
		auto doc = tinyxml2::load_document (testXml);
		// create new CZ element in branch below selected <C> element with new CX and new CY
		auto ne = append_element (find_element (*doc, "/A/B[@id='three']/C[@code='9ABC']"s), "CX/CY[@id='099']/CZ", {{"id"s, "0998"s}, {"code"s, "ASDF"s}}, "magnum"s);
		// now use the inserted element to insert another after it
		auto czId {"1233"s};
		auto czCode {"ZXCV"s};
		auto czData {"corneto"s};
		insert_next_element  (ne, "CZ", {{"id"s, czId}, {"code"s, czCode}}, czData);

		// todo:
		// what do we want find_element to do when path is empty?
		// . find the first child element (which is what it does now) ?
		// . or simply return the parent element passed in ?

		tinyxml2::XMLPrinter printer;
		doc -> Print (&printer);
		cout << printer .CStr() << endl;
	}
	catch (tinyxml2::XmlException & e)
	{
		cout << e .what() << endl;
	}


	/////////////////////// copy
	try
	{
		auto source = tinyxml2::load_document (testXml);
		auto dest = std::make_unique <tinyxml2::XMLDocument>();
		auto e = dest -> NewElement ("mycopy");
		dest -> InsertEndChild (e);

		xcopy (source -> FirstChildElement(), e, {{"extern", "internal"}, {"B4", "Bee Four"}});

		tinyxml2::XMLPrinter printer;
		dest -> Print (&printer);
		cout << printer .CStr() << endl;
	}
	catch (tinyxml2::XmlException & e)
	{
		cout << e .what() << endl;
	}



	// hold console window open so we can see the output
	std::cout << "hit any key to close" << std::endl;
	auto c = _getch();

	return 0;
}
