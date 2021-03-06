#include "../../semanticreasonergtests.hpp"
#include <onsem/common/utility/noresult.hpp>
#include <onsem/texttosemantic/tool/semexpgetter.hpp>
#include <onsem/texttosemantic/languagedetector.hpp>
#include <onsem/semantictotext/semanticconverter.hpp>
#include <onsem/semantictotext/semexpoperators.hpp>
#include <onsem/semantictotext/semanticmemory/semanticmemory.hpp>


namespace onsem
{
namespace
{

std::string _operator_answerIDontKnow(const std::string& pText,
                                      bool pForQuestion,
                                      const linguistics::LinguisticDatabase& lingDb)
{
  SemanticLanguageEnum language = linguistics::getLanguage(pText, lingDb);
  auto semExp =
      converter::textToSemExp(pText,
                              TextProcessingContext(SemanticAgentGrounding::currentUser,
                                                    SemanticAgentGrounding::me,
                                                    language),
                              lingDb);
  auto answerSemExp = pForQuestion ? memoryOperation::answerIDontKnow(*semExp) :
                                     memoryOperation::answerICannotDo(*semExp);
  if (answerSemExp)
  {
    std::string res;
    SemanticMemory semMem;
    converter::semExpToText(res, std::move(*answerSemExp),
                            TextProcessingContext(SemanticAgentGrounding::me,
                                                  SemanticAgentGrounding::currentUser,
                                                  language),
                            false, semMem, lingDb, nullptr);
    return res;
  }
  return constant::noResult;
}

std::string operator_answerIDontKnow(const std::string& pText,
                                                 const linguistics::LinguisticDatabase& lingDb)
{
  return _operator_answerIDontKnow(pText, true, lingDb);
}

std::string operator_answerICannotDo(const std::string& pText,
                                                const linguistics::LinguisticDatabase& lingDb)
{
  return _operator_answerIDontKnow(pText, false, lingDb);
}

}
} // End of namespace onsem


using namespace onsem;


TEST_F(SemanticReasonerGTests, operator_answerIDontKnow_basic)
{
  const linguistics::LinguisticDatabase& lingDb = *lingDbPtr;

  // english
  EXPECT_EQ(constant::noResult, operator_answerIDontKnow("You are on the floor", lingDb));
  EXPECT_EQ(constant::noResult, operator_answerIDontKnow("look left", lingDb));
  EXPECT_EQ("I don't know if I am on floor.", operator_answerIDontKnow("Are you on the floor?", lingDb));
  EXPECT_EQ("I don't know if I like chocolate.", operator_answerIDontKnow("I like chocolate. Do you like chocolate?", lingDb));
  EXPECT_EQ("I don't know what I like.", operator_answerIDontKnow("What do you like? Me, I like chocolate.", lingDb));
  EXPECT_EQ("I don't know if I am happy. I don't know why I am shy.", operator_answerIDontKnow("Are you happy? Why are you shy?", lingDb));
  EXPECT_EQ("I don't know if I want to see a video that describes Carrefour.", operator_answerIDontKnow("Do you want to see a video that describes Carrefour", lingDb));
  EXPECT_EQ("I don't know what you like.", operator_answerIDontKnow("What do I like?", lingDb));
  EXPECT_EQ("I don't know who plays football.", operator_answerIDontKnow("Who plays football", lingDb));


  // french
  EXPECT_EQ(constant::noResult, operator_answerIDontKnow("Je suis content", lingDb));
  EXPECT_EQ(constant::noResult, operator_answerIDontKnow("regarde ?? droite", lingDb));
  EXPECT_EQ("Je ne sais pas qui est Paul.", operator_answerIDontKnow("Qui est Paul ?", lingDb));
  EXPECT_EQ("Je ne sais pas ce que j'en pense.", operator_answerIDontKnow("Je ne sais pas. Qu'est-ce que tu en penses ?", lingDb));
  EXPECT_EQ("Je ne sais pas comment je vais.", operator_answerIDontKnow("Comment ??a va ? Moi ??a va bien.", lingDb));
  EXPECT_EQ("Je ne sais pas comment Paul s'est senti.", operator_answerIDontKnow("Comment Paul s???est-il senti ?", lingDb));
  EXPECT_EQ("Je ne sais pas si je vais au Japon.", operator_answerIDontKnow("Est-ce que tu vas au Japon ?", lingDb));
  EXPECT_EQ("Je ne sais pas si je veux voir une vid??o qui d??crit Carrefour.", operator_answerIDontKnow("Veux tu voir une vid??o qui d??crit Carrefour", lingDb));
  EXPECT_EQ("Je ne sais pas qui est le pr??sident.", operator_answerIDontKnow("c???est qui le pr??sident ?", lingDb));
  EXPECT_EQ("Je ne sais pas ce que c'est une tomate.", operator_answerIDontKnow("C???est quoi une tomate ?", lingDb));
  EXPECT_EQ("Je ne sais pas quelle est la diff??rence entre un b??b?? et un crocodile.", operator_answerIDontKnow("Quelle est la diff??rence entre un b??b?? et un crocodile?", lingDb));
  EXPECT_EQ("Je ne sais pas quand et o?? Anne est n??e.", operator_answerIDontKnow("Quand et o?? est n??e Anne", lingDb));
  EXPECT_EQ("Je ne sais pas o?? est mon p??re.", operator_answerIDontKnow("Il est o?? ton p??re", lingDb));
}


TEST_F(SemanticReasonerGTests, operator_answerICannotDo_basic)
{
  const linguistics::LinguisticDatabase& lingDb = *lingDbPtr;

  // english
  EXPECT_EQ(constant::noResult, operator_answerICannotDo("You are on the floor", lingDb));
  EXPECT_EQ(constant::noResult, operator_answerICannotDo("Are you are on the floor?", lingDb));
  EXPECT_EQ("I can't look left.", operator_answerICannotDo("look left", lingDb));
  EXPECT_EQ("I can't look left. I can't salute.", operator_answerICannotDo("look left and salute", lingDb));
  EXPECT_EQ("I can't look left. I can't say hello.", operator_answerICannotDo("look left or say hello", lingDb));
  EXPECT_EQ("I can't show you a video that describes N5.", operator_answerICannotDo("Show me a video that describes N5", lingDb));
  EXPECT_EQ("I can't jump.", operator_answerICannotDo("Jump", lingDb));

  // french
  EXPECT_EQ(constant::noResult, operator_answerICannotDo("Je suis content", lingDb));
  EXPECT_EQ(constant::noResult, operator_answerICannotDo("Qui est Paul ?", lingDb));
  EXPECT_EQ("Je ne sais pas regarder ?? droite.", operator_answerICannotDo("regarde ?? droite", lingDb));
  EXPECT_EQ("Je ne sais pas te montrer une vid??o qui d??crit N5.", operator_answerICannotDo("Montre moi une vid??o qui d??crit N5", lingDb));
}
