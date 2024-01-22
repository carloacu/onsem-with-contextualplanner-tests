#include <onsem/optester/loadchatbot.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <contextualplanner/types/condition.hpp>
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
    int nbOfIteration = 0;
    int priority = 1;
    std::string goalName;
    std::string goalGroupId;
    for (auto& currGoalWithPriorityTree : currGoalListTree.second)
    {
      if (nbOfIteration == 0)
        priority = mystd::lexical_cast<int>(currGoalWithPriorityTree.second.get_value<std::string>());
      else if (nbOfIteration == 1)
        goalName = currGoalWithPriorityTree.second.get_value<std::string>();
      else
        goalGroupId = currGoalWithPriorityTree.second.get_value<std::string>();
      ++nbOfIteration;
    }
    if (!goalName.empty())
      pGoals[priority].push_back(cp::Goal(goalName, -1, goalGroupId));
  }
}


cp::SetOfInferences _loadInferences(const boost::property_tree::ptree& pTree)
{
  cp::SetOfInferences setOfInferences;
  for (auto& currInferenceTree : pTree)
  {
    auto condition = cp::Condition::fromStr(currInferenceTree.second.get("condition", ""));
    auto effect = cp::WorldStateModification::fromStr(currInferenceTree.second.get("effect", ""));

    if (condition && effect)
    {
      cp::Inference inference(std::move(condition), std::move(effect));

      auto parametersTreeOpt = currInferenceTree.second.get_child_optional("parameters");
      if (parametersTreeOpt)
        for (auto& currParameterTree : *parametersTreeOpt)
          inference.parameters.push_back(currParameterTree.second.get_value<std::string>());
      setOfInferences.addInference(inference);
    }
  }
  return setOfInferences;
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
        auto idFromJson = currActionTree.second.get("id", "");
        cp::ActionId actionId = idFromJson;
        std::string actionText = currActionTree.second.get("text", "");
        if (actionId.empty())
          actionId = actionText;
        actionId = newId(actionId, pChatbotDomain.actions);
        auto& currChatbotAction = pChatbotDomain.actions[actionId];
        currChatbotAction.idFromJson = idFromJson;
        currChatbotAction.language = language;


        auto singleTrigger = currActionTree.second.get("trigger", "");
        if (!singleTrigger.empty())
          currChatbotAction.triggers.emplace_back(singleTrigger);

        auto triggersTreeOpt = currActionTree.second.get_child_optional("triggers");
        if (triggersTreeOpt)
          for (auto& currTriggerTree : *triggersTreeOpt)
            currChatbotAction.triggers.emplace_back(currTriggerTree.second.get_value<std::string>());


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

        auto priorityStr = currActionTree.second.get("priority", "");

        auto prefereInContexStrInput = currActionTree.second.get("preferInContext", "");
        std::stringstream prefereInContextSs;
        prefereInContextSs << prefereInContexStrInput;
        int timeEstimation = mystd::lexical_cast<int>(currActionTree.second.get("timeEstimation", "0"));
        if (priorityStr == "low")
          timeEstimation += 5;
        bool noPrefernContext = prefereInContexStrInput.empty();
        while (timeEstimation > 0)
        {
          if (noPrefernContext)
            noPrefernContext = false;
          else
            prefereInContextSs << " & ";
          prefereInContextSs << "lowPriority" << timeEstimation;
          --timeEstimation;
        }
        currChatbotAction.preferInContext = cp::Condition::fromStr(prefereInContextSs.str());
        currChatbotAction.precondition = cp::Condition::fromStr(currActionTree.second.get("precondition", ""));
        currChatbotAction.effect = cp::WorldStateModification::fromStr(currActionTree.second.get("effect", ""));
        currChatbotAction.potentialEffect = cp::WorldStateModification::fromStr(currActionTree.second.get("potentialEffect", ""));
        currChatbotAction.description = currActionTree.second.get("description", "");
        currChatbotAction.goalDescription = currActionTree.second.get("goalDescription", "");

        auto pushFrontGoalsTreeOpt = currActionTree.second.get_child_optional("pushFrontGoals");
        if (pushFrontGoalsTreeOpt)
          _loadGoals(currChatbotAction.pushFrontGoals, *pushFrontGoalsTreeOpt);

        auto pushBackGoalsTreeOpt = currActionTree.second.get_child_optional("pushBackGoals");
        if (pushBackGoalsTreeOpt)
          _loadGoals(currChatbotAction.pushBackGoals, *pushBackGoalsTreeOpt);
      }
    }
    else if (currChatbotAttr.first == "inferences")
    {
      pChatbotDomain.compiledDomain.addSetOfInferences(_loadInferences(currChatbotAttr.second));
    }
  }
}


