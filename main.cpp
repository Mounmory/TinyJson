#include "json.hpp"

#include <iostream>

//am errro json string
std::string jsonErro = u8R"(
{
	"files" : 
	[
		"D:\\6.1-6.15.xlsx",
		"D:\\5\\5.17-5.31.xls",//这里多了一个逗号
	]
}
)";

//a correct json string with comment
std::string jsonContain = u8R"(
{
	"files" : 
	[//这个是注释
		"D:\\6.1-6.15.xlsx",
		"D:\\5\\5.17-5.31.xls"
	]
	#这个也是注释
	/*这个也是注释*/
}
)";

int main()
{
	
	Json::Value jvObj;

	std::string strErr = Json::load(jsonErro, jvObj);
	if (!strErr.empty()) 
	{
		std::cout << "parse Json error: " << strErr << std::endl;
	}
#if 0
	//直接加载Json文件
	strErr = Json::load_from_file("D:/VMs/build/bin/RelWithDebInfo/config/colDef.json",jvObj);
#else
	strErr = Json::load(jsonContain, jvObj);
#endif
	if (!strErr.empty())
	{
		std::cout << "parse Json error: " << strErr << std::endl;
	}
	std::cout << "style Json string:" << std::endl;
	std::cout << jvObj.dumpStyle() << std::endl;
	std::cout << "fast Json string:" << std::endl;
	std::cout << jvObj.dumpFast() << std::endl;

	jvObj["name"] = "zhangsan";

	jvObj["array"].append("3");
	jvObj["array"].append("5");
	jvObj["array"].append(6.9);
	jvObj["array"].append(18);

	std::cout << "modified json string: " << std::endl; 
	std::cout << jvObj.dumpStyle() << std::endl;

	Json::Value jvObj2(std::move(jvObj));
	std::cout << "jvObj after move: " << std::endl;
	std::cout << jvObj.dumpStyle() << std::endl;

	jvObj2 = jvObj2;//自赋值
	jvObj2 = std::move(jvObj2);
	
	std::cout << "jvObj2 after move: " << std::endl;
	std::cout << jvObj2.dumpStyle() << std::endl;

	jvObj = jvObj2;
	std::cout << jvObj.dumpStyle() << std::endl;

	jvObj2.clear();
	std::cout << jvObj.dumpStyle() << std::endl;

	std::cout << "press any key to exit..." << std::endl;
	std::cin.get();

	return 0;
}
