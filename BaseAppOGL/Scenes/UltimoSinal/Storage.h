#pragma once
#include "Investigation.h"
#include "Settings.h"
#include "Spatial.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <cctype>
namespace ultimo {
struct Snapshot{Investigation game;float x=0,y=1.7f,z=17,yaw=-90,pitch=0;double elapsed=0;bool complete=false;};
inline std::string Encode(const Snapshot& s){
 std::ostringstream out;out<<std::setprecision(12);
 for(bool v:s.game.discovered)out<<v<<' ';
 for(bool v:s.game.breakers)out<<v<<' ';
 out<<s.game.powerSolved<<' '<<s.game.radioSolved<<' '<<s.game.radioChannel<<' '<<s.game.helpStage<<' '<<s.complete
    <<' '<<s.x<<' '<<s.y<<' '<<s.z<<' '<<s.yaw<<' '<<s.pitch<<' '<<s.elapsed
    <<' '<<s.game.lensSolved<<' '<<s.game.lensManual;
 for(bool v:s.game.lensLong)out<<' '<<v;
 for(int choice:s.game.answers)out<<' '<<choice;
 return out.str();
}
namespace storage_detail {
inline bool ReadBit(std::istream& in,bool& value){
 int bit;
 if(!(in>>bit)||bit<0||bit>1)return false;
 value=bit!=0;
 return true;
}
template<std::size_t N>
bool ReadBits(std::istream& in,std::array<bool,N>& values){
 for(auto& value:values)if(!ReadBit(in,value))return false;
 return true;
}
inline bool ReadWorld(std::istream& in,Snapshot& s){
 return ReadBit(in,s.complete)&&bool(in>>s.x>>s.y>>s.z>>s.yaw>>s.pitch>>s.elapsed);
}
inline bool ReadLens(std::istream& in,Investigation& game){
 return ReadBit(in,game.lensSolved)&&ReadBit(in,game.lensManual)&&ReadBits(in,game.lensLong);
}
inline bool FinishDecode(Snapshot& s,Snapshot& result){
 if(!std::isfinite(s.x)||!std::isfinite(s.y)||!std::isfinite(s.z)||!std::isfinite(s.yaw)||!std::isfinite(s.pitch)||!std::isfinite(s.elapsed))return false;
 if(std::abs(s.x)>12.5f||std::abs(s.z)>20.5f||s.y<1.5f||s.y>9.05f||(s.y>1.9f&&!spatial::ValidPose(s.x,s.y,s.z)))return false;
 if(std::abs(s.pitch)>89||std::abs(s.yaw)>1e8||s.elapsed<0||s.elapsed>1e8||s.game.radioChannel<1||s.game.radioChannel>16||s.game.helpStage<0||s.game.helpStage>3)return false;
 if(s.game.powerSolved&&(!s.game.discovered[2]||!s.game.discovered[5]||s.game.breakers[0]||!s.game.breakers[1]||!s.game.breakers[2]))return false;
 if(s.game.radioSolved&&(!s.game.powerSolved||!s.game.discovered[6]||!s.game.discovered[7]))return false;
 if(s.game.discovered[7]!=s.game.radioSolved)return false;
 if(s.game.DoorOpen()&&!s.game.powerSolved)return false;
 if(s.y>1.9f&&!s.game.DoorOpen())return false;
 if((s.game.discovered[4]||s.game.discovered[8])&&!s.game.DoorOpen())return false;
 if(s.game.lensSolved&&(!s.game.powerSolved||!s.game.DoorOpen()||!s.game.radioSolved||!s.game.discovered[4]||!s.game.discovered[8]||!s.game.lensManual||!std::all_of(s.game.lensLong.begin(),s.game.lensLong.end(),[](bool v){return v;})))return false;
 if(s.complete&&(!s.game.Ready()||!s.game.AnswersCorrect()))return false;
 s.game.submitted=s.complete;
 s.game.screen=s.complete?Screen::Complete:Screen::Explore;
 result=s;
 return true;
}
}
inline bool Decode(const std::string& text,Snapshot& result){
 using namespace storage_detail;
 std::istringstream in(text);
 Snapshot s;
 if(!ReadBits(in,s.game.discovered)||!ReadBits(in,s.game.breakers)||!ReadBit(in,s.game.powerSolved)||!ReadBit(in,s.game.radioSolved))return false;
 if(!(in>>s.game.radioChannel>>s.game.helpStage)||!ReadWorld(in,s)||!ReadLens(in,s.game))return false;
 for(std::size_t i=0;i<s.game.answers.size();++i){
  int& choice=s.game.answers[i];
  if(!(in>>choice)||choice<UnansweredConclusion||choice>=int(ConclusionQuestions[i].choices.size()))return false;
 }
 in>>std::ws;
 return in.eof()&&FinishDecode(s,result);
}
inline bool DecodeLegacy(const std::string& text,Snapshot& result){
 using namespace storage_detail;
 std::istringstream in(text);
 Snapshot s;
 std::array<bool,EntryCount> evidence{};
 int hypothesis;
 std::string justification;
 if(!ReadBits(in,s.game.discovered)||!ReadBits(in,s.game.breakers)||!ReadBits(in,evidence)||!ReadBit(in,s.game.powerSolved)||!ReadBit(in,s.game.radioSolved))return false;
 if(!(in>>s.game.radioChannel>>hypothesis>>s.game.helpStage)||!ReadWorld(in,s)||!(in>>std::quoted(justification)))return false;
 if(hypothesis< -1||hypothesis>2||justification.size()>2047)return false;
 in>>std::ws;
 if(!in.eof()){
  std::string extension;
  if(!(in>>extension)||extension!="LENS1"||!ReadLens(in,s.game))return false;
  in>>std::ws;
  if(!in.eof())return false;
 }else if(s.complete){
  // Earlier completed saves predate the lens diagnostic.
  s.game.lensSolved=true;s.game.lensManual=true;s.game.lensLong.fill(true);
 }
 if(s.complete){
  if(!s.game.Ready()||hypothesis<0||std::count(evidence.begin(),evidence.end(),true)<2||
     std::none_of(justification.begin(),justification.end(),[](unsigned char c){return !std::isspace(c);}))return false;
  s.complete=hypothesis==1;
  if(s.complete)s.game.answers=CorrectConclusionAnswers;
 }
 return FinishDecode(s,result);
}
inline std::uint64_t Fingerprint(const std::string& text){std::uint64_t h=1469598103934665603ull;for(unsigned char c:text){h^=c;h*=1099511628211ull;}return h;}
inline bool ReadSave(const std::filesystem::path& path,Snapshot& snapshot){
 std::ifstream file(path,std::ios::binary);if(!file)return false;
 std::string header;std::getline(file,header);std::uint64_t checksum;std::istringstream hi(header);std::string format;
 if(!(hi>>format>>checksum)||(format!="ULTIMO4"&&format!="ULTIMO3"))return false;
 std::string payload((std::istreambuf_iterator<char>(file)),{});if(payload.size()>8192||Fingerprint(payload)!=checksum)return false;
 return format=="ULTIMO4"?Decode(payload,snapshot):DecodeLegacy(payload,snapshot);
}
}
