TinyJson
-------
### 0. Overview

`TinyJson` is a C++ library for <font color="red">processing JSON data</font> and <font color="red">easily converting between structs and JSON</font>. Its features are as follows:

- `TinyJson` is a lightweight C++ library for processing JSON data. It consists of only a single header file, uses C++14 features, and implements JSON format data processing in just a few hundred lines of code.
- By leveraging C++ template features, it achieves easy bidirectional conversion between structs and the JSON format simply by adding a macro definition to the class or struct, all within a few hundred lines of code.

### 1. JSON Processing

#### 1.1. Description

The source code references the project [SimpleJSON](https://github.com/nbsdx/SimpleJSON) and includes the following improvements:

- Added JSON file read/save interfaces.
- Provides error location information when JSON parsing fails.
- Added support for handling comments in JSON data, allowing the use of "//" and other forms of comments in JSON documents.
- Other optimizations and numerous bug fixes. For details, see the source file `json.hpp`.

#### 1.2. Usage Example

`main.cpp` includes tests for parsing invalid JSON strings, parsing JSON files, parsing JSON data with comments, formatted output, quick output, assignment, moving, int64/uint64 data types, list initialization, and other usage methods.

### 2. JSON and Struct Conversion

#### 2.1. Description

 `JsonConverter.hpp`  enables easy bidirectional conversion between structs and JSON through macro definitions. The member types supported within the struct are as follows:

- String and numeric types (std::string, integer, floating-point, boolean, etc.)
- Enumeration types
- JSON type
- Custom data structures that have already used the macro-defined struct conversion
- `std::shared_ptr`/`std::unique_ptr` types whose template parameters are also fundamental data types or structs that define a conversion interface. For inheritance hierarchies, a base class pointer will not handle derived class data types
- `std::optional`, where the template parameter can be a fundamental data type or a struct that defines the conversion interface
- Arrays (`std::vector`, `std::list`), where the template parameter can be a fundamental data type or a struct that defines the conversion interface
-  `std::map`, must use `std::string` for its keys. The values can be either fundamental data types, structs that define a conversion interface

Smart pointer types are temporarily not supported (mainly due to issues involving polymorphism and base class pointers pointing to derived classes. While JSON containing base class data can be generated, generating a base class pointer pointing to a derived object from JSON containing derived class data is complex). If JSON containing derived class data is parsed using a base class object, only the base class portion of the data will be retained.

#### 2.2. Usage Example

##### 2.2.1. Struct Member Registration

The header file `jsonConverter.hpp` defines two variadic macros:

- `ADD_JSON_MEMBER`: Used to register member objects within a struct.
- `ADD_JSON_MEMBER_INHERIT`: Specifies the parent class, which must have used the registration macro, enabling the conversion of the parent class's data.

```cpp
// Define an enumeration type
enum class EM_type
{
    TYPE_1,
    TYPE_2
};

// A class that can serve as a base class
struct BaseData
{
    std::optional<int32_t> lData;
    std::optional<double> dData;
    std::string str = "Hello";
    float fData = 4.6;
    EM_type ty1 = EM_type::TYPE_2;
    Json::Value jvData;
    ADD_JSON_MEMBER(lData, dData, str, fData, ty1, jvData);
};

// Inherits from the base class
struct DeriveData : public BaseData
{
    double dValue = 12.9;
    uint64_t uintMax = 12;
    ADD_JSON_MEMBER_INHERIT(BaseData, dValue, uintMax);
};

// A class containing structs
struct WrapData
{
    std::list<DeriveData> listDataDer; // Array of derived classes
    std::optional<BaseData> optBase; // Saves the base class part from a derived class
    std::optional<DeriveData> optDerive; // Derived class
    std::optional<DeriveData> optDataEmpty; // Saves an empty optional
    float fValue = 2.9;
    ADD_JSON_MEMBER(listDataDer, optBase, optDerive, optDataEmpty, fValue);
};

```

#### 2.2.2. Bidirectional Conversion between Struct and JSON
The `Json::Value` class overloads the operators` >>` and`<<` to enable easy conversion between structs and JSON.

```cpp
WrapData data;
Json::Value outJson;
// Assign values to the struct members
// ...
outJson << data; // Convert struct to JSON

WrapData outWrap;
outJson >> outWrap; // Convert JSON to struct
```

#### 2.2.3. Exception Handling
During the conversion from JSON to a struct, for data members and submembers, except for std::optional and array types (std::vector, std::list), data of other types must be present in the JSON; otherwise, an exception is thrown.

```cpp
// Define the ErrorTest class. When converting JSON to a struct, the sData and fData members must exist in the JSON.
struct ErrorTest
{
    uint16_t sData = 65535;
    std::optional<int32_t> opLData = 0;
    float fData = 0.3;
    ADD_JSON_MEMBER(sData, opLData, fData);
};

```

Test code example:

```cpp
try
{
    std::cout << "/******************  Exception when non-optional member is missing  ********************/" << std::endl;
    ErrorTest eData;
    Json::Value jvData;
    jvData["sData"] = 123;
    jvData >> eData;
}
catch (const std::exception& e)
{
    std::cout << "exception info:" << e.what() << std::endl;
}
```