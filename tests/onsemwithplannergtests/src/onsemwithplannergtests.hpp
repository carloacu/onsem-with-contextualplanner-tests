#ifndef ONSEMWITHPLANNERGTESTS_ONSEMWITHPLANNERGTESTS_HPP
#define ONSEMWITHPLANNERGTESTS_ONSEMWITHPLANNERGTESTS_HPP

#include <gtest/gtest.h>


class OnsemWithPlannerGTests : public testing::Test
{
public:

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  /**
   * @brief This function will be called before all tests.
   */
  static void SetUpTestCase()
  {
  }

  /**
   * @brief This function will be called immediately after the constructor
   * (right before each test).
   */
  void SetUp()
  {
  }

  /**
   * @brief This function will be called immediately after each test
   * (right before the destructor).
   */
  void TearDown()
  {
  }

  /**
   * @brief This function will be called after all tests.
   */
  static void TearDownTestCase()
  {
  }

};


#endif // ONSEMWITHPLANNERGTESTS_ONSEMWITHPLANNERGTESTS_HPP
