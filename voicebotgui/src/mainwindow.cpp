#include "mainwindow.h"
#include <iostream>
#include <sstream>
#include <QString>
#include <QFileDialog>
#include <QTimer>
#include <QMessageBox>
#include <QInputDialog>
#include <QKeyEvent>
#include <QFileDialog>
#include "ui_mainwindow.h"
#include <boost/property_tree/xml_parser.hpp>
#include <contextualplanner/util/replacevariables.hpp>
#include <onsem/common/utility/string.hpp>
#include <onsem/common/utility/uppercasehandler.hpp>
#include <onsem/texttosemantic/dbtype/semanticexpression/feedbackexpression.hpp>
#include <onsem/texttosemantic/dbtype/semanticexpression/groundedexpression.hpp>
#include <onsem/texttosemantic/dbtype/semanticexpression/listexpression.hpp>
#include <onsem/texttosemantic/dbtype/semanticexpression/metadataexpression.hpp>
#include <onsem/texttosemantic/dbtype/semanticgrounding/semanticconceptualgrounding.hpp>
#include <onsem/texttosemantic/dbtype/semanticgrounding/semanticlanguagegrounding.hpp>
#include <onsem/texttosemantic/dbtype/semanticgrounding/semantictimegrounding.hpp>
#include <onsem/texttosemantic/dbtype/semanticgrounding/semanticstatementgrounding.hpp>
#include <onsem/texttosemantic/tool/semexpgetter.hpp>
#include <onsem/texttosemantic/tool/semexpmodifier.hpp>
#include <onsem/texttosemantic/languagedetector.hpp>
#include <onsem/semantictotext/triggers.hpp>
#include <onsem/semantictotext/tool/semexpcomparator.hpp>
#include <onsem/semantictotext/outputter/executiondataoutputter.hpp>
#include <onsem/semantictotext/outputter/virtualoutputter.hpp>
#include <onsem/semantictotext/serialization.hpp>
#include <onsem/semantictotext/semanticmemory/semanticmemory.hpp>
#include <onsem/semantictotext/semanticmemory/semanticbehaviordefinition.hpp>
#include <onsem/semantictotext/semexpoperators.hpp>
#include <onsem/semantictotext/semanticconverter.hpp>
#include <onsem/semanticdebugger/dotsaver.hpp>
#include <onsem/semanticdebugger/syntacticgraphresult.hpp>
#include <onsem/semanticdebugger/diagnosisprinter.hpp>
#include <onsem/tester/reactOnTexts.hpp>
#include <onsem/tester/syntacticanalysisxmlsaver.hpp>


namespace {
const QString _anyLangLabel = "Any Language";
const QString _currentResultsStr = "current results";
const QString _referenceResultsStr = "reference results";
const int _bottomBoxHeight = 70;
const int _leftTokens = 10;
const int _leftGramPoss = 170;
const int _leftFinalGramPoss = 390;
const int _leftFinalConcepts = 610;
const int _leftContextualInfos = 830;
static const QString microStr = "micro";
static const QString stopMicroStr = "stop";
const std::string _tmpFolder = ".";

void _convertParameters(
        std::map<std::string, std::string>& pOutParameters,
        const std::map<std::string, std::vector<std::string>>& pInParameters)
{
  for (auto& currParamWithValues : pInParameters)
  {
    if (!currParamWithValues.second.empty())
    {
      auto paramValue = currParamWithValues.second[0];
      mystd::replace_all(paramValue, " ", "_");
      pOutParameters[currParamWithValues.first] = paramValue;
    }
  }
}


void _convertToParametersPrintable(
        std::map<std::string, std::string>& pOutParameters,
        const std::map<std::string, std::set<std::string>>& pInParameters)
{
  for (auto& currParamWithValues : pInParameters)
  {
    if (!currParamWithValues.second.empty())
    {
      auto paramValue = *currParamWithValues.second.begin();
      onsem::lowerCaseFirstLetter(paramValue);
      mystd::replace_all(paramValue, "_", " ");
      pOutParameters[currParamWithValues.first] = paramValue;
    }
  }
}

std::string _semExptoStr(const SemanticExpression& pSemExp,
                         SemanticLanguageEnum pLanguage,
                         SemanticMemory& pMemory,
                         const linguistics::LinguisticDatabase& pLingDb)
{
  TextProcessingContext outContext(SemanticAgentGrounding::me,
                                   SemanticAgentGrounding::currentUser,
                                   pLanguage);
  OutputterContext outputterContext(outContext);
  ExecutionDataOutputter executionDataOutputter(pMemory, pLingDb);
  executionDataOutputter.processSemExp(pSemExp, outputterContext);
  return executionDataOutputter.rootExecutionData.run(pMemory, pLingDb);
}

std::string _imperativeToMandatory(const SemanticExpression& pSemExp,
                                   SemanticLanguageEnum pLanguage,
                                   SemanticMemory& pMemory,
                                   const linguistics::LinguisticDatabase& pLingDb)
{
  UniqueSemanticExpression semExpCopied = pSemExp.clone();
  converter::imperativeToMandatory(semExpCopied);
  return _semExptoStr(*semExpCopied, pLanguage, pMemory, pLingDb);
}

void _imperativeToIndicativeSemExp(SemanticExpression& pSemExp,
                                   bool pNegated)
{
  auto* grdExpPtr = pSemExp.getGrdExpPtr_SkipWrapperPtrs();
  if (grdExpPtr != nullptr)
  {
    auto* statGrdPtr = grdExpPtr->grounding().getStatementGroundingPtr();
    if (statGrdPtr != nullptr)
    {
      statGrdPtr->requests.erase(SemanticRequestType::ACTION);
    }
    if (pNegated)
      SemExpModifier::invertPolarityFromGrdExp(*grdExpPtr);
  }

  auto* listExpPtr = pSemExp.getListExpPtr_SkipWrapperPtrs();
  if (listExpPtr != nullptr)
    for (auto& currElt : listExpPtr->elts)
      _imperativeToIndicativeSemExp(*currElt, pNegated);
}

UniqueSemanticExpression _sayOk()
{
  return std::make_unique<GroundedExpression>
      ([]()
  {
    auto okGrounding = std::make_unique<SemanticGenericGrounding>();
    okGrounding->word.language = SemanticLanguageEnum::ENGLISH;
    okGrounding->word.lemma = "ok";
    okGrounding->word.partOfSpeech = PartOfSpeech::ADVERB; // TODO: check that this meaning exists
    return okGrounding;
  }());
}

std::string _imperativeToIndicative(const SemanticExpression& pSemExp,
                                    SemanticLanguageEnum pLanguage,
                                    SemanticMemory& pMemory,
                                    bool pNegated,
                                    bool pAddFeedback,
                                    const linguistics::LinguisticDatabase& pLingDb)
{
  UniqueSemanticExpression semExpCopied = pSemExp.clone();
  _imperativeToIndicativeSemExp(*semExpCopied, pNegated);
  if (pAddFeedback)
    semExpCopied = std::make_unique<FeedbackExpression>(_sayOk(), std::move(semExpCopied));
  return _semExptoStr(*semExpCopied, pLanguage, pMemory, pLingDb);
}


std::string _mergeFactAndReason(std::string& pFact,
                                std::string& pReason)
{
  onsem::lowerCaseFirstLetter(pReason);
  if (pFact[pFact.size() - 1] == '.')
    pFact = pFact.substr(0, pFact.size() - 1);
  if (pFact[pFact.size() - 1] == '!')
    pFact = pFact.substr(0, pFact.size() - 1);
  if (pFact[pFact.size() - 1] == ' ')
    pFact = pFact.substr(0, pFact.size() - 1);
  return pFact + " parce que " + pReason;
}

std::string _mergeFactAndReasonConst(std::string& pFact,
                                     const std::string& pReason)
{
  auto reason = pReason;
  return _mergeFactAndReason(pFact, reason);
}

}

