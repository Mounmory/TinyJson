/**
 * @file json.hpp
 * @brief Json处理类
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date 
 *
 *
 * 源码参考：https://github.com/nbsdx/SimpleJSON
 * 修改内容：
 * - 修改源码格式化输出有问题
 * - 增加快速输出接口
 * - Json解析失败时，给出错误位置信息
 * - 增加读取Json数据注释处理，可在Json文档中使用“//”等形式进行注释
 * - Int数据支持了int64
 * - 其它一些优化项及bug处理
 */

#ifndef MMR_UTIL_JSON_HPP
#define MMR_UTIL_JSON_HPP

#include <cstdint>
#include <cmath>
#include <cctype>
#include <string>
#include <deque>
#include <map>
#include <type_traits>
#include <initializer_list>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Json {

class Value
{
	union BackingData { //定义内联数据
		BackingData(double d) : Float(d) {}
		BackingData(int64_t   l) : Int(l) {}
		BackingData(bool   b) : Bool(b) {}
		BackingData(std::string s) : String(new std::string(std::move(s))) {}
		BackingData() : Int(0) {}

		std::deque<Value>        *List;
		std::map<std::string, Value>   *Map;
		std::string             *String;
		double              Float;
		int64_t            Int;
		bool                Bool;
	} ;

public:
	enum class emJsonType : uint8_t	{
		Null,
		Object,
		Array,
		String,
		Floating,
		Integral,
		Boolean
	};

	template <typename Container>//Arrry或Objec迭代器
	class JSONWrapper {
		Container *object;

	public:
		JSONWrapper(Container *val) : object(val) {}
		JSONWrapper(std::nullptr_t) : object(nullptr) {}

		typename Container::iterator begin() { return object ? object->begin() : typename Container::iterator(); }
		typename Container::iterator end() { return object ? object->end() : typename Container::iterator(); }
		typename Container::const_iterator begin() const { return object ? object->begin() : typename Container::iterator(); }
		typename Container::const_iterator end() const { return object ? object->end() : typename Container::iterator(); }
	};

	template <typename Container>
	class JSONConstWrapper {
		const Container *object;

	public:
		JSONConstWrapper(const Container *val) : object(val) {}
		JSONConstWrapper(std::nullptr_t) : object(nullptr) {}

		typename Container::const_iterator begin() const { return object ? object->begin() : typename Container::const_iterator(); }
		typename Container::const_iterator end() const { return object ? object->end() : typename Container::const_iterator(); }
	};

	Value() : Internal(), Type(emJsonType::Null) {}

	Value(std::initializer_list<Value> list)
		: Value()
	{
		SetType(emJsonType::Object);
		for (auto i = list.begin(), e = list.end(); i != e; ++i, ++i)
			operator[](i->toString()) = *std::next(i);
	}

	Value(Value&& other)
		: Internal(other.Internal)
		, Type(std::exchange(other.Type, emJsonType::Null))
	{
		other.Internal.Map = nullptr;
	}

	Value& operator = (Value&& other) {
		if (this != &other)
		{
			ClearInternal();
			Internal = other.Internal;
			Type = other.Type;
			other.Internal.Map = nullptr;
			other.Type = emJsonType::Null;
		}
		return *this;
	}

	Value(const Value &other) {
		switch (other.Type) {
		case emJsonType::Object:
			Internal.Map =
				new std::map<std::string, Value>(other.Internal.Map->begin(),
					other.Internal.Map->end());
			break;
		case emJsonType::Array:
			Internal.List =
				new std::deque<Value>(other.Internal.List->begin(),
					other.Internal.List->end());
			break;
		case emJsonType::String:
			Internal.String =
				new std::string(*other.Internal.String);
			break;
		default:
			Internal = other.Internal;
		}
		Type = other.Type;
	}

