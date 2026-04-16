/**
 * @file JsonConverter.hpp
 * @brief Json与结构体转换处理函数
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date
 */

#ifndef MMR_UTIL_JSON_CONVERTER_HPP
#define MMR_UTIL_JSON_CONVERTER_HPP
#include "json.hpp"
#include "StdOptional.hpp"
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <list>

/*
	JsonConverter实现通过宏定义，将结构体与Json之间实现轻松互转
	结构体内成员函数支持如下类型
		- 数字类型（整型、浮点型、布尔型等（暂不支uint64）
		- 枚举类型
		- Josn类型
		- 已经定义了结构转化的其它结构体
		- std::shared_ptr或std::unique_ptr类型，模板参数可以时基本数据类型及定义转换接口的结构体，对于继承类型，基类指针不会处理派生类数据类型
		- std::optional，模板参数可以时基本数据类型及定义转换接口的结构体
		- 数组（std::vector、std::list），模板参数可以时基本数据类型及定义转换接口的结构体
		- std::map，key必须为std::string,value必须是可以时基本数据类型及定义转换接口的结构体
	暂不支持智能指针类型（主要涉及多态，指向派生类的基类指针，可以生成包含基类数据的Json，但通过包含基类数据的Json生成指向派生对象的基类指针比较复杂）
	将一个包含派生类数据的Json，使用基类对象解析后，只保留基类部分数据
*/


namespace mmrUtil
{

//获取包装下一层的类型
template<typename T>
struct extract_value_type { using type = T; };
template<typename T>
struct extract_value_type<std::vector<T>> { using type = T; };
template<typename T>
struct extract_value_type<std::list<T>> { using type = T; };
template<typename T>
struct extract_value_type<std::shared_ptr<T>> { using type = T; };
template<typename T>
struct extract_value_type<std::unique_ptr<T>> { using type = T; };
template<typename T>
struct extract_value_type<std::optional<T>> { using type = T; };
template<typename T>
struct extract_value_type<std::map<std::string, T>> { using type = T; };
template<typename T>
using extract_value_type_t = typename extract_value_type<T>::type;

//获取多层模板嵌套的最底层类型
template<typename T>
struct basic_value_type { using type = T; };
template<typename T>
struct basic_value_type<std::vector<T>> { using type = typename basic_value_type<T>::type; };
template<typename T>
struct basic_value_type<std::list<T>> { using type = typename basic_value_type<T>::type; };
template<typename T>
struct basic_value_type<std::shared_ptr<T>> { using type = typename basic_value_type<T>::type; };
template<typename T>
struct basic_value_type<std::unique_ptr<T>> { using type = typename basic_value_type<T>::type; };
template<typename T>
struct basic_value_type<std::optional<T>> { using type = typename basic_value_type<T>::type; };
template<typename T>
struct basic_value_type<std::map<std::string, T>> { using type = typename basic_value_type<T>::type; };
template<typename T>
using basic_value_type_t = typename basic_value_type<T>::type;

//去除前后空格
inline std::string trim(const char* start, const char* end)
{
	while (start < end && std::isspace(static_cast<unsigned char>(*start))) 
		++start;
	while (end > start && std::isspace(static_cast<unsigned char>(*(end - 1))))
		--end;
	return std::string(start, end);
}

//是否为json互转方法的结构体有Json互转方法
template<typename T, typename = void>
struct enable_json_convert : std::false_type {};
template<typename T>
struct enable_json_convert<T,
	decltype(std::declval<T>().generateJson()
		//, std::declval<T>().generateJson((std::declval<Json::Value&>()))
		, std::declval<T>().parseJson(std::declval<const Json::Value&>())
		, void())> //包含generateJson和parseJson接口
	:std::true_type {};

//可直接转换为Json类型
template<typename T>
struct is_convertable_to_json_type
{
	static constexpr bool value = std::is_arithmetic<std::decay_t<T>>::value//基本数字类型
		|| std::is_same<std::decay_t<T>, std::string>::value//string类型
		|| std::is_same<std::decay_t<T>, Json::Value>::value//Json类型
		|| std::is_enum<std::decay_t<T>>::value//枚举类型
		|| enable_json_convert<T>::value;//定义了转化接口的结构体
};

//是否为shared_ptr或者unique_ptr
template<typename T>
struct is_smart_ptr : std::false_type {};
template<typename T>
struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};
template<typename T>
struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};

//是否为optional
template<typename T>
struct is_optional :std::false_type {};
template<typename T>
struct is_optional<std::optional<T>> : std::true_type {};

