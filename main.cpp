#include "json.hpp"

#include <iostream>

std::string jsonErro = u8R"(
{
	"files" : 
	[
		"D:\\6.1-6.15.xlsx",
		"D:\\5\\5.17-5.31.xls",//这里多了一个逗号
	]
}
)";

std::string jsonContain = u8R"(
{
	"files" : 
	[//这个是注释
		"D:\\6.1-6.15.xlsx",
		"D:\\5\\5.17-5.31.xls"
	],
	"Int64Max" : 9223372036854775807,
	"Int64Min" : -9223372036854775808//注释
	#这个也是注释
	/*这个也是注释*/
}
)";

int main()
{
	Json::Value jvObj;

	std::string strErr = jvObj.load(jsonErro);
	if (!strErr.empty()) 
	{
		std::cout << "解析Json错误：" << strErr << std::endl;
	}
#if 0
	//直接加载Json文件
	strErr = jvObj.load_from_file("D:/VMs/build/bin/RelWithDebInfo/config/colDef.json");
#else
	strErr = jvObj.load(jsonContain);
#endif
	if (!strErr.empty())
	{
		std::cout << "解析Json错误：" << strErr << std::endl;
	}
	std::cout << "格式化输出Json内容" << std::endl;
	std::cout << jvObj.dumpStyle() << std::endl;
	std::cout << "非格式化输出Json内容" << std::endl;
	std::cout << jvObj.dumpFast() << std::endl;

	jvObj["name"] = "zhangsan";

	jvObj["array"].append("3");
	jvObj["array"].append("5");
	jvObj["array"].append(6.9);
	jvObj["array"].append(18);

	int64_t llMin = jvObj["Int64Min"].toInt();
	std::cout << "Int64Min " << llMin << std::endl;

	std::cout << "修改后输出Json内容" << std::endl; 
	std::cout << jvObj.dumpStyle() << std::endl;

	Json::Value jvObj2(std::move(jvObj));
	std::cout << "移动后Json1内容" << std::endl;
	std::cout << jvObj.dumpStyle() << std::endl;

	jvObj2 = jvObj2;//自赋值
	jvObj2 = std::move(jvObj2);
	
	std::cout << "移动后Json2内容" << std::endl;
	std::cout << jvObj2.dumpStyle() << std::endl;

	jvObj = jvObj2;
	std::cout << "赋值后jvObj内容" << std::endl;
	std::cout << jvObj.dumpStyle() << std::endl;

	auto jvArray = Json::Value::Make_Array(1, 2.5, "hello", 6, std::move(jvObj), true);
	std::cout << "make arry json is " << jvArray.dumpStyle() << std::endl;

	std::cout << "移动后jvObj内容" << std::endl;
	std::cout << jvObj.dumpStyle() << std::endl;

	std::cout << "输入任意字符继续..." << std::endl;
	std::cin.get();

	return 0;
}
