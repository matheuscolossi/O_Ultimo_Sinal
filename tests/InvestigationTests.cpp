#include "../BaseAppOGL/Scenes/UltimoSinal/Storage.h"
#include "../BaseAppOGL/Scenes/UltimoSinal/Geometry.h"
#include "../BaseAppOGL/Engine/MouseMotion.h"
#include <iostream>
#include <queue>
#include <set>
#include <limits>
#include <iomanip>
#include <sstream>
static int checks=0;void Check(bool ok,const char* s){++checks;if(!ok){std::cerr<<"FAIL "<<s<<std::endl;std::exit(1);}}
using namespace ultimo;
void Explore(Investigation& g){g.screen=Screen::Explore;}
void Read(Investigation& g,int i){Explore(g);g.InspectEntry(i);Explore(g);}
bool CompletePath(Investigation g){
 for(int i:{0,1,2,5,6,9})Read(g,i);
 g.screen=Screen::Panel;g.breakers={false,true,true};if(!g.RestorePower())return false;
 g.screen=Screen::Radio;g.radioChannel=6;if(!g.TuneRadio())return false;Explore(g);
 for(int i:{3,4,8})Read(g,i);
 if(!g.lensSolved){g.screen=Screen::LensConsole;g.lensManual=true;g.lensLong={true,true,true};if(!g.VerifyLens())return false;}
 return g.Ready();
}
std::string LegacyPayload(const Snapshot& snapshot,int hypothesis=-1,bool includeLens=false){
 const auto& game=snapshot.game;
 std::ostringstream out;
 out<<std::setprecision(12);
 for(bool value:game.discovered)out<<value<<' ';
 for(bool value:game.breakers)out<<value<<' ';
 for(std::size_t i=0;i<EntryCount;++i)out<<(hypothesis>=0&&(i==7||i==8))<<' ';
 out<<game.powerSolved<<' '<<game.radioSolved<<' '<<game.radioChannel<<' '<<hypothesis<<' '
    <<game.helpStage<<' '<<snapshot.complete<<' '<<snapshot.x<<' '<<snapshot.y<<' '<<snapshot.z<<' '
    <<snapshot.yaw<<' '<<snapshot.pitch<<' '<<snapshot.elapsed<<' '
    <<std::quoted(hypothesis>=0?"Horários e o registro marcado \"LENS1\".":"");
 if(includeLens){
  out<<" LENS1 "<<game.lensSolved<<' '<<game.lensManual;
  for(bool value:game.lensLong)out<<' '<<value;
 }
 return out.str();
}
void SelectLens(Investigation& g,int mask){g.lensManual=bool(mask&8);for(int i=0;i<3;++i)g.lensLong[i]=bool(mask&(1<<i));}
int main(){
 Investigation g;Check(!g.Inspect(ClueId::Diary),"no collect title");g.Start();Check(!g.Inspect(ClueId::Count),"invalid classic ID");Check(!g.InspectEntry(10),"invalid entry");Check(!g.Inspect(ClueId::Key),"key locked unpowered");Check(!g.Inspect(ClueId::Photograph),"lens locked");Check(!g.BeginConclusion(),"no premature report");
 g.screen=Screen::Panel;g.breakers={false,true,true};Check(!g.RestorePower(),"no puzzle bypass without evidence");Read(g,5);g.screen=Screen::Panel;Check(!g.RestorePower(),"need wet circuit evidence too");Read(g,2);
 for(int mask=0;mask<8;++mask){Investigation p=g;p.screen=Screen::Panel;p.breakers={bool(mask&1),bool(mask&2),bool(mask&4)};Check(p.RestorePower()==(mask==6),"all circuit combinations");}
 g.screen=Screen::Panel;g.breakers={false,true,true};Check(g.RestorePower(),"power correct");Check(g.helpStage==0,"hint resets stage");
 g.screen=Screen::Radio;g.radioChannel=6;Check(!g.TuneRadio(),"channel needs chart");Read(g,6);
 for(int channel=1;channel<=16;++channel){Investigation r=g;r.screen=Screen::Radio;r.radioChannel=channel;Check(r.TuneRadio()==(channel==6),"all radio channels");}
 g.screen=Screen::Radio;g.radioChannel=6;Check(g.TuneRadio(),"receive transcript");Check(g.discovered[7]&&g.readingReturn==Screen::Radio,"accessible transcript and return");g.CloseReading();Check(g.screen==Screen::Radio,"read back radio");
 for(int i:{0,1,3,4,8})Read(g,i);Check(!g.Ready(),"critical observations still require local reconstruction");Check(g.Count()==5,"five classics preserved");Read(g,0);Check(g.Count()==5,"no duplicate");
 // The diagnostic must reconcile real observations, rather than unlock from guessed controls.
 for(int missing:{-1,3,4,8}){
  Investigation p=g;p.screen=Screen::LensConsole;p.lensManual=true;p.lensLong={true,true,true};
  if(missing==-1)p.powerSolved=false;else p.discovered[missing]=false;
  Check(!p.VerifyLens(),"lens requires service access and all three source records");
 }
 {Investigation p=g;p.radioSolved=false;p.screen=Screen::LensConsole;p.lensManual=true;p.lensLong={true,true,true};Check(!p.VerifyLens(),"lens requires actual solved radio");}
 {Investigation p=g;p.screen=Screen::Explore;p.lensManual=true;p.lensLong={true,true,true};Check(!p.VerifyLens(),"lens cannot be solved outside console");}
 for(int mask=0;mask<16;++mask){
  Investigation p=g;p.screen=Screen::LensConsole;SelectLens(p,mask);const auto records=p.discovered;
  Check(p.VerifyLens()==(mask==15),"all mode and pulse combinations");
  Check(p.discovered==records,"diagnostic never invents a historical observation");
  Check(p.lensSolved==(mask==15),"only coherent reconstruction is recorded");
 }
 g.screen=Screen::LensConsole;SelectLens(g,15);g.helpStage=3;Check(g.VerifyLens(),"manual long sequence verified");Check(g.helpStage==0&&g.Ready(),"verified reconstruction enables report and resets hint");
 Check(!g.VerifyLens()&&g.lensSolved,"solved reconstruction is not repeated");
 Explore(g);g.ToggleJournal();g.ReadEntry(8);g.CloseReading();Check(g.screen==Screen::Journal,"reread returns journal");
 Explore(g);Check(g.BeginConclusion(),"questions enabled after investigation");
 Check(g.answers==EmptyConclusionAnswers&&!g.CanSubmit()&&!g.Submit()&&!g.conclusionIncorrect,"unanswered questions cannot be submitted");
 g.answers=CorrectConclusionAnswers;g.answers[1]=-1;
 Check(!g.HasAllAnswers()&&!g.CanSubmit()&&!g.Submit(),"one missing answer blocks submission");
 g.answers[1]=int(ConclusionQuestions[1].choices.size());
 Check(!g.HasAllAnswers()&&!g.CanSubmit(),"out-of-range answer blocks submission");
 std::size_t combinations=1;
 for(const auto& question:ConclusionQuestions)combinations*=question.choices.size();
 for(std::size_t combination=0;combination<combinations;++combination){
  Investigation attempt=g;
  std::size_t remaining=combination;
  for(std::size_t question=0;question<attempt.answers.size();++question){
   const auto choiceCount=ConclusionQuestions[question].choices.size();
   attempt.answers[question]=int(remaining%choiceCount);
   remaining/=choiceCount;
  }
  const bool allCorrect=attempt.answers==CorrectConclusionAnswers;
  Check(attempt.HasAllAnswers()&&attempt.CanSubmit(),"complete answer combinations can be evaluated");
  Check(attempt.Submit()==allCorrect,"only the correct combination completes the case");
  Check(attempt.submitted==allCorrect&&attempt.screen==(allCorrect?Screen::Complete:Screen::Conclusion)
        &&attempt.conclusionIncorrect!=allCorrect,"wrong answers prompt review without ending the investigation");
 }
 g.answers=CorrectConclusionAnswers;g.answers[1]=(g.answers[1]+1)%3;
 Check(!g.Submit()&&g.conclusionIncorrect&&g.screen==Screen::Conclusion&&!g.submitted,"incorrect answer leaves quiz open for retry");
 const auto attemptedAnswers=g.answers;
 Snapshot unfinished;unfinished.game=g;Snapshot restored;
 Check(Decode(Encode(unfinished),restored)&&!restored.complete&&!restored.game.submitted
       &&restored.game.answers==attemptedAnswers&&restored.game.Ready(),"incorrect attempt preserves the unfinished save and clue access");
 g.OpenJournal(Screen::Conclusion);
 Check(g.ReadEntry(7),"transcript is readable after an unsuccessful attempt");g.CloseReading();
 g.CloseJournal();
 Check(g.screen==Screen::Conclusion&&g.answers==attemptedAnswers&&g.conclusionIncorrect,
       "returning from evidence review restores the quiz and retains selected answers");
 g.answers=CorrectConclusionAnswers;Check(g.Submit()&&g.AnswersCorrect()&&g.screen==Screen::Complete,"correct retry ends the case");
 // Consulting evidence after closure must keep the solved case intact.
 g.ToggleJournal();Check(g.ReadEntry(7),"submitted transcript remains readable");g.CloseReading();Check(g.screen==Screen::Journal,"submitted reading returns to journal");
 g.CloseJournal();Check(g.screen==Screen::Complete,"closing the journal restores the completed case");
 Explore(g);Check(!g.BeginConclusion()&&g.screen==Screen::Explore,"closed case cannot enter the quiz again");
 g.screen=Screen::Conclusion;Check(!g.CanSubmit()&&!g.Submit(),"second submission rejected");
 Check(g.submitted&&g.answers==CorrectConclusionAnswers,"consultation preserves the solved case");g.screen=Screen::Complete;
 Investigation fresh=g;fresh.Start();Check(!fresh.submitted&&!fresh.conclusionIncorrect&&fresh.answers==EmptyConclusionAnswers,"new investigation clears the quiz");
 Snapshot snap;snap.game=g;snap.complete=true;snap.elapsed=531.4;auto encoded=Encode(snap);Check(Decode(encoded,restored),"save round trip");
 Check(restored.game.AnswersCorrect()&&restored.game.submitted&&restored.game.screen==Screen::Complete&&restored.elapsed==snap.elapsed,"ending preserved");
 Check(restored.game.lensSolved&&restored.game.lensManual&&std::all_of(restored.game.lensLong.begin(),restored.game.lensLong.end(),[](bool v){return v;}),"new solved diagnostic round trip");
 restored.game.screen=Screen::Explore;Check(!restored.game.BeginConclusion(),"loaded submitted report remains frozen");restored.game.screen=Screen::Conclusion;Check(!restored.game.Submit(),"loaded report cannot be resubmitted");
 Check(!Decode(encoded+" junk",restored),"trailing corruption rejected");Check(!Decode("garbage",restored),"invalid payload");snap.x=100;Check(!Decode(Encode(snap),restored),"unsafe position rejected");snap.x=0;snap.game.powerSolved=false;Check(!Decode(Encode(snap),restored),"inconsistent progress rejected");
 snap.game=g;snap.game.answers[1]=(snap.game.answers[1]+1)%3;
 Check(!Decode(Encode(snap),restored),"completed save requires all correct answers");
 unfinished.game.answers[0]=int(ConclusionQuestions[0].choices.size());
 Check(!Decode(Encode(unfinished),restored),"invalid saved answer rejected");
 // Existing correct reports preserve closure. Older incorrect submissions can be retried.
 snap.game=g;
 for(bool includeLens:{false,true}){
  Check(DecodeLegacy(LegacyPayload(snap,1,includeLens),restored),"legacy correctly completed report migrates");
  Check(restored.complete&&restored.game.submitted&&restored.game.answers==CorrectConclusionAnswers
        &&restored.game.lensSolved&&restored.game.lensManual&&restored.game.lensLong==std::array<bool,3>{true,true,true},"legacy correct ending and diagnostic preserved");
  Check(DecodeLegacy(LegacyPayload(snap,2,includeLens),restored),"legacy wrong submission migrates");
  Check(!restored.complete&&!restored.game.submitted&&restored.game.discovered==g.discovered&&restored.game.BeginConclusion(),"legacy incorrect outcome can be investigated again");
 }
 Check(DecodeLegacy(LegacyPayload(snap,1),restored),"legacy closure available again for resubmission check");
 restored.game.screen=Screen::Conclusion;Check(!restored.game.Submit(),"migrated submitted report cannot be rewritten");
 Snapshot oldPartial;oldPartial.game=g;oldPartial.game.submitted=false;oldPartial.game.answers=EmptyConclusionAnswers;
 Check(DecodeLegacy(LegacyPayload(oldPartial,2),restored),"legacy complete observations but unfinished report migrates");
 Check(!restored.game.lensSolved&&!restored.game.lensManual&&restored.game.lensLong==std::array<bool,3>{false,false,false}&&!restored.game.Ready(),"legacy unfinished investigation requires new diagnostic");
 Check(restored.game.discovered==oldPartial.game.discovered&&restored.game.answers==EmptyConclusionAnswers,"legacy clues preserved and new questions begin unanswered");
 Snapshot early;early.game.Start();Read(early.game,2);Check(DecodeLegacy(LegacyPayload(early),restored)&&restored.game.discovered[2]&&!restored.game.lensSolved,"early legacy progress migrates without fabricated records");
 Snapshot partialQuiz=oldPartial;partialQuiz.game.answers[0]=CorrectConclusionAnswers[0];
 Check(Decode(Encode(partialQuiz),restored)&&!restored.complete&&restored.game.answers==partialQuiz.game.answers,"partial quiz selection survives saving");
 // Every new control draft survives save/load, including incorrect trials.
 for(int mask=0;mask<16;++mask){Snapshot draft=oldPartial;draft.game.lensSolved=false;SelectLens(draft.game,mask);Check(Decode(Encode(draft),restored),"lens draft decodes");Check(restored.game.lensManual==draft.game.lensManual&&restored.game.lensLong==draft.game.lensLong&&!restored.game.lensSolved,"mode and all pulse draft controls preserved");}
 const auto legacyPrefix=LegacyPayload(oldPartial);
 for(const auto* extension:{" LENS2 0 0 0 0 0"," LENS1"," LENS1 0 1 1 1"," LENS1 0 1 1 1 1 0"," LENS1 0 2 1 1 1"," LENS1 0 1 1 -1 1"," LENS1 1 0 1 1 1"," LENS1 1 1 1 0 1"," LENS1 0 0 0 0 0 LENS1 0 0 0 0 0"})Check(!DecodeLegacy(legacyPrefix+extension,restored),"unknown truncated trailing nonboolean or incoherent legacy extension rejected");
 for(int missing:{3,4,7,8}){Snapshot inconsistent=oldPartial;inconsistent.game.lensSolved=true;SelectLens(inconsistent.game,15);inconsistent.game.discovered[missing]=false;Check(!Decode(Encode(inconsistent),restored),"solved save requires observed sources and access");}
 Snapshot inconsistent=oldPartial;inconsistent.game.lensSolved=true;SelectLens(inconsistent.game,15);inconsistent.game.radioSolved=false;Check(!Decode(Encode(inconsistent),restored),"solved save requires real radio state");
 inconsistent=oldPartial;inconsistent.complete=true;inconsistent.game.lensSolved=false;SelectLens(inconsistent.game,15);Check(!Decode(Encode(inconsistent),restored),"new completed save cannot bypass diagnostic");
 // Exhaust reachable observations, mechanisms and local-console drafts in every order.
 std::queue<Investigation> queue;std::set<unsigned long> seen;Investigation initial;initial.Start();queue.push(initial);int states=0;
 while(!queue.empty()){
  auto state=queue.front();queue.pop();unsigned long signature=0;for(int i=0;i<10;++i)if(state.discovered[i])signature|=1ul<<i;if(state.powerSolved)signature|=1ul<<10;if(state.radioSolved)signature|=1ul<<11;if(state.lensSolved)signature|=1ul<<12;if(state.lensManual)signature|=1ul<<13;for(int i=0;i<3;++i)if(state.lensLong[i])signature|=1ul<<(14+i);
  if(!seen.insert(signature).second)continue;++states;Check(CompletePath(state),"every reachable state retains a completion path");
  Snapshot partial;partial.game=state;Snapshot loaded;Check(Decode(Encode(partial),loaded),"save valid at each progress stage");
  for(int i=0;i<10;++i){auto next=state;Explore(next);if(next.InspectEntry(i)){Explore(next);queue.push(next);}}
  if(!state.powerSolved){auto next=state;next.screen=Screen::Panel;next.breakers={false,true,true};if(next.RestorePower()){Explore(next);queue.push(next);}}
  if(!state.radioSolved){auto next=state;next.screen=Screen::Radio;next.radioChannel=6;if(next.TuneRadio()){Explore(next);queue.push(next);}}
  if(state.powerSolved&&state.DoorOpen()&&!state.lensSolved){
   for(int control=0;control<4;++control){auto next=state;if(control==3)next.lensManual=!next.lensManual;else next.lensLong[control]=!next.lensLong[control];queue.push(next);}
   auto next=state;next.screen=Screen::LensConsole;if(next.VerifyLens()){Explore(next);queue.push(next);}
  }
 }
 MouseMotion mouse;mouse.Add(0,0);mouse.Add(2,3);mouse.Add(7,5);Check(mouse.dx==7&&mouse.dy== -5,"all mouse events");mouse.Clear();Check(mouse.dx==0,"mouse consumed");mouse.Reset();mouse.Add(999,999);Check(mouse.dx==0,"recapture reset");
 Bounds wall{{0,1,0},{.001f,2,3}};Check(SegmentHits({-1,1,0},{1,1,0},wall),"thin wall occlusion");Check(!SegmentHits({-1,3,0},{1,3,0},wall),"clear segment");Check(BlocksWalker(0,0,wall),"walker blocked");Settings settings;settings.sensitivity=std::numeric_limits<float>::quiet_NaN();Check(settings.Sensitivity()==.06f,"safe sensitivity");
 std::cout<<checks<<" verificacoes aprovadas; "<<states<<" estados de progresso alcancaveis; 8 circuitos, 16 canais e 16 configuracoes da lente."<<std::endl;
}