	Value& operator = (const Value &other) {
		if (this != &other) 
		{
			ClearInternal();
			switch (other.Type) {
			case emJsonType::Object:
				Internal.Map =
					new std::map<std::string, Value>(other.Internal.Map->begin(),
						other.Internal.Map->end());
				break;
			case emJsonType::Array:
				Internal.List =
					new std::deque<Value>(other.Internal.List->begin(),
						other.Internal.List->end());
				break;
			case emJsonType::String:
				Internal.String =
					new std::string(*other.Internal.String);
				break;
			default:
				Internal = other.Internal;
			}
			Type = other.Type;
		}
		return *this;
	}

	~Value() {
		ClearInternal();
	}

	void clear() {
		ClearInternal();
		Type = emJsonType::Null;
		Internal.Int = 0;
	}

	template <typename T>
	Value(T b, typename std::enable_if<std::is_same<T, bool>::value>::type* = 0)
		: Internal(b)
		, Type(emJsonType::Boolean) {}

	template <typename T>
	Value(T i, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value>::type* = 0)
		: Internal((int64_t)i),
		Type(emJsonType::Integral) {}

	template <typename T>
	Value(T f, typename std::enable_if<std::is_floating_point<T>::value>::type* = 0)
		: Internal((double)f)
		, Type(emJsonType::Floating) {}

	template <typename T>
	Value(T&& s, typename std::enable_if<std::is_convertible<T, std::string>::value>::type* = 0)
		: Internal(std::string(std::forward<T>(s)))//显式的转换为std::string
		, Type(emJsonType::String) {}
	
	Value(std::nullptr_t) : Internal(), Type(emJsonType::Null) {}

	static Value Make(emJsonType type) {
		Value ret; 
		ret.SetType(type);
		return ret;
	}

	template <typename... _T>//构建数组
	static	Value Make_Array(_T&&... args){
		Value arr = Value::Make(Value::emJsonType::Array);
		arr.append(std::forward<_T>(args)...);
		return arr;
	}

	template <typename T>
	void append(T arg) {
		SetType(emJsonType::Array); Internal.List->emplace_back(arg);
	}

	template <typename T, typename... U>
	void append(T arg, U... args) {
		append(arg); append(args...);
	}

	template <typename T>
	typename std::enable_if<std::is_same<T, bool>::value, Value&>::type operator=(T b) {
		SetType(emJsonType::Boolean); Internal.Bool = b; return *this;
	}

	template <typename T>
	typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, Value&>::type operator=(T i) {
		SetType(emJsonType::Integral); Internal.Int = i; return *this;
	}

	template <typename T>
	typename std::enable_if<std::is_floating_point<T>::value, Value&>::type operator=(T f) {
		SetType(emJsonType::Floating); Internal.Float = f; return *this;
	}

	template <typename T>
	typename std::enable_if<std::is_convertible<T, std::string>::value, Value&>::type operator=(T s) {
		SetType(emJsonType::String); *Internal.String = std::string(s); return *this;
	}

	Value& operator[](const std::string &key) {
		SetType(emJsonType::Object); return Internal.Map->operator[](key);
	}

	Value& operator[](unsigned index) {
		SetType(emJsonType::Array);
		if (index >= Internal.List->size()) Internal.List->resize(index + 1);
		return Internal.List->operator[](index);
	}

	Value &at(const std::string &key) {
		return operator[](key);
	}

	const Value &at(const std::string &key) const {
		return Internal.Map->at(key);
	}

	Value &at(unsigned index) {
		return operator[](index);
	}

	const Value &at(unsigned index) const {
		return Internal.List->at(index);
	}

	int length() const {
		if (Type == emJsonType::Array)
			return Internal.List->size();
		else
			return -1;
	}

	bool hasKey(const std::string &key) const {
		if (Type == emJsonType::Object)
			return Internal.Map->find(key) != Internal.Map->end();
		return false;
	}

	bool eraseKey(const std::string& key) {
		if (Type == emJsonType::Object)
			return Internal.Map->erase(key);
		return false;
	}

