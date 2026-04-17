#include "JsonConverter.hpp"
#include <iostream>
#include <functional>

// 基类定义（使用多态宏）
struct Shape
{
	std::string color = "red";
	int borderWidth = 1;

	ADD_JSON_MEMBER_BASE_DEPL(Shape, color, borderWidth);
};

// 派生类定义
struct Circle : public Shape
{
	double radius = 10.0;

	ADD_JSON_MEMBER_DERIVE_DEPL(Shape, Circle, radius);
};

// 另一个派生类
struct Rectangle : public Shape
{
	double width = 20.0;
	double height = 15.0;

	ADD_JSON_MEMBER_DERIVE_DEPL(Shape, Rectangle, width, height);
};

#if __cplusplus < 201703L//小于cpp17标准，注册类型
 REGISTER_TEPE(Shape);
 REGISTER_TEPE(Circle);
 REGISTER_TEPE(Rectangle);
#endif

 // 包含多态指针的结构体
 struct Drawing
 {
	 std::shared_ptr<Shape> shape;
	 std::string name = "My Drawing";

	 ADD_JSON_MEMBER(shape, name);
 };

int main()
{
	try
	{
		// 创建派生类对象，使用基类指针指向
		Drawing drawing;
		auto ptrShap = std::make_shared<Circle>();
		ptrShap->radius = 300;
		drawing.shape = ptrShap;
		drawing.shape->color = "blue";

		// 结构体转Json（保留派生类完整数据）
		Json::Value outJson;
		outJson << drawing;

		// 输出包含完整的派生类数据和类型信息
		// {"shape":{"color":"blue","borderWidth":1,"radius":300.0,"PolymorphicFinalClassType":"Circle"},"name":"My Drawing"}
		std::cout << "多态结构体转Json：" << outJson.dumpStyle() << std::endl;

		// Json转结构体（根据类型信息正确创建派生类对象）
		Drawing newDrawing;
		outJson >> newDrawing;

		// 通过多态指针访问派生类
		if (auto* circle = dynamic_cast<Circle*>(newDrawing.shape.get())) 
		{
			std::cout << "Radius: " << circle->radius << std::endl;
		}
		std::cout << "Json转多态结构后：" << newDrawing.generateJson().dumpStyle() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << "exception info " << e.what() << std::endl;
	}

	std::cout << "输入回车继续..." << std::endl;
	std::cin.get();

	return 0;
}