using namespace onsem;

MainWindow::MainWindow(const std::filesystem::path& pCorpusEquivalencesFolder,
                       const std::filesystem::path& pCorpusResultsFolder,
                       const std::filesystem::path& pInputScenariosFolder,
                       const std::filesystem::path& pOutputScenariosFolder,
                       const std::filesystem::path& pCorpusFolder,
                       linguistics::LinguisticDatabaseStreams& pIStreams,
                       QWidget *parent) :
  QMainWindow(parent),
  _ui(new Ui::MainWindow),
  _sizeWindow(0, 0),
  _corpusEquivalencesFolder(pCorpusEquivalencesFolder),
  _corpusResultsFolder(pCorpusResultsFolder),
  _inputScenariosFolder(pInputScenariosFolder),
  _outputScenariosFolder(pOutputScenariosFolder),
  _corpusFolder(pCorpusFolder),
  _listenToANewTokenizerStep(false),
  _lingDb(pIStreams),
  _currentLanguage(SemanticLanguageEnum::UNKNOWN),
  _currReformulationInSameLanguage(),
  fLangToTokenizerSteps(),
  _newOrOldVersion(true),
  _semMemoryPtr(std::make_unique<SemanticMemory>()),
  _semMemoryBinaryPtr(std::make_unique<SemanticMemory>()),
  _infActionAddedConnection(),
  _chatbotDomain(),
  _chatbotProblem(),
  _scenarioContainer(),
  _inLabel("in:"),
  _outFontColor("grey"),
  _inFontColor("black"),
  _isSpeaking(false),
  _nbOfSecondToWaitAfterTtsSpeech(0),
  _asrIsWaiting(true),
  _shouldWaitForNewSpeech(false),
  fSentenceLoader(),
  _lineEditHistorical()
{
  _ui->setupUi(this);
  resize(1267, 750);
  _clearLoadedScenarios();

  _ui->pushButton_history_microForChat->setText(microStr);

  _ui->textBrowser_chat_history->viewport()->setAutoFillBackground(false);
  _ui->textBrowser_chat_history->setAttribute(Qt::WA_TranslucentBackground, true);

  _ui->textBrowser_goals->viewport()->setAutoFillBackground(false);
  _ui->textBrowser_goals->setAttribute(Qt::WA_TranslucentBackground, true);

  _ui->textBrowser_facts->viewport()->setAutoFillBackground(false);
  _ui->textBrowser_facts->setAttribute(Qt::WA_TranslucentBackground, true);

  _ui->textBrowser_PrintMemory->viewport()->setAutoFillBackground(false);
  _ui->textBrowser_PrintMemory->setAttribute(Qt::WA_TranslucentBackground, true);
  _ui->textBrowser_PrintMemory->setLineWrapMode(QTextEdit::NoWrap);

  _lineEditHistorical.emplace(_ui->lineEdit_history_newText,
                              LineEditHistoricalWrapper(_ui->lineEdit_history_newText, this));

  // Fit objects to the windows every seconds
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(onRefresh()));
  timer->start(100);

  // fill tokenizer steps
  auto fillDebugSteps = [this](std::list<std::string>& pSteps,
      SemanticLanguageEnum pLanguageType)
  {
    const auto& specLingDb = _lingDb.langToSpec[pLanguageType];
    for (const auto& currContextFilter : specLingDb.getContextFilters())
      pSteps.emplace_back(currContextFilter->getName());
  };
  fillDebugSteps(fLangToTokenizerSteps[SemanticLanguageEnum::ENGLISH], SemanticLanguageEnum::ENGLISH);
  fillDebugSteps(fLangToTokenizerSteps[SemanticLanguageEnum::FRENCH], SemanticLanguageEnum::FRENCH);
  _updateCurrentLanguage(_currentLanguage);

  SemanticTimeGrounding::setAnHardCodedTimeElts(true, true);
}

MainWindow::~MainWindow()
{
  _infActionAddedConnection.disconnect();
  delete _ui;
  _ui = nullptr;
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (_ui == nullptr)
    return QObject::eventFilter(obj, event);
  auto itHistorical = _lineEditHistorical.find(obj);
  if (itHistorical != _lineEditHistorical.end())
  {
    if (event->type() == QEvent::FocusIn)
      itHistorical->second.activate();
    else if (event->type() == QEvent::FocusOut)
      itHistorical->second.desactivate();
  }
  return QObject::eventFilter(obj, event);
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
  switch (event->key())
  {
  case Qt::Key_Up:
  {
    for (auto& currHistorical : _lineEditHistorical)
      currHistorical.second.displayPrevText();
    break;
  }
  case Qt::Key_Down:
  {
    for (auto& currHistorical : _lineEditHistorical)
      currHistorical.second.displayNextText();
    break;
  }
  case Qt::Key_F1:
  {
    for (auto& currHistorical : _lineEditHistorical)
      currHistorical.second.concat();
    break;
  }
  }
}

void MainWindow::onRefresh()
{
  if (_sizeWindow.first != width() || _sizeWindow.second != height())
  {
    _sizeWindow.first = width();
    _sizeWindow.second = height();

    onRescale();
  }

  if (_isSpeaking)
  {
    std::ifstream ttsFinishedFile;
    ttsFinishedFile.open("tts_finished.txt");
    std::string line;
    _isSpeaking = getline(ttsFinishedFile, line) && line == "n";
    if (!_isSpeaking)
    {
      _nbOfSecondToWaitAfterTtsSpeech = 20;
      _asrIsWaiting = false;
      _shouldWaitForNewSpeech = true;
    }
  }
  else if (_nbOfSecondToWaitAfterTtsSpeech > 0)
  {
    --_nbOfSecondToWaitAfterTtsSpeech;
  }

  bool microForChatEnabled = _ui->pushButton_history_microForChat->text() == stopMicroStr;
  if (microForChatEnabled)
  {
    bool textEnd = false;
    auto asrText = _getAsrText(textEnd);
    if (!asrText.empty())
    {
      auto asrTextQStr = QString::fromUtf8(asrText.c_str());
      if (microForChatEnabled && (textEnd || _ui->lineEdit_history_newText->text() != asrTextQStr))
      {
        _ui->lineEdit_history_newText->setText(asrTextQStr);
        if (textEnd)
          on_lineEdit_history_newText_returnPressed();
      }
    }
  }
}


SemanticLanguageEnum MainWindow::_getSelectedLanguageType()
{
  return SemanticLanguageEnum::ENGLISH; //semanticLanguageTypeGroundingEnumFromStr(_getSelectedLanguageStr());
}


void MainWindow::_updateCurrentLanguage(SemanticLanguageEnum pNewLanguage)
{
  if (pNewLanguage != _currentLanguage)
    _currentLanguage = pNewLanguage;
}



