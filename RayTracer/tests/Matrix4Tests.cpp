#include <gtest/gtest.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"

namespace {
void AssertMatricesEqual(const Math::Matrix4D &my_matrix, const glm::mat4 &glm_matrix) {
    auto my_data = my_matrix.GetSpan().data();
    for (auto col{0zu}; col < 4; ++col) {
        for (auto row{0zu}; row < 4; ++row) {
            ASSERT_FLOAT_EQ(my_data[row + col * 4], glm_matrix[col][row]);
        }
    }
}

void OutputMatrix(const Math::Matrix4D &matrix) {
    auto matrix_data = matrix.GetSpan().data();
    for (auto col{0zu}; col < 4; ++col) {
        for (auto row{0zu}; row < 4; ++row) {
            std::cout << matrix_data[row + col * 4] << " ";
        }
    }
    std::cout << "\n";
}

void OutputMatrix(glm::mat4 &matrix) {
    for (auto col{0zu}; col < 4; ++col) {
        for (auto row{0zu}; row < 4; ++row) {
            std::cout << matrix[col][row] << " ";
        }
    }
    std::cout << "\n";
}
} // namespace

TEST(Matrix4, Multiply) {
    auto my_mat1 = Matrix4{
        {1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f}};

    auto my_mat2 = Matrix4{
        {1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f}};

    auto glm_mat1 = glm::mat4{1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f,
                              1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f};

    auto glm_mat2 = glm::mat4{1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f,
                              1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f};

    auto my_result = my_mat1 * my_mat2;
    auto expected = glm_mat1 * glm_mat2;

    AssertMatricesEqual(my_result, expected);

    if (HasFailure()) {
        std::cout << "--- my_result data -------------------\n";
        OutputMatrix(my_result);
        std::cout << "--- expected  data -------------------\n";
        OutputMatrix(expected);
    }
}

TEST(Matrix4, LookAt) {
    auto view = Matrix4::LookAt(/*Eye*/ fVector3{0.0f, 1.0f, 3.0f}, /*LookAt*/ fVector3{0.0f, 0.0f, 0.0f},
                                /*Up*/ fVector3{0.0f, 1.0f, 0.0f});
    auto expected =
        glm::lookAt(glm::vec3{0.0f, 1.0f, 3.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
    AssertMatricesEqual(view, expected);

    if (HasFailure()) {
        std::cout << "--- my_result data -------------------\n";
        OutputMatrix(view);
        std::cout << "--- expected  data -------------------\n";
        OutputMatrix(expected);
    }
}