//是否为vector或list
template<typename T>
struct is_std_list : std::false_type {};
template<typename T>
struct is_std_list<std::vector<T>> : std::true_type {};
template<typename T>
struct is_std_list<std::list<T>> : std::true_type {};

//是否为map
template<typename T>
struct is_map :std::false_type {};
template<typename T>
struct is_map<std::map<std::string, T>> : std::true_type {};


/*****************************************************************/
/*                          数据类型转Json                       */
/*****************************************************************/
//基本数据类型、string和Json::Value类型
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<std::is_arithmetic<std::decay_t<_Ty>>::value
	|| std::is_same<std::decay_t<_Ty>, std::string>::value
	|| std::is_same<std::decay_t<_Ty>, Json::Value>::value
	, Json::Value>::type
{
	return  data;
}

//枚举类型
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<std::is_enum<_Ty>::value, Json::Value>::type
{
	using Type = typename std::underlying_type<_Ty>::type;
	return static_cast<Type>(data);
}

//自定义了转Json接口的结构体
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<enable_json_convert<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type<_Ty>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");
	return data.generateJson();
}

//shared_ptr或者unique_ptr值
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<is_smart_ptr<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");
	if(nullptr == data)
		return Json::Value();
	else 
		return toJson<extract_type>(*data);
}

//optional，有值或没有值
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<is_optional<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");
	if (data.has_value())
	{
		return toJson<extract_type>(data.value());
	}
	return Json::Value();
}

//容器数组
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<is_std_list<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");	Json::Value jvRet;
	for (const auto& iterData : data)
	{
		jvRet.append(toJson<extract_type>(iterData));
	}
	return jvRet;
}

//map转Json Object
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<is_map<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");
	Json::Value jvRet;
	for (const auto& iterData : data)
	{
		jvRet[iterData.first] = toJson<extract_type>(iterData.second);
	}
	return jvRet;
}

/*****************************************************************/
/*                          Json转数据类型                       */
/*****************************************************************/
//数字或字符串类型
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData) ->typename std::enable_if<std::is_arithmetic<_Ty>::value
	|| std::is_same<_Ty, std::string>::value, _Ty>::type
{
	return jvData.get<_Ty>();
}

//JSON类型
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData) ->typename std::enable_if<std::is_same<_Ty, Json::Value>::value, Json::Value>::type
{
	return jvData;
}

//枚举类型
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<std::is_enum<_Ty>::value, _Ty>::type
{
	using Type = typename std::underlying_type<_Ty>::type;
	return static_cast<_Ty>(jvData.toNum<Type>());
}

//自定义了转Json接口的结构体
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<enable_json_convert<_Ty>::value, _Ty>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");
	_Ty data;
	data.parseJson(jvData);
	return data;
}

//shar_ptr或unique_ptr
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<is_smart_ptr<_Ty>::value, _Ty>::type 
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");
	if (jvData.IsNull())
		return nullptr;
	else
		return std::make_unique<extract_type>(fromJson<extract_type>(jvData));
}

//optional
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<is_optional<_Ty>::value, _Ty>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");
	if (jvData.IsNull())
		return std::nullopt;
	else
		return fromJson<extract_type>(jvData);
}

//容器数组
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<is_std_list<_Ty>::value, _Ty>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");	_Ty retArray;
	for (const auto& iterJv : jvData.ArrayRange())
	{
		retArray.emplace_back(fromJson<extract_type>(iterJv));
	}
	return retArray;
}

//map
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<is_map<_Ty>::value, _Ty>::type
{
	using extract_type = typename extract_value_type_t<_Ty>;
	using basic_type = typename basic_value_type_t<_Ty>;
	static_assert(is_convertable_to_json_type<basic_type>::value, "basic type error");
	_Ty mapRet;
	for (const auto& iterMap : jvData.ObjectRange())
	{
		mapRet[iterMap.first] = fromJson<extract_type>(iterMap.second);
	}
	return mapRet;
}

/*****************************************************************/
/*                         类型数据转Json函数                    */
/*****************************************************************/
// 基础情况
inline void generatJsonVars(Json::Value& jvData, const char* name) {}

// 递归模板函数
template<typename T, typename... Args>
inline void generatJsonVars(Json::Value& jvData, const char* name, const T& value, Args&&... args)
{
	// 提取当前变量名（直到第一个逗号或结束）
	const char* comma = strchr(name, ',');
	if (comma)
	{
		jvData[trim(name, comma)] = toJson(value);
		generatJsonVars(jvData, comma + 1, args...);
	}
	else
	{
		jvData[trim(name, name + strlen(name))] = toJson(value);
	}
}