void MainWindow::_showImageInACanvas
(const std::string& pImagePath,
 const QWidget& pHoldingWidget,
 QLabel& pLabelWeToDisplayTheImage)
{
  QPixmap img;
  img.load(QString::fromUtf8(pImagePath.c_str()));
  if (img.isNull())
  {
    return;
  }

  float coefImg = static_cast<float>(img.width()) / static_cast<float>(img.height());
  float coefPanel = static_cast<float>(pHoldingWidget.width()) /
      static_cast<float>(pHoldingWidget.height());
  int x, y, w, h;
  y = 0;
  // if we will fit the image with the width
  if (coefImg > coefPanel)
  {
    x = 0;
    w = pHoldingWidget.width();
    h = static_cast<int>(static_cast<float>(w) / coefImg);
  }
  else // if we will fit the image with the height
  {
    h = pHoldingWidget.height();
    w = static_cast<int>(static_cast<float>(h) * coefImg);
    x = (pHoldingWidget.width() - w) / 2;
  }
  pLabelWeToDisplayTheImage.setPixmap(img.scaled(QSize(w, h), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
  pLabelWeToDisplayTheImage.setGeometry(x, y, w, h);
}


void MainWindow::on_pushButton_AATester_PrevSentence_clicked()
{
  std::string sentence;
  fSentenceLoader.getPrevSentence(sentence);
  if (!_newOrOldVersion)
    xDisplayOldResult();
}


void MainWindow::on_pushButton_AATester_NextSentence_clicked()
{
  std::string sentence;
  fSentenceLoader.getNextSentence(sentence);
  if (!_newOrOldVersion)
    xDisplayOldResult();
}

void MainWindow::xDisplayOldResult()
{
  SyntacticAnalysisResultToDisplay autoAnnotToDisplay;
  const auto& resToDisp =
      *fSentenceLoader.getOldResults().oldResultThatDiffers[fSentenceLoader.getCurrIndex()];
  SemanticDebug::semAnalResultToStructToDisplay(autoAnnotToDisplay, resToDisp.semAnal);
  autoAnnotToDisplay.highLevelResults = resToDisp.semAnalHighLevelResults;
}


void MainWindow::onRescale()
{
  onRescaleChatPanel();
}

void MainWindow::onRescaleChatPanel()
{
  int lineEditHeight = 40;
  int grouBoxSidesWidth = 130;
  int asrFrameY = _ui->tab_Chat->height() - _bottomBoxHeight - 10;

  _ui->label_chat_title->setGeometry(10, 10, _ui->tab_Chat->width(), _ui->label_chat_title->height());
  {
    int mainFrameY = 90;
    int asrFrameY = _ui->tab_Chat->height() - _bottomBoxHeight - 10;
    int goalsHeight = 300;
    int goalsWidth = _ui->tab_Chat->width() / 2;

    _ui->textBrowser_chat_history->setGeometry(10,
                                               mainFrameY,
                                               _ui->tab_Chat->width() - 20 - goalsWidth,
                                               asrFrameY - mainFrameY - 20);

    _ui->label_goals->setGeometry(10 + _ui->textBrowser_chat_history->width(),
                                  mainFrameY,
                                  goalsWidth,
                                  _ui->label_goals->height());

    _ui->textBrowser_goals->setGeometry(10 + _ui->textBrowser_chat_history->width(),
                                        mainFrameY + 10 + _ui->label_goals->height(),
                                        goalsWidth,
                                        goalsHeight);

    _ui->label_world_state->setGeometry(10 + _ui->textBrowser_chat_history->width(),
                                  mainFrameY + goalsHeight,
                                  goalsWidth,
                                  _ui->label_world_state->height());

    _ui->textBrowser_facts->setGeometry(10 + _ui->textBrowser_chat_history->width(),
                                        mainFrameY + goalsHeight + 10 + _ui->label_world_state->height(),
                                        goalsWidth,
                                        asrFrameY - mainFrameY - 20  - goalsHeight);
  }

  // speech frame
  _ui->frame_history_asr->setGeometry(10, asrFrameY, _ui->tab_Chat->width() - 20, _bottomBoxHeight);
  _ui->lineEdit_history_newText->setGeometry(20 + grouBoxSidesWidth, 15,
                                             _ui->frame_history_asr->width() - 40 - grouBoxSidesWidth * 2, lineEditHeight);
  _ui->widgetRight_history->setGeometry(_ui->frame_history_asr->width() - 10 - grouBoxSidesWidth, 0,
                                        grouBoxSidesWidth, _bottomBoxHeight);
}


void MainWindow::onRescaleChatDiagnosisPanel()
{
  _ui->textBrowser_PrintMemory->setGeometry(10, 100, _ui->tab_chatdiagnosis->width() - 20,
                                            _ui->tab_chatdiagnosis->height() - 110);
}



void MainWindow::on_tabWidget_currentChanged(int index)
{
  switch (index)
  {
  case 0:
    onRescaleChatPanel();
    break;
  case 1:
    onRescaleChatDiagnosisPanel();
    break;
  }
}



void MainWindow::on_texts_load_triggered()
{
  SemanticLanguageEnum langType = _getSelectedLanguageType();
  const std::string textCorpusFolder = _corpusFolder.string() + "/input";
  if (langType == SemanticLanguageEnum::UNKNOWN)
    _loadSentences(true, textCorpusFolder);
  else
    _loadSentences(true, textCorpusFolder + "/" + semanticLanguageEnum_toLegacyStr(langType));
}


void MainWindow::_loadSentences
(bool pTxtFirstChoice,
 const std::string& pTextCorpusPath)
{
  std::string filter = pTxtFirstChoice ?
        "Text file (*.txt);;Xml file (*.xml)" :
        "Xml file (*.xml);;Text file (*.txt)";
  QString fichier = QFileDialog::getOpenFileName
      (this, "Load a sentences file", QString::fromUtf8(pTextCorpusPath.c_str()), filter.c_str());
  std::string filename = fichier.toUtf8().constData();
  if (!filename.empty())
    fSentenceLoader.loadFile(filename);
}


void MainWindow::on_actionExport_to_ldic_triggered()
{
  const QString extensionLdic = ".ldic";
  const QString firstStr = "Dictionaries (*" + extensionLdic + ")";
  QString selectedFilter;
  QString fileToWrite = QFileDialog::getSaveFileName
      (this, "Export the dictionaries", QString(),
       firstStr, &selectedFilter);
  std::string fileToWriteStr = std::string(fileToWrite.toUtf8().constData());
  if (fileToWriteStr.empty())
    return;
  fileToWriteStr += extensionLdic.toUtf8().constData();
  boost::property_tree::ptree propTree;
  serialization::saveLingDatabase(propTree, _lingDb);
  serialization::propertyTreeToZipedFile(propTree, fileToWriteStr, ".ldic");
}


void MainWindow::on_actionImport_from_ldic_triggered()
{
  const QString extensionLdic = ".ldic";
  const QString firstStr = "Dictionaries (*" + extensionLdic + ")";

  std::string filename = QFileDialog::getOpenFileName
      (this, "Import dictionaries", QString(), firstStr).toUtf8().constData();
  if (filename.empty())
    return;
  boost::property_tree::ptree propTree;
  serialization::propertyTreeFromZippedFile(propTree, filename);
  serialization::loadLingDatabase(propTree, _lingDb);
}

void MainWindow::on_lineEdit_history_newText_returnPressed()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  _onNewTextSubmitted(_ui->lineEdit_history_newText->text().toUtf8().constData(), now);
  _ui->lineEdit_history_newText->clear();
}


void MainWindow::_execDataToRaskIds(
    std::list<RobotTaskId>& pRobotTaskIds,
    const ExecutionData& pExecutionData)
{
  if (pExecutionData.resource)
  {
    RobotTaskId newTaskId;
    if (pExecutionData.resource->label == "action")
    {
      newTaskId.outAnctionId = pExecutionData.resource->value;
      newTaskId.parameters = pExecutionData.resourceParameters;
    }
    else if (pExecutionData.resource->label == "removeGoal")
    {
      newTaskId.goalToRemove = pExecutionData.resource->value;
    }
    pRobotTaskIds.emplace_back(std::move(newTaskId));
  }

  for (auto& currElt : pExecutionData.toRunSequencially)
    _execDataToRaskIds(pRobotTaskIds, currElt);
  for (auto& currElt : pExecutionData.toRunInParallel)
    _execDataToRaskIds(pRobotTaskIds, currElt);
  for (auto& currElt : pExecutionData.toRunInBackground)
    _execDataToRaskIds(pRobotTaskIds, currElt);
}


void MainWindow::_operator_match(
    ContextualAnnotation& pContextualAnnotation,
    std::list<std::string>& pReferences,
    const SemanticExpression& pSemExp,
    SemanticLanguageEnum& pTextLanguage,
    std::list<RobotTaskId>& pRobotTaskIds)
{
  auto& semMemory = *_semMemoryPtr;
  mystd::unique_propagate_const<UniqueSemanticExpression> reaction;
  triggers::match(reaction, semMemory, pSemExp.clone(), _lingDb, nullptr);
  if (!reaction)
    return;
  pContextualAnnotation = SemExpGetter::extractContextualAnnotation(**reaction);
  SemExpGetter::extractReferences(pReferences, **reaction);

  TextProcessingContext outContext(SemanticAgentGrounding::me,
                                   SemanticAgentGrounding::currentUser,
                                   pTextLanguage);
  OutputterContext outputterContext(outContext);
  ExecutionDataOutputter executionDataOutputter(semMemory, _lingDb);
  executionDataOutputter.processSemExp(**reaction, outputterContext);
  _execDataToRaskIds(pRobotTaskIds, executionDataOutputter.rootExecutionData);
}

void MainWindow::_operator_resolveCommand(
    ContextualAnnotation& pContextualAnnotation,
    std::list<std::string>& pReferences,
    const SemanticExpression& pSemExp,
    SemanticLanguageEnum& pTextLanguage,
    std::list<RobotTaskId>& pRobotTaskIds)
{
  auto& semMemory = *_semMemoryPtr;
  auto reaction = memoryOperation::resolveCommand(pSemExp, semMemory, _lingDb);
  if (!reaction)
    return;
  pContextualAnnotation = SemExpGetter::extractContextualAnnotation(**reaction);
  SemExpGetter::extractReferences(pReferences, **reaction);

  TextProcessingContext outContext(SemanticAgentGrounding::me,
                                   SemanticAgentGrounding::currentUser,
                                   pTextLanguage);
  OutputterContext outputterContext(outContext);
  ExecutionDataOutputter executionDataOutputter(semMemory, _lingDb);
  executionDataOutputter.processSemExp(**reaction, outputterContext);
  _execDataToRaskIds(pRobotTaskIds, executionDataOutputter.rootExecutionData);
}

std::string MainWindow::_operator_react(
    ContextualAnnotation& pContextualAnnotation,
    std::list<std::string>& pReferences,
    const SemanticExpression& pSemExp,
    SemanticLanguageEnum& pTextLanguage)
{
  auto& semMemory = *_semMemoryPtr;
  mystd::unique_propagate_const<UniqueSemanticExpression> reaction;
  memoryOperation::react(reaction, semMemory, pSemExp.clone(), _lingDb, nullptr);
  if (!reaction)
    return "";
  pContextualAnnotation = SemExpGetter::extractContextualAnnotation(**reaction);
  SemExpGetter::extractReferences(pReferences, **reaction);

  TextProcessingContext outContext(SemanticAgentGrounding::me,
                                   SemanticAgentGrounding::currentUser,
                                   pTextLanguage);
  OutputterContext outputterContext(outContext);
  ExecutionDataOutputter executionDataOutputter(semMemory, _lingDb);
  executionDataOutputter.processSemExp(**reaction, outputterContext);

  return executionDataOutputter.rootExecutionData.run(semMemory, _lingDb);
}



void MainWindow::_onNewTextSubmitted(const std::string& pText,
                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  _ui->textBrowser_chat_history->setTextColor(_inFontColor);

  LineEditHistoricalWrapper& hWrapper = _lineEditHistorical[_ui->lineEdit_history_newText];
  hWrapper.addNewText(pText, true);
  hWrapper.goToEndOfHistorical();

  std::list<TextWithLanguage> textsToSay;
  if (!pText.empty() && pText[0] == '+')
  {
    auto factToAddStr = pText.substr(1, pText.size() - 1);
    _ui->textBrowser_chat_history->append("addFact: " + QString::fromUtf8(factToAddStr.c_str()));
    _chatbotProblem->problem.modifyFacts(cp::FactModification::fromStr(factToAddStr), pNow);
    _proactivityFromPlanner(textsToSay, pNow);
  }
  else if (!pText.empty() && pText[0] == '-')
  {
    auto factToAddStr = pText.substr(1, pText.size() - 1);
    _ui->textBrowser_chat_history->append("removeFact: " + QString::fromUtf8(factToAddStr.c_str()));
    _chatbotProblem->problem.modifyFacts(cp::FactModification::fromStr("!" + factToAddStr), pNow);
    _proactivityFromPlanner(textsToSay, pNow);
  }
  else
  {
    if (pText.empty())
      _ui->textBrowser_chat_history->append("ping");
    else
      _ui->textBrowser_chat_history->append(QString::fromUtf8(_inLabel.c_str()) + " \"" +
                                            QString::fromUtf8(pText.c_str()) + "\"");
    auto contextualAnnotation = ContextualAnnotation::ANSWER;

    std::list<std::string> references;

    auto& semMemory = *_semMemoryPtr;
    auto textLanguage = linguistics::getLanguage(pText, _lingDb);
    TextProcessingContext inContext(SemanticAgentGrounding::currentUser,
                                    SemanticAgentGrounding::me,
                                    textLanguage);
    inContext.spellingMistakeTypesPossible.insert(SpellingMistakeType::CONJUGATION);
    auto semExp =
        converter::textToContextualSemExp(pText, inContext,
                                          SemanticSourceEnum::ASR, _lingDb);

    memoryOperation::mergeWithContext(semExp, semMemory, _lingDb);
    if (textLanguage == SemanticLanguageEnum::UNKNOWN)
      textLanguage = semMemory.defaultLanguage;

    std::list<RobotTaskId> robotTaskIds;
    _operator_match(contextualAnnotation, references, *semExp,
                    textLanguage, robotTaskIds);

    if (robotTaskIds.empty())
      _operator_resolveCommand(contextualAnnotation, references, *semExp,
                               textLanguage, robotTaskIds);

    auto inputCategory = memoryOperation::categorize(*semExp);

    if (!robotTaskIds.empty())
    {
      for (auto& currRobotTaskId : robotTaskIds)
      {
        if (!currRobotTaskId.goalToRemove.empty())
        {
          if (_chatbotProblem->problem.removeGoals(currRobotTaskId.goalToRemove, pNow))
          {
            auto itRemovalConf = _chatbotProblem->goalToRemovalConfirmation.find(currRobotTaskId.goalToRemove);
            if (itRemovalConf != _chatbotProblem->goalToRemovalConfirmation.end())
            {
              _printChatRobotMessage("tts: \"" + itRemovalConf->second + "\"");
              textsToSay.emplace_back(itRemovalConf->second, textLanguage);
            }
          }
        }
        else if (!currRobotTaskId.outAnctionId.empty())
        {
          auto itAction = _chatbotDomain->actions.find(currRobotTaskId.outAnctionId);
          if (itAction != _chatbotDomain->actions.end())
          {
            const ChatbotAction& cbAction = itAction->second;
            std::string text = cbAction.text;

            auto actionDescription = cbAction.description;

            // notify memory of the text said
            if (!text.empty())
              _saySemExp(text, actionDescription, textsToSay, _chatbotProblem->variables, textLanguage);

            std::map<std::string, std::string> parameters;
            _convertParameters(parameters, currRobotTaskId.parameters);
            if (cbAction.effect)
              _chatbotProblem->problem.modifyFacts(cbAction.effect->clone(&parameters), pNow);

            cp::replaceVariables(actionDescription, parameters);
            _chatbotProblem->variables["currentAction"] = actionDescription;

            if (!cbAction.goalsToAdd.empty())
            {
              auto intentionNaturalLanguage = _imperativeToMandatory(*semExp, textLanguage, semMemory, _lingDb);
              for (const auto& currGoalWithPririty : cbAction.goalsToAdd)
                for (const auto& currGoal : currGoalWithPririty.second)
                  _chatbotProblem->problem.pushFrontGoal(cp::Goal(currGoal, &parameters, &intentionNaturalLanguage), pNow, currGoalWithPririty.first);

              auto* goalPtr = _chatbotProblem->problem.getCurrentGoalPtr();
              if (goalPtr == nullptr || goalPtr->getGoalGroupId() != intentionNaturalLanguage)
              {
                std::string sayIWillDoItAfter;
                if (inputCategory == SemanticExpressionCategory::COMMAND)
                {
                  UniqueSemanticExpression inSemExp = semExp->clone();
                  _imperativeToIndicativeSemExp(*inSemExp, false);
                  SemExpModifier::modifyVerbTenseOfSemExp(*inSemExp, SemanticVerbTense::FUTURE);
                  auto* inGrdExpPtr = inSemExp->getGrdExpPtr_SkipWrapperPtrs();
                  if (inGrdExpPtr != nullptr)
                  {
                    SemExpModifier::addChild(*inGrdExpPtr, GrammaticalType::TIME,
                                             std::make_unique<GroundedExpression>(std::make_unique<SemanticConceptualGrounding>("time_relative_later")));
                    sayIWillDoItAfter = _semExptoStr(*inSemExp, textLanguage, *_semMemoryPtr, _lingDb);
                  }
                }

                if (sayIWillDoItAfter.empty())
                  sayIWillDoItAfter = "D'accord, je le ferai plus tard";
                auto itIntention = _chatbotProblem->variables.find("intention");
                if (itIntention != _chatbotProblem->variables.end())
                  sayIWillDoItAfter = _mergeFactAndReasonConst(sayIWillDoItAfter, itIntention->second);
                else
                  sayIWillDoItAfter += " parce que je fais autre chose.";
                _printChatRobotMessage("tts: \"" + sayIWillDoItAfter + "\"");
                textsToSay.emplace_back(sayIWillDoItAfter, textLanguage);
              }

              _chatbotProblem->goalToRemovalConfirmation[intentionNaturalLanguage] =
                  _imperativeToIndicative(*semExp, textLanguage, semMemory, true, true, _lingDb);

              // Add triggers to remove the goal
              // TODO: track the goal life to remove this trigger
              UniqueSemanticExpression invertedSemExp = semExp->clone();
              SemExpModifier::invertPolarity(*invertedSemExp);
              auto outputResourceGrdExp =
                  std::make_unique<GroundedExpression>(
                    converter::createResourceWithParameters("removeGoal", intentionNaturalLanguage, {},
                                                            *invertedSemExp, _lingDb, textLanguage));

              triggers::add(std::move(invertedSemExp), std::move(outputResourceGrdExp), semMemory, _lingDb);
            }
          }
          _printGoalsAndFacts();
        }
      }
    }
    else
    {
      if (inputCategory == SemanticExpressionCategory::QUESTION)
      {
        _printChatRobotMessage("llm answer of: \"" + pText + "\"");
        return;
      }

      auto text = _operator_react(contextualAnnotation, references, *semExp, textLanguage);
      if (!text.empty())
      {
        _printChatRobotMessage("tts: \"" + text + "\"");
        textsToSay.emplace_back(text, textLanguage);
      }
    }

    if (contextualAnnotation != ContextualAnnotation::QUESTION &&
        contextualAnnotation != ContextualAnnotation::TEACHINGFEEDBACK)
      _proactivityFromPlanner(textsToSay, pNow);
  }

  if (_ui->checkBox_enable_tts->isChecked() && !textsToSay.empty())
    _sayText(textsToSay);
}


void MainWindow::_proactivityFromPlanner(std::list<TextWithLanguage>& pTextsToSay,
                                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (_chatbotDomain && _chatbotProblem)
  {
    std::set<std::string> actionIdsToSkip;
    while (true)
    {
      auto oneStepOfPlannerResult = cp::lookForAnActionToDo(_chatbotProblem->problem, *_chatbotDomain->compiledDomain, pNow, &_chatbotProblem->problem.historical);
      if (oneStepOfPlannerResult && actionIdsToSkip.count(oneStepOfPlannerResult->actionInstance.actionId) == 0)
      {
        auto actionId = oneStepOfPlannerResult->actionInstance.actionId;
        auto itAction = _chatbotDomain->actions.find(actionId);
        if (itAction != _chatbotDomain->actions.end())
        {
          const ChatbotAction& cbAction = itAction->second;

          std::map<std::string, std::string> printableParameters;
          _convertToParametersPrintable(printableParameters, oneStepOfPlannerResult->actionInstance.parameters);

          auto actionDescription = cbAction.description;
          if (!cbAction.text.empty())
          {
            std::string text = cbAction.text;
            _saySemExp(text, actionDescription, pTextsToSay, printableParameters, cbAction.language);
          }
          else
          {
            _printChatRobotMessage(oneStepOfPlannerResult->actionInstance.toStr());
          }

          cp::replaceVariables(actionDescription, printableParameters);
          _chatbotProblem->variables["currentAction"] = actionDescription;

          cp::notifyActionDone(_chatbotProblem->problem, *_chatbotDomain->compiledDomain, *oneStepOfPlannerResult, pNow);

          if (cbAction.potentialEffect && !cbAction.effect)
            break;
          actionIdsToSkip.insert(oneStepOfPlannerResult->actionInstance.actionId);
          continue;
        }
      }
      break;
    }
    _printGoalsAndFacts();
  }
}


void MainWindow::_printChatRobotMessage(const std::string& pText)
{
  _ui->textBrowser_chat_history->setTextColor(_outFontColor);
  _ui->textBrowser_chat_history->append(QString::fromUtf8(pText.c_str()));
}

void MainWindow::_saySemExp(std::string& pText,
                            std::string& pActionDescription,
                            std::list<TextWithLanguage>& pTextsToSay,
                            const std::map<std::string, std::string>& pVariables,
                            SemanticLanguageEnum pLanguage)
{
  cp::replaceVariables(pText, pVariables);
  mystd::replace_all(pText, " est les ", " sont les ");

  TextProcessingContext outContext(SemanticAgentGrounding::me,
                                   SemanticAgentGrounding::currentUser,
                                   pLanguage);
  auto from = SemanticSourceEnum::ASR;
  auto semExp =
      converter::textToContextualSemExp(pText, outContext, from, _lingDb);
  memoryOperation::mergeWithContext(semExp, *_semMemoryPtr, _lingDb);
  if (pActionDescription.empty())
  {
    auto* metadataExpPtr = semExp->getMetadataPtr_SkipWrapperPtrs();
    if (metadataExpPtr != nullptr && metadataExpPtr->source)
    {
      SemExpModifier::modifyVerbTenseOfSemExp(**metadataExpPtr->source, SemanticVerbTense::PRESENT);
      pActionDescription = _semExptoStr(**metadataExpPtr->source, pLanguage,
                                       *_semMemoryPtr, _lingDb);
      // Put back the source verb tense
      SemExpModifier::modifyVerbTenseOfSemExp(**metadataExpPtr->source, SemanticVerbTense::PUNCTUALPAST);
    }
  }
  memoryOperation::inform(std::move(semExp), *_semMemoryPtr, _lingDb);

  _printChatRobotMessage("tts: \"" + pText + "\"");
  pTextsToSay.emplace_back(pText, pLanguage);
}


void MainWindow::_sayText(std::list<TextWithLanguage>& pTextsToSay)
{
  system("echo 'n' > tts_finished.txt");
  std::string comandLine;
  std::size_t i = 0;
  for (auto& currTextToSay : pTextsToSay)
  {
    mystd::replace_all(currTextToSay.text, "\"", "\\\"");
    const std::string languageCode = currTextToSay.language == SemanticLanguageEnum::FRENCH ? "fr" : "en";
    std::stringstream ssOutSoundFilename;
    ssOutSoundFilename << "out_tts_" << i << ".mp3";
    std::string outSoundFilename = ssOutSoundFilename.str();
    comandLine += "gtts-cli \"" + currTextToSay.text + "\" --output " + outSoundFilename + " -l " + languageCode + " && ";
    comandLine += "play " + outSoundFilename + " && rm " + outSoundFilename + "  && ";
    ++i;
  }
  comandLine += "echo 'y' > tts_finished.txt &";
  system(comandLine.c_str());
  _isSpeaking = true;
}



void MainWindow::on_actionNew_triggered()
{
  _clearLoadedScenarios();
}

void MainWindow::on_actionQuit_triggered()
{
  this->close();
}


void MainWindow::on_actionSave_triggered()
{
  // Save the current scenario to a file
  const std::string scenarioContent =
      _ui->textBrowser_chat_history->document()->toPlainText().toUtf8().constData();
  if (scenarioContent.empty())
  {
    QMessageBox::warning(this, this->tr("Nothing to save"),
                         this->tr("The current scenario is empty."),
                         QMessageBox::Ok);
    return;
  }

  std::string pathToFile;
  if (_scenarioContainer.isEmpty())
  {
    bool okOfEnterScenarioName;
    QString fileName = QInputDialog::getText(this, this->tr("Enter Scenario name"),
                                             this->tr("Scenario name:"),
                                             QLineEdit::Normal, "",
                                             &okOfEnterScenarioName);
    if (!okOfEnterScenarioName || fileName.isEmpty())
    {
      return;
    }

    pathToFile = _inputScenariosFolder.string() + "/" + std::string(fileName.toUtf8().constData()) + ".txt";
    if (ScenarioContainer::doesFileExist(pathToFile))
    {
      QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "Scenario name already exist",
                                    "Do you want to overwrite it ?",
                                    QMessageBox::Yes | QMessageBox::No);
      if (reply != QMessageBox::Yes)
        return;
    }
  }
  else
  {
    _scenarioContainer.getCurrScenarioFilename(pathToFile);
    pathToFile = _inputScenariosFolder.string() + "/" + pathToFile;
  }

  std::list<std::string> inputLabels{_inLabel};
  ScenarioContainer::writeScenarioToFile(pathToFile, scenarioContent, inputLabels);
}

