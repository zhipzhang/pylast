#include <cstddef>
#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "ConfigSystem.hh"
#include "ConfigMacros.hh"
#include "doctest/doctest.h"

TEST_CASE("Test splitPath")
{
    auto path = "a.b.c";
    auto parts = config::Configurable::splitPath(path);
    CHECK(parts.size() == 3);
    CHECK(parts[0] == "a");
    CHECK(parts[1] == "b");
    CHECK(parts[2] == "c");
}
class test_person: public config::Configurable
{
    public:
        CONFIG_CONSTRUCTORS(test_person);
        ~test_person() = default;
        void registerParams() override
        {
            registerParam<std::string>("name", "default", name);
            registerParam<int>("age", 0, age);
        };
        void setUp() override
        {
        };

        std::string name;
        int age;
};

class test_Configurable: public config::Configurable
{
public:
    CONFIG_CONSTRUCTORS(test_Configurable);
    ~test_Configurable() = default;
    void registerParams() override
    {
        registerParam<double>("x", 10, x);
        registerParam<int>("y", 10, y);
        registerParam<std::string>("str", std::string("default"), str);
        registerParam<std::string>("test_embedded_class_name", std::string("test_person"), test_embedded_class_name);
        registerParam<json>("test_person", json::parse(R"({
            "name": "Anna",
            "age": 25
        })"));
    }
    void setUp() override
    {
        if(test_embedded_class_name == "test_person")
        {
            if(getConfig().contains("test_person"))
                person = std::make_unique<test_person>(getConfig()["test_person"]);
            else
                person = std::make_unique<test_person>();
        }

    };
    double x;
    int  y;
    std::string str;
    std::string test_embedded_class_name;
    std::unique_ptr<test_person> person;
};


TEST_CASE("Default configuration")
{
    auto config = test_Configurable{};
    CHECK(config.x == 10);
    CHECK(config.y == 10);
    CHECK(config.str == "default");

    CHECK(config.getConfig().at("x") == 10);
    CHECK(config.getConfig().at("y") == 10);
    CHECK(config.getConfig().at("str") == "default");
    CHECK(config.person->name == "Anna");
    CHECK(config.person->age == 25);
}
TEST_CASE("Embedded configuration")
{
    auto config_str = R"({
        "test_embedded_class_name": "test_person",
        "test_person": {
            "name": "Ricardo",
            "age": 30
        }
    })";
    auto config = test_Configurable{std::string(config_str)};
    CHECK(config.person->name == "Ricardo");
    CHECK(config.person->age == 30);

    auto config_str2 = R"({
        "test_embedded_class_name": "test_person",
        "test_person.name": "John",
        "test_person.age": 25
    })";
    auto config2 = test_Configurable{std::string(config_str2)};
    CHECK(config2.person->name == "John");
    CHECK(config2.person->age == 25);
    std::cout << config2.getConfig().dump(2) << std::endl;

}
TEST_CASE("User-defined configuration with string")
{
    auto config_str = R"({
        "x": 100.0,
        "y": 200,
        "str": "user-defined"
    })";
    auto config = test_Configurable{std::string(config_str)};
    CHECK(config.x == 100.0);
    CHECK(config.y == 200);
    CHECK(config.str == "user-defined");
}
