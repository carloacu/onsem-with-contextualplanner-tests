#ifndef ONSEMWITHPLANNERTESTER_LOADCHATBOT_HPP
#define ONSEMWITHPLANNERTESTER_LOADCHATBOT_HPP

#include <istream>
#include <contextualplanner/contextualplanner.hpp>
#include <onsem/common/enum/semanticlanguageenum.hpp>
#include <onsem/semantictotext/semanticmemory/semanticmemory.hpp>
#include "api.hpp"

namespace onsem
{
const std::string beginOfActionId = "actionId-";

struct ONSEMWITHPLANNERTESTER_API ChatbotParam
{
  std::string text{};
  std::vector<std::string> questions{};
};


struct ONSEMWITHPLANNERTESTER_API ChatbotAction
{
  SemanticLanguageEnum language{SemanticLanguageEnum::UNKNOWN};
  std::string trigger{};
  std::string text{};
  std::string description{};
  std::vector<ChatbotParam> parameters{};
  std::unique_ptr<cp::FactCondition> precondition{};
  std::unique_ptr<cp::FactCondition> preferInContext{};
  std::unique_ptr<cp::FactModification> effect{};
  std::unique_ptr<cp::FactModification> potentialEffect{};
  std::map<int, std::vector<cp::Goal>> goalsToAdd{};
};

struct ONSEMWITHPLANNERTESTER_API ChatbotDomain
{
  std::map<SemanticLanguageEnum, std::vector<std::string>> inform{};
  std::map<cp::ActionId, ChatbotAction> actions{};
  std::unique_ptr<cp::Domain> compiledDomain{};
};


struct ONSEMWITHPLANNERTESTER_API ChatbotProblem
{
  SemanticLanguageEnum language{SemanticLanguageEnum::UNKNOWN};
  cp::Problem problem{};
  std::map<std::string, std::string> variables{};
};


ONSEMWITHPLANNERTESTER_API
void loadChatbotDomain(ChatbotDomain& pChatbotDomain,
                       std::istream& pIstream);


ONSEMWITHPLANNERTESTER_API
void loadChatbotProblem(ChatbotProblem& pChatbotProblem,
                        std::istream& pIstream);



ONSEMWITHPLANNERTESTER_API
void addChatbotDomaintoASemanticMemory(
    SemanticMemory& pSemanticMemory,
    ChatbotDomain& pChatbotDomain,
    const linguistics::LinguisticDatabase& pLingDb);


} // End of namespace onsem

#endif // ONSEMWITHPLANNERTESTER_LOADCHATBOT_HPP