void MainWindow::on_scenarios_load_triggered()
{
  _scenarioContainer.load(_inputScenariosFolder.string());
  _loadCurrScenario();
}


void MainWindow::_loadCurrScenario()
{
  _ui->pushButton_Chat_PrevScenario->setEnabled(true);
  _ui->pushButton_Chat_NextScenario->setEnabled(true);

  std::string scenarioFilename;
  _scenarioContainer.getCurrScenarioFilename(scenarioFilename);

  std::list<std::string> linesToDisplay;
  _scenarioContainer.getOldContent(linesToDisplay);
  _switchToReferenceButtonSetEnabled(!linesToDisplay.empty());

  if (_ui->pushButton_Chat_SwitchToReference->text() == _referenceResultsStr)
  {
    this->setWindowTitle("reference - " + QString::fromUtf8(scenarioFilename.c_str()));
  }
  else
  {
    this->setWindowTitle(QString::fromUtf8(scenarioFilename.c_str()));
    linesToDisplay.clear();
    getResultOfAScenario(linesToDisplay,
                         (_inputScenariosFolder / scenarioFilename).string(), *_semMemoryPtr, _lingDb);
  }
  _ui->textBrowser_chat_history->clear();
  _ui->textBrowser_goals->clear();
  _ui->textBrowser_facts->clear();
  _appendLogs(linesToDisplay);
}


