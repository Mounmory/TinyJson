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
		- 数组（std::vector、std::list），模板参数可以时基本数据类型及定义转换接口的结构体
		- std::optional，模板参数可以时基本数据类型及定义转换接口的结构体
	暂不支持智能指针类型（主要涉及多态，指向派生类的基类指针，可以生成包含基类数据的Json，但通过包含基类数据的Json生成指向派生对象的基类指针比较复杂）
	将一个包含派生类数据的Json，使用基类对象解析后，只保留基类部分数据
*/


namespace mmrUtil
{
//是否为json互转方法的结构体有Json互转方法
template<typename T, typename = void>
struct enable_json_convert : std::false_type {};
template<typename T>
struct enable_json_convert<T,
	decltype(std::declval<T>().generateJson(), std::declval<T>().parseJson(std::declval<const Json::Value&>()), void())> //包含generateJson和parseJson接口
	:std::true_type {};

//是否为容器
template<typename T>
struct is_std_container : std::false_type {};
template<typename T>
struct is_std_container<std::vector<T>> : std::true_type {};
template<typename T>
struct is_std_container<std::list<T>> : std::true_type {};

//是否为optional
template<typename T>
struct is_optional :std::false_type {};
template<typename T>
struct is_optional<std::optional<T>> : std::true_type {};

//获取包装下的基础类型
template<typename T>
struct extract_value_type { using type = T; };
template<typename T>
struct extract_value_type<std::vector<T>> { using type = T; };
template<typename T>
struct extract_value_type<std::list<T>> { using type = T; };
template<typename T>
struct extract_value_type<std::optional<T>> { using type = T; };

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


inline std::string trim(const char* start, const char* end)
{
	while (start < end && std::isspace(static_cast<unsigned char>(*start))) {
		++start;
	}
	while (end > start && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
		--end;
	}
	return std::string(start, end);
}

/*****************************************************************/
/*                          数据类型转Json                       */
/*****************************************************************/
#if  0//__cplusplus >= CPP_17 //暂时不用cpp17特性
template<typename _Ty>
inline Json::Value toJson(const _Ty& data)
{
	using extract_type = typename extract_value_type<std::decay_t<_Ty>>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");

	if constexpr (std::is_arithmetic<std::decay_t<_Ty>>::value
		|| std::is_same<std::decay_t<_Ty>, std::string>::value
		|| std::is_same<std::decay_t<_Ty>, Json::Value>::value)
	{
		return  data;
	}
	else if constexpr (std::is_enum<std::decay_t<_Ty>>::value)
	{
		using Type = std::underlying_type_t<_Ty>;
		return static_cast<Type>(data);
	}
	else if constexpr (enable_json_convert<_Ty>::value)
	{
		return data.generateJson();
	}
	else if constexpr (is_std_container<_Ty>::value)
	{
		Json::Value jvRet;
		for (const auto& iterData : data)
		{
			jvRet.append(toJson(iterData));
		}
		return jvRet;
	}
	else if constexpr (is_optional<_Ty>::value)
	{
		if (data.has_value())
		{
			return toJson(data.value());
		}
		return Json::Value();
	}
	else
	{
		static_assert([] { return false; }(), "invalid data type");
	}
}

#else
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
inline auto toJson(const _Ty& data)->typename std::enable_if<std::is_enum<std::decay_t<_Ty>>::value, Json::Value>::type
{
	using Type = std::underlying_type_t<_Ty>;
	return static_cast<Type>(data);
}

//自定义了转Json接口的结构体
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<enable_json_convert<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type<std::decay_t<_Ty>>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");
	return data.generateJson();
}

//容器数组
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<is_std_container<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type<std::decay_t<_Ty>>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");
	Json::Value jvRet;
	for (const auto& iterData : data)
	{
		jvRet.append(toJson(iterData));
	}
	return jvRet;
}

//optional，有值或没有值
template<typename _Ty>
inline auto toJson(const _Ty& data)->typename std::enable_if<is_optional<_Ty>::value, Json::Value>::type
{
	using extract_type = typename extract_value_type<std::decay_t<_Ty>>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");
	if (data.has_value())
	{
		return toJson(data.value());
	}
	return Json::Value();
}
#endif


