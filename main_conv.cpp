#include <iostream>
#include "JsonConverter.hpp"


enum class EM_type
{
	TYPE_1,
	TYPE_2
};

//一个基类
struct BaseData
{
	std::optional<int32_t> lData;
	std::optional<double> dData;
	std::string str = "你好";
	float fData = 4.6;
	EM_type ty1 = EM_type::TYPE_2;
	Json::Value jvData;
	std::map<std::string, std::vector<std::optional<std::shared_ptr<std::string>>>> mapData;//一个多层嵌套模板类型成员
	ADD_JSON_MEMBER(lData, dData, str, fData, ty1, jvData, mapData);
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
	std::map<std::string, std::optional<DeriveData>> mapData;
	std::shared_ptr<BaseData> ptrBase;
	std::shared_ptr<int32_t> ptrlData;
	std::unique_ptr<float> ptrfData;
	std::unique_ptr<double> ptrdData;
	float fValue = 2.9;
	ADD_JSON_MEMBER(listDataDer, optBase, optDerive, optDataEmpty, mapData, ptrBase, ptrlData, ptrfData, ptrdData, fValue);
};

struct ErrorData
{
	uint16_t sData = 65535;
	std::optional<int32_t> opLData = 0;
	float fData = 0.3;
	ADD_JSON_MEMBER(sData, opLData, fData);
};


int main() 
{

	Json::Value jv;
	jv["haha"] = 33;
	for (const auto iterMap : jv.ObjectRange())
	{
		std::cout << iterMap.first << " " << iterMap.second.get<int32_t>() << std::endl;
	}
	//option成员为空，不异常
	try
	{
		std::cout << "/******************  option成员为空，不异常  ********************/" << std::endl;
		ErrorData eData;
		Json::Value jvData;
		jvData["sData"] = 123;
		jvData["fData"] = 133;
		jvData >> eData;
		std::cout <<"sData:"<< eData.sData << std::endl;
	}
	catch (const std::exception& e) 
	{
		std::cout << "exception info:" << e.what() << std::endl;
	}

	try
	{
		std::cout << "/******************  非option成员为空，异常  ********************/" << std::endl;
		ErrorData eData;
		Json::Value jvData;
		jvData["sData"] = 123;
		jvData >> eData;
	}
	catch (const std::exception& e)
	{
		std::cout << "exception info:" << e.what() << std::endl;
	}

	//auto jvType = Json::json_number_type<uint64_t>::value;
	//std::cout << static_cast<int>(jvType) << std::endl;
	try
	{
		std::cout << "/******************  结构体与Json互转  ********************/" << std::endl;
		WrapData data;
		DeriveData dataDerive;
		Json::Value outJson;

		data.listDataDer.emplace_back(dataDerive);
		dataDerive.mapData["maltimap"] = { std::make_shared<std::string>(("value")) };//多层模板嵌套类型
		dataDerive.lData = 12;
		dataDerive.str = "str2";
		dataDerive.jvData["test"] = 20;
		dataDerive.uintMax = 0xFFFFFFFFFFFFFFFF;
		data.listDataDer.emplace_back(dataDerive);
		data.optBase = dataDerive;//将派生类赋值给基类
		data.optDerive = dataDerive;//派生类赋值给派生类
		data.mapData["aa"] = dataDerive;
		data.mapData["bb"];
		data.ptrBase = std::make_unique<BaseData>();
		data.ptrBase->fData = 12.04;
		data.ptrfData = std::make_unique<float>(777.998);
		outJson << data;
		std::cout <<"结构体转json结果\n" << outJson.dumpStyle() << std::endl;

		WrapData outWrap;
		outJson >> outWrap;
		std::cout << "json转结构体结果\n" << outJson.dumpStyle() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << "exception info " << e.what() << std::endl;
	}

	std::cout << "输入回车继续..." << std::endl;
	std::cin.get();

	return 0;
}