void MainWindow::_switchToReferenceButtonSetEnabled(bool pEnabled)
{
  bool allowToEnterANewText = true;
  if (pEnabled)
  {
    _ui->pushButton_Chat_SwitchToReference->setEnabled(true);
    allowToEnterANewText = _ui->pushButton_Chat_SwitchToReference->text() == _currentResultsStr;
  }
  else
  {
    _ui->pushButton_Chat_SwitchToReference->setEnabled(false);
    _ui->pushButton_Chat_SwitchToReference->setText(_currentResultsStr);
    allowToEnterANewText = true;
  }
  _ui->lineEdit_history_newText->setEnabled(allowToEnterANewText);
  _ui->pushButton_history_microForChat->setEnabled(allowToEnterANewText);
}


void MainWindow::_appendLogs(const std::list<std::string>& pLogs)
{
  for (const auto& currLog : pLogs)
  {
    _ui->textBrowser_chat_history->setTextColor
        (currLog.compare(0, _inLabel.size(), _inLabel) == 0 ?
           _inFontColor : _outFontColor);
    _ui->textBrowser_chat_history->append(QString::fromUtf8(currLog.c_str()));
  }
}


void MainWindow::_clearLoadedScenarios()
{
  this->setWindowTitle("Social robotics planning lab");
  auto& semMemory = *_semMemoryPtr;
  semMemory.clear();
  _semMemoryBinaryPtr->clear();
  // clear the planner
  {
    _chatbotDomain.reset();
    _chatbotProblem.reset();
  }
  _lingDb.reset();
  memoryOperation::defaultKnowledge(semMemory, _lingDb);

  _infActionAddedConnection.disconnect();
  _infActionAddedConnection =
      semMemory.memBloc.infActionAdded.connectUnsafe([&](intSemId, const GroundedExpWithLinks* pGroundedExpWithLinksPtr)
  {
    if (_chatbotProblem && pGroundedExpWithLinksPtr != nullptr)
    {
      auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
      auto textProcToRobot = TextProcessingContext::getTextProcessingContextToRobot(SemanticLanguageEnum::FRENCH);
      auto textProcFromRobot = TextProcessingContext::getTextProcessingContextFromRobot(SemanticLanguageEnum::FRENCH);
      auto behaviorDef = SemanticMemoryBlock::extractActionFromMemorySentence(*pGroundedExpWithLinksPtr);
      UniqueSemanticExpression formulation1;
      UniqueSemanticExpression formulation2;
      converter::getInfinitiveToTwoDifferentPossibleWayToAskForIt(formulation1, formulation2, std::move(behaviorDef.label));
      std::map<std::string, std::string> varToValue;
      converter::semExpToText(varToValue["comportement_appris"], std::move(formulation1),
                              textProcToRobot, false, semMemory, _lingDb, nullptr);
      converter::semExpToText(varToValue["comportement_appris_2"], std::move(formulation2),
                              textProcToRobot, false, semMemory, _lingDb, nullptr);
      converter::semExpToText(varToValue["comportement_appris_resultat"], converter::getFutureIndicativeAssociatedForm(std::move(behaviorDef.composition)),
                              textProcFromRobot, false, semMemory, _lingDb, nullptr);
      _chatbotProblem->problem.addVariablesToValue(varToValue, now);
      _chatbotProblem->problem.addFact(cp::Fact("robot_learnt_a_behavior"), now);
    }
  });

  _scenarioContainer.clear();
  _ui->textBrowser_chat_history->clear();
  _ui->textBrowser_goals->clear();
  _ui->textBrowser_facts->clear();
  _ui->pushButton_Chat_PrevScenario->setEnabled(false);
  _ui->pushButton_Chat_NextScenario->setEnabled(false);
  _switchToReferenceButtonSetEnabled(false);
}

