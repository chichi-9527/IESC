// test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "../IESC/IECS.h"

#include <iostream>

struct Component1
{
    int a = 0;
};
struct Component2
{
    float a = 0;
};
struct Component3
{
    int a = 0;
    int b = 0;
};
struct Component4
{
    bool a = false;
};

int main()
{
    auto world = IECS::IWorld::CreateWorld();

    auto entity1 = world->CreateEntity();

    world->AddComponent<Component1>(entity1);
    world->GetComponent<Component1>(entity1)->a = 1;
    std::cout << "Component1.a = " << world->GetComponent<Component1>(entity1)->a << "\n";
    std::cout << "entity1 has Component2? = " << world->HasComponent<Component2>(entity1) << "\n";

    auto entity2 = world->CreateEntity();
    world->AddComponent<Component3>(entity2);
    std::cout << "entity2 has Component3? = " << world->HasComponent<Component3>(entity2) << "\n";

    auto entity3 = world->CreateEntity();

    world->DestroyEntity(entity2);
    auto entity4 = world->CreateEntity();
    std::cout << "entity4 has Component3? = " << world->HasComponent<Component2>(entity4) << "\n";
    world->AddComponent<Component1>(entity4)->a = 2;

    auto ComponentSet1 = world->GetComponentSet<Component1>();
    for (auto& [comp, e] : *ComponentSet1)
    {
        std::cout << "entity.ID = " << e.Id << " entity.version = " << e.Version << " comp1.a = " << comp.a << "\n";
    }


    return 0;
}

// 结果：
//Component1.a = 1
//entity1 has Component2 ? = 0
//entity2 has Component3 ? = 1
//entity4 has Component3 ? = 0
//entity.ID = 0 entity.version = 0 comp1.a = 1
//entity.ID = 1 entity.version = 1 comp1.a = 2

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
