#include "engine/hello.h"
#include "vulkan/vulkan.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <Eigen/Dense>

using Eigen::MatrixXd;

using namespace std;

const string generateHelloString(const string &personName)
{
    MatrixXd m(2, 2);
    m(0, 0) = 3;
    m(1, 0) = 2.5;
    m(0, 1) = -1;
    m(1, 1) = m(1, 0) + m(0, 1);
    std::cout << m << std::endl;
    glm::mat4 mat;
    return "Hello2 " + personName;
}