void loadChatbotProblem(ChatbotProblem& pChatbotProblem,
                        ChatbotDomain& pChatbotDomain,
                        std::istream& pIstream,
                        const std::string& pPath)
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
    else if (currChatbotAttr.first == "domains")
    {
      for (auto& currDomainTree : currChatbotAttr.second)
      {
        auto filenameStr = pPath + currDomainTree.second.get_value<std::string>();
        std::ifstream file(filenameStr.c_str(), std::ifstream::in);
        loadChatbotDomain(pChatbotDomain, file);
      }
    }
    else if (currChatbotAttr.first == "facts")
    {
      for (auto& currFactTree : currChatbotAttr.second)
        pChatbotProblem.problem.worldState.addFact(cp::Fact::fromStr(currFactTree.second.get_value<std::string>()),
                                                   pChatbotProblem.problem.goalStack, pChatbotDomain.compiledDomain.getSetOfInferences(), now);
    }
    else if (currChatbotAttr.first == "goals")
    {
      std::map<int, std::vector<cp::Goal>> goals;
      _loadGoals(goals, currChatbotAttr.second);
      pChatbotProblem.problem.goalStack.addGoals(goals, pChatbotProblem.problem.worldState, now);
    }
    else if (currChatbotAttr.first == "inferences")
    {
      throw std::runtime_error("the inferences should be in the domain");
      assert(false);
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
    if (!currAction.triggers.empty())
    {
      auto textProcToRobot = TextProcessingContext::getTextProcessingContextToRobot(currAction.language);
      textProcToRobot.isTimeDependent = false;

      std::map<std::string, std::vector<std::string>> parameterLabelToQuestionsStrs;
      for (const auto& currParameter : currAction.parameters)
        parameterLabelToQuestionsStrs[currParameter.text] = currParameter.questions;

      for (auto& currTrigger : currAction.triggers)
      {
        auto triggerSemExp = converter::textToSemExp(currTrigger, textProcToRobot, pLingDb);

        auto outputResourceGrdExp =
            std::make_unique<GroundedExpression>(
              converter::createResourceWithParameters("action", currActionWithId.first, parameterLabelToQuestionsStrs,
                                                      *triggerSemExp, pLingDb, currAction.language));

        auto infinitiveActionSemExp = converter::imperativeToInfinitive(*triggerSemExp);
        if (infinitiveActionSemExp)
        {
          mystd::unique_propagate_const<UniqueSemanticExpression> reaction;
          memoryOperation::teachSplitted(reaction, pSemanticMemory,
                                         (*infinitiveActionSemExp)->clone(), outputResourceGrdExp->clone(),
                                         pLingDb, memoryOperation::SemanticActionOperatorEnum::BEHAVIOR);
        }

        triggers::add(std::move(triggerSemExp), std::move(outputResourceGrdExp), pSemanticMemory, pLingDb);
      }
    }

    cp::Action action(currAction.precondition ? currAction.precondition->clone() : std::unique_ptr<cp::Condition>(),
                      currAction.effect ? currAction.effect->clone(nullptr) : std::unique_ptr<cp::WorldStateModification>(),
                      currAction.preferInContext ? currAction.preferInContext->clone() : std::unique_ptr<cp::Condition>());
    for (auto& currParam : currAction.parameters)
      action.parameters.emplace_back(currParam.text);
    if (currAction.potentialEffect)
      action.effect.potentialWorldStateModification = currAction.potentialEffect->clone(nullptr);
    actions.emplace(currActionWithId.first, std::move(action));
  }
  auto setOfInferences = pChatbotDomain.compiledDomain.getSetOfInferences();
  pChatbotDomain.compiledDomain = cp::Domain(actions);
  for (auto& currSoi : setOfInferences)
    pChatbotDomain.compiledDomain.addSetOfInferences(currSoi.second, currSoi.first);
}


} // End of namespace onsem