	int size() const {
		if (Type == emJsonType::Object)
			return Internal.Map->size();
		else if (Type == emJsonType::Array)
			return Internal.List->size();
		else
			return -1;
	}

	emJsonType JSONType() const { return Type; }

	/// Functions for getting primitives from the Value object.
	bool IsNull() const { return Type == emJsonType::Null; }

	std::string toString() const { bool b; return toString(b); }

	std::string toString(bool &ok) const {
		ok = (Type == emJsonType::String);
		return ok ? json_escape(*Internal.String) : std::string("");
	}

	double toFloat() const { bool b; return toFloat(b); }
	double toFloat(bool &ok) const {
		ok = (Type == emJsonType::Floating);
		return ok ? Internal.Float : 0.0;
	}

	int64_t toInt() const { bool b; return toInt(b); }
	int64_t toInt(bool &ok) const {
		ok = (Type == emJsonType::Integral);
		return ok ? Internal.Int : 0;
	}

	bool toBool() const { bool b; return toBool(b); }
	bool toBool(bool &ok) const {
		ok = (Type == emJsonType::Boolean);
		return ok ? Internal.Bool : false;
	}

	JSONWrapper<std::map<std::string, Value>> ObjectRange() {
		if (Type == emJsonType::Object)
			return JSONWrapper<std::map<std::string, Value>>(Internal.Map);
		return JSONWrapper<std::map<std::string, Value>>(nullptr);
	}

	JSONWrapper<std::deque<Value>> ArrayRange() {
		if (Type == emJsonType::Array)
			return JSONWrapper<std::deque<Value>>(Internal.List);
		return JSONWrapper<std::deque<Value>>(nullptr);
	}

	JSONConstWrapper<std::map<std::string, Value>> ObjectRange() const {
		if (Type == emJsonType::Object)
			return JSONConstWrapper<std::map<std::string, Value>>(Internal.Map);
		return JSONConstWrapper<std::map<std::string, Value>>(nullptr);
	}

	JSONConstWrapper<std::deque<Value>> ArrayRange() const {
		if (Type == emJsonType::Array)
			return JSONConstWrapper<std::deque<Value>>(Internal.List);
		return JSONConstWrapper<std::deque<Value>>(nullptr);
	}

	std::string dumpFast() const {
		std::string strRet;//return value optimization
		switch (Type) {
		case emJsonType::Null:
			strRet = "null";
			break;
		case emJsonType::Object: {
			strRet = "{";
			bool skip = true;
			for (auto &p : *Internal.Map) {
				if (!skip) strRet += ",";
				strRet += ("\"" + p.first + "\":" + p.second.dumpFast());
				skip = false;
			}
			strRet += "}";
			break;
		}
		case emJsonType::Array: {
			strRet = "[";
			bool skip = true;
			for (auto &p : *Internal.List)
			{
				if (!skip) strRet += ",";
				strRet += p.dumpFast();
				skip = false;
			}
			strRet += "]";
			break;
		}
		case emJsonType::String:
			strRet = "\"" + json_escape(*Internal.String) + "\"";
			break;
		case emJsonType::Floating:
			strRet = std::to_string(Internal.Float);
			break;
		case emJsonType::Integral:
			strRet = std::to_string(Internal.Int);
			break;
		case emJsonType::Boolean:
			strRet = Internal.Bool ? "true" : "false";
			break;
		default:
			break;
		}
		return strRet;
	}

