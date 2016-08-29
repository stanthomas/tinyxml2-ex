Examples of tinyxml2-ex usage can be found in the test folder.
Extracts are reproduced here with explanatory notes. But please download the test project and play around.

The easiest way to use this extension is to download tinyxml2ex.h and save it to the same folder as tinyxml2.
TinyXML2 can be downloaded from it's project site at {http://www.grinninglizard.com/tinyxml2/index.html} and is also on `nuget`.

In your code, include the header for tinyxml2ex which, in turn, includes tinyxml2.h. Remember to put the folder on your include path.
```#include <tixml2ex.h>```


All examples use this simple xml document stored in a string:
```c++
std::string testXml = R"-(
<?xml version="1.0" encoding="UTF-8"?>
<A>
	<B id="one">
		<C code="1234">
			A-B(one)-C.1234
		</C>
		<C code="5678">
			A-B(one)-C.1234
		</C>
		<C code="9ABC">
			A-B(one)-C.1234
		</C>
		<D />
	</B>
	<B id="two">
	</B>
	<B id="three" org="extern">
		<C code="1234">
			A-B(three)-C.1234
		</C>
		<C code="9ABC">
			A-B(three)-C.9ABC
		</C>
		<D description="A-B(three)-D.9ABC" />
	</B>
	<B id="four">
	</B>
</A>
)-";
```
##### Load xml document from string using one of our helper functions:
```c++
auto doc = tinyxml2::load_document (testXml);
```
The type of doc is `std::unique_ptr<tinyxml2::XMLDocument>`; an exception will be thrown if the XML cannot be parsed.
We could also have create the XMLDocument using TinyXML2 directly.
An exception of type tinyxml2::tixml2ex::XmlException will be thrown
if `load_document` is unable to parse the string.


### Read XML document
##### At its simplest, you can find an element in a document:
```c++
auto bThree = find_element (*doc, "A/B[@id='three']"s);
```
`bThree` is the first \<B> element with attribute id='three'.
The type of bThree is `tinyxml2::XMLElement *` and will be a `nullptr` if a matching element is not found.


##### You can find a child element of an element:
```c++
auto c5678 = find_element (bThree, "C");
```
Which returns the first \<C> element that is a child of the \<B> element with attribute id='three'.


##### You can iterate over the immediate child elements of an element:
```c++
for (auto const cc : bThree)
	std::cout << cc -> Name() << text (cc) << std::endl;
```
The type of cc is `const tinyxml2::XMLElement *` and
'XMLElement::Name()' is the standard tinyxml2 method returning a `const char *`.
`text` is a helper function to return the text of the element as a string.


##### And iterate over a subset of the immediate child elements:
```c++
for (auto const cc : selection (bThree, "C[@code]"s))
   std::cout << attribute_value (cc, "code") << std::endl;;
```
Iterates over all \<C> elements with a code attribute that are children of \<B> element(s) with attribute id='three'.
`attribute_value` helper function returns the value of the attribute as a string;
the string is empty when the element does not have the attribute specified
(which cannot happen here clearly).


### Modify XML document
##### Create a new CZ element in branch below given <C> element with newly created CX and CY elements:
```c++
auto ne = append_element (
   find_element (*doc, "/A/B[@id='three']/C[@code='9ABC']"s),      // element to append to
   "CX/CY[@id='099']/CZ",                                          // new element(s)
   {{"id"s, "0998"s}, {"code"s, "ASDF"s}}, "magnum"s);             // attributes for new element
```
A new branch comprising new CX, CY and CZ elements is created and a pointer to the final element is returned. Attributes for the elements added can be specified in the path (XPath syntax) or, for the final element as a list of attribute name/value pairs.
Note use of ```find_element``` here, which will return ```nullptr``` if it can't find a matching element. So either test the returned XMLElement pointer for null or be ready to catch the exception thrown by ```append_element```.

##### And now use the inserted element to insert another after it:
```c++
auto czId {"1233"s};
auto czCode {"ZXCV"s};
auto czData {"corneto"s};
insert_next_element  (ne, "CZ", {{"id"s, czId}, {"code"s, czCode}}, czData);
```
The attribute values, czId etc., are assigned to variables to illustrate how you might change the document programmatically.

The result is two new CZ elements in a new branch CX/CY below the element ```<C code="9ABC">``` from the original document.
```xml
        <C code="9ABC">
            <CX>
                <CY id="099">
                    <CZ id="0998" code="ASDF">magnum</CZ>
                    <CZ id="1233" code="ZXCV">corneto</CZ>
                </CY>
            </CX>A-B(three)-C.9ABC</C>
```