void MainWindow::on_pushButton_Chat_PrevScenario_clicked()
{
  _scenarioContainer.getMoveToPrevScenario();
  _loadCurrScenario();
}

void MainWindow::on_pushButton_Chat_NextScenario_clicked()
{
  _scenarioContainer.getMoveToNextScenario();
  _loadCurrScenario();
}


void MainWindow::on_pushButton_Chat_SwitchToReference_clicked()
{
  if (_ui->pushButton_Chat_SwitchToReference->text() == _currentResultsStr)
    _ui->pushButton_Chat_SwitchToReference->setText(_referenceResultsStr);
  else
    _ui->pushButton_Chat_SwitchToReference->setText(_currentResultsStr);
  _loadCurrScenario();
}

void MainWindow::on_pushButton_History_compareResults_clicked()
{
  std::string bilan;
  _ui->textBrowser_Chat_RegressionTests->clear();
  if (_scenarioContainer.compareScenariosToReferenceResults(bilan, _inputScenariosFolder.string(),
                                                            _outputScenariosFolder.string(),
                                                            &getResultOfAScenario, _lingDb))
  {
    _ui->pushButton_Chat_PrevScenario->setEnabled(true);
    _ui->pushButton_Chat_NextScenario->setEnabled(true);
    _loadCurrScenario();
  }
  else
  {
    _clearLoadedScenarios();
  }
  _ui->textBrowser_Chat_RegressionTests->append(QString::fromUtf8(bilan.c_str()));
}

