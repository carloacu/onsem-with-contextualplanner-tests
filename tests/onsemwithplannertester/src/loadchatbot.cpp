#include <onsem/optester/loadchatbot.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <contextualplanner/types/factcondition.hpp>
#include <contextualplanner/types/setofinferences.hpp>
#include <onsem/common/utility/lexical_cast.hpp>
#include <onsem/texttosemantic/dbtype/semanticexpression/fixedsynthesisexpression.hpp>
#include <onsem/texttosemantic/dbtype/semanticexpression/groundedexpression.hpp>
#include <onsem/semantictotext/semanticconverter.hpp>
#include <onsem/semantictotext/semexpoperators.hpp>
#include <onsem/semantictotext/triggers.hpp>

namespace onsem
{
namespace
{

std::string newId(
    const std::string& pBaseName,
    const std::map<cp::ActionId, ChatbotAction>& pActions) {
  if (pActions.count(pBaseName) == 0 && !pBaseName.empty())
    return pBaseName;

  std::size_t nextId = 1;
  while (true)
  {
    std::stringstream ss;
    ss << pBaseName << nextId++;
    std::string res;
    res = ss.str();
    if (pActions.count(res) == 0)
      return res;
  }
  assert(false);
  return "";
}

void _loadGoals(
    std::map<int, std::vector<cp::Goal>>& pGoals,
    const boost::property_tree::ptree& pTree)
{
  for (auto& currGoalListTree : pTree)
  {
    bool firstIteration = true;
    int priority = 1;
    for (auto& currGoalWithPriorityTree : currGoalListTree.second)
    {
      if (firstIteration)
      {
        priority = mystd::lexical_cast<int>(currGoalWithPriorityTree.second.get_value<std::string>());
        firstIteration = false;
      }
      else
      {
        pGoals[priority].push_back(currGoalWithPriorityTree.second.get_value<std::string>());
      }
    }
  }
}

}

void loadChatbotDomain(ChatbotDomain& pChatbotDomain,
                       std::istream& pIstream)
{
  boost::property_tree::ptree pTree;
  boost::property_tree::read_json(pIstream, pTree);
  auto language = SemanticLanguageEnum::UNKNOWN;

  for (auto& currChatbotAttr : pTree)
  {
    if (currChatbotAttr.first == "language")
    {
      auto languageStr = currChatbotAttr.second.get_value<std::string>();
      if (languageStr == "en")
        language = SemanticLanguageEnum::ENGLISH;
      else if (languageStr == "fr")
        language = SemanticLanguageEnum::FRENCH;
      else if (languageStr == "ja")
        language = SemanticLanguageEnum::JAPANESE;
      else
        language = SemanticLanguageEnum::UNKNOWN;
    }
    else if (currChatbotAttr.first == "inform")
    {
      auto& inform = pChatbotDomain.inform[language];
      for (auto& currActionTree : currChatbotAttr.second)
        inform.push_back(currActionTree.second.get_value<std::string>());
    }
    else if (currChatbotAttr.first == "actions")
    {
      for (auto& currActionTree : currChatbotAttr.second)
      {
        cp::ActionId actionId = currActionTree.second.get("id", "");
        std::string actionText = currActionTree.second.get("text", "");
        if (actionId.empty())
          actionId = actionText;
        actionId = newId(actionId, pChatbotDomain.actions);
        auto& currChatbotAction = pChatbotDomain.actions[actionId];
        currChatbotAction.language = language;

        currChatbotAction.trigger = currActionTree.second.get("trigger", "");
        currChatbotAction.text = actionText;

        auto parametersTreeOpt = currActionTree.second.get_child_optional("parameters");
        if (parametersTreeOpt)
        {
          for (auto& currParametersTree : *parametersTreeOpt)
          {
            currChatbotAction.parameters.emplace_back();
            auto& currParam = currChatbotAction.parameters.back();
            bool ifrstIteration = true;
            for (auto& currParameterTree : currParametersTree.second)
            {
              if (ifrstIteration)
              {
                ifrstIteration = false;
                currParam.text = currParameterTree.second.get_value<std::string>();
              }
              else
              {
                for (auto& currParameterQuestionTree : currParameterTree.second)
                  currParam.questions.emplace_back(currParameterQuestionTree.second.get_value<std::string>());
              }
            }
          }
        }

        currChatbotAction.precondition = cp::FactCondition::fromStr(currActionTree.second.get("precondition", ""));
        currChatbotAction.preferInContext = cp::FactCondition::fromStr(currActionTree.second.get("preferInContext", ""));
        currChatbotAction.effect = cp::FactModification::fromStr(currActionTree.second.get("effect", ""));
        currChatbotAction.potentialEffect = cp::FactModification::fromStr(currActionTree.second.get("potentialEffect", ""));

        auto goalsToAddTreeOpt = currActionTree.second.get_child_optional("goalsToAdd");
        if (goalsToAddTreeOpt)
          _loadGoals(currChatbotAction.goalsToAdd, *goalsToAddTreeOpt);
      }
    }
  }
}


void loadChatbotProblem(ChatbotProblem& pChatbotProblem,
                        std::istream& pIstream)
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  boost::property_tree::ptree pTree;
  boost::property_tree::read_json(pIstream, pTree);

