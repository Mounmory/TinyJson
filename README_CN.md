TinyJson
-------

### 0、概述

 `TinyJson`是<font color="red">处理Json数据</font>及<font color="red">结构体与Json转换</font>的C++库，特点如下：

- `TinyJson`是一个轻量化的的处理Json数据C++库，它只有一个头文件，使用C++14特性，仅用几百行代码实现Json格式数据处理。
- 通过应用C++模板特性，几百行代码实现了在类或结构体中添加一个宏定义，就能轻松实现结构体与Json格式的互转。

### 1、Json处理

#### 1.1、说明

源码参考项目[SimpleJSON](https://github.com/nbsdx/SimpleJSON)，并做了如下改进：

- 增加Json文件读取/保存接口

- Json解析失败时，给出错误位置信息

- 增加读取Json数据注释处理，可在Json文档中使用“//”等形式进行注释

- 其它一些优化项及诸多bug处理，详见源码`json.hpp`


#### 1.2、使用示例

  `main.cpp`中测试了解析错误的Json字符串、解析Json文件、解析带注释的Json数据、格式化输出、快速输出、赋值、移动、int64/uint64数据类型、列表初始化等使用方法。

### 2、Json与结构体转换

#### 2.1、说明

  `JsonConverter.hpp`中实现通过宏定义，将结构体与Json之间实现轻松互转，结构体内成员函数支持如下类型

- 字符串及数字类型（std::string、整型、浮点型、布尔型等）
- 枚举类型
- Josn类型
- 已经使用宏定义了结构转化的自定义数据结构
- 数组（std::vector、std::list），模板参数可以是基本数据类型及定义转换接口的结构体
- std::optional，模板参数可以是基本数据类型及定义转换接口的结构体

  暂不支持智能指针类型（主要涉及多态，指向派生类的基类指针，可以生成包含基类数据的Json，但通过包含基类数据的Json生成指向派生对象的基类指针比较复杂），将一个包含派生类数据的Json，使用基类对象解析后，只会保留基类部分数据

 #### 2.2、使用示例

##### 2.2.1、结构体成员注册

`jsonConverter.hpp`头文件中定义了2个可变参数宏宏，

- `ADD_JSON_MEMBER`：用于注册结构体中成员对象
- `ADD_JSON_MEMBER_INHERIT`：指定父类，其中父类必须使用了注册宏，这样就能实现父类的数据的转换功能。

```cpp
//定义枚举类型
enum class EM_type
{
	TYPE_1,
	TYPE_2
};

//一个类，可作为基类
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

//继承自基类
struct DeriveData :public  BaseData
{
	double dValue = 12.9;
	uint64_t uintMax = 12;
	ADD_JSON_MEMBER_INHERIT(BaseData, dValue, uintMax);
};

//包含结构体的类
struct WrapData
{
	std::list<DeriveData> listDataDer;//派生类数组
	std::optional<BaseData> optBase;//保存派生类中基类
	std::optional<DeriveData> optDerive;//派生类
	std::optional<DeriveData> optDataEmpty;//保存空的
	float fValue = 2.9;
	ADD_JSON_MEMBER(listDataDer, optBase, optDerive, optDataEmpty, fValue);
};
```

##### 2.2.2、结构体与Json互转

在`Json::Value`类中通过重载操作符`>>`及`<<`，可以对结构体与Json轻松转换

```cpp
WrapData data;
Json::Value outJson;
//结构体赋值操作
....
outJson << data;//结构体转Json

WrapData outWrap;
outJson >> outWrap;//Json转结构体
```

##### 2.2.3、异常处理

在Json转结构体中，数据成员及子成员中，除了std::optional及数组（std::vector、std::list）类型外，其他类型数据必须在Json中存在，否则抛出异常

```cpp
//定义ErrorTest类，在Json转结构体时，成员sData及fData数据必须存在
struct ErrorTest
{
	uint16_t sData = 65535;
	std::optional<int32_t> opLData = 0;
	float fData = 0.3;
	ADD_JSON_MEMBER(sData, opLData, fData);
};
```

测试代码如下

```
try
{
    std::cout << "/******************  非option成员为空，异常  ********************/" << std::endl;
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

