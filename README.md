# TinyJson

## 0、概述

TinyJson是一个轻量级的C++库，专注于**Json数据处理**和**结构体与Json互转**，具有以下特点：

- **轻量高效**：仅包含几个头文件，使用C++14特性，几百行代码实现完整的Json处理功能
- **易于使用**：通过简单的宏定义，即可实现结构体与Json格式的无缝互转
- **功能丰富**：支持Json解析、生成、文件读写、注释处理等多种功能
- **类型安全**：支持多种数据类型，包括基本类型、枚举、容器、智能指针等

## 1、项目结构

TinyJson项目由以下文件组成：

- `json.hpp`：核心Json处理类，实现Json的解析、生成和操作
- `jsonConverter.hpp`：结构体与Json互转功能，通过宏定义实现
- `StdOptional.hpp`：C++17 std::optional的兼容实现（用于C++14环境）
- `StdStringView.hpp`：C++17 std::string_view的兼容实现（用于C++14环境）
- `main.cpp`：Json处理功能的测试示例
- `main_conv.cpp`：结构体与Json互转功能的测试示例

## 2、Json处理

### 2.1、功能特性

TinyJson的Json处理功能基于[SimpleJSON](https://github.com/nbsdx/SimpleJSON)项目，并做了以下改进：

- **文件操作**：增加Json文件读取/保存接口
- **错误处理**：Json解析失败时，给出详细的错误位置信息（行号和列号）
- **注释支持**：可在Json文档中使用`//`、`/* */`和`#`形式的注释
- **类型支持**：增强了对int64/uint64等数据类型的支持
- **性能优化**：添加了快速输出接口，提高Json序列化速度
- **其他改进**：修复了格式化输出问题及其他bug

### 2.2、基本使用

#### 2.2.1、创建Json对象

```cpp
// 创建空Json对象
Json::Value json;

// 使用初始化列表创建Json对象
Json::Value person = {"name", "John", "age", 30, "isStudent", false};

// 创建数组
Json::Value array = Json::Value::Make_Array(1, 2, 3, 4, 5);
```

#### 2.2.2、访问和修改Json数据

```cpp
// 访问对象成员
std::string name = person["name"].get<std::string>();
int age = person["age"].get<int>();

// 修改对象成员
person["age"] = 31;
person["email"] = "john@example.com";

// 访问数组元素
int firstElement = array[0].get<int>();

// 修改数组元素
array[1] = 100;
// 添加数组元素
array.append(6);
```

#### 2.2.3、Json序列化

```cpp
// 格式化输出（带缩进）
std::string formatted = person.dumpStyle();
std::cout << formatted << std::endl;

// 快速输出（无缩进，更高效）
std::string compact = person.dumpFast();
std::cout << compact << std::endl;
```

#### 2.2.4、文件操作

```cpp
// 从文件加载Json
Json::Value jsonFromFile;
std::string error = jsonFromFile.load_from_file("data.json");
if (!error.empty()) {
    std::cout << "Error loading JSON: " << error << std::endl;
}

// 保存Json到文件
bool success = jsonFromFile.save_to_file("output.json");
if (!success) {
    std::cout << "Error saving JSON to file" << std::endl;
}
```

#### 2.2.5、解析Json字符串

```cpp
// 解析Json字符串
Json::Value jsonFromString;
std::string jsonStr = R"({"name": "John", "age": 30})";
std::string parseError = jsonFromString.load(jsonStr);
if (!parseError.empty()) {
    std::cout << "Parse error: " << parseError << std::endl;
}
```

## 3、Json与结构体转换

### 3.1、支持的类型

TinyJson通过`jsonConverter.hpp`实现结构体与Json的互转，支持以下类型：

- **基本类型**：字符串、整型、浮点型、布尔型等
- **枚举类型**：自动转换为底层整数类型
- **Json类型**：直接支持Json::Value类型
- **自定义结构体**：已使用宏定义注册的结构体
- **智能指针**：std::shared_ptr和std::unique_ptr
- **可选类型**：std::optional
- **容器类型**：std::vector、std::list
- **映射类型**：std::map（key必须为std::string）
- <font color=red>**支持多层模板嵌套**</font>：可以定义类似std::map<std::string, std::vector<std::optional<std::shared_ptr<std::string>>>>成员

### 3.2、使用方法

#### 3.2.1、结构体注册

使用`ADD_JSON_MEMBER`宏注册结构体成员：

```cpp
// 定义枚举类型
enum class EM_type
{
    TYPE_1,
    TYPE_2
};

// 基本结构体
struct BaseData
{
    std::optional<int32_t> lData;
    std::optional<double> dData;
    std::string str = "你好";
    float fData = 4.6;
    EM_type ty1 = EM_type::TYPE_2;
    Json::Value jvData;
    ADD_JSON_MEMBER(lData, dData, str, fData, ty1, jvData);
};

// 继承结构体
struct DeriveData : public BaseData
{
    double dValue = 12.9;
    uint64_t uintMax = 12;
    ADD_JSON_MEMBER_INHERIT(BaseData, dValue, uintMax);
};

// 包含复杂类型的结构体
struct WrapData
{
    std::list<DeriveData> listDataDer;     // 派生类数组
    std::optional<BaseData> optBase;       // 可选基类
    std::optional<DeriveData> optDerive;   // 可选派生类
    std::shared_ptr<BaseData> spBase;      // 智能指针
    std::vector<int> vecData;              // 整型数组
    std::map<std::string, std::string> mapData; // 字符串映射
    float fValue = 2.9;
    ADD_JSON_MEMBER(listDataDer, optBase, optDerive, spBase, vecData, mapData, fValue);
};
```

#### 3.2.2、结构体与Json互转

使用操作符`<<`和`>>`进行转换：

```cpp
// 创建并初始化结构体
WrapData data;
data.fValue = 3.14;
data.vecData = {1, 2, 3, 4, 5};
data.mapData["key1"] = "value1";
data.mapData["key2"] = "value2";
data.spBase = std::make_shared<BaseData>();
data.spBase->str = "Shared Base";

// 结构体转Json
Json::Value outJson;
outJson << data;

// 输出Json
std::cout << outJson.dumpStyle() << std::endl;

// Json转结构体
WrapData newData;
outJson >> newData;

// 使用转换后的数据
std::cout << "fValue: " << newData.fValue << std::endl;
std::cout << "vecData size: " << newData.vecData.size() << std::endl;
```

#### 3.2.3、异常处理

在Json转结构体时，除了`std::optional`和容器类型外，其他类型的数据必须在Json中存在，否则会抛出异常：

```cpp
// 定义测试结构体
struct ErrorTest
{
    uint16_t sData = 65535;              // 必须存在
    std::optional<int32_t> opLData = 0;   // 可选
    float fData = 0.3;                    // 必须存在
    ADD_JSON_MEMBER(sData, opLData, fData);
};

// 测试异常
 try
 {
     ErrorTest eData;
     Json::Value jvData;
     jvData["sData"] = 123;
     // 故意不设置fData，会抛出异常
     jvData >> eData;
 }
 catch (const std::exception& e)
 {
     std::cout << "Exception: " << e.what() << std::endl;
     // 输出: Exception: Key [fData] do not exist.
 }
```

## 4、技术特点

### 4.1、模板元编程

TinyJson充分利用了C++模板元编程技术，实现了：

- **类型推导**：自动识别不同类型的数据并进行相应处理
- **编译时检查**：在编译时检查类型是否支持转换
- **递归处理**：通过递归模板函数处理复杂的数据结构

### 4.2、错误处理

- **详细的错误信息**：解析失败时提供行号和列号
- **异常机制**：使用标准异常机制处理错误情况
- **类型安全**：在类型转换失败时抛出异常

## 5、性能特点

- **快速解析**：优化了解析算法，提高解析速度
- **高效序列化**：提供快速输出接口，减少序列化开销
- **内存高效**：紧凑的数据结构设计，减少内存使用
- **零依赖**：不依赖第三方库，仅使用标准C++库

- **C++标准**：C++14或更高版本
- **编译器**：支持C++14的编译器（目前测试了GCC 5+、MSVC 2015+）
- **依赖**：无第三方依赖，仅使用标准C++库

## 6、示例代码

项目中包含两个测试文件：

- `main.cpp`：测试Json处理功能，包括解析、生成、文件操作等
- `main_conv.cpp`：测试结构体与Json互转功能，包括各种类型的转换

## 7、总结

TinyJson是一个轻量、高效、易用的C++ Json处理库，通过简洁的API和强大的模板技术，为C++开发者提供了便捷的Json处理和结构体转换功能。它的设计理念是"简单易用，功能强大"，适合各种规模的项目使用。

无论是处理简单的配置文件，还是复杂的数据结构，TinyJson都能轻松应对，为开发者节省时间和精力。