void MainWindow::on_pushButton_History_updateResults_clicked()
{
  _scenarioContainer.updateScenariosResults(_inputScenariosFolder.string(), _outputScenariosFolder.string(),
                                            &getResultOfAScenario, _lingDb);
}

void MainWindow::on_pushButton_clicked()
{
  _ui->textBrowser_PrintMemory->setText
      (QString::fromUtf8
       (diagnosisPrinter::diagnosis({"memory", "memoryInformations"}, *_semMemoryPtr, _lingDb).c_str()));
}


std::filesystem::path MainWindow::_getEquivalencesFilename()
{
  std::string languageStr = semanticLanguageEnum_toLegacyStr(_currentLanguage);
  return _corpusEquivalencesFolder /
      std::filesystem::path(languageStr + "_equivalences.xml");
}

void MainWindow::_readEquivalences(std::map<std::string, std::string>& pEquivalences,
                                   const std::filesystem::path& pPath)
{
  try
  {
    boost::property_tree::ptree tree;
    boost::property_tree::read_xml(pPath.string(), tree);
    for (const auto& currSubTree : tree.get_child("equivalences"))
    {
      const boost::property_tree::ptree& attrs = currSubTree.second.get_child("<xmlattr>");
      pEquivalences.emplace(attrs.get<std::string>("in"),
                            attrs.get<std::string>("out"));
    }
  }
  catch (...) {}
}

void MainWindow::_writeEquivalences(const std::map<std::string, std::string>& pEquivalences,
                                    const std::filesystem::path& pPath)
{
  boost::property_tree::ptree tree;
  boost::property_tree::ptree& resultsTree = tree.add_child("equivalences", {});

  for (const auto& currEqu : pEquivalences)
  {
    boost::property_tree::ptree& textTree = resultsTree.add_child("text", {});
    textTree.put("<xmlattr>.in", currEqu.first);
    textTree.put("<xmlattr>.out", currEqu.second);
  }
  boost::property_tree::write_xml(pPath.string(), tree);
}


void MainWindow::on_actionLoad_chat_content_triggered()
{
  const QString extension = ".txt";
  const QString firstStr = "Text corpus (*" + extension + ")";

  std::string filenameStr = QFileDialog::getOpenFileName
      (this, "Import text corpus", QString(), firstStr).toUtf8().constData();
  if (filenameStr.empty())
    return;
  std::size_t nbOfInforms = 0;
  loadOneFileInSemanticMemory(nbOfInforms, filenameStr, *_semMemoryPtr, _lingDb, true, &_tmpFolder);

  if (nbOfInforms > 0)
  {
    std::stringstream message;
    message << "<" << nbOfInforms << " texts informed>\n";
    _ui->textBrowser_chat_history->setTextColor(_outFontColor);
    _ui->textBrowser_chat_history->append(QString::fromUtf8(message.str().c_str()));
  }
}


void MainWindow::on_actionSave_memory_triggered()
{
  const QString extensionSmem = ".smem";
  const QString firstStr = "Memories (*" + extensionSmem + ")";
  QString selectedFilter;
  QString fileToWrite = QFileDialog::getSaveFileName
      (this, "Serialize the memory", QString(),
       firstStr, &selectedFilter);
  std::string fileToWriteStr = std::string(fileToWrite.toUtf8().constData());
  if (fileToWriteStr.empty())
    return;
  fileToWriteStr += extensionSmem.toUtf8().constData();

  boost::property_tree::ptree memoryTree;
  serialization::saveSemMemory(memoryTree, *_semMemoryPtr);
  serialization::propertyTreeToZipedFile(memoryTree, fileToWriteStr, ".smem");
}