	std::string dumpStyle(int depth = 0, std::string tab = "\t") const {
		std::string pad = "";
		for (int i = 0; i < depth; ++i, pad += tab);
		std::string strRet;//return value optimization

		switch (Type) {
		case emJsonType::Null:
			strRet = "null";
			break;
		case emJsonType::Object: {
			//strRet = 0 == depth ? pad + "{\n" : "\n" + pad + "{\n";
			strRet = "{\n" ;
			bool skip = true;
			for (auto &p : *Internal.Map) 
			{
				if (!skip) strRet += ",\n";
				strRet += (pad + tab + "\"" + p.first + "\" : " + p.second.dumpStyle(depth + 1, tab));
				skip = false;
			}
			strRet += ("\n" + pad + "}");
			break;
		}
		case emJsonType::Array: {
			//strRet = 0 == depth ? pad + "[\n" : "\n" + pad + "[\n";
			strRet = "[\n" ;
			std::string childPad = pad + tab;
			bool skip = true;
			for (auto &p : *Internal.List) 
			{
				if (!skip) strRet += ",\n";
				strRet += childPad;
				strRet += p.dumpStyle(depth + 1, tab);
				skip = false;
			}
			strRet += "\n" + pad + "]";
			break;
		}
		case emJsonType::String:
			strRet = "\"" + json_escape(*Internal.String) + "\"";
			break;
		case emJsonType::Floating:
			strRet = std::to_string(Internal.Float);
			break;
		case emJsonType::Integral:
			strRet = std::to_string(Internal.Int);
			break;
		case emJsonType::Boolean:
			strRet = Internal.Bool ? "true" : "false";
			break;
		default:
			break;;
		}
		return strRet;
	}

	std::string load(const std::string &str){
		size_t offset = 0;
		std::string strErr;
		try
		{
			Value temJv = parse_next(str, offset);//防止解析失败改变原有值
			(*this) = std::move(temJv);
		}
		catch (const std::exception& e)
		{
			strErr = e.what();
			uint32_t rowNum = 0;
			uint32_t colNum = 0;
			if (offset > 0)
			{
				size_t posLineEnd = 0;//记录最后一个回车的offset
				do
				{
					auto poslastEnd = str.find_first_of('\n', posLineEnd);
					if (poslastEnd != std::string::npos) {
						++poslastEnd;
						++rowNum;
						if (poslastEnd > offset) {
							colNum = offset - posLineEnd + 1;
							break;
						}
						else
							posLineEnd = poslastEnd;
					}
					else {
						colNum = offset - posLineEnd + 1;
						break;
					}

				} while (posLineEnd < offset);
			}
			strErr = strErr + " row:" + std::to_string(rowNum) + " col:" + std::to_string(colNum);
		}
		return strErr;
	}

	std::string load_from_file(const std::string& strFile) {
		std::string retStr;
		do
		{
			std::string contents;
			std::ifstream input(strFile);
			if (!input.is_open())
			{
				retStr = "open file " + strFile + " failed!";
				break;
			}
			input.seekg(0, std::ios::end);
			contents.reserve(input.tellg());
			input.seekg(0, std::ios::beg);

			contents.assign((std::istreambuf_iterator<char>(input)),
				std::istreambuf_iterator<char>());

			input.close();
			retStr = load(contents);

		} while (false);

		return retStr;
	}

private:
	void SetType(emJsonType type) {
		if (type == Type)
			return;
		ClearInternal();
		switch (type) {
		case emJsonType::Null:      Internal.Map = nullptr;                break;
		case emJsonType::Object:    Internal.Map = new std::map<std::string, Value>(); break;
		case emJsonType::Array:     Internal.List = new std::deque<Value>();     break;
		case emJsonType::String:    Internal.String = new std::string();           break;
		case emJsonType::Floating:  Internal.Float = 0.0;                    break;
		case emJsonType::Integral:  Internal.Int = 0;                      break;
		case emJsonType::Boolean:   Internal.Bool = false;                  break;
		}
		Type = type;
	}

	void ClearInternal() {
		switch (Type) {
		case emJsonType::Object: delete Internal.Map;    break;
		case emJsonType::Array:  delete Internal.List;   break;
		case emJsonType::String: delete Internal.String; break;
		default:;
		}
	}

