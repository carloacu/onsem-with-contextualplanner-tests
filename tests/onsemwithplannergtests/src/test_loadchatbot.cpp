#include "onsemwithplannergtests.hpp"
#include <gtest/gtest.h>
#include <onsem/optester/loadchatbot.hpp>

using namespace onsem;




TEST_F(OnsemWithPlannerGTests, test_loadchatbotProblem)
{
  std::size_t goalPriority = 5;

  std::stringstream ss;
  ss << "{                                                        \n";
  ss << "  \"language\" : \"fr\",                                 \n";
  ss << "  \"goals\" : [                                          \n";
  ss << "    [" << goalPriority << ", \"remonter-le-moral\"]      \n";
  ss << "  ]                                                      \n";
  ss << "}                                                        \n";

  ChatbotProblem chatbotProblem;
  ChatbotDomain chatbotDomain;
  loadChatbotProblem(chatbotProblem, chatbotDomain, ss, "");
  EXPECT_EQ(SemanticLanguageEnum::FRENCH, chatbotProblem.language);
  ASSERT_EQ(1, chatbotProblem.problem.goalStack.goals().size());
  EXPECT_EQ(cp::Goal("remonter-le-moral"), chatbotProblem.problem.goalStack.goals().find(goalPriority)->second[0]);
}