void MainWindow::on_actionLoad_memory_triggered()
{
  const QString extensionSmem = ".smem";
  const QString firstStr = "Memories (*" + extensionSmem + ")";

  std::string filename = QFileDialog::getOpenFileName
      (this, "Deserialize a memory", QString(), firstStr).toUtf8().constData();
  if (filename.empty())
    return;

  boost::property_tree::ptree memoryTree;
  serialization::propertyTreeFromZippedFile(memoryTree, filename);
  _clearLoadedScenarios();
  serialization::loadSemMemory(memoryTree, *_semMemoryPtr, _lingDb);
}


void MainWindow::on_actionload_a_smb_triggered()
{
  const QString extensionSmem = ".smb";
  const QString firstStr = "Memories (*" + extensionSmem + ")";

  std::string filenameStr = QFileDialog::getOpenFileName
      (this, "Load a binary memory", QString(), firstStr).toUtf8().constData();
  if (filenameStr.empty())
    return;

  _clearLoadedScenarios();
  _semMemoryBinaryPtr->memBloc.loadBinaryFile(filenameStr);
  _semMemoryPtr->memBloc.subBlockPtr = &_semMemoryBinaryPtr->memBloc;
}


void MainWindow::on_pushButton_history_microForChat_clicked()
{
  if (_ui->pushButton_history_microForChat->text() == microStr)
  {
    _ui->pushButton_history_microForChat->setText(stopMicroStr);
  }
  else
  {
    bool textEnd = false;
    auto asrText = _getAsrText(textEnd);
    auto asrTextQString = QString::fromUtf8(asrText.c_str());
    if (!asrText.empty() && (textEnd || _ui->lineEdit_history_newText->text() != asrTextQString))
    {
      _ui->lineEdit_history_newText->setText(QString::fromUtf8(asrText.c_str()));
      if (textEnd)
        on_lineEdit_history_newText_returnPressed();
    }
    _ui->pushButton_history_microForChat->setText(microStr);
  }
}


std::string MainWindow::_getAsrText(bool& pTextEnd)
{
  if (_isSpeaking || _nbOfSecondToWaitAfterTtsSpeech > 0)
    return "";
  static std::string lastAsrText = "";
  std::ifstream fin;
  fin.open("out_asr.txt");
  static const std::string partialBeginOfStr = "  \"partial\" : \"";
  static const auto partialBeginOfStr_size = partialBeginOfStr.size();
  static const std::string textBeginOfStr = "  \"text\" : \"";
  static const auto textBeginOfStr_size = textBeginOfStr.size();
  std::string res;
  std::string line;
  bool firstLine = true;
  while (getline(fin, line))
  {
    if (firstLine)
    {
      if (line == "o")
        _asrIsWaiting = true;
      else if (_asrIsWaiting)
      {
        _asrIsWaiting = false;
        _shouldWaitForNewSpeech = false;
      }
      firstLine = false;
      continue;
    }
    if (line.compare(0, partialBeginOfStr_size, partialBeginOfStr) == 0 &&
        line.size() > partialBeginOfStr_size - 1)
    {
      lastAsrText = "";
      res = line.substr(partialBeginOfStr_size, line.size() - partialBeginOfStr_size - 1);
      if (!res.empty() && !_shouldWaitForNewSpeech)
      {
        fin.close();
        pTextEnd = false;
        return res;
      }
    }
    else if (line.compare(0, textBeginOfStr_size, textBeginOfStr) == 0 &&
             line.size() > textBeginOfStr_size - 1)
    {
      res = line.substr(textBeginOfStr_size, line.size() - textBeginOfStr_size - 1);
      if (res != lastAsrText)
      {
        lastAsrText = res;
        if (!_shouldWaitForNewSpeech)
        {
          fin.close();
          pTextEnd = true;
          return res;
        }
      }
    }
    break;
  }
  fin.close();
  pTextEnd = false;
  return "";
}


void MainWindow::on_actionAdd_domain_triggered()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  const QString extension = ".json";
  const QString firstStr = "Chatbot domain (*" + extension + ")";

  std::string filenameStr = QFileDialog::getOpenFileName
      (this, "Import chatbot domain", QString(), firstStr).toUtf8().constData();
  if (filenameStr.empty())
    return;
  std::ifstream file(filenameStr.c_str(), std::ifstream::in);
  if (!_chatbotDomain)
    _chatbotDomain = std::make_unique<ChatbotDomain>();
  loadChatbotDomain(*_chatbotDomain, file);
  addChatbotDomaintoASemanticMemory(*_semMemoryPtr, *_chatbotDomain, _lingDb);
  if (!_chatbotProblem)
    _chatbotProblem = std::make_unique<ChatbotProblem>();
  _proactivelyAskThePlanner(now);
}



void MainWindow::on_actionSet_problem_triggered()
{
  auto now = std::make_unique<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
  const QString extension = ".json";
  const QString firstStr = "Chatbot problem (*" + extension + ")";

  std::string filenameStr = QFileDialog::getOpenFileName
      (this, "Import chatbot problem", QString(), firstStr).toUtf8().constData();
  if (filenameStr.empty())
    return;
  std::ifstream file(filenameStr.c_str(), std::ifstream::in);
  _chatbotProblem = std::make_unique<ChatbotProblem>();
  loadChatbotProblem(*_chatbotProblem, file);
  _proactivelyAskThePlanner(now);
}

void MainWindow::_printGoalsAndFacts()
{

  // Print goals
  const auto& goals = _chatbotProblem->problem.goals();
  std::stringstream ss;
  ss << "Priority     Goal\n";
  ss << "-----------------------\n";

  std::string mainGoal;
  for (auto itGoalPrority = goals.rbegin(); itGoalPrority != goals.rend(); ++itGoalPrority)
  {
    for (auto& currGoal : itGoalPrority->second)
    {
      if (mainGoal.empty())
        mainGoal = currGoal.getGoalGroupId();
      ss << itGoalPrority->first << "                  "
         << currGoal.toStr() << "\n";
    }
  }
  _chatbotProblem->variables["intention"] = mainGoal;

  if (!mainGoal.empty())
  {
    _chatbotProblem->variables["becauseIntention"] = "Parce que " + mainGoal;
    auto currentAction = _chatbotProblem->variables["currentAction"];
    if (!currentAction.empty())
    {
      _chatbotProblem->variables["currentActionWithIntention"] =  _mergeFactAndReason(currentAction, mainGoal);
    }
    else
    {
      _chatbotProblem->variables["currentActionWithIntention"] = "Je ne sais pas.";
    }
  }
  else
  {
    _chatbotProblem->variables["becauseIntention"] = "Je ne sais pas.";
    _chatbotProblem->variables["currentActionWithIntention"] = "Je ne sais pas.";
  }

  _ui->textBrowser_goals->clear();
  _ui->textBrowser_goals->append(QString::fromUtf8(ss.str().c_str()));

  // Print facts
  {
    const auto& facts = _chatbotProblem->problem.facts();
    std::stringstream ssFacts;
    for (auto& currFact : facts)
      ssFacts << currFact.toStr() << "\n";

    _ui->textBrowser_facts->clear();
    _ui->textBrowser_facts->append(QString::fromUtf8(ssFacts.str().c_str()));
  }
}


void MainWindow::_proactivelyAskThePlanner(const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  std::list<TextWithLanguage> textsToSay;
  _proactivityFromPlanner(textsToSay, pNow);
  if (_ui->checkBox_enable_tts->isChecked() && !textsToSay.empty())
    _sayText(textsToSay);
}