	std::string json_escape(const std::string &str) const {//转义输出
		std::string output;
		output.reserve(str.capacity());
		for (const auto iter : str)
			switch (iter) {
			case '\"': output += "\\\""; break;
			case '\\': output += "\\\\"; break;
			case '\b': output += "\\b";  break;
			case '\f': output += "\\f";  break;
			case '\n': output += "\\n";  break;
			case '\r': output += "\\r";  break;
			case '\t': output += "\\t";  break;
			default: output += iter; break;
			}
		return output;
	}
	
	void consume_ws(const std::string &str, size_t &offset) {//去掉空格和注释
	
		static const std::string strEndLind = "\n";
		static const std::string strEndComment = "*/";
		while (isspace(str[offset])) ++offset;

		if (str[offset] == '#')//使用“#”注释
		{
			size_t endPos = 0;
			endPos = str.find(strEndLind, offset);
			if (endPos != std::string::npos)
			{
				offset = endPos + 1;
				consume_ws(str, offset);
			}
		}
		else if (str[offset] == '/')
		{
			size_t endPos = 0;
			if (str[offset + 1] == '/')//使用“//”注释
			{
				endPos = str.find(strEndLind, offset);
				if (endPos != std::string::npos)
				{
					offset = endPos + 1;
					consume_ws(str, offset);
				}
			}
			else if (str[offset + 1] == '*')//使用“/*  */”多行注注释
			{
				endPos = str.find(strEndComment, offset);
				if (endPos != std::string::npos)
				{
					offset = endPos + 2;
					consume_ws(str, offset);
				}
			}
		}
	}

	Value parse_object(const std::string &str, size_t &offset) {
		Value Object = Value::Make(Value::emJsonType::Object);

		do
		{
			++offset;
			consume_ws(str, offset);
			if (str[offset] == '}')
			{
				++offset;
				break;;
			}

			while (true)
			{
				Value Key = parse_next(str, offset);
				consume_ws(str, offset);
				if (str[offset] != ':')
				{
					std::stringstream ss;
					ss << "Error: Object: Expected ':', found '" << str[offset] << "'.";
					throw std::invalid_argument(ss.str());
					//break;
				}
				consume_ws(str, ++offset);
				Value Value = parse_next(str, offset);
				Object[Key.toString()] = Value;

				consume_ws(str, offset);
				if (str[offset] == ',')
				{
					++offset;
					continue;
				}
				else if (str[offset] == '}')
				{
					++offset;
					break;
				}
				else
				{
					std::stringstream ss;
					ss << "ERROR: Object: Expected ',' or '}', found '" << str[offset] << "'.";
					throw std::invalid_argument(ss.str());
					//break;
				}
			}
		} while (false);

		return Object;
	}

	Value parse_array(const std::string &str, size_t &offset) {
		Value Array = Value::Make(Value::emJsonType::Array);
		unsigned index = 0;

		do
		{
			++offset;
			consume_ws(str, offset);
			if (str[offset] == ']')
			{
				++offset;
				break;;
			}

			while (true)
			{
				Array[index++] = parse_next(str, offset);
				consume_ws(str, offset);

				if (str[offset] == ',')
				{
					++offset;
					continue;
				}
				else if (str[offset] == ']')
				{
					++offset;
					break;
				}
				else
				{
					std::stringstream ss;
					ss << "ERROR: Array: Expected ',' or ']', found '" << str[offset] << "'.";
					throw std::invalid_argument(ss.str());
				}
			}
		} while (false);
		return Array;
	}

