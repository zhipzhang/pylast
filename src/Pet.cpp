#include "Pet.hh"


Pet::Pet(std::string name, int age) : name(name), age(age) {};

Dog::Dog(std::string name, int age) : Pet(name, age) {};
std::string Dog::bark() const {
    return name + " says woof";
}