/*****************************************************************/
/*                          Json转数据类型                       */
/*****************************************************************/
#if 0 //__cplusplus >= CPP_17 //暂时不用cpp17特性
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)
{
	using retType = std::decay_t<_Ty>;
	using extract_type = typename extract_value_type<retType>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");

	if constexpr (std::is_arithmetic<retType>::value || std::is_same<retType, std::string>::value)
	{
		return jvData.get<retType>();
	}
	else if constexpr (std::is_same<retType, Json::Value>::value)
	{
		return jvData;
	}
	else if constexpr (std::is_enum<retType>::value)
	{
		return static_cast<_Ty>(jvData.toNum<std::underlying_type_t<_Ty>>());
	}
	else if constexpr (enable_json_convert<retType>::value)
	{
		retType data;
		data.parseJson(jvData);
		return data;
	}
	else if constexpr (is_std_container<retType>::value)
	{
		retType retArray;
		for (const auto& iterJv : jvData.ArrayRange())
		{
			retArray.emplace_back(fromJson<extract_type>(iterJv));
		}
		return retArray;
	}
	else if constexpr (is_optional<retType>::value)
	{
		return fromJson<extract_type>(jvData);
	}
	else
	{
		static_assert([] { return false; }(), "invalid data type");
	}
}
#else
//数字或字符串类型
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData) ->typename std::enable_if<std::is_arithmetic<std::decay_t<_Ty>>::value
	|| std::is_same<std::decay_t<_Ty>, std::string>::value, _Ty>::type
{
	return jvData.get<std::decay_t<_Ty>>();
}

//JSON类型
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData) ->typename std::enable_if<std::is_same<std::decay_t<_Ty>, Json::Value>::value, Json::Value>::type
{
	return jvData;
}

//枚举类型
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<std::is_enum<std::decay_t<_Ty>>::value, std::decay_t<_Ty>>::type
{
	using Type = std::underlying_type_t<_Ty>;
	return static_cast<std::decay_t<_Ty>>(jvData.toNum<Type>());
}

//自定义了转Json接口的结构体
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<enable_json_convert<std::decay_t<_Ty>>::value, std::decay_t<_Ty>>::type
{
	using extract_type = typename extract_value_type<std::decay_t<_Ty>>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");
	std::decay_t<_Ty> data;
	data.parseJson(jvData);
	return data;
}


//容器数组
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<is_std_container<std::decay_t<_Ty>>::value, std::decay_t<_Ty>>::type
{
	using extract_type = typename extract_value_type<std::decay_t<_Ty>>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");
	std::decay_t<_Ty> retArray;
	for (const auto& iterJv : jvData.ArrayRange())
	{
		retArray.emplace_back(fromJson<extract_type>(iterJv));
	}
	return retArray;
}

//optional
template<typename _Ty>
inline auto fromJson(const Json::Value& jvData)->typename std::enable_if<is_optional<std::decay_t<_Ty>>::value, std::decay_t<_Ty>>::type
{
	using extract_type = typename extract_value_type<std::decay_t<_Ty>>::type;
	static_assert(is_convertable_to_json_type<extract_type>::value, "subtype error");
	if (jvData.IsNull())
		return std::nullopt;
	else
		return fromJson<extract_type>(jvData);
}
#endif

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
		else if (!is_optional<dataType>::value && !is_std_container<dataType>::value)
			throw std::runtime_error(std::string("Key [" + strKey + "] do not exist."));
		parseJsonVars(jvData, comma + 1, args...);
	}
	else
	{
		std::string strKey = trim(name, name + strlen(name));
		if (jvData.hasKey(strKey))
			value = fromJson<dataType>(jvData.at(strKey));
		else if (!is_optional<dataType>::value && !is_std_container<dataType>::value)
			throw std::runtime_error(std::string("Key [" + strKey + "] do not exist."));
	}
}

}

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
void generateJson(Json::Value& ANameShouldNotBeSameWithStructMember23af00fa9) const\
{\
	Base::generateJson(ANameShouldNotBeSameWithStructMember23af00fa9);\
	mmrUtil::generatJsonVars(ANameShouldNotBeSameWithStructMember23af00fa9, #__VA_ARGS__, __VA_ARGS__); \
}

#endif