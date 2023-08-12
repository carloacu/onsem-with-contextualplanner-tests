#ifndef ONSEMVOICEBOT_MAINWINDOW_H
#define ONSEMVOICEBOT_MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <list>
#include <vector>
#include <string>
#include <filesystem>
#include <onsem/guiutility/lineedithistoricwrapper.hpp>
#include <onsem/common/enum/semanticlanguageenum.hpp>
#include <onsem/common/enum/contextualannotation.hpp>
#include <onsem/common/keytostreams.hpp>
#include <onsem/texttosemantic/dbtype/linguisticdatabase.hpp>
#include <onsem/tester/sentencesloader.hpp>
#include <onsem/tester/scenariocontainer.hpp>
#include <contextualplanner/contextualplanner.hpp>
#include <onsem/optester/loadchatbot.hpp>
#include "qobject/scrollpanel.hpp"

using namespace onsem;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(const std::filesystem::path& pCorpusEquivalencesFolder,
                      const std::filesystem::path &pCorpusResultsFolder,
                      const std::filesystem::path &pScenariosFolder,
                      const std::filesystem::path &pOutputScenariosFolder,
                      const std::filesystem::path &pCorpusFolder,
                      linguistics::LinguisticDatabaseStreams& pIStreams,
                      QWidget *parent = 0);
  ~MainWindow();

  bool eventFilter(QObject *obj, QEvent *event);
  void keyPressEvent(QKeyEvent *event);




private Q_SLOTS:
  void on_actionload_a_smb_triggered();
  void on_actionSave_memory_triggered();
  void on_actionLoad_memory_triggered();

  void on_actionLoad_chat_content_triggered();

  void onRefresh();
  void onRescale();
  void onRescaleChatPanel();
  void onRescaleChatDiagnosisPanel();

  /// Click on "prev sentence" button event.
  void on_pushButton_AATester_PrevSentence_clicked();

  /// Click on "next sentence" button event.
  void on_pushButton_AATester_NextSentence_clicked();


  void on_tabWidget_currentChanged(int index);

  void on_texts_load_triggered();

  void on_actionExport_to_ldic_triggered();

  void on_actionImport_from_ldic_triggered();

  void on_lineEdit_history_newText_returnPressed();

  void on_actionNew_triggered();

  void on_actionQuit_triggered();

  void on_actionSave_triggered();

  void on_scenarios_load_triggered();

  void on_pushButton_Chat_PrevScenario_clicked();

  void on_pushButton_Chat_NextScenario_clicked();


  void on_pushButton_Chat_SwitchToReference_clicked();

  void on_pushButton_History_compareResults_clicked();

  void on_pushButton_History_updateResults_clicked();

  void on_pushButton_clicked();

  void on_pushButton_history_microForChat_clicked();

  void on_actionAdd_domain_triggered();

  void on_actionSet_problem_triggered();

private:
  /*
  struct ChatbotSemExpParam
  {
    std::string urlizeInput{};
    UniqueSemanticExpression semExp{};
    UniqueSemanticExpression semExpMergedWithContext{};
    cp::SetOfFacts effect{};
    std::vector<cp::Goal> goalsToAdd{};
  };
  */
  struct TextWithLanguage
  {
    TextWithLanguage(const std::string& pText,
                     SemanticLanguageEnum pLanguage)
      : text(pText),
        language(pLanguage)
    {
    }
    std::string text;
    SemanticLanguageEnum language;
  };
  Ui::MainWindow* _ui;
  /// current size of the window
  std::pair<int, int> _sizeWindow;
  const std::filesystem::path _corpusEquivalencesFolder;
  const std::filesystem::path _corpusResultsFolder;
  const std::filesystem::path _inputScenariosFolder;
  const std::filesystem::path _outputScenariosFolder;
  const std::filesystem::path _corpusFolder;
  bool _listenToANewTokenizerStep;
  onsem::linguistics::LinguisticDatabase _lingDb;
  onsem::SemanticLanguageEnum _currentLanguage;
  std::string _currReformulationInSameLanguage;
  std::map<onsem::SemanticLanguageEnum, std::list<std::string>> fLangToTokenizerSteps;
  bool _newOrOldVersion;
  std::unique_ptr<onsem::SemanticMemory> _semMemoryPtr;
  std::unique_ptr<onsem::SemanticMemory> _semMemoryBinaryPtr;
  mystd::observable::Connection _infActionAddedConnection;
  std::unique_ptr<onsem::ChatbotDomain> _chatbotDomain;
  std::unique_ptr<onsem::ChatbotProblem> _chatbotProblem;
  //std::list<ChatbotSemExpParam> _currentActionParameters;
  //std::unique_ptr<cp::SetOfFacts> _effectAfterCurrentInput;
  ScenarioContainer _scenarioContainer;
  const std::string _inLabel;
  QColor _outFontColor;
  QColor _inFontColor;
  bool _isSpeaking;
  std::size_t _nbOfSecondToWaitAfterTtsSpeech;
  bool _asrIsWaiting;
  bool _shouldWaitForNewSpeech;

  /// Display the dot image
  onsem::SentencesLoader fSentenceLoader;
  std::map<QObject*, onsem::LineEditHistoricalWrapper> _lineEditHistorical;

  onsem::SemanticLanguageEnum _getSelectedLanguageType();
  void _updateCurrentLanguage(onsem::SemanticLanguageEnum pNewLanguage);

  /// Refresh the syntactic graph image of the sentence.
  void _showImageInACanvas
  (const std::string& pImagePath,
   const QWidget& pHoldingWidget,
   QLabel& pLabelWeToDisplayTheImage);

  void xDisplayOldResult();

  void _loadSentences(bool pTxtFirstChoice,
                      const std::string& pTextCorpusPath);

  std::string _operator_react(
      onsem::ContextualAnnotation& pContextualAnnotation,
      std::list<std::string>& pReferences,
      const std::string& pText,
      SemanticLanguageEnum& pTextLanguage,
      std::string& pOutAnctionId,
      std::map<std::string, std::vector<std::string>>& pParameters);
  void _onNewTextSubmitted(const std::string& pText,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
  void _proactivityFromPlanner(std::list<TextWithLanguage>& pTextsToSay,
                               std::set<std::string>& pActionIdsToSkip,
                               const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
  void _printParametersAndNotifyPlanner(const cp::OneStepOfPlannerResult& pOneStepOfPlannerResult,
                                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
  void _printChatRobotMessage(const std::string& pText);
  void _sayText(std::list<TextWithLanguage>& pTextsToSay);
  void _loadCurrScenario();
  void _switchToReferenceButtonSetEnabled(bool pEnabled);
  void _appendLogs(const std::list<std::string>& pLogs);
  void _clearLoadedScenarios();
  std::filesystem::path _getEquivalencesFilename();
  void _readEquivalences(std::map<std::string, std::string>& pEquivalences,
                         const std::filesystem::path &pPath);
  void _writeEquivalences(const std::map<std::string, std::string>& pEquivalences,
                          const std::filesystem::path &pPath);

  std::string _getAsrText(bool& pTextEnd);
  void _printGoalsAndFacts();
  void _proactivelyAskThePlanner(const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow);
};

#endif // ONSEMVOICEBOT_MAINWINDOW_H