	Value parse_string(const std::string &str, size_t &offset) {
		Value String;
		std::string val;
		for (char c = str[++offset]; c != '\"'; c = str[++offset])
		{
			if (c == '\\') {
				switch (str[++offset]) {
				case '\"': val += '\"'; break;
				case '\\': val += '\\'; break;
				case '/': val += '/'; break;
				case 'b': val += '\b'; break;
				case 'f': val += '\f'; break;
				case 'n': val += '\n'; break;
				case 'r': val += '\r'; break;
				case 't': val += '\t'; break;
				case 'u': {
					val += "\\u";
					for (unsigned i = 1; i <= 4; ++i) {
						c = str[offset + i];
						if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
							val += c;
						else {
							std::stringstream ss;
							ss << "ERROR: String: Expected hex character in unicode escape, found '" << c << "'.";
							throw std::invalid_argument(ss.str());
						}
					}
					offset += 4;
				} break;
				default: val += '\\'; break;
				}
			}
			else
				val += c;
		}
		++offset;
		String = val;
		return String;
	}

	Value parse_number(const std::string &str, size_t &offset) {
		Value Number;
		std::string val, exp_str;
		char c;
		bool isDouble = false;
		int64_t exp = 0;
		while (true) {
			c = str[offset++];
			if ((c == '-') || (c >= '0' && c <= '9'))
				val += c;
			else if (c == '.') {
				val += c;
				isDouble = true;
			}
			else
				break;
		}
		if (c == 'E' || c == 'e') {
			c = str[offset++];
			if (c == '-') { ++offset; exp_str += '-'; }
			while (true) {
				c = str[offset++];
				if (c >= '0' && c <= '9')
					exp_str += c;
				else if (!isspace(c) && c != ',' && c != ']' && c != '}') {
					std::stringstream ss;
					ss << "ERROR: Number: Expected a number for exponent, found '" << c << "'.";
					throw std::invalid_argument(ss.str());
				}
				else
					break;
			}
			exp = std::stol(exp_str);
		}
		else if (!isspace(c) && c != ',' && c != ']' && c != '}'&& c != '#'&& c != '/')
		{
			std::stringstream ss;
			ss << "ERROR: Number: unexpected character '" << c << "'.";
			throw std::invalid_argument(ss.str());
		}
		--offset;

		if (isDouble)
			Number = std::stod(val) * std::pow(10, exp);
		else
		{
			if (!exp_str.empty())
				Number = std::stoll(val) * std::pow(10, exp);
			else
				Number = std::stoll(val);
		}
		return Number;
	}

	Value parse_bool(const std::string &str, size_t &offset) {
		Value Bool;
		if (str.substr(offset, 4) == "true")
			Bool = true;
		else if (str.substr(offset, 5) == "false")
			Bool = false;
		else {
			std::stringstream ss;
			ss << "ERROR: Bool: Expected 'true' or 'false', found '" << str.substr(offset, 5) << "'.";
			throw std::invalid_argument(ss.str());
		}
		offset += (Bool.toBool() ? 4 : 5);
		return Bool;
	}

	Value parse_null(const std::string &str, size_t &offset) {
		Value Null;
		if (str.substr(offset, 4) != "null") {
			std::stringstream ss;
			ss << "ERROR: Null: Expected 'null', found '" << str.substr(offset, 4) << "'.";
			throw std::invalid_argument(ss.str());
		}
		offset += 4;
		return Null;
	}

	Value parse_next(const std::string &str, size_t &offset) {
		char value;
		consume_ws(str, offset);
		value = str[offset];
		switch (value) {
		case '[': return parse_array(str, offset);
		case '{': return parse_object(str, offset);
		case '\"': return parse_string(str, offset);
		case 't':
		case 'f': return parse_bool(str, offset);
		case 'n': return parse_null(str, offset);
		default: if ((value <= '9' && value >= '0') || value == '-')
			return parse_number(str, offset);
		}

		{
			std::stringstream ss;
			ss << "ERROR: Parse: Unknown starting character '" << value << "'.";
			throw std::invalid_argument(ss.str());
		}
		return Value();
	}
private:
	BackingData Internal;	//internal data
	emJsonType Type = emJsonType::Null;
};

}

#endif // !MMR_UTIL_JSON_HPP