  for (auto& currChatbotAttr : pTree)
  {
    if (currChatbotAttr.first == "language")
    {
      auto languageStr = currChatbotAttr.second.get_value<std::string>();
      if (languageStr == "en")
        pChatbotProblem.language = SemanticLanguageEnum::ENGLISH;
      else if (languageStr == "fr")
        pChatbotProblem.language = SemanticLanguageEnum::FRENCH;
      else if (languageStr == "ja")
        pChatbotProblem.language = SemanticLanguageEnum::JAPANESE;
      else
        pChatbotProblem.language = SemanticLanguageEnum::UNKNOWN;
    }
    else if (currChatbotAttr.first == "facts")
    {
      for (auto& currFactTree : currChatbotAttr.second)
        pChatbotProblem.problem.addFact(cp::Fact::fromStr(currFactTree.second.get_value<std::string>()), now);
    }
    else if (currChatbotAttr.first == "goals")
    {
      std::map<int, std::vector<cp::Goal>> goals;
      _loadGoals(goals, currChatbotAttr.second);
      pChatbotProblem.problem.addGoals(goals, now);
    }
    else if (currChatbotAttr.first == "inferences")
    {
      std::size_t inferenceCounter = 1;
      for (auto& currInferenceTree : currChatbotAttr.second)
      {
        auto condition = cp::FactCondition::fromStr(currInferenceTree.second.get("condition", ""));
        auto effect = cp::FactModification::fromStr(currInferenceTree.second.get("effect", ""));

        if (condition && effect)
        {
          auto setOfInferences = std::make_shared<cp::SetOfInferences>();
          pChatbotProblem.problem.addSetOfInferences("soi", setOfInferences);
          cp::Inference inference(std::move(condition), std::move(effect));

          auto parametersTreeOpt = currInferenceTree.second.get_child_optional("parameters");
          if (parametersTreeOpt)
            for (auto& currParameterTree : *parametersTreeOpt)
              inference.parameters.push_back(currParameterTree.second.get_value<std::string>());
          std::stringstream ssId;
          ssId << "inference" << inferenceCounter;
          ++inferenceCounter;
          setOfInferences->addInference(ssId.str(), inference);
        }
      }
    }
  }
}


void addChatbotDomaintoASemanticMemory(
    SemanticMemory& pSemanticMemory,
    ChatbotDomain& pChatbotDomain,
    const linguistics::LinguisticDatabase& pLingDb)
{
  for (const auto& currInform : pChatbotDomain.inform)
  {
    auto textProc = TextProcessingContext::getTextProcessingContextToRobot(currInform.first);
    for (const auto& currText : currInform.second)
    {
      auto semExp = converter::textToContextualSemExp(currText, textProc, SemanticSourceEnum::UNKNOWN, pLingDb);
      memoryOperation::addAgentInterpretations(semExp, pSemanticMemory, pLingDb);
      memoryOperation::informAxiom(std::move(semExp), pSemanticMemory, pLingDb);
    }
  }

  std::map<cp::ActionId, cp::Action> actions;
  for (const auto& currActionWithId : pChatbotDomain.actions)
  {
    const ChatbotAction& currAction = currActionWithId.second;
    if (!currAction.trigger.empty())
    {
      auto textProcToRobot = TextProcessingContext::getTextProcessingContextToRobot(currAction.language);
      textProcToRobot.isTimeDependent = false;
      auto triggerSemExp = converter::textToSemExp(currAction.trigger, textProcToRobot, pLingDb);

      std::map<std::string, std::vector<std::string>> parameterLabelToQuestionsStrs;
      for (const auto& currParameter : currAction.parameters)
        parameterLabelToQuestionsStrs[currParameter.text] = currParameter.questions;

      auto outputResourceGrdExp =
          std::make_unique<GroundedExpression>(
            converter::createResourceWithParameters("action", currActionWithId.first, parameterLabelToQuestionsStrs,
                                                    *triggerSemExp, pLingDb, currAction.language));
      triggers::add(std::move(triggerSemExp), std::move(outputResourceGrdExp), pSemanticMemory, pLingDb);
    }

    cp::Action action(currAction.precondition ? currAction.precondition->clone() : std::unique_ptr<cp::FactCondition>(),
                      currAction.effect ? currAction.effect->clone(nullptr) : std::unique_ptr<cp::FactModification>(),
                      currAction.preferInContext ? currAction.preferInContext->clone() : std::unique_ptr<cp::FactCondition>());
    for (auto& currParam : currAction.parameters)
      action.parameters.emplace_back(currParam.text);
    if (currAction.potentialEffect)
      action.effect.potentialFactsModifications = currAction.potentialEffect->clone(nullptr);
    actions.emplace(currActionWithId.first, std::move(action));
  }
  pChatbotDomain.compiledDomain = std::make_unique<cp::Domain>(actions);
}


} // End of namespace onsem