/*****************************************************************/
/*                        Json转数据类型函数                     */
/*****************************************************************/
inline void parseJsonVars(const Json::Value& jvData, const char* name) {}

// 递归模板函数
template<typename T, typename... Args>
inline void parseJsonVars(const Json::Value& jvData, const char* name, T& value, Args&&... args)
{
	// 提取当前变量名（直到第一个逗号或结束）
	using dataType = std::decay_t<T>;
	const char* comma = strchr(name, ',');
	if (comma)
	{
		std::string strKey = trim(name, comma);
		if (jvData.hasKey(strKey))
			value = fromJson<dataType>(jvData.at(strKey));
		else if (!is_optional<dataType>::value && !is_std_list<dataType>::value && !is_smart_ptr<dataType>::value)
			throw std::runtime_error(std::string("Key [" + strKey + "] do not exist."));
		parseJsonVars(jvData, comma + 1, args...);
	}
	else
	{
		std::string strKey = trim(name, name + strlen(name));
		if (jvData.hasKey(strKey))
			value = fromJson<dataType>(jvData.at(strKey));
		else if (!is_optional<dataType>::value && !is_std_list<dataType>::value && !is_smart_ptr<dataType>::value)
			throw std::runtime_error(std::string("Key [" + strKey + "] do not exist."));
	}
}

}

//template<typename _Ty>
//inline auto operator << (Json::Value& jvData, const _Ty& data) ->typename std::enable_if<mmrUtil::enable_json_convert<_Ty>::value, Json::Value&>::type
//{
//	jvData.clear();
//	data.generateJson(jvData);
//	return jvData;
//}
//
//template<typename _Ty>
//inline auto operator >> (Json::Value& jvData, _Ty& data) ->typename std::enable_if<mmrUtil::enable_json_convert<_Ty>::value, Json::Value&>::type
//{
//	data.parseJson(jvData);
//	return jvData;
//}

#define ADD_JSON_MEMBER(...) 	\
public:\
Json::Value generateJson() const\
{\
	Json::Value ANameShouldNotBeSameWithStructMember23af00fa9;\
	mmrUtil::generatJsonVars(ANameShouldNotBeSameWithStructMember23af00fa9, #__VA_ARGS__, __VA_ARGS__);\
	return ANameShouldNotBeSameWithStructMember23af00fa9;\
}\
void parseJson(const Json::Value& ANameShouldNotBeSameWithStructMember23af00fa9)\
{\
	mmrUtil::parseJsonVars(ANameShouldNotBeSameWithStructMember23af00fa9, #__VA_ARGS__, __VA_ARGS__); \
}\
protected:\
friend class Json::Value;\
void generateJson(Json::Value& ANameShouldNotBeSameWithStructMember23af00fa9) const\
{\
	mmrUtil::generatJsonVars(ANameShouldNotBeSameWithStructMember23af00fa9, #__VA_ARGS__, __VA_ARGS__); \
}


#define ADD_JSON_MEMBER_INHERIT(Base, ...) 	\
public:\
Json::Value generateJson() const\
{\
	Json::Value ANameShouldNotBeSameWithStructMember23af00fa9;\
	Base::generateJson(ANameShouldNotBeSameWithStructMember23af00fa9);\
	mmrUtil::generatJsonVars(ANameShouldNotBeSameWithStructMember23af00fa9, #__VA_ARGS__, __VA_ARGS__);\
	return ANameShouldNotBeSameWithStructMember23af00fa9;\
}\
void parseJson(const Json::Value& ANameShouldNotBeSameWithStructMember23af00fa9)\
{\
	Base::parseJson(ANameShouldNotBeSameWithStructMember23af00fa9);\
	mmrUtil::parseJsonVars(ANameShouldNotBeSameWithStructMember23af00fa9, #__VA_ARGS__, __VA_ARGS__); \
}\
protected:\
friend class Json::Value;\
void generateJson(Json::Value& ANameShouldNotBeSameWithStructMember23af00fa9) const\
{\
	Base::generateJson(ANameShouldNotBeSameWithStructMember23af00fa9);\
	mmrUtil::generatJsonVars(ANameShouldNotBeSameWithStructMember23af00fa9, #__VA_ARGS__, __VA_ARGS__); \
}

#endif