Examples of tinyxml2-ex usage can be found in the test project.
Extracts are reproduced here with explanatory notes.
But please download the test project and play around.

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


##### At its simplest, you can find an element in a document:
```c++
auto bThree = find_element (*doc, "A/B[@id='three']"s);
```
`bThree` is the first \<B> element with attribute id='three'.
The type of bThree is `const tinyxml2::XMLElement *` and will be a `nullptr` if a matching element is not found.


##### You can find a child element of an element:
```c++
auto c5678 = find_element (bThree, "C");
```
Which returns the first \<C> element that is a child of the \<B> element with attribue id='three'.


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
Iterates over all \<C> elements with a code attribute that are children of \<B> element(s) with attribue id='three'.
`attribute_value` helper function returns the value of the attribute as a string;
the string is empty when the element does not have the attribute specified
(which cannot happen here clearly).
