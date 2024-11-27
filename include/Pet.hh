#pragma once
#include <string>
struct Pet {
    std::string name;
    int age;
    Pet(std::string name, int age);
};

struct Dog : Pet {
    Dog(std::string name, int age);
    std::string bark() const;
